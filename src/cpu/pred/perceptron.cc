

#include "cpu/pred/perceptron.hh"


PerceptronBP::PerceptronBP(const PerceptronBPParams *params) 
    : BPredUnit(params)
{
}

// PREDICTOR UPDATE
void
PerceptronBP::update(ThreadID tid, Addr branch_pc, bool taken, void* bp_history,
              bool squashed, const StaticInstPtr & inst, Addr corrTarget)
{ 
}

void
PerceptronBP::squash(ThreadID tid, void *bp_history)
{
}

bool
PerceptronBP::lookup(ThreadID tid, Addr branch_pc, void* &bp_history)
{
    std::cout << "perceptron prediction" << std::endl;
    return true;
}

void
PerceptronBP::btbUpdate(ThreadID tid, Addr branch_pc, void* &bp_history)
{   
}

void
PerceptronBP::uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history)
{
}

PerceptronBP*
PerceptronBPParams::create()
{
    return new PerceptronBP(this);
}
