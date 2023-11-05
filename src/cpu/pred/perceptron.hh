
#ifndef __CPU_PRED_PERCEPTRON
#define __CPU_PRED_PERCEPTRON

#include "base/types.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/PerceptronBP.hh"

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
    inline uint32_t Size() const { return threadGlobalHistory.size(); }
};

struct Perceptron{
    int32_t bias;
    std::vector<int32_t> weights;
    int32_t minWeight;
    int32_t maxWeight;
    uint32_t threshold;

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

    inline void Backword(const GHR& input, int32_t output, bool predict, bool trueValue){
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
        return;
    }
    
};


class PerceptronBP: public BPredUnit{
private:
    GHR* globalHistoryReg;
    std::vector<Perceptron*> preceptronTable;
    uint32_t tableSize;
    
    struct BPHistory{
        bool taken;
        uint32_t perceptronIndex;
        int32_t perceptronOutput;
        GHR globalHistory;
        BPHistory(bool taken, uint32_t perceptronIndex, int32_t perceptronOutput, GHR& globalHistory): 
        taken(taken), 
        perceptronIndex(perceptronIndex), 
        perceptronOutput(perceptronOutput), 
        globalHistory(globalHistory){
        }
    };

public:
    PerceptronBP(const PerceptronBPParams *params);
    ~PerceptronBP();
        
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