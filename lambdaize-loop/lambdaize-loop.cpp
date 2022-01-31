namespace {
    class LambdaizeLoop : public llvm::PassInfoMixin<LambdaizeLoop> {
    public:
        // NOTE: REQUIRES LOOPS SIMPLIFIED
        // NOTE: only loops satisfing following will be obfuscated
        //// having exactly one exit block
        //// every terminator inside is branch or switch
        llvm::PreservedAnalyses run(llvm::Loop &Loop, llvm::LoopAnalysisManager &, llvm::LoopStandardAnalysisResults &, llvm::LPMUpdater &)
        {
            if (!Loop.isLoopSimplifyForm()) {
                llvm::errs() << "Loop is not simplified.\n";
                return llvm::PreservedAnalyses::none();
            }
            return extractLoopIntoFunction(Loop) ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
        }

    private:
        bool extractLoopIntoFunction(llvm::Loop &Loop)
        {
            auto *Preheader = Loop.getLoopPreheader();
            llvm::IRBuilder Builder(Preheader->getTerminator());
            // HACK: ArgsToLooper[0] should contain pointer to Extracted, so reserve place
            std::vector<llvm::Value *> ArgsToLooper(1);
            if ((ArgsToLooper[0] = createExtracted(Loop, std::back_inserter(ArgsToLooper)))) {
                Builder.CreateCall(getLooperFC(*Preheader->getModule()), llvm::ArrayRef(ArgsToLooper));
                return true;
            }
            return false;
        }
        template <class OutputIterator>
        llvm::Function *createExtracted(llvm::Loop &Loop, OutputIterator NeededArguments)
        {
            auto *Module = Loop.getHeader()->getModule();
            auto &Context = Loop.getHeader()->getContext();
            std::vector<llvm::BasicBlock *> BlocksFromLoop;
            if (!removeLoop(Loop, std::back_inserter(BlocksFromLoop))) {
                return nullptr;
            }
            std::vector<llvm::Value *> OutsideDefined;
            setOutsideDefinedVariables(BlocksFromLoop.begin(), BlocksFromLoop.end(), std::back_inserter(OutsideDefined));
            std::copy(OutsideDefined.begin(), OutsideDefined.end(), NeededArguments);
            auto *Extracted = llvm::Function::Create(
                getExtractedFunctionType(Context),
                llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                "extracted",
                *Module);
            llvm::IRBuilder Builder(llvm::BasicBlock::Create(Context, "", Extracted));
            std::map<llvm::Value *, llvm::Value *> ArgAddrMap;
            for (auto *OD : OutsideDefined) {
                ArgAddrMap[OD] = Builder.CreateVAArg(Extracted->getArg(0), OD->getType());
            }
            Builder.CreateBr(BlocksFromLoop.front());
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
        template <class OutputIterator>
        bool removeLoop(llvm::Loop &Loop, OutputIterator Dest)
        {
            auto *OriginalHeader = Loop.getHeader(), *OriginalExit = Loop.getExitBlock();
            if (!OriginalExit) {
                return false;
            }
            for (auto *Block : Loop.blocks()) {
                if (!llvm::BranchInst::classof(Block->getTerminator()) &&
                    !llvm::SwitchInst::classof(Block->getTerminator())) {
                    return false;
                }
            }
            auto &Context = Loop.getHeader()->getContext();
            auto *LoopContinue = llvm::BasicBlock::Create(Context);
            LoopContinue->getInstList().push_back(
                llvm::ReturnInst::Create(Context, llvm::ConstantInt::getTrue(Context)));
            auto *LoopBreak = llvm::BasicBlock::Create(Context);
            LoopBreak->getInstList().push_back(
                llvm::ReturnInst::Create(Context, llvm::ConstantInt::getFalse(Context)));
            Loop.getLoopPreheader()->getTerminator()->setSuccessor(0, Loop.getExitBlock());
            std::vector<llvm::BasicBlock *> ToBeRemoved;
            for (auto *Block : Loop.blocks()) {
                auto *Term = Block->getTerminator();
                Term->replaceSuccessorWith(OriginalHeader, LoopContinue);
                Term->replaceSuccessorWith(OriginalExit, LoopBreak);
                ToBeRemoved.push_back(Block);
            }
            for (auto *Block : ToBeRemoved) {
                Block->removeFromParent();
                for (auto *LoopPtr = &Loop; LoopPtr; LoopPtr = LoopPtr->getParentLoop()) {
                    LoopPtr->removeBlockFromLoop(Block);
                }
                *Dest++ = Block;
            }
            *Dest++ = LoopContinue;
            *Dest++ = LoopBreak;
            return true;
        }
        template <class InputIterator, class OutputIterator>
        OutputIterator setOutsideDefinedVariables(InputIterator first, InputIterator last, OutputIterator result)
        {
            std::set<llvm::Value *> Declared, Arguments;
            for (auto itr = first; itr != last; ++itr) {
                for (auto &&Inst : **itr) {
                    Declared.insert(&Inst);
                    for (auto *Op : Inst.operand_values()) {
                        if (!Op->getType()->isLabelTy() &&
                            !llvm::isa<llvm::GlobalValue>(Op) &&
                            !llvm::isa<llvm::Constant>(Op)) {
                            Arguments.insert(Op);
                        }
                    }
                }
            }
            return std::set_difference(
                Arguments.begin(), Arguments.end(),
                Declared.begin(), Declared.end(),
                result);
        }
        llvm::FunctionCallee getLooperFC(llvm::Module &Module)
        {
            auto &Context = Module.getContext();
            return Module.getOrInsertFunction(
                "looper",
                llvm::FunctionType::get(
                    llvm::Type::getVoidTy(Context),
                    llvm::ArrayRef<llvm::Type *>{getExtractedFunctionType(Context)->getPointerTo()},
                    true));
        }
        llvm::FunctionType *getExtractedFunctionType(llvm::LLVMContext &Context)
        {
            return llvm::FunctionType::get(
                llvm::Type::getInt1Ty(Context),
                llvm::ArrayRef<llvm::Type *>{getVaListType(Context)->getPointerTo()},
                false);
        }
        llvm::StructType *getVaListType(llvm::LLVMContext &Context)
        {
            const std::string Name = "struct.va_list";
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
        "Lambdaize loop (under development)",
        "0.0.0",
        [](llvm::PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::FunctionPassManager &FPM, llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                    if (Name == "lambdaize-loop") {
                        FPM.addPass(llvm::LoopSimplifyPass());
                        FPM.addPass(llvm::createFunctionToLoopPassAdaptor(LambdaizeLoop()));
                        FPM.addPass(llvm::SimplifyCFGPass());
                        return true;
                    }
                    return false;
                });
        }};
}
