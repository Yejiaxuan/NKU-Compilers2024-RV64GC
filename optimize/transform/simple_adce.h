#ifndef SIMPLE_ADCE_H
#define SIMPLE_ADCE_H

#include "../../include/ir.h"
#include "../pass.h"
#include "../analysis/dominator_tree.h"

class ADCEPass : public IRPass {
private:
    DomAnalysis* domtrees;
    std::vector<std::vector<LLVMBlock>> BuildCDG(CFG* C);
    Instruction FindTerminal(CFG* C, int bbid);
    void ADCE(CFG* C);
    
public:
    ADCEPass(LLVMIR* IR, DomAnalysis* dom) : IRPass(IR) { domtrees = dom; }
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
