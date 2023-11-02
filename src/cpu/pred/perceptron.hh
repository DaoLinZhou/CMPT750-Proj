/* @file
 * Implementation of a perceptron branch predictor as described by Jimenez et. al in 
    https://www.cs.utexas.edu/~lin/papers/hpca01.pdf
 */

#ifndef __CPU_PRED_PERCEPTRON_PRED_HH__
#define __CPU_PRED_PERCEPTRON_PRED_HH__

#include "base/sat_counter.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/PerceptronBP.hh"

/**
 * Implements a perceptron predictor.
 */

typedef struct Perceptron {
    std::vector<signed> weights;

    Perceptron(size_t size) : weights(size) {
        weights[0] = 1;
    }
} Perceptron;

class PerceptronBP : public BPredUnit
{
  public:
    PerceptronBP(const PerceptronBPParams *params);
    void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);
    void squash(ThreadID tid, void *bp_history);
    bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);
    void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr & inst, Addr corrTarget);
    
  private:
    void updateGlobalHistReg(ThreadID tid, bool taken);
    void clamp(signed& value);

    struct BPHistory {
      unsigned globalHistoryReg;
      bool taken;
      int product;
      int perceptronIndex;
    };

    std::vector<unsigned> globalHistoryReg;
    unsigned threshold;
    unsigned tableSize;
    unsigned historyBits;
    unsigned historyRegisterMask;
    signed maxWeight;
    signed minWeight;

    // perceptron table
    std::vector<Perceptron> perceptronTable;
};

#endif // __CPU_PRED_PERCEPTRON_PRED_HH__