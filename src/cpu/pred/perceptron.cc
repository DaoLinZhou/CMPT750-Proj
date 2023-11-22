

#include "cpu/pred/perceptron.hh"
#include "debug/Perceptron.hh"

PerceptronBP::~PerceptronBP(){
    delete[] globalHistoryReg;
    for(auto& table: preceptronTable){
        delete[] table;
    }
}

PerceptronBP::PerceptronBP(const PerceptronBPParams *params) 
    : BPredUnit(params), 
    globalHistoryReg(new GHR[params->numThreads]),
    preceptronTable(params->numThreads),
    tableSize(params->perceptronTableSize)
{
    for(uint32_t i = 0; i < params->numThreads; i++){
        globalHistoryReg[i].Init(params->globalHistorySize);
    }
    for(auto& table: preceptronTable){
        table = new Perceptron[params->perceptronTableSize];
        for(uint32_t i = 0; i < params->perceptronTableSize; i++){
            table[i].Init(params->globalHistorySize, params->minWeight, params->maxWeight, params->threshold);
        }
    }
}

// PREDICTOR UPDATE
void
PerceptronBP::update(ThreadID tid, Addr branch_pc, bool taken, void* bp_history,
              bool squashed, const StaticInstPtr & inst, Addr corrTarget)
{
    if(!bp_history) return;
    
    BPHistory *history = static_cast<BPHistory*>(bp_history);
    auto& globalHistory = globalHistoryReg[tid];
    auto& preceptron = preceptronTable[tid][history->perceptronIndex];
    
    if(squashed){
        globalHistory = history->globalHistory;
        globalHistory.PushBack(taken);
        return;
    }
    preceptron.Backword(history->globalHistory, history->perceptronOutput, history->taken, taken);
    delete history;
}

void
PerceptronBP::squash(ThreadID tid, void *bp_history)
{
    if(!bp_history) return;
    BPHistory *history = static_cast<BPHistory*>(bp_history);
    globalHistoryReg[tid] = history->globalHistory;
    delete history;
}

bool
PerceptronBP::lookup(ThreadID tid, Addr branch_pc, void* &bp_history)
{
    auto& ptable = preceptronTable[tid];
    auto perceptronIndex = (branch_pc >> instShiftAmt) % tableSize;
    auto& model = ptable[perceptronIndex];
    int32_t predictResult = model.Forward(globalHistoryReg[tid]);
    
    bool taken = predictResult >= 0;
    BPHistory *history = new BPHistory(taken, perceptronIndex, predictResult, globalHistoryReg[tid]);
    auto& globalHistory = globalHistoryReg[tid];
    bp_history = static_cast<void*>(history);
    globalHistory.PushBack(taken);

    return taken;
}

int
PerceptronBP::confidence(ThreadID tid, Addr branch_pc) {
    auto& ptable = preceptronTable[tid];
    auto perceptronIndex = (branch_pc >> instShiftAmt) % tableSize;
    auto& model = ptable[perceptronIndex];
    return model.Forward(globalHistoryReg[tid]);
}

void
PerceptronBP::btbUpdate(ThreadID tid, Addr branch_pc, void* &bp_history)
{
}

void
PerceptronBP::uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history)
{
    // do nothing, avoid GHR pollution, only update history
    auto perceptronIndex = (br_pc >> instShiftAmt) % tableSize;
    BPHistory *history = new BPHistory(true, perceptronIndex, true, globalHistoryReg[tid]);
    bp_history = static_cast<void*>(history);
}

PerceptronBP*
PerceptronBPParams::create()
{
    return new PerceptronBP(this);
}
