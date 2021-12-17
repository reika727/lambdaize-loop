namespace {
    class LambdaizeLoop : public llvm::PassInfoMixin<LambdaizeLoop> {
    public:
        // NOTE: REQUIRES LOOPS SIMPLIFIED AND INSTRUCTIONS NAMED
        llvm::PreservedAnalyses run(llvm::Loop &Loop, llvm::LoopAnalysisManager &, llvm::LoopStandardAnalysisResults &, llvm::LPMUpdater &)
        {
            return extractLoopIntoFunction(Loop) ? llvm::PreservedAnalyses::none() : llvm::PreservedAnalyses::all();
        }

    private:
        bool extractLoopIntoFunction(llvm::Loop &Loop)
        {
            // TODO: handle nested loop
            if (!Loop.isInnermost()) {
                return false;
            }
            // TODO: handle loop with multiple exit and exiting
            if (!Loop.getExitingBlock() || !Loop.getExitBlock()) {
                return false;
            }
            // TODO: handle latch block with conditional branch
            if (!llvm::dyn_cast<llvm::BranchInst>(Loop.getLoopLatch()->getTerminator())->isUnconditional()) {
                return false;
            }
            auto *Module = Loop.getHeader()->getParent()->getParent();
            auto *Term = llvm::dyn_cast<llvm::BranchInst>(Loop.getLoopPreheader()->getTerminator());
            auto *Exit = Loop.getExitBlock();
            std::vector<llvm::Value *> Needed;
            if (auto *Extracted = createExtracted(Loop, std::back_inserter(Needed))) {
                Term->setSuccessor(0, Exit);
                llvm::IRBuilder Builder(Term);
                Needed.insert(Needed.begin(), createPassToExtracted(*Module, Extracted));
                Builder.CreateCall(getLooperFC(*Module), llvm::ArrayRef(Needed));
                return true;
            }
            return false;
        }
        template <class OutputIterator>
        llvm::Function *createExtracted(llvm::Loop &Loop, OutputIterator NeededArguments)
        {
            auto *Module = Loop.getHeader()->getParent()->getParent();
            auto &Context = Module->getContext();
            std::vector<llvm::Value *> Unreferenced;
            setUnreferencedVariables(Loop, std::back_inserter(Unreferenced));
            std::copy(Unreferenced.begin(), Unreferenced.end(), NeededArguments);
            std::vector<llvm::Type *> Types;
            std::transform(
                Unreferenced.begin(), Unreferenced.end(),
                std::back_inserter(Types),
                [](const llvm::Value *V) { return V->getType(); });
            auto *Extracted = llvm::Function::Create(
                llvm::FunctionType::get(llvm::Type::getInt1Ty(Context), llvm::ArrayRef(Types), false),
                llvm::GlobalValue::LinkageTypes::InternalLinkage,
                "extracted",
                *Module);
            for (size_t i = 0; i < Extracted->arg_size(); ++i) {
                auto *A = Extracted->getArg(i);
                A->setName(Unreferenced[i]->getName());
            }
            std::vector<llvm::BasicBlock *> BlocksFromLoop;
            if (!RemoveLoop(Loop, std::back_inserter(BlocksFromLoop))) {
                return nullptr;
            }
            for (auto *Block : BlocksFromLoop) {
                Block->insertInto(Extracted);
            }
            JustifyFunction(Extracted);
            return Extracted;
        }
        llvm::Function *createPassToExtracted(llvm::Module &Module, llvm::Function *Extracted)
        {
            auto &Context = Module.getContext();
            auto *PassToExtracted = llvm::Function::Create(
                llvm::FunctionType::get(
                    llvm::Type::getInt1Ty(Context),
                    llvm::ArrayRef<llvm::Type *>{getVaListType(Context)->getPointerTo()},
                    false),
                llvm::GlobalValue::LinkageTypes::InternalLinkage,
                "pass_to_" + Extracted->getName(),
                Module);
            llvm::IRBuilder Builder(llvm::BasicBlock::Create(Context, "", PassToExtracted));
            std::vector<llvm::Value *> Casted;
            for (auto &&A : Extracted->args()) {
                Casted.push_back(
                    Builder.CreateBitCast(
                        Builder.CreateCall(
                            getVaArgPtrFC(Module),
                            llvm::ArrayRef<llvm::Value *>(PassToExtracted->getArg(0))),
                        A.getType()));
            }
            Builder.CreateRet(Builder.CreateCall(Extracted, llvm::ArrayRef(Casted)));
            return PassToExtracted;
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
        template <class OutputIterator>
        bool RemoveLoop(llvm::Loop &Loop, OutputIterator Dest)
        {
            auto &Context = Loop.getHeader()->getParent()->getParent()->getContext();
            auto *CondBr = llvm::dyn_cast<llvm::BranchInst>(Loop.getExitingBlock()->getTerminator());
            // TODO: handle exiting block with unconditional branch
            if (!CondBr->isConditional()) {
                return false;
            }
            llvm::BasicBlock *EndBlock;
            if (auto *IfTrue = CondBr->getSuccessor(0), *IfFalse = CondBr->getSuccessor(1);
                Loop.contains(IfTrue) && !Loop.contains(IfFalse)) {
                EndBlock = llvm::BasicBlock::Create(Context, IfFalse->getName());
                auto *Cond = CondBr->getCondition();
                EndBlock->getInstList().push_back(llvm::ReturnInst::Create(Context, Cond));
            } else if (!Loop.contains(IfTrue) && Loop.contains(IfFalse)) {
                EndBlock = llvm::BasicBlock::Create(Context, IfTrue->getName());
                auto *Cond = llvm::BinaryOperator::CreateNot(CondBr->getCondition());
                EndBlock->getInstList().push_back(llvm::ReturnInst::Create(Context, Cond));
            } else /* TODO: handle !Loop.contains(IfTrue) && !Loop.contains(IfFalse) */ {
                return false;
            }
            for (auto *Block : Loop.getBlocks()) {
                Block->removeFromParent();
                if (Loop.isLoopLatch(Block)) {
                    auto *BrInst = llvm::dyn_cast<llvm::BranchInst>(Block->getTerminator());
                    for (size_t i = 0; i < BrInst->getNumSuccessors(); ++i) {
                        if (BrInst->getSuccessor(i) == Loop.getHeader()) {
                            BrInst->setSuccessor(i, EndBlock);
                        }
                    }
                }
                *Dest = Block;
            }
            *Dest = EndBlock;
            return true;
        }
        void JustifyFunction(llvm::Function *Function)
        {
            std::map<llvm::StringRef, llvm::Value *> VMap;
            for (size_t i = 0; i < Function->arg_size(); ++i) {
                auto *A = Function->getArg(i);
                VMap[A->getName()] = A;
            }
            for (auto &&Block : *Function) {
                VMap[Block.getName()] = &Block;
                for (auto &&Inst : Block) {
                    if (!Inst.getName().empty()) {
                        VMap[Inst.getName()] = &Inst;
                    }
                }
            }
            for (auto &&Block : *Function) {
                for (auto &&Inst : Block) {
                    for (auto &&Op : Inst.operands()) {
                        if (VMap.count(Op->getName())) {
                            Op = VMap[Op->getName()];
                        }
                    }
                }
            }
        }
        llvm::FunctionCallee getLooperFC(llvm::Module &Module)
        {
            const std::string Name = "looper";
            auto *Type = llvm::FunctionType::get(
                llvm::Type::getVoidTy(Module.getContext()),
                true);
            return Module.getOrInsertFunction(Name, Type);
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
