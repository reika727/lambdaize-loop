/**
 * @file lambdaize-loop.cpp
 * @brief LambdaizeLoop パス
 */

#define DEBUG_TYPE "lambdaize-loop"

namespace {
    llvm::cl::opt<bool> all (
        "all",
        llvm::cl::desc("Obfuscate unannotated loops"),
        llvm::cl::init(false)
    );

    llvm::cl::opt<double> probability (
        "prob",
        llvm::cl::desc("Obfuscation rate"),
        llvm::cl::init(1.)
    );

    std::mt19937_64 engine(std::random_device{}());
    std::uniform_real_distribution<> dist(0., 1.);

    /**
     * @brief LambdaizeLoop パスの実装
     */
    class LambdaizeLoop : public llvm::PassInfoMixin<LambdaizeLoop> {
    public:
        /**
         * @brief パスの処理の実体
         * @note lambdaizeloop メタデータを持つループのみ処理を行う
         */
        llvm::PreservedAnalyses run(llvm::Function &Function, llvm::FunctionAnalysisManager &FAM)
        {
            auto &LoopInfo = FAM.getResult<llvm::LoopAnalysis>(Function);
            bool Changed = false;
            for (auto *Loop : LoopInfo.getLoopsInPreorder()) {
                if (!Loop->isLoopSimplifyForm()) {
                    LLVM_DEBUG(llvm::dbgs() << "Loop is not simplified.\n");
                    continue;
                }
                if (!all && !LoopContainsMetadata(*Loop, "lambdaizeloop")) {
                    LLVM_DEBUG(llvm::dbgs() << "\"lambdaizeloop\" metadata is not set.\n";);
                    continue;
                }
                if (dist(engine) >= probability) {
                    continue;
                }
                Changed |= extractLoopIntoFunction(*Loop);
            }
            return Changed ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
        }

    private:
        /**
         * @brief 指定の文字列がループメタデータに含まれるか判定する
         * @param Loop 判定対象のループ
         * @param Str 判定対象の文字列
         * @return Loop のメタデータ内に Str が含まれるか
         */
        bool LoopContainsMetadata(const llvm::Loop &Loop, const llvm::StringRef Str)
        {
            // "For legacy reasons, the first item of a loop metadata node must be a reference to itself."
            // see https://llvm.org/docs/LangRef.html#llvm-loop

            if (auto *LoopID = Loop.getLoopID()) {
                // TODO: replace with std:: when C++20 is available.
                return llvm::any_of(
                    LoopID->operands().drop_front(),
                    [Str](const auto &MDOperand) {
                        const auto Metadata = llvm::cast<llvm::MDNode>(MDOperand.get());
                        return Metadata->getOperand(0).equalsStr(Str);
                    });
            }
            return false;
        }

        /**
         * @brief ループを extracted 関数で置換する
         * @param Loop 置換対象のループ
         * @return 置換が行われたか否か
         */
        bool extractLoopIntoFunction(llvm::Loop &Loop)
        {
            auto *Preheader = Loop.getLoopPreheader();
            llvm::IRBuilder Builder(Preheader->getTerminator());

            // HACK: ArgsToLooper[0] should contain pointer to Extracted, so reserve place
            std::vector<llvm::Value *> ArgsToLooper(1);
            if ((ArgsToLooper[0] = createExtracted(Loop, std::back_inserter(ArgsToLooper)))) {
                // preheader の終端命令（この時点で exit ブロックへの branch に書き換えられている）の前に
                // looper 関数の呼び出しを挿入する
                Builder.CreateCall(getLooperFC(*Preheader->getModule()), llvm::ArrayRef(ArgsToLooper));
                return true;
            }

            return false;
        }

        /**
         * @brief ループから extracted 関数を作成し、
         * @brief さらに作成された extracted 関数に渡される必要がある変数の一覧を取得する
         * @param Loop 変形対象のループ
         * @param[out] NeededArguments Value* への出力イテレータ
         * @return extracted 関数が作成された場合はそのポインタ
         * @return 作成されなかった場合は nullptr
         */
        template <class OutputIterator>
        llvm::Function *createExtracted(llvm::Loop &Loop, OutputIterator NeededArguments)
        {
            auto *Module = Loop.getHeader()->getModule();
            auto &Context = Loop.getHeader()->getContext();

            // ループを構成するブロック群を除外する
            std::vector<llvm::BasicBlock *> BlocksFromLoop;
            if (!removeLoop(Loop, std::back_inserter(BlocksFromLoop))) {
                return nullptr;
            }

            // ループ内で使用されている変数のうち、外部で宣言されている者の一覧を取得する
            std::vector<llvm::Value *> OutsideDefined;
            setOutsideDefinedVariables(BlocksFromLoop.begin(), BlocksFromLoop.end(), std::back_inserter(OutsideDefined));
            llvm::copy(OutsideDefined, NeededArguments); // TODO: replace with std:: when C++20 is available.

            auto *Extracted = llvm::Function::Create(
                getExtractedFunctionType(Context),
                llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                "extracted",
                *Module);

            llvm::IRBuilder Builder(llvm::BasicBlock::Create(Context, "", Extracted));

            // extracted 関数の先頭で va_list の中身をすべて取り出す命令を挿入し、
            // 取り出された変数とアドレスの対応を記録する
            std::map<llvm::Value *, llvm::Value *> ArgAddrMap;
            for (auto *OD : OutsideDefined) {
                ArgAddrMap[OD] = Builder.CreateVAArg(Extracted->getArg(0), OD->getType());
            }

            // ループから取り出されたブロック群の先頭に branch する
            Builder.CreateBr(BlocksFromLoop.front());

            // ループから取り出された各ブロックに対し、外部で宣言された変数
            // （すなわち extraceted 関数の引数として与えなおされるもの）のアドレスを修正しつつ、
            // extracted 関数に挿入する
            for (auto *Block : BlocksFromLoop) {
                for (auto &&Inst : *Block) {
                    for (auto &&Op : Inst.operands()) {
                        if (ArgAddrMap.count(Op)) {
                            Op = ArgAddrMap[Op];
                        }
                    }
                }
                Block->insertInto(Extracted);
            }

            return Extracted;
        }

        /**
         * @brief 変形条件を満たすループ（を構成する basic block 群）を除外したうえで出力する
         * @param[in] Loop 変形するループ
         * @param[out] Dest BasicBlock* への出力イテレータ
         * @return 変形が行われたか否か
         * @note exit ブロックをちょうど一つ持ち、なおかつ内部の終端命令が全て
         * @note branch 命令か switch 命令である場合のみ変形を行う
         */
        template <class OutputIterator>
        bool removeLoop(llvm::Loop &Loop, OutputIterator Dest)
        {
            auto *OriginalHeader = Loop.getHeader(), *OriginalExit = Loop.getExitBlock();

            // exit block がちょうど一つでない場合は対象外
            if (!OriginalExit) {
                LLVM_DEBUG(llvm::dbgs() << "multiple exit blocks. skipped.\n";);
                return false;
            }

            // 終端命令が branch でも switch でもない場合（例外を投げる場合など）は対象外
            for (auto *Block : Loop.blocks()) {
                if (!llvm::BranchInst::classof(Block->getTerminator()) &&
                    !llvm::SwitchInst::classof(Block->getTerminator())) {
                    LLVM_DEBUG(llvm::dbgs() << "terminator is neither branch or switch. skipped.\n";);
                    return false;
                }
            }

            auto &Context = Loop.getHeader()->getContext();

            // ループが継続される場合は True を返す
            auto *LoopContinue = llvm::BasicBlock::Create(Context);
            llvm::IRBuilder(LoopContinue).CreateRet(llvm::ConstantInt::getTrue(Context));

            // ループが終了する場合は False を返す
            auto *LoopBreak = llvm::BasicBlock::Create(Context);
            llvm::IRBuilder(LoopBreak).CreateRet(llvm::ConstantInt::getFalse(Context));

            // preheader の（唯一の）後継ブロックを exit ブロックに書き換え、ループを閉じる
            Loop.getLoopPreheader()->getTerminator()->setSuccessor(0, Loop.getExitBlock());

            // ループ内のすべてのブロックに対し back edge と exit edge の行き先を書き換えつつ、
            // 「除外リスト」に追加していく
            std::vector<llvm::BasicBlock *> ToBeRemoved;
            for (auto *Block : Loop.blocks()) {
                auto *Term = Block->getTerminator();
                Term->replaceSuccessorWith(OriginalHeader, LoopContinue);
                Term->replaceSuccessorWith(OriginalExit, LoopBreak);
                ToBeRemoved.push_back(Block);
            }

            // 「除外リスト」内のすべてのブロックをループ内から削除し、
            // LoopContinue と LoopBreak とともに最終出力に追加する
            for (auto *Block : ToBeRemoved) {
                Block->removeFromParent();
                for (auto *LoopPtr = &Loop; LoopPtr; LoopPtr = LoopPtr->getParentLoop()) {
                    if (LoopPtr->contains(Block)) {
                        LoopPtr->removeBlockFromLoop(Block);
                    }
                }
                *Dest++ = Block;
            }
            *Dest++ = LoopContinue;
            *Dest++ = LoopBreak;

            return true;
        }

