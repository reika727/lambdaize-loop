#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>

namespace {
    class CountCyclomaticComplexity : public llvm::PassInfoMixin<CountCyclomaticComplexity> {
    public:
        llvm::PreservedAnalyses run(llvm::Module &Module, llvm::ModuleAnalysisManager &)
        {
            int ComponentsCount = 0;
            int EdgesSum = 0, NodesSum = 0;
            for (auto &&Function : Module.functions()) {
                ++ComponentsCount;
                for (auto &&Block : Function) {
                    EdgesSum += Block.getTerminator()->getNumSuccessors();
                    ++NodesSum;
                }
            }
            llvm::errs() << "Functions Count: " << ComponentsCount << '\n';
            llvm::errs() << "Edges Count: " << EdgesSum << '\n';
            llvm::errs() << "Nodes Count: " << NodesSum << '\n';
            llvm::errs() << "Cyclomatic Complexity: " << EdgesSum - NodesSum + 2 * ComponentsCount << '\n';
            return llvm::PreservedAnalyses::none();
        }
    };
}

extern "C" LLVM_ATTRIBUTE_WEAK llvm::PassPluginLibraryInfo llvmGetPassPluginInfo()
{
    return {
        LLVM_PLUGIN_API_VERSION,
        "Count Cyclomatic Complexity",
        "1.0.0",
        [](llvm::PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](llvm::StringRef Name, llvm::ModulePassManager &MPM, llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
                    if (Name == "count-cyclomatic-complexity") {
                        MPM.addPass(CountCyclomaticComplexity());
                        return true;
                    }
                    return false;
                });
        }};
}
