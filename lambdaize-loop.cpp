namespace {
    class LambdaizeLoop : public llvm::PassInfoMixin<LambdaizeLoop> {
    private:
        llvm::StructType *getVaListType(llvm::LLVMContext &Context)
        {
            const std::string Name = "struct.__va_list_tag";
            if (auto *Type = llvm::StructType::getTypeByName(Context, Name)) {
                return Type;
            }
            auto *i32 = llvm::IntegerType::getInt32Ty(Context);
            auto *i8p = llvm::IntegerType::getInt8PtrTy(Context);
            return llvm::StructType::create(Name, i32, i32, i8p, i8p);
        }
        llvm::FunctionCallee getVaArgPtrFC(llvm::Module &Module)
        {
            const std::string Name = "va_arg_ptr";
            auto *Type = llvm::FunctionType::get(
                llvm::IntegerType::getInt8PtrTy(Module.getContext()),
                llvm::ArrayRef<llvm::Type *>{
                    getVaListType(Module.getContext())->getPointerTo()},
                false);
            return Module.getOrInsertFunction(Name, Type);
        }
        llvm::FunctionCallee getLooperFC(llvm::Module &Module)
        {
            const std::string Name = "looper";
            auto *Type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(Module.getContext()),
                true);
            return Module.getOrInsertFunction(Name, Type);
        }
        template <class OutputIterator>
        OutputIterator setUnreferencedVariables(llvm::Loop &Loop, OutputIterator result)
        {
            std::vector<llvm::Value *> Declared, Arguments;
            for (auto *Block : Loop.blocks()) {
                for (auto &&Inst : *Block) {
                    if (!Inst.getName().empty()) {
                        Declared.push_back(&Inst);
                    }
                    for (auto *Op : Inst.operand_values()) {
                        if (!Op->getType()->isLabelTy() && !Op->getName().empty()) {
                            Arguments.push_back(Op);
                        }
                    }
                }
            }
            auto compValueByName = [](llvm::Value *V1, llvm::Value *V2) {
                return V1->getName().compare(V2->getName()) < 0;
            };
            std::sort(Declared.begin(), Declared.end(), compValueByName);
            Declared.erase(std::unique(Declared.begin(), Declared.end()), Declared.end());
            std::sort(Arguments.begin(), Arguments.end(), compValueByName);
            Arguments.erase(std::unique(Arguments.begin(), Arguments.end()), Arguments.end());
            return std::set_difference(
                Arguments.begin(), Arguments.end(),
                Declared.begin(), Declared.end(),
                result,
                compValueByName);
        }
        void extractLoopIntoFunction(llvm::Loop &Loop, std::string BaseName = "extracted")
        {
            static auto Count = 0;
            // TODO: handle nested loop
            if (!Loop.isInnermost()) {
                return;
            }
            // TODO: handle loop with multiple exit and exiting
            if (!Loop.getExitingBlock() || !Loop.getExitBlock()) {
                return;
            }
            // TODO: handle latch block with conditional branch
            if (!llvm::dyn_cast<llvm::BranchInst>(Loop.getLoopLatch()->getTerminator())->isUnconditional()) {
                return;
            }
            // TODO: handle exiting block with unconditional branch
            auto *CondBr = llvm::dyn_cast<llvm::BranchInst>(Loop.getExitingBlock()->getTerminator());
            if (!CondBr->isConditional()) {
                return;
            }
            auto *IfTrue = CondBr->getSuccessor(0);
            auto *IfFalse = CondBr->getSuccessor(1);
            bool IsContinueCondition;
            if (Loop.contains(IfTrue) && !Loop.contains(IfFalse)) {
                IsContinueCondition = true;
            } else if (!Loop.contains(IfTrue) && Loop.contains(IfFalse)) {
                IsContinueCondition = false;
            } else /* !Loop.contains(IfTrue) && !Loop.contains(IfFalse) */ {
                return;
            }
            // detour loop
            auto *Term = llvm::dyn_cast<llvm::BranchInst>(Loop.getLoopPreheader()->getTerminator());
            Term->setSuccessor(0, Loop.getExitBlock());
            // make "extracted"
            auto *Module = Loop.getHeader()->getParent()->getParent();
            auto &Context = Module->getContext();
            std::vector<llvm::Value *> Args;
            setUnreferencedVariables(Loop, std::back_inserter(Args));
            std::vector<llvm::Type *> Types;
            std::transform(
                Args.begin(), Args.end(),
                std::back_inserter(Types),
                [](const llvm::Value *V) {
                    return V->getType();
                });
            auto *Extracted = llvm::Function::Create(
                llvm::FunctionType::get(
                    llvm::Type::getInt1Ty(Context),
                    llvm::ArrayRef(Types),
                    false),
                llvm::GlobalValue::LinkageTypes::InternalLinkage,
                BaseName + std::to_string(++Count),
                *Module);
            // create function body
            for (auto *Block : Loop.getBlocks()) {
                Block->removeFromParent();
                Block->insertInto(Extracted);
            }
            auto Builder = llvm::IRBuilder(
                llvm::BasicBlock::Create(
                    Context,
                    IsContinueCondition ? IfFalse->getName() : IfTrue->getName(),
                    Extracted));
            Builder.CreateRet(
                IsContinueCondition
                    ? CondBr->getCondition()
                    : llvm::BinaryOperator::CreateNot(CondBr->getCondition()));
            // redirect latch block to end block
            for (auto &&Block : *Extracted) {
                if (auto *BrInst = llvm::dyn_cast<llvm::BranchInst>(Block.getTerminator())) {
                    for (size_t i = 0; i < BrInst->getNumSuccessors(); ++i) {
                        if (BrInst->getSuccessor(i)->isEntryBlock()) {
                            BrInst->setSuccessor(i, &Extracted->back());
                        }
                    }
                }
            }
            // reset values
            std::map<llvm::StringRef, llvm::Value *> VMap;
            for (size_t i = 0; i < Extracted->arg_size(); ++i) {
                auto *A = Extracted->getArg(i);
                A->setName(Args[i]->getName());
                VMap[A->getName()] = A;
            }
            for (auto &&Block : *Extracted) {
                VMap[Block.getName()] = &Block;
                for (auto &&Inst : Block) {
                    if (!Inst.getName().empty()) {
                        VMap[Inst.getName()] = &Inst;
                    }
                }
            }
            for (auto &&Block : *Extracted) {
                for (auto &&Inst : Block) {
                    for (auto &&Op : Inst.operands()) {
                        if (VMap.count(Op->getName())) {
                            Op = VMap[Op->getName()];
                        }
                    }
                }
            }
            // make "pass_to_extracted"
            auto *PassToExtracted = llvm::Function::Create(
                llvm::FunctionType::get(
                    llvm::Type::getInt1Ty(Context),
                    llvm::ArrayRef<llvm::Type *>{getVaListType(Context)->getPointerTo()},
                    false),
                llvm::GlobalValue::LinkageTypes::InternalLinkage,
                "pass_to_" + BaseName + std::to_string(Count),
                *Module);
            Builder.SetInsertPoint(llvm::BasicBlock::Create(Context, "entry", PassToExtracted));
            std::vector<llvm::Value *> Casted;
            for (auto *T : Types) {
                Casted.push_back(
                    Builder.CreateBitCast(
                        Builder.CreateCall(
                            getVaArgPtrFC(*Module),
                            llvm::ArrayRef<llvm::Value *>(PassToExtracted->getArg(0))),
                        T));
            }
            Builder.CreateRet(Builder.CreateCall(Extracted, llvm::ArrayRef(Casted)));
            // call looper
            Args.insert(Args.begin(), PassToExtracted);
            Builder.SetInsertPoint(Term);
            Builder.CreateCall(getLooperFC(*Module), llvm::ArrayRef(Args));
        }

    public:
        // NOTE: REQUIRES LOOPS SIMPLIFIED AND INSTRUCTIONS NAMED
        llvm::PreservedAnalyses run(llvm::Loop &Loop, llvm::LoopAnalysisManager &, llvm::LoopStandardAnalysisResults &, llvm::LPMUpdater &)
        {
            extractLoopIntoFunction(Loop);
            // TODO: research return value
            return llvm::PreservedAnalyses::none();
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK llvm::PassPluginLibraryInfo llvmGetPassPluginInfo()
{
    return {
        LLVM_PLUGIN_API_VERSION,
        "Lambdaize loop (under development)",
        LLVM_VERSION_STRING,
        [](llvm::PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::FunctionPassManager &FPM, llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                    if (Name == "lambdaize-loop") {
                        FPM.addPass(llvm::LoopSimplifyPass());
                        FPM.addPass(llvm::InstructionNamerPass());
                        FPM.addPass(llvm::createFunctionToLoopPassAdaptor(LambdaizeLoop()));
                        return true;
                    }
                    return false;
                });
        }};
}
