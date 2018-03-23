/*
 * MayPointToAnalysis.cpp
 * Copyright (C) 2018-03-14 taoxuy@gmail.com <XuyangTao>
 *
 * Distributed under terms of the MIT license.
 */
#include <iterator>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <algorithm>
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/InitializePasses.h"
#include <stdio.h>
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Module.h"
#include "231DFA.h"
#include <sstream>
using namespace std;
using namespace llvm;



class MayPointToInfo : public Info {
    public:
        map<string, set<string>> ptrMap;

        static int getFirstInt(string str) {
            int res = 0;
            for (int i = 0; i < str.length(); ++i) {
                if (str[i] - '0' < 0 || str[i] - '0' > 9) {
                    if (res == 0) continue;
                    return res;
                }
                res = res * 10 + (str[i] - '0');
            }
            return res;
        }
        void print() {
            vector<string> data;
            for (auto iter = ptrMap.begin(); iter != ptrMap.end(); ++iter) {
                string R = iter->first; 
                string combinedM = "";
                for (string M : iter->second) {
                    combinedM = combinedM + M + "/";
                }
                if (combinedM == "") continue;
                data.push_back(R + "->(" + combinedM + ")");
            }
            std::sort(data.begin(), data.end(), [](string &a, string &b) -> bool {
                    if (a[0] == 'R' && b[0] == 'M') return -1;
                    if (a[0] == 'M' && b[0] == 'R') return 1;
                    int a_r = getFirstInt(a), b_r = getFirstInt(b);
                    return a_r - b_r < 0 ?  1 : 0;
                    });
            for (int i = 0; i < data.size(); ++i) {
                errs() << data[i] << "|";
            }
            errs() << "\n";
        }

        static bool equals(MayPointToInfo *info1, MayPointToInfo *info2) {
            if (info1->ptrMap.size() != info2->ptrMap.size()) return false;
            auto lhs = info1->ptrMap;
            auto rhs = info2->ptrMap;
            return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
        }

        static void combineMayPointToInfo(MayPointToInfo * result, MayPointToInfo * info) {
            for (auto itr = info->ptrMap.begin(); itr != info->ptrMap.end(); ++itr) {
                string var = itr -> first;
                if (result->ptrMap.find(var) != result->ptrMap.end()) {
                    result->ptrMap[var].insert(info->ptrMap[var].begin(), info->ptrMap[var].end());
                } else {
                    result->ptrMap[var] = info->ptrMap[var];
                }
            }
        } 

        static MayPointToInfo *join(MayPointToInfo *info1, MayPointToInfo *info2, MayPointToInfo *result) {
            if (result == nullptr) {
                result = new MayPointToInfo();
            }
            if (info1 != nullptr) {
                combineMayPointToInfo(result, info1);
            }
            if (info2 != nullptr) {
                combineMayPointToInfo(result, info2);
            }
            return result;
        }
};




template<class Info, bool Direction>
class MayPointToAnalysis: public DataFlowAnalysis<Info, Direction> {
    private:
        void handleAlloca(int index, Info * newInfo, Instruction *I, vector<unsigned> &IncomingEdges, vector<unsigned> &OutgoingEdges, vector<Info *> &Infos) {
            string from = "R" + to_string(index), to = "M" + to_string(index);	
            newInfo->ptrMap[from].insert(to);
        }

        void handleBitCastGetelementptr(int index, Info * newInfo, Instruction *I, vector<unsigned> &IncomingEdges, vector<unsigned> &OutgoingEdges, vector<Info *> &Infos) {
            string from = "R" + to_string(index);
            Instruction *instr = (Instruction *)I->getOperand(0);
            if (this->InstrToIndex.find(instr) == this->InstrToIndex.end()) return; 
            unsigned v = this->InstrToIndex[instr];
            string RV = "R" + to_string(v);
            newInfo->ptrMap[from].insert(newInfo->ptrMap[RV].begin(), newInfo->ptrMap[RV].end());
        }
        void handleLoad(int index, Info * newInfo, Instruction *I, vector<unsigned> &IncomingEdges, vector<unsigned> &OutgoingEdges, vector<Info *> &Infos){
            if (!I->getType()->isPointerTy()) return;
            string from = "R" + to_string(index);
            Instruction *instr = (Instruction *)I->getOperand(0);
            if (this->InstrToIndex.find(instr) == this->InstrToIndex.end()) return;
            unsigned p = this->InstrToIndex[instr];
            string RP = "R" + to_string(p);
            set<string> tos;
            for (string x : newInfo->ptrMap[RP])
                tos.insert(newInfo->ptrMap[x].begin(), newInfo->ptrMap[x].end());
            newInfo->ptrMap[from].insert(tos.begin(), tos.end());
        }

        void handleStore(int index, Info * newInfo, Instruction *I, vector<unsigned> &IncomingEdges, vector<unsigned> &OutgoingEdges, vector<Info *> &Infos){
            Instruction *instr0 = (Instruction*)(((StoreInst*)I)->getValueOperand()), *instr1 = (Instruction*)(((StoreInst*)I)->getPointerOperand());
            if (!this->InstrToIndex.count(instr0) || !this->InstrToIndex.count(instr1)) return;
            unsigned v = this->InstrToIndex[instr0], p = this->InstrToIndex[instr1];
            string RV = "R" + to_string(v), RP = "R" + to_string(p);
            set<string> tos;
            for (string x : newInfo->ptrMap[RV]) {
                tos.insert(x);
            }
            for (string y : newInfo->ptrMap[RP]) {
                newInfo->ptrMap[y].insert(tos.begin(), tos.end());
            }
        }

        void handleSelect(int index, Info * newInfo, Instruction *I, vector<unsigned> &IncomingEdges, vector<unsigned> &OutgoingEdges, vector<Info *> &Infos) {
            string from = "R" + to_string(index);
            Instruction *instr1 = (Instruction *)I->getOperand(1), *instr2 = (Instruction *)I->getOperand(2);
            set<string> tos;
            if (this->InstrToIndex.count(instr1)) {
                string R1 = "R" + to_string(this->InstrToIndex[instr1]);
                tos.insert(newInfo->ptrMap[R1].begin(), newInfo->ptrMap[R1].end());
            }
            if (this->InstrToIndex.count(instr2)) {
                string R2 = "R" + to_string(this->InstrToIndex[instr2]);
                tos.insert(newInfo->ptrMap[R2].begin(), newInfo->ptrMap[R2].end());
            }
            newInfo->ptrMap[from].insert(tos.begin(), tos.end());
        }

        void handlePhi(int index, Info * newInfo, Instruction *I, vector<unsigned> &IncomingEdges, vector<unsigned> &OutgoingEdges, vector<Info *> &Infos) {
            int lo = index, hi = index;
            while (this->IndexToInstr.find(hi) != this->IndexToInstr.end() &&
                    this->IndexToInstr[hi] != nullptr &&
                    string(this->IndexToInstr[hi]->getOpcodeName()) == "phi") hi++;
            for (int idx = lo; idx < hi; ++idx) {
                string from = "R" + to_string(index);
                set<string> tos;
                unsigned num = I->getNumOperands();
                for (int k = 0; k < num; ++k) {
                    Instruction *instRK = (Instruction *)I->getOperand(k);
                    if (!this->InstrToIndex.count(instRK)) continue;
                    string RK = "R" + to_string(this->InstrToIndex[instRK]);
                    tos.insert(newInfo->ptrMap[RK].begin(), newInfo->ptrMap[RK].end());
                }
                newInfo->ptrMap[from].insert(tos.begin(), tos.end());
            }
        }

        virtual void flowfunction(Instruction *I, vector<unsigned> &IncomingEdges, vector<unsigned> &OutgoingEdges, vector<Info *> &Infos) {
            if (I == nullptr) return;
            unsigned index = this->InstrToIndex[I];
            string opName = I->getOpcodeName();

            Info *newInfo = new Info();
            for (unsigned src : IncomingEdges) {
                pair<unsigned, unsigned> e = make_pair(src, index);
                newInfo = Info::join(this->EdgeToInfo[e], newInfo, newInfo);
            }
            if (opName == "alloca") handleAlloca(index, newInfo, I, IncomingEdges, OutgoingEdges, Infos);
            if (opName == "bitcast" || opName == "getelementptr")handleBitCastGetelementptr(index, newInfo, I, IncomingEdges, OutgoingEdges, Infos);
            if (opName == "load") handleLoad(index, newInfo, I, IncomingEdges, OutgoingEdges, Infos);
            if (opName == "store") handleStore(index, newInfo, I, IncomingEdges, OutgoingEdges, Infos);
            if (opName == "select") handleSelect(index, newInfo, I, IncomingEdges, OutgoingEdges, Infos);
            if (opName == "phi") handlePhi(index, newInfo, I, IncomingEdges, OutgoingEdges, Infos);

            for (int i = 0; i < Infos.size(); ++i)
                Info::join(Infos[i], newInfo, Infos[i]);
            delete newInfo;
        }	
    public:
        MayPointToAnalysis(Info & bottom, Info &initialState): DataFlowAnalysis<Info, Direction>::DataFlowAnalysis(bottom, initialState) {}
        ~MayPointToAnalysis() {}
};
struct MayPointToAnalysisPass : public FunctionPass {
    static char ID;
    MayPointToAnalysisPass() : FunctionPass(ID) {}
    bool runOnFunction(Function &F) override {
        MayPointToInfo bottom;
        MayPointToAnalysis <MayPointToInfo, true> mpa(bottom, bottom);
        mpa.runWorklistAlgorithm(&F);
        mpa.print();
        return false;
    }

};
char MayPointToAnalysisPass::ID = 0;
static RegisterPass<MayPointToAnalysisPass> X("cse231-maypointto", "abc", false, false);
