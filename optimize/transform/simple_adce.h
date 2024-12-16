#ifndef SIMPLE_ADCE_H
#define SIMPLE_ADCE_H

#include "../../include/ir.h"
#include "../pass.h"
#include "dominator_tree.h"
#include <set>
#include <map>
#include <queue>
#include <deque>

/**
 * @class ADCEPass
 * @brief 激进的死代码消除（ADCE）优化 pass。
 *
 * ADCEPass 负责识别并删除程序中对最终结果没有影响的代码。
 */
class ADCEPass : public IRPass {
private:
    // 指向支配树分析的指针
    DomAnalysis *dom_analysis;

    // 控制依赖图（CDG）：映射每个基本块到其控制依赖的基本块集合
    std::map<int, std::set<int>> control_dependence;

    /**
     * @brief 构建控制依赖图（CDG）。
     *
     * 利用支配树分析的支配前沿（Dominance Frontier）来构建控制依赖图。
     *
     * @param C 指向当前处理的控制流图（CFG）。
     */
    void BuildControlDependence(CFG *C);

    /**
     * @brief 计算基本块中活跃的指令集合。
     *
     * 根据 ADCE 算法，通过迭代标记和传播活跃指令来识别哪些指令是有用的。
     *
     * @param C 指向当前处理的控制流图（CFG）。
     * @return 返回活跃指令的集合。
     */
    std::set<BasicInstruction*> ComputeLiveInstructions(CFG *C);

public:
    /**
     * @brief 构造函数。
     *
     * @param IR 指向当前 LLVM IR 的指针。
     * @param dom 指向支配树分析的指针。
     */
    ADCEPass(LLVMIR *IR, DomAnalysis *dom) : IRPass(IR), dom_analysis(dom) {}

    /**
     * @brief 执行 ADCE 优化。
     *
     * 对每个函数的控制流图执行 ADCE，以删除死代码。
     */
    void Execute();
};

#endif // SIMPLE_ADCE_H

