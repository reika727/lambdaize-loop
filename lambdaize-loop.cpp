namespace {
    class LambdaizeLoop : public llvm::PassInfoMixin<LambdaizeLoop> {
    public:
        // NOTE: REQUIRES LOOPS SIMPLIFIED AND INSTRUCTIONS NAMED
        // NOTE: only loops with exactly one exit block will be obfuscated
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
            llvm::IRBuilder Builder(Loop.getLoopPreheader()->getTerminator());
            // HACK: ArgsToLooper[0] should contain pointer to Extracted, so reserve place
            std::vector<llvm::Value *> ArgsToLooper(1);
            if (auto *Extracted = createExtracted(Loop, std::back_inserter(ArgsToLooper))) {
                ArgsToLooper[0] = Extracted;
                Builder.CreateCall(
                    getLooperFC(*Builder.GetInsertBlock()->getModule()),
                    llvm::ArrayRef(ArgsToLooper));
                return true;
            }
            return false;
        }
        template <class OutputIterator>
        llvm::Function *createExtracted(llvm::Loop &Loop, OutputIterator NeededArguments)
        {
            auto &Context = Loop.getHeader()->getContext();
            std::vector<llvm::Value *> OutsideDefined;
            setOutsideDefinedVariables(Loop, std::back_inserter(OutsideDefined));
            std::copy(OutsideDefined.begin(), OutsideDefined.end(), NeededArguments);
            auto *Extracted = llvm::Function::Create(
                getExtractedFunctionType(Context),
                llvm::GlobalValue::LinkageTypes::PrivateLinkage,
                "extracted",
                *Loop.getHeader()->getModule());
            std::map<llvm::StringRef, llvm::Value *> ArgAddrMap;
            llvm::IRBuilder Builder(llvm::BasicBlock::Create(Context, "", Extracted));
            for (auto *OD : OutsideDefined) {
                ArgAddrMap[OD->getName()] = Builder.CreateVAArg(Extracted->getArg(0), OD->getType());
            }
            std::vector<llvm::BasicBlock *> BlocksFromLoop;
            if (!RemoveLoop(Loop, std::back_inserter(BlocksFromLoop))) {
                return nullptr;
            }
            Builder.CreateBr(BlocksFromLoop.front());
            for (auto *Block : BlocksFromLoop) {
                for (auto &&Inst : *Block) {
                    for (auto &&Op : Inst.operands()) {
                        if (ArgAddrMap.count(Op->getName())) {
                            Op = ArgAddrMap[Op->getName()];
                        }
                    }
                }
                Block->insertInto(Extracted);
            }
            return Extracted;
        }
        template <class OutputIterator>
        OutputIterator setOutsideDefinedVariables(llvm::Loop &Loop, OutputIterator result)
        {
            std::set<llvm::Value *> Declared, Arguments;
            for (auto *Block : Loop.blocks()) {
                for (auto &&Inst : *Block) {
                    if (!Inst.getName().empty()) {
                        Declared.insert(&Inst);
                    }
                    for (auto *Op : Inst.operand_values()) {
                        if (!Op->getType()->isLabelTy() && !Op->getName().empty()) {
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
        template <class OutputIterator>
        bool RemoveLoop(llvm::Loop &Loop, OutputIterator Dest)
        {
            if (!Loop.getExitBlock()) {
                return false;
            }
            llvm::dyn_cast<llvm::BranchInst>(Loop.getLoopPreheader()->getTerminator())
                ->setSuccessor(0, Loop.getExitBlock());
            llvm::IRBuilder Builder(llvm::BasicBlock::Create(Loop.getHeader()->getContext()));
            auto *PHI = Builder.CreatePHI(Builder.getInt1Ty(), 0);
            for (auto *Block : Loop.getBlocks()) {
                auto *BrInst = llvm::dyn_cast<llvm::BranchInst>(Block->getTerminator());
                if (Loop.isLoopExiting(Block)) {
                    if (Loop.contains(BrInst->getSuccessor(0))) {
                        BrInst->setSuccessor(1, Builder.GetInsertBlock());
                        PHI->addIncoming(BrInst->getCondition(), Block);
                    } else /* Loop.contains(Condbr->getSuccessor(1)) */ {
                        BrInst->setSuccessor(0, Builder.GetInsertBlock());
                        PHI->addIncoming(Builder.CreateNot(BrInst->getCondition()), Block);
                    }
                }
                if (Loop.isLoopLatch(Block)) {
                    for (size_t i = 0; i < BrInst->getNumSuccessors(); ++i) {
                        if (BrInst->getSuccessor(i) == Loop.getHeader()) {
                            BrInst->setSuccessor(i, Builder.GetInsertBlock());
                            PHI->addIncoming(Builder.getTrue(), Block);
                        }
                    }
                }
                Block->removeFromParent();
                *Dest++ = Block;
            }
            Builder.CreateRet(PHI);
            *Dest++ = Builder.GetInsertBlock();
            return true;
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
