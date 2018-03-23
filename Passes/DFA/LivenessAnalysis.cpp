/*
 * LivenessAnalysis.cpp
         * Copyright (C) 2018-03-13 taoxuy@gmail.com <XuyangTao>
 *
 * Distributed under terms of the MIT license.
 */
#include <vector>
#include <map>
#include <set>
#include <string>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/InitializePasses.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "231DFA.h"
using namespace std;
using namespace llvm;


class LivenessInfo : public Info {
    public:
        set<unsigned> lives;
        void print() {
            for (auto life : lives) {
                errs() << life << "|";
            }
            errs() << "\n";
        }

        static bool equals(LivenessInfo *info1, LivenessInfo *info2) {
            return info1->lives == info2->lives;
        }

        static LivenessInfo *join(LivenessInfo *info1, LivenessInfo *info2, LivenessInfo *result) {
            if (result == nullptr) {
                result = new LivenessInfo();
            }
            if (info1 != nullptr && info1 != result) {
                result->lives.insert(info1->lives.begin(), info1->lives.end());
            }
            if (info2 != nullptr && info2 != result) {
                result->lives.insert(info2->lives.begin(), info2->lives.end());
            }
            return result;
        }
};


template<class Info, bool Direction>
class LivenessAnalysis: public DataFlowAnalysis<Info, Direction> {
    public:
        LivenessAnalysis(Info &bottom, Info &initialState):
            DataFlowAnalysis<Info, Direction>::DataFlowAnalysis(bottom, initialState) {}

        ~LivenessAnalysis() {}

    private:
        int getInstrType(Instruction * I) {
            string opName = I->getOpcodeName();
            if (opName == "phi") return 3;
            if (opName == "icmp" || opName == "fcmp" ||
                    opName == "add" || opName == "fadd" || opName == "sub" || opName == "fsub" || opName == "mul" || opName == "fmul" ||
                    opName == "shl" || opName == "lshr" || opName == "ashr" || opName == "and" || opName == "or" || opName == "xor" ||
                    opName == "udiv" || opName == "sdiv" || opName == "fdiv" || opName == "urem" || opName == "srem" || opName == "frem" ||
                    opName == "alloca" || opName == "load" || opName == "getelementptr" || opName == "select" ) return 1;
            return 2;
        }
        void addToInfo(Info * info, Instruction * I) {
            for (int i = 0; i < I->getNumOperands(); ++i) {
                Instruction *operand = (Instruction *)I->getOperand(i);
                if (this->InstrToIndex.find(operand) == this->InstrToIndex.end()) continue;
                info->lives.insert(this->InstrToIndex[operand]);
            }
        }
        void handlePhi(Info* newInfo, int index, Instruction *I, vector<unsigned> &IncomingEdges, vector<unsigned> &OutgoingEdges, vector<Info *> &Infos) {

            int start = index, end = index;
            while (this->IndexToInstr.find(end) != this->IndexToInstr.end() &&
                        this->IndexToInstr[end] != nullptr &&                        
                        string(this->IndexToInstr[end]->getOpcodeName()) == "phi") {
                ++end;
            }

            for (int i = start; i < end; ++i) {
                for (int j = 0; j < Infos.size(); ++j) {
                    Infos[j]->lives.erase(i);
                }
                newInfo->lives.erase(i);
            }

            BasicBlock* B = I->getParent();
            for (auto itr = B->begin(), end = B->end(); itr != end ; ++itr) {
                Instruction * instr = &*itr;
                if (!isa<PHINode>(instr)) continue;
                PHINode* pInstr = (PHINode*) instr;
                for(unsigned i = 0, e = pInstr->getNumIncomingValues() ;
                        i < e ; i++){
                    Instruction* pValue = (Instruction*)(pInstr->getIncomingValue(i));
                    if(this->InstrToIndex.find((Instruction*) pValue) == this->InstrToIndex.end()){
                        continue;
                    }
                    Instruction* prevInstr = (Instruction *)pInstr->getIncomingBlock(i)->getTerminator();
                    unsigned prevInstrIndex = this->InstrToIndex[prevInstr];

                    for(int out = 0, outEnd = OutgoingEdges.size() ; out < outEnd ; out++){
                        if(OutgoingEdges[out] != prevInstrIndex) continue;
                        Infos[out]->lives.insert(this->InstrToIndex[pValue]);
                    }
                }
            }
        }


        virtual void flowfunction(Instruction *I, vector<unsigned> &IncomingEdges, vector<unsigned> &OutgoingEdges, vector<Info *> &Infos) {
            if (I == nullptr) return;
            unsigned index = this->InstrToIndex[I];
            Info * newInfo = new Info();
            for (unsigned src : IncomingEdges) {
                pair<unsigned, unsigned> e = make_pair(src, index);
                Info::join(this->EdgeToInfo[e], nullptr, newInfo);
            }

            switch(getInstrType(I)) {
                case 1:
                    newInfo->lives.erase(index);
                    addToInfo(newInfo, I);
                    break;
                case 2 :
                    addToInfo(newInfo, I);
                    break;
                case 3:
                    handlePhi(newInfo, index, I, IncomingEdges, OutgoingEdges, Infos);
                    break;

            } 
            for (int i = 0; i < Infos.size(); ++i) 
                Infos[i]->lives.insert(newInfo->lives.begin(), newInfo->lives.end());
            delete newInfo;

        }
};
namespace {
    struct LivenessAnalysisPass : public FunctionPass {
        public:
            static char ID;
            LivenessAnalysisPass() : FunctionPass(ID) {}
            bool runOnFunction(Function &F) override { 
                LivenessInfo * bottom = new LivenessInfo();
                LivenessInfo * init = new LivenessInfo();
                LivenessAnalysis<LivenessInfo, false> la(*bottom, *init);
                la.runWorklistAlgorithm(&F);
                la.print();
                return false;
            }
    }; 
}
char LivenessAnalysisPass::ID = 0;
static RegisterPass<LivenessAnalysisPass> X("cse231-liveness",
        "",
        false,
        false);
