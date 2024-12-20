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
    std::unordered_map<int, Instruction> defmap;              // 定义指令映射
    std::unordered_set<Instruction> liveInstructionset;       // 活跃指令集合
    std::vector<bool> liveBBset;                              // 活跃基本块标记
    std::unordered_map<int, Instruction> terminalInstructions; // 缓存终结指令

    DominatorTree DomTree;
    DominatorTree PostDomTree;

    std::vector<std::vector<LLVMBlock>> BuildCDG(CFG* C);
    Instruction FindTerminal(CFG* C, int bbid);
    void ADCE(CFG* C);

public:
    ADCEPass(LLVMIR* IR) 
        : IRPass(IR), DomTree(), PostDomTree() { 
    }

    void Initialize(CFG* C);
    void Execute();
};

#endif

/*#ifndef SIMPLE_ADCE_H
#define SIMPLE_ADCE_H

#include "../../include/ir.h"
#include "../pass.h"
#include "dominator_tree.h"
#include <set>
#include <map>
#include <queue>
#include <deque>
#include <unordered_set>
class ADCEPass : public IRPass {
private:
    // 指向支配树分析的指针
    DomAnalysis *dom_analysis;

    // 控制依赖图（CDG）：映射每个基本块到其控制依赖的基本块集合
    std::map<int, std::set<int>> control_dependence;

    void BuildControlDependence(CFG *C);
    std::set<BasicInstruction*> ComputeLiveInstructions(CFG *C);
    void RemoveUselessControlFlow(CFG *cfg);
    void RemoveEmptyBlocks(CFG *cfg);
    int GetAlternatePredecessor(CFG *C, int block_id);
    int FindFinalTarget(CFG *C, int block_id, std::unordered_set<int>& visited);
public:
    ADCEPass(LLVMIR *IR, DomAnalysis *dom) : IRPass(IR), dom_analysis(dom) {}
    void Execute();
};

#endif // SIMPLE_ADCE_H
*/