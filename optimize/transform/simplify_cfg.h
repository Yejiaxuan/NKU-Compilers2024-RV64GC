#ifndef SIMPLIFY_CFG_H
#define SIMPLIFY_CFG_H
#include "../../include/ir.h"
#include "../pass.h"
#include <string>
#include <map>
#include <unordered_set>
#include <vector>

class SimplifyCFGPass : public IRPass {
private:
    // TODO():添加更多你需要的成员变量
    void EliminateUnreachedBlocksInsts(CFG *C);
    // 修剪不可达指令，保留到最后一条有效指令
    void TrimUnreachableInstructions(CFG *C, int block_id);

    // 删除无条件跳转指令的相关图边
    void RemoveUnconditionalBranch(CFG *C, int block_id, Instruction last_inst);

    // 删除条件跳转指令的相关图边
    void RemoveConditionalBranch(CFG *C, int block_id, Instruction last_inst);

    // 删除图中指向不可达基本块的边
    void RemoveEdgeFromGraph(CFG *C, int block_id, int target_block_no);

    // 处理基本块最后一条指令的跳转逻辑
    void ProcessBlockLastInstruction(CFG *C, std::map<int, bool> &visited, int block_id, std::stack<int> &s);

    // 删除不可达的基本块及其相关的图边
    void RemoveUnreachableBlocks(CFG *C, std::map<int, bool> &visited);
public:
    SimplifyCFGPass(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
};

#endif
