#ifndef SIMPLE_DCE_H
#define SIMPLE_DCE_H
#include "../../include/ir.h"
#include "../pass.h"
#include <string>
#include <map>
#include <unordered_set>
#include <vector>

class SimpleDCEPass : public IRPass {
private:
    void EliminateDeadCode(CFG *C);

public:
    SimpleDCEPass(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
};

#endif
