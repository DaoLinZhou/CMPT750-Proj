/* @file
 * Implementation of a perceptron branch predictor as described by Jimenez et. al in 
    https://www.cs.utexas.edu/~lin/papers/hpca01.pdf
 */

#include "cpu/pred/perceptron.hh"

#include "base/bitfield.hh"
#include "base/intmath.hh"

void
PerceptronBP::clamp(signed& value) {
    if (value < minWeight) value = minWeight;
    else if (value > maxWeight) value = maxWeight;
}

PerceptronBP::PerceptronBP(const PerceptronBPParams *params)
    : BPredUnit(params),
      globalHistoryReg(params->numThreads, 0),
      tableSize(params->tableSize),
      historyBits(ceilLog2(params->tableSize)),
      perceptronTable(params->tableSize, Perceptron(historyBits))
{
    if (!isPowerOf2(tableSize))
        fatal("Invalid perceptron predictor table size.\n");

    historyRegisterMask = mask(historyBits);
    threshold = 1.93*historyBits + 14; // Optimal threshold as per Jimenez et.
    maxWeight = (1<<(historyBits-1))-1;
    minWeight = -(maxWeight+1);
}


void
PerceptronBP::uncondBranch(ThreadID tid, Addr pc, void * &bpHistory)
{
    // Set the corresponding perceptron to max weight and set taken to true for unconditionals
    unsigned perceptronIndex = (pc << instShiftAmt) & historyRegisterMask;
    assert(perceptronIndex < tableSize);
    BPHistory *history = new BPHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    history->taken = true;
    history->globalHistoryReg = maxWeight;
    history->perceptronIndex = perceptronIndex;
    bpHistory = static_cast<void*>(history);
    updateGlobalHistReg(tid, true);
    return;
}


void
PerceptronBP::squash(ThreadID tid, void *bpHistory)
{
    BPHistory *history = static_cast<BPHistory*>(bpHistory);
    globalHistoryReg[tid] = history->globalHistoryReg;

    delete history;
    return;
}


bool
PerceptronBP::lookup(ThreadID tid, Addr branchAddr, void * &bpHistory)
{
    unsigned perceptronIndex = (branchAddr << instShiftAmt) & historyRegisterMask;
    assert(perceptronIndex < tableSize);

    Perceptron perceptron = perceptronTable[perceptronIndex];
    signed product = perceptron.weights[0]; // Initialize the product to the bias weight

    for(unsigned i=1, mask=1; i<historyBits; i++, mask<<=1) {
        if(globalHistoryReg[tid] & mask) {
            // If the GHR entry was taken (1), add the weight
            product += perceptron.weights[i];
        }
        else {
            // If the GHR entry was not taken (-1), subtract the weight
            product -= perceptron.weights[i];
        }
    }

    //TODO: Implement Debugging Support
    //std::cout << "Perceptron Entry " << perceptronIndex <<", calculated product " << product << std::endl;
    uint8_t prediction = product >= 0;

    BPHistory *history = new BPHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    history->perceptronIndex = perceptronIndex;
    history->taken = prediction;
    history->product = product;
    bpHistory = static_cast<void*>(history);

    return prediction;
}


void
PerceptronBP::btbUpdate(ThreadID tid, Addr branchAddr, void * &bpHistory)
{
    globalHistoryReg[tid] &= (historyRegisterMask & ~ULL(1));
}


void
PerceptronBP::update(ThreadID tid, Addr branchAddr, bool taken, void *bpHistory,
                 bool squashed, const StaticInstPtr & inst, Addr corrTarget)
{
    assert(bpHistory);

    BPHistory *history = static_cast<BPHistory*>(bpHistory);

    // We do not update the counters speculatively on a squash.
    // We just restore the global history register.
    if (squashed) {
        globalHistoryReg[tid] = (history->globalHistoryReg << 1) | taken;
        return;
    }

    globalHistoryReg[tid] = (globalHistoryReg[tid] << 1) | taken;

    if((history->product >= threshold && taken) || (history->product <= threshold && !taken)) {
        // If the product is >= or <= threshold and prediction was correct, no need to update weights
        return;
    }

    Perceptron *perc = &perceptronTable[history->perceptronIndex];

    // Update bias weight based on taken
    perc->weights[0] = taken ? perc->weights[0]+1 : perc->weights[0]-1;
    clamp(perc->weights[0]);

    for (unsigned i=1, mask=1; i<historyBits; i++, mask<<=1) {
        // A common trick to convert to boolean => !!x is 1 iff x is not zero, in this case history is 
        // positively correlated with branch outcome (Taken from Jimenez et. al ChampSim implementation)
        if (!!(history->globalHistoryReg & mask) == taken) { 
            perc->weights[i] = perc->weights[i] + 1;
        } else {
            perc->weights[i] = perc->weights[i] - 1;
        }
        clamp(perc->weights[i]);
    }

    delete history;
    return;
}

void
PerceptronBP::updateGlobalHistReg(ThreadID tid, bool taken)
{
    globalHistoryReg[tid] = taken ? (globalHistoryReg[tid] << 1) | 1 :
                               (globalHistoryReg[tid] << 1);
    globalHistoryReg[tid] &= historyRegisterMask;
}


PerceptronBP*
PerceptronBPParams::create()
{
    return new PerceptronBP(this);
}
