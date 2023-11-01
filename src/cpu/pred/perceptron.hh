
#ifndef __CPU_PRED_PERCEPTRON
#define __CPU_PRED_PERCEPTRON

#include "cpu/pred/bpred_unit.hh"
#include "base/types.hh"
#include "params/PerceptronBP.hh"

class PerceptronBP: public BPredUnit{

public:
    PerceptronBP(const PerceptronBPParams *params);

    // Base class methods.
    void uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history) override;
    bool lookup(ThreadID tid, Addr branch_addr, void* &bp_history) override;
    void btbUpdate(ThreadID tid, Addr branch_addr, void* &bp_history) override;
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr & inst,
                Addr corrTarget) override;
    virtual void squash(ThreadID tid, void *bp_history) override;
};

#endif // __CPU_PRED_PERCEPTRON