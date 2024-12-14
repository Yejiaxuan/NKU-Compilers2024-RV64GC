#ifndef INLINE_H
#define INLINE_H

#include "../../include/ir.h"
#include "../pass.h"

class InlinePass : public IRPass {
private:
    // 内联行数阈值(例如200行)
    static const int INLINE_THRESHOLD = 200;

    bool CanInlineFunction(FuncDefInstruction funcDef);
    void InlineFunctionCall(CFG *callerCFG, BasicBlock *callerBlock, CallInstruction *callInst, FuncDefInstruction calleeFunc);

public:
    InlinePass(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
};

#endif