        /**
         * @brief BasicBlock のリストを受け取り、その外部で宣言されている非グローバル変数の一覧を取得する
         * @param[in] first BasicBlock* のリストへの始点イテレータ
         * @param[in] last BasicBlock* のリストへの終点イテレータ
         * @param[out] result Value* への出力イテレータ
         * @return 書き込まれた範囲の終点イテレータ
         */
        template <class InputIterator, class OutputIterator>
        OutputIterator setOutsideDefinedVariables(InputIterator first, InputIterator last, OutputIterator result)
        {
            // ブロック内で宣言された変数の一覧
            std::set<llvm::Value *> Declared;

            // ブロック内で使用されている引数の一覧
            std::set<llvm::Value *> Arguments;
            for (auto itr = first; itr != last; ++itr) {
                for (auto &&Inst : **itr) {
                    Declared.insert(&Inst);
                    for (auto *Op : Inst.operand_values()) {
                        // ラベルでも定数でもない非グローバル変数のみを追加する
                        if (!Op->getType()->isLabelTy() &&
                            !llvm::isa<llvm::GlobalValue>(Op) &&
                            !llvm::isa<llvm::Constant>(Op)) {
                            Arguments.insert(Op);
                        }
                    }
                }
            }

            // ブロック内で使用されている引数であって、宣言はされていないものを取得する
            return std::set_difference(
                Arguments.begin(), Arguments.end(),
                Declared.begin(), Declared.end(),
                result);
        }

        /**
         * @brief looper 関数の FunctionCallee を作成する
         * @details looper 関数は extracted 関数へのポインタと可変長引数を受け取る
         * @return looper 関数の FunctionCallee
         */
        llvm::FunctionCallee getLooperFC(llvm::Module &Module)
        {
            auto &Context = Module.getContext();
            return Module.getOrInsertFunction(
                "looper",
                llvm::FunctionType::get(
                    llvm::Type::getVoidTy(Context),
                    llvm::ArrayRef<llvm::Type *>{getExtractedFunctionType(Context)->getPointerTo()},
                    true /* variadic */));
        }

        /**
         * @brief extracted 関数の型を作成する
         * @details extracted 関数は va_list を受け取り、boolean を返却する
         * @return extracted 関数の型
         */
        llvm::FunctionType *getExtractedFunctionType(llvm::LLVMContext &Context)
        {
            return llvm::FunctionType::get(
                llvm::Type::getInt1Ty(Context),
                llvm::ArrayRef<llvm::Type *>{getVaListType(Context)->getPointerTo()},
                false /* NOT variadic */);
        }

        /**
         * @brief va_list 型を作成する
         * @return va_list 型
         * @remark System V AMD64 ABI において va_list は以下のように定義される
         * @code {.c}
         * typedef struct {
         *    unsigned int gp_offset;
         *    unsigned int fp_offset;
         *    void *overflow_arg_area;
         *    void *reg_save_area;
         * } va_list[1];
         * @endcode
         * @see https://gitlab.com/x86-psABIs/x86-64-ABI
         */
        llvm::StructType *getVaListType(llvm::LLVMContext &Context)
        {
            const llvm::StringRef Name = "struct.va_list";
            if (auto *Type = llvm::StructType::getTypeByName(Context, Name)) {
                return Type;
            }
            auto *i32 = llvm::IntegerType::getInt32Ty(Context);
            auto *i8p = llvm::IntegerType::getInt8PtrTy(Context);
            return llvm::StructType::create(Name, i32, i32, i8p, i8p);
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK llvm::PassPluginLibraryInfo llvmGetPassPluginInfo()
{
    return {
        LLVM_PLUGIN_API_VERSION,
        "Lambdaize loop",
        "1.0.0",
        [](llvm::PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::FunctionPassManager &FPM, llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                    if (Name == "lambdaize-loop") {
                        FPM.addPass(llvm::LoopSimplifyPass());
                        FPM.addPass(LambdaizeLoop());
                        return true;
                    }
                    return false;
                });
        }};
}
