// inline.h
#ifndef INLINE_H
#define INLINE_H

#include "../../include/ir.h"
#include "../pass.h"
#include <functional>
#include <map>
#include <unordered_map>

class InlinePass : public IRPass {
private:
    std::unordered_map<CFG*, bool> CFGvis;
    std::unordered_map<Instruction, bool> reinlinecallI; 
    bool is_reinline;

    void MakeFunctionOneExit(CFG* C);
    void RetMotion(CFG* C);
    void InlineDFS(CFG* uCFG);
    CFG* CopyCFG(CFG* uCFG);
    void InlineCFG(CFG* uCFG, CFG* vCFG, uint32_t CallINo);
    Operand InlineCFG(CFG* uCFG, CFG* vCFG, LLVMBlock StartBB, LLVMBlock EndBB,
                      std::map<int,int>& reg_replace_map,
                      std::map<int,int>& label_replace_map);
    void EliminateUselessFunction();

public:
    InlinePass(LLVMIR* ir) : IRPass(ir) {
        is_reinline = false;
    }

    void Execute();
};

#endif