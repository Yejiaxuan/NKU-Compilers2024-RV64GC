#ifndef TCO_H
#define TCO_H

#include "../../include/ir.h"
#include "../pass.h"
#include <deque>
#include <unordered_map>

class TCOPass : public IRPass {
private:    
    bool TailRecursiveEliminateCheck(CFG* C);
    void TailRecursiveEliminate(CFG* C);

public:
    TCOPass(LLVMIR* ir) : IRPass(ir) {}
    void Execute() override;
};

#endif