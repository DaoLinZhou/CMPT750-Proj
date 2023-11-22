#include "cpu/pred/always_not_taken.hh"

NotTaken::NotTaken(const NotTakenParams *params) 
    : BPredUnit(params)
{
}

// PREDICTOR UPDATE
void
NotTaken::update(ThreadID tid, Addr branch_pc, bool taken, void* bp_history,
              bool squashed, const StaticInstPtr & inst, Addr corrTarget)
{
}

void
NotTaken::squash(ThreadID tid, void *bp_history)
{
}

bool
NotTaken::lookup(ThreadID tid, Addr branch_pc, void* &bp_history)
{
    return false;
}

void
NotTaken::btbUpdate(ThreadID tid, Addr branch_pc, void* &bp_history)
{
}

void
NotTaken::uncondBranch(ThreadID tid, Addr br_pc, void* &bp_history)
{
}

NotTaken*
NotTakenParams::create()
{
    return new NotTaken(this);
}
