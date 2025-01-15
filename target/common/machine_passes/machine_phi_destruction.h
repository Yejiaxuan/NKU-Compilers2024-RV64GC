#ifndef MACHINE_PHI_DESTRUCTION_H
#define MACHINE_PHI_DESTRUCTION_H

#include "machine_pass.h"

class MachinePhiDestruction : public MachinePass {
private:
    void PhiDestructionInCurrentFunction();
    bool BlockHasPhi(MachineBlock *block);
    void InsertBranchOnlyBlock(int predecessor_id, int current_block_id);
    void ProcessPhiInstructions(MachineBlock *block);
    void HandleParallelCopyList(MachineBlock *block);

public:
    void Execute();
    MachinePhiDestruction(MachineUnit *unit) : MachinePass(unit) {}
};

#endif

