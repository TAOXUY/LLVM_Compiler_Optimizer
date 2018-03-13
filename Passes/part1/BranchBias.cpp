#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace std;

namespace {
    struct BranchBias : public FunctionPass {
        static char ID;
        BranchBias() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            Module *M = F.getParent();
            LLVMContext &C = M->getContext();

            Function *update_func = (Function *)M->getOrInsertFunction(
                    "updateBranchInfo", 
                    Type::getVoidTy(C), 
                    IntegerType::get(C, 1));
        
            Function *print_func = (Function *)M->getOrInsertFunction(
                    "printOutBranchInfo", 
                    Type::getVoidTy(C));

            for (Function::iterator blk_itr = F.begin(), blk_end = F.end(); blk_itr != blk_end; ++blk_itr) {
                IRBuilder<> Builder(&*blk_itr);
                Builder.SetInsertPoint(blk_itr->getTerminator());

                BranchInst *branch_inst = dyn_cast<BranchInst>(blk_itr->getTerminator());
                if (branch_inst != NULL && branch_inst->isConditional()) {
                    vector<Value *> args;
                    args.push_back(branch_inst->getCondition());
                    Builder.CreateCall(update_func, args);
                }

                for (BasicBlock::iterator ins_itr = blk_itr->begin(), ins_end = blk_itr->end(); ins_itr != ins_end; ++ins_itr) {
                    if ((string) ins_itr->getOpcodeName() == "ret") {
                        Builder.SetInsertPoint(&*ins_itr);
                        Builder.CreateCall(print_func);
                    }
                }
            }

            return false;
        }
    }; 
}  

char BranchBias::ID = 0;
static RegisterPass<BranchBias> X("cse231-bb", "Compute Branch Bias",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
