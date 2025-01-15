// riscv64_lowerimm.h
#ifndef RISCV64_LOWERIMM_H
#define RISCV64_LOWERIMM_H

#include "../riscv64.h"

struct Multiplier {
    Uint64 m;
    int l;
};

class RiscV64LowerImm : public MachinePass {
public:
    RiscV64LowerImm(MachineUnit *unit) : MachinePass(unit) {}
    void Execute();

private:
    // Helper functions for lowering immediate instructions
    bool NeedsImmediateLowering(RiscV64Instruction *ins);
    void LowerLoadStoreInstruction(std::list<MachineBaseInstruction *>::iterator &it, RiscV64Instruction *ins, MachineBlock *block);
    void LowerITypeInstruction(std::list<MachineBaseInstruction *>::iterator &it, RiscV64Instruction *ins, MachineBlock *block);
};

class RiscV64AlgStrengthReduce : public MachinePass {
public:
    RiscV64AlgStrengthReduce(MachineUnit *unit) : MachinePass(unit) {}
    void Execute();

private:
    // Helper functions for strength reduction
    void OptimizeMultiply(std::list<MachineBaseInstruction *>::iterator &it, RiscV64Instruction *ins, MachineBlock *block, std::map<int, int> &constPool);
    void OptimizeDivide(std::list<MachineBaseInstruction *>::iterator &it, RiscV64Instruction *ins, MachineBlock *block, std::map<int, int> &constPool);
    Multiplier chooseMultiplier(Uint32 d, int p);
};

#endif

