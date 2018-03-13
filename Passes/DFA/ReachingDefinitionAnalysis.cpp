/*
 * ReachingDefinitionAnalysis.cpp
 * Copyright (C) 2018-02-07 taoxuy@gmail.com <XuyangTao>
 *
 * Distributed under terms of the MIT license.
 */
#include "llvm/Pass.h"
#include <cassert>
#include <set>
#include <string>

#include "231DFA.h"
using namespace llvm;
using namespace std;


class ReachingInfo : public Info {

    public: 
        set<unsigned> dfns;

        ReachingInfo() {}
        virtual void print() {
            for (auto itr = dfns.begin(); itr != dfns.end(); ++itr) {
                errs() << *itr << '|';
            }
            errs() << '\n';
        }

        static bool equals(ReachingInfo * info1, ReachingInfo * info2) {
            return info1->dfns == info2->dfns;
        }

        static ReachingInfo * join(ReachingInfo * info1, ReachingInfo * info2, ReachingInfo * result) {
            if (result == nullptr) {
                result = new ReachingInfo();
            }
            for (unsigned dfn : info1->dfns) {
                result->dfns.insert(dfn);
            }
            for (unsigned dfn : info2->dfns) {
                result->dfns.insert(dfn);
            }
            return result;
        }
};

template <class ReachingInfo, bool Direction>
class ReachingDefinitionAnalysis : public DataFlowAnalysis<ReachingInfo, Direction> {
    public: 
        ReachingDefinitionAnalysis(ReachingInfo &bottom, ReachingInfo &initialState):
            DataFlowAnalysis<ReachingInfo, Direction>::DataFlowAnalysis(bottom, initialState) {}

        ~ReachingDefinitionAnalysis() {
            this->getEdgeToInfo().clear();
        }

    private:
        virtual void flowfunction(Instruction *I, vector<unsigned> &IncomingEdges, vector<unsigned> &OutgoingEdges, vector<ReachingInfo *> &Infos) {
            if (I == nullptr) return;

            unsigned index = this->getInstrToIndex()[I];
            string op = I->getOpcodeName();

            ReachingInfo *out_info = new ReachingInfo();
            for (unsigned fromIndex : IncomingEdges) {
                pair<unsigned, unsigned> e = make_pair(fromIndex, index);
                out_info = ReachingInfo::join(this->getEdgeToInfo()[e], out_info, out_info);
            }

            if (op == "add" || op == "fadd" || op == "sub" || op == "fsub" || op == "mul" || op == "fmul" ||
                    op == "udiv" || op == "sdiv" || op == "fdiv" || op == "urem" || op == "srem" || op == "frem" ||
                    op == "shl" || op == "lshr" || op == "ashr" || op == "and" || op == "or" || op == "xor" ||
                    op == "icmp" || op == "fcmp" ||
                    op == "alloca" || op == "load" || op == "getelementptr" || op == "select") {
                out_info->dfns.insert(index);
            }

            while (op == "phi") {
                out_info->dfns.insert(index++);
                if (this->getIndexToInstr().find(index) == this->getIndexToInstr().end()) break;
                Instruction *instr = this->getIndexToInstr()[index];
                if (string(instr->getOpcodeName()) != "phi") break;
            }
            for (int i = 0; i < Infos.size(); ++i)
                Infos[i]->dfns = out_info->dfns;
            delete out_info;
        } 


};

namespace {
    struct ReachingDefinitionAnalysisPass : public FunctionPass {
        static char ID;
        ReachingDefinitionAnalysisPass() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            ReachingInfo bottom;
            ReachingInfo initialState;
            ReachingDefinitionAnalysis<ReachingInfo, true> rda(bottom, initialState);

            rda.runWorklistAlgorithm(&F);
            rda.print();

            return false;
        }
    }; 
}
char ReachingDefinitionAnalysisPass::ID = 0;
static RegisterPass<ReachingDefinitionAnalysisPass> X("cse231-reaching", "Reaching definition on CFG",
        false,
        false);
