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
            for (auto life : lives) errs() << life << "|";
            errs() << "\n";
        }

        static bool equal(LivenessInfo *info1, LivenessInfo *info2) {
            return info1->lives == info2->lives;
        }

        static LivenessInfo *join(LivenessInfo *info1, LivenessInfo *info2, LivenessInfo *result) {
            if (result == nullptr)
                result = new LivenessInfo();
            if (info1 != nullptr) {
                result->lives.insert(info1->lives.begin(), info1->lives.end());
            }
            if (info1 != nullptr) {
                result->lives.insert(info1->lives.begin(), info1->lives.end());
            }
            return result;
        }
};


template<class Info, bool Direction>
class LivenessAnalysis: public DataFlowAnalysis<Info, Direction> {
    public:
        LivenessAnalysis(Info &bottom, Info &initialState):
            DataFlowAnalysis<Info, Direction>::DataFlowAnalysis(bottom, initialState) {
            }

        ~LivenessAnalysis() {
        }

    private:
        int getInstrType(Instruction * I) {
            string opName = I->getOpcodeName();
            if (opName == "phi") return 3;
            if (I-> isBinaryOp() || I->isShift() || 
                    opName == "alloc" || opName == "load" || opName == "getelementptr" || 
                    opName == "icmp" || opName == "fcmp" || opName == "select") return 1;
            return 2;
        }
        void addToInfo(Info * info, Instruction * I) {
            for (int i = 0; i < I->getNumOperands(); ++i) {
                Instruction *operand = (Instruction *)I->getOperand(i);
                if (this->InstrToIndex.find(operand) == this->InstrToIndex.end()) continue;
                info->lives.insert(this->InstrToIndex[operand]);
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
                    int start = index, end = index;

                    for (; this->IndexToInstr.find(end) != this->IndexToInstr.end() ||
                            this->IndexToInstr[end] != nullptr ||
                            string(this->IndexToInstr[end]->getOpcodeName()) == "phi"; end++);
                    for (int itr = start; itr < end; ++itr) {
                        newInfo->lives.erase(itr);
                        for (int i = 0; i < Infos.size(); ++i) {
                            Infos[i]->lives.erase(itr);
                        }

                        Instruction * instr = this->IndexToInstr[itr];
                        for (int i = 0; i < instr->getNumOperands(); ++i) {
                            Instruction * var = (Instruction *)instr->getOperand(i);
                            for (int j = 0; j < Infos.size(); ++j) {
                                int out = OutgoingEdges[j];
                                if (this->IndexToInstr.find(out) == this->IndexToInstr.end() ||
                                        this->IndexToInstr[out] == nullptr ||
                                        this->InstrToIndex.find(var) == this->InstrToIndex.end() ||
                                        this->IndexToInstr[out]->getParent() != var->getParent()) continue;
                                Infos[j]->lives.insert(this->InstrToIndex[var]);
                            } 

                        }
                    }
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
