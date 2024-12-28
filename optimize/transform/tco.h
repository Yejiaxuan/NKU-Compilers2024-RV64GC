#ifndef TCO_H
#define TCO_H

#include "../../include/ir.h"
#include "../pass.h"
#include <deque>
#include <unordered_map>
#include <functional>

class TCOPass : public IRPass {
private:    
    void TailRecursionToLoop(CFG *cfg, FuncDefInstruction defI);
    void ConvertTailRecursionToLoop(CFG *cfg, BasicBlock *block, CallInstruction *call_instr,FuncDefInstruction defI);
    void MakeFunctionOneExit(CFG* C);
    void RetMotion(CFG* C);

public:
    TCOPass(LLVMIR* ir) : IRPass(ir) {}
    void Execute() override;
};

#endif