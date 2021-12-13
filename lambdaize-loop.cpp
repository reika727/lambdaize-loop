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
        std::vector<llvm::Value *> getUnreferencedVariables(llvm::Loop &Loop)
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
            std::sort(Arguments.begin(), Arguments.end(), compValueByName);
            std::vector<llvm::Value *> Unreferenced;
            std::set_difference(
                Arguments.begin(), Arguments.end(),
                Declared.begin(), Declared.end(),
                std::inserter(Unreferenced, Unreferenced.begin()),
                compValueByName);
            Unreferenced.erase(
                std::unique(Unreferenced.begin(), Unreferenced.end()),
                Unreferenced.end());
            return Unreferenced;
        }
        llvm::Function *extractLoopIntoFunction(llvm::Loop &Loop, std::string BaseName = "extracted" /*, std::string EntryLabel = "entry"*/)
        {
            // TODO: handle nested loop
            if (!Loop.isInnermost()) {
                return nullptr;
            }
            // TODO: handle loop with multiple exit and exiting
            if (!Loop.getExitingBlock() || !Loop.getExitBlock()) {
                return nullptr;
            }
            static int Count = 0;
            auto *Module = Loop.getHeader()->getParent()->getParent();
            auto &Context = Module->getContext();
            // make "extracted"
            auto *Extracted = llvm::Function::Create(
                llvm::FunctionType::get(
                    llvm::Type::getInt1Ty(Context),
                    llvm::ArrayRef<llvm::Type *>{getVaListType(Context)->getPointerTo()},
                    false),
                /*llvm::GlobalValue::LinkageTypes::InternalLinkage*/ llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                BaseName + std::to_string(++Count),
                /*nullptr*/ *Module);
            // call looper insteed of process loop
            auto Args = getUnreferencedVariables(Loop);
            Args.insert(Args.begin(), Extracted);
            auto *Term = llvm::dyn_cast<llvm::BranchInst>(Loop.getLoopPreheader()->getTerminator());
            Term->setSuccessor(0, Loop.getExitBlock());
            auto Builder = llvm::IRBuilder(Context);
            Builder.SetInsertPoint(Term);
            Builder.CreateCall(
                getLooperFC(*Module),
                llvm::ArrayRef<llvm::Value *>(Args));
            // move loop-blocks to "extracted"
            for (auto *Block : Loop.getBlocks()) {
                Block->eraseFromParent();
                // Block->removeFromParent();
                // Block->insertInto(Extracted);
            }
            return Extracted;
        }

    public:
        // NOTE: REQUIRES LOOP SIMPLIFIED AND INSTRUCTIONS NAMED
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
