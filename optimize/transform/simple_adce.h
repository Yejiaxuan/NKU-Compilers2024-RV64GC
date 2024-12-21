#ifndef SIMPLE_ADCE_H
#define SIMPLE_ADCE_H

#include "../../include/ir.h"
#include "../pass.h"
#include "../analysis/dominator_tree.h"
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <vector>

class ADCEPass : public IRPass {
private:
    std::deque<Instruction> worklist;                         // 工作列表
    std::unordered_map<int, Instruction> defmap;              // 定义映射
    std::unordered_set<Instruction> liveInstructionset;       // 活跃指令集合
    std::vector<bool> liveBlock;                              // 活跃基本块标记
    std::unordered_map<int, Instruction> terminalInstructions; // 缓存终结指令

    DominatorTree DomTree;
    DominatorTree PostDomTree;

    void CacheTerminalInstructions(CFG* C);
    void ResetliveBlock(CFG* C);
    std::vector<std::vector<LLVMBlock>> BuildCDG_precursor(CFG* C);
    void PopulateWorklistAndDefMap(CFG* C);
    void ProcessInstruction(Instruction I, const std::vector<std::vector<LLVMBlock>>& CDG_precursor, CFG* C);
    void RemoveDeadInstructions(CFG* C);
    std::vector<std::vector<LLVMBlock>> BuildCDG(CFG* C);
    Instruction FindTerminal(CFG* C, int bbid);
    void ADCE(CFG* C);
    bool HasSideEffect(const Instruction& instr);

public:
    // 构造函数
    ADCEPass(LLVMIR* IR)
        : IRPass(IR), DomTree(), PostDomTree() {}
    void Initialize(CFG* C);
    void Execute();
};

#endif // SIMPLE_ADCE_H
