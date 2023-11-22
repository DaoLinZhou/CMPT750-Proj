#include "cpu/pred/perceptronForest.hh"

PerceptronForestBP::~PerceptronForestBP(){
  delete[] globalHistoryReg;
  for(auto& table: preceptronTable){
    delete[] table;
  }
}

PerceptronForestBP::PerceptronForestBP(const PerceptronForestBPParams *params):
    BPredUnit(params),
      globalHistoryReg(new GHR[params->numThreads]),
      preceptronTable(params->numThreads),
      tableSize(params->perceptronTableSize)
{
  for(uint32_t i = 0; i < params->numThreads; i++){
    globalHistoryReg[i].Init(params->globalHistorySize);
  }
  for(auto& table: preceptronTable){
    table = new PerceptronForest[params->perceptronTableSize];
    for(uint32_t i = 0; i < params->perceptronTableSize; i++){
      table[i].Init(params->globalHistorySize, params->perceptronNum, params->minWeight, params->maxWeight, params->avgPerceptronLength);
    }
  }
}

// PREDICTOR UPDATE
void
PerceptronForestBP::update(ThreadID tid, Addr branch_pc, bool taken, void* bp_history,
                     bool squashed, const StaticInstPtr & inst, Addr corrTarget)
{
  if(!bp_history) return;

  BPHistory *history = static_cast<BPHistory*>(bp_history);
  auto& globalHistory = globalHistoryReg[tid];
  auto& preceptron = preceptronTable[tid][history->perceptronIndex];

  // FIXME(jiatang)
  // globalHistory.PushBack(taken);

  if(squashed){
    globalHistory = history->globalHistory;
    globalHistory.PushBack(taken);
    return;
  }
  preceptron.Backward(history->globalHistory, history->perceptronOutput, taken);
  delete history;
}

void
PerceptronForestBP::squash(ThreadID tid, void *bp_history)
{
  if(!bp_history) return;
  BPHistory *history = static_cast<BPHistory*>(bp_history);
  globalHistoryReg[tid] = history->globalHistory;
  delete history;
}

bool
PerceptronForestBP::lookup(ThreadID tid, Addr branch_pc, void* &bp_history)
{
  auto& ptable = preceptronTable[tid];
  auto perceptronIndex = (branch_pc >> instShiftAmt) % tableSize;
  auto& model = ptable[perceptronIndex];

  std::vector<int32_t> predictResult;

  bool taken = model.Forward(globalHistoryReg[tid], predictResult) >= 0;

  BPHistory *history = new BPHistory(taken, perceptronIndex, predictResult, globalHistoryReg[tid]);
  auto& globalHistory = globalHistoryReg[tid];
  bp_history = static_cast<void*>(history);
  // FIXME(jiatang)
  globalHistory.PushBack(taken);

  return taken;
}

void
PerceptronForestBP::btbUpdate(ThreadID tid, Addr branch_pc, void* &bp_history)
{
}

void
PerceptronForestBP::uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history)
{
  // do nothing, avoid GHR pollution, only update history
//   auto perceptronIndex = (br_pc >> instShiftAmt) % tableSize;
//   BPHistory *history = new BPHistory(true, perceptronIndex, std::vector<int32_t>(), globalHistoryReg[tid]);
//   bp_history = static_cast<void*>(history);
}

PerceptronForestBP*
PerceptronForestBPParams::create()
{
    return new PerceptronForestBP(this);
}
