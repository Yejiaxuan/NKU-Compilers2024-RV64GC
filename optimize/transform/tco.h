#ifndef TCO_H
#define TCO_H

#include "../../include/ir.h"
#include "../pass.h"
#include <deque>
#include <unordered_map>
#include <functional>

class TCOPass : public IRPass {
private:    
    bool TailRecursiveEliminateCheck(CFG* C);
    void TailRecursiveEliminate(CFG* C);
    void MakeFunctionOneExit(CFG* C);
    void RetMotion(CFG* C);

public:
    TCOPass(LLVMIR* ir) : IRPass(ir) {}
    void Execute() override;
};

#endif