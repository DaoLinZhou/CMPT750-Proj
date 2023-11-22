
#ifndef __CPU_PRED_PERCEPTRON
#define __CPU_PRED_PERCEPTRON

#include <random>
#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/PerceptronForestBP.hh"

struct GHR{
  std::vector<bool> threadGlobalHistory;
  uint32_t startPoint;
  GHR(GHR& o) = default;
  GHR() = default;

  void Init(uint32_t size){
    threadGlobalHistory = std::vector<bool>(size, false);
    startPoint = 0;
  }

  GHR& operator=(const GHR o){
    threadGlobalHistory = o.threadGlobalHistory;
    startPoint = o.startPoint;
    return *this;
  }

  inline bool Get(uint32_t index) const {
    return threadGlobalHistory[(index + startPoint) % threadGlobalHistory.size()];
  }

  inline void PushBack(bool value){
    threadGlobalHistory[startPoint] = value;
    startPoint = (startPoint + 1) % threadGlobalHistory.size();
  }
  inline void Pop(){
    startPoint = (startPoint - 1 + threadGlobalHistory.size()) % threadGlobalHistory.size();;
  }
  inline uint32_t Size() const { return threadGlobalHistory.size(); }
};


struct Perceptron{
  int32_t bias;
  std::vector<int32_t> weights;
  int32_t minWeight;
  int32_t maxWeight;
  int32_t threshold;

  Perceptron() = default;
  Perceptron(Perceptron&) = default;

  void Init(uint32_t size, int32_t minWeight_, int32_t maxWeight_, uint32_t threshold_)
  {
    bias = 1;
    weights = std::vector<int32_t>(size, 0);
    minWeight = minWeight_;
    maxWeight = maxWeight_;
    threshold = threshold_;
  }

  inline void Clamp(int32_t& value) {
    if (value < minWeight) value = minWeight;
    else if (value > maxWeight) value = maxWeight;
  }

  // GHR is a cycle queue, start_point is the
  inline int32_t Forward(const GHR& ghr){
    assert(ghr.Size() == weights.size());
    int32_t output = bias;
    for(uint32_t i = 0; i < weights.size(); i++){
      if(ghr.Get(i)){
        output += weights[i];
      }else{
        output -= weights[i];
      }
    }
    return output;
  }

  inline void Backward(const GHR& input, int32_t output, bool trueValue){
    bool predict = output >= 0;
    if(predict != trueValue || ((-threshold <= output) && (output <= threshold))){
      // update weights
      bias += trueValue ? 1 : -1;
      Clamp(bias);
      for(uint32_t i = 0; i < weights.size(); i++){
        int32_t x = input.Get(i) ? 1 : -1;
        int32_t updateValue = trueValue ? x : -x;
        weights[i] += updateValue;
        Clamp(weights[i]);
      }
    }
  }

};

class PerceptronForest{
public:
  std::vector<Perceptron> perceptronArr;
  std::vector<int32_t> weights;
  std::vector<std::pair<uint32_t, uint32_t>> range;

  inline void IncreaseWeight(int32_t& w){
    if(w >= 10) return;
    w++;
  }

  inline void DecreaseWeight(int32_t& w){
    if(w == 0) return;
    w--;
  }

  inline std::vector<GHR> getsubrange(const GHR& input){
    std::vector<GHR> out(range.size());
    for(uint32_t i = 0; i < range.size(); i++){
      auto start = range[i].first;
      auto end = range[i].second;
      out[i].Init(end - start);
      for(uint j = 0; j < (end - start); j++){
        out[i].PushBack(input.Get(j+start));
      }
    }
    return out;
  }

  inline void Init(uint32_t historySize, uint32_t perceptronNum, int32_t minWeight_, int32_t maxWeight_, uint32_t averageLength){
    range = std::vector<std::pair<uint32_t, uint32_t>>(perceptronNum);
    weights = std::vector<int32_t>(perceptronNum);
    std::random_device rd;      // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd());
    std::geometric_distribution<uint32_t> distrib(0.3);
    std::uniform_int_distribution<uint32_t> udistrib(0, averageLength+averageLength);
    std::set<std::pair<uint32_t, uint32_t>> counter;
    for(uint32_t i = 0; i < perceptronNum; i++){
      uint32_t offset = std::min(distrib(gen), historySize/2);
      uint32_t endPoint = historySize - offset;
      uint32_t startPoint = endPoint - std::min(udistrib(gen), endPoint);
      range[i] = {startPoint, endPoint};
    }


    perceptronArr = std::vector<Perceptron>(range.size());
    for(uint32_t i = 0; i < range.size(); i++){
      uint32_t history_len = range[i].second - range[i].first;
      weights[i] = 1;
      perceptronArr[i].Init(history_len, minWeight_, maxWeight_, (uint32_t)(history_len*1.93)+14);
    }
  }

  inline void Backward(const GHR& input, const std::vector<int32_t>& output, bool trueValue){
    std::vector<GHR> ghrs(getsubrange(input));
    for(int i = 0; i < weights.size(); i++){
      bool local_predict = output[i] >= 0;
      if(local_predict == trueValue){
        IncreaseWeight(weights[i]);
      }else{
        DecreaseWeight(weights[i]);
      }
      perceptronArr[i].Backward(ghrs[i], output[i], trueValue);
    }
  }


  inline int64_t Forward(const GHR& ghr, std::vector<int32_t>& output){
    std::vector<GHR> ghrs(getsubrange(ghr));
    int64_t wout = 0;
    output.resize(perceptronArr.size());
    for(int i = 0; i < perceptronArr.size(); i++){
      output[i] = perceptronArr[i].Forward(ghrs[i]);
      if(output[i] >= 0){
        wout += weights[i];
      }else{
        wout -= weights[i];
      }
    }
    return wout;
  }
};


class PerceptronForestBP:public BPredUnit{
private:

  GHR* globalHistoryReg;
  std::vector<PerceptronForest*> preceptronTable;
  uint32_t tableSize;
public:
  struct BPHistory{
    bool taken;
    uint32_t perceptronIndex;
    std::vector<int32_t> perceptronOutput;
    GHR globalHistory;
    BPHistory(bool taken, uint32_t perceptronIndex, const std::vector<int32_t>& perceptronOutput, GHR& globalHistory):
        taken(taken),
        perceptronIndex(perceptronIndex),
        perceptronOutput(perceptronOutput),
        globalHistory(globalHistory){
    }
  };

public:
  PerceptronForestBP(const PerceptronForestBPParams *params);
  ~PerceptronForestBP();

  // Base class methods.
  void uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history) ;
  bool lookup(ThreadID tid, Addr branch_addr, void* &bp_history) ;
  void btbUpdate(ThreadID tid, Addr branch_addr, void* &bp_history) ;
  void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
              bool squashed, const StaticInstPtr & inst,
              Addr corrTarget);
  virtual void squash(ThreadID tid, void *bp_history);
};

#endif // __CPU_PRED_PERCEPTRON