#include "llvm/ADT/SmallVector.h"
#include "llvm/Pass.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <vector>

using namespace llvm;
using namespace std;

namespace {
    struct CountDynamicInstructions : public FunctionPass {
        static char ID;
        CountDynamicInstructions() : FunctionPass(ID) {}

        bool runOnFunction(Function &func) override {
            Module *module = func.getParent();
            LLVMContext &context = module->getContext();
            
            Function *update_func = (Function *)module->getOrInsertFunction(
                    "updateInstrInfo",
                    Type::getVoidTy(context),
                    Type::getInt32Ty(context),
                    Type::getInt32PtrTy(context),
                    Type::getInt32PtrTy(context)
                    );

            Function *print_func = (Function *)module->getOrInsertFunction(
                    "printOutInstrInfo",
                    Type::getVoidTy(context)
                    );

            for (Function::iterator blk_itr = func.begin(), blk_end = func.end(); blk_itr != blk_end; ++blk_itr) {
                map<int, int> op_counter;

                for (BasicBlock::iterator ins_itr = blk_itr->begin(), end = blk_itr->end(); ins_itr != end; ++ins_itr) {
                    ++op_counter[ins_itr->getOpcode()];
                }

                
                int num = op_counter.size();
                vector<Value *> args;
                vector<Constant *> keys;
                vector<Constant *> values;
                
                for (map<int, int>::iterator iter = op_counter.begin(), end = op_counter.end(); iter != end; ++iter) {
                    keys.push_back(ConstantInt::get(Type::getInt32Ty(context), iter->first));
                    values.push_back(ConstantInt::get(Type::getInt32Ty(context), iter->second));
                }

                IRBuilder<> Builder(&*blk_itr);
                ArrayType *array_type = ArrayType::get(Type::getInt32Ty(context), num);
                Value* cast_list[2] = {ConstantInt::get(Type::getInt32Ty(context), 0), ConstantInt::get(Type::getInt32Ty(context), 0)};

                args.push_back(ConstantInt::get(Type::getInt32Ty(context), num));
                args.push_back(Builder.CreateInBoundsGEP(
                    new GlobalVariable(*module, array_type, true, 
                    GlobalVariable::InternalLinkage, ConstantArray::get(array_type, keys)), 
                    cast_list));
                args.push_back(Builder.CreateInBoundsGEP(
                    new GlobalVariable(*module, ArrayType::get(Type::getInt32Ty(context), num), true, 
                    GlobalVariable::InternalLinkage, ConstantArray::get(array_type, values))
                    , cast_list));

                
                Builder.SetInsertPoint(blk_itr->getTerminator());
                Builder.CreateCall(update_func, args);

                for (BasicBlock::iterator ins_itr = blk_itr->begin(), end = blk_itr->end(); ins_itr != end; ++ins_itr) {
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

char CountDynamicInstructions::ID = 0;
static RegisterPass<CountDynamicInstructions> X("cse231-cdi", "Count Dynamic Instructions",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
