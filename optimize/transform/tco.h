#ifndef TCO_H
#define TCO_H

#include "../../include/ir.h"
#include "../pass.h"
#include <vector>

class TCOPass : public IRPass {
public:
    TCOPass(LLVMIR *IR) : IRPass(IR) {}

    void Execute();

private:
    void TailRecursionToLoop(CFG* cfg, FuncDefInstruction defI);
    void ConvertTailRecursionToLoop(CFG* cfg, BasicBlock* block, CallInstruction* call_instr, FuncDefInstruction defI);
    bool IsTailRecursiveCall(BasicBlock* block, FuncDefInstruction defI, CallInstruction*& call_instr);
    void UpdatePhiNodes(CFG* cfg, FuncDefInstruction defI, BasicBlock* block, CallInstruction* call_instr);
};

#endif // TCO_H