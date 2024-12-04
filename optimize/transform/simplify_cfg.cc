#include "simplify_cfg.h"

void SimplifyCFGPass::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        EliminateUnreachedBlocksInsts(cfg);
    }
}

// 删除从函数入口开始到达不了的基本块和指令
// 不需要考虑控制流为常量的情况，你只需要从函数入口基本块(0号基本块)开始dfs，将没有被访问到的基本块和指令删去即可
void SimplifyCFGPass::EliminateUnreachedBlocksInsts(CFG *C) { 
    // Step 1: 使用DFS标记可达的基本块
    std::set<int> visited;
    std::stack<int> stack;

    // 假设入口基本块的 ID 为 0
    int entryBlockId = 0;
    if (C->block_map->find(entryBlockId) == C->block_map->end()) {
        return; // 如果入口基本块不存在，直接返回
    }

    stack.push(entryBlockId);
    visited.insert(entryBlockId);

    while (!stack.empty()) {
        int currentBlockId = stack.top();
        stack.pop();

        // 获取当前基本块的所有后继基本块
        for (auto &succBlock : C->G[currentBlockId]) {
            int succBlockId = succBlock->block_id;
            if (visited.find(succBlockId) == visited.end()) {
                visited.insert(succBlockId);
                stack.push(succBlockId);
            }
        }
    }

    // Step 2: 收集不可达的基本块
    std::vector<int> unreachableBlocks;
    for (const auto &[blockId, block] : *(C->block_map)) {
        if (visited.find(blockId) == visited.end()) {
            unreachableBlocks.push_back(blockId);
        }
    }

    // Step 3: 删除不可达的基本块
    for (int blockId : unreachableBlocks) {
        // 清空 G 和 invG 中的相关边
        if (blockId < (int)C->G.size()) {
            C->G[blockId].clear();
        }
        if (blockId < (int)C->invG.size()) {
            C->invG[blockId].clear();
        }

        // 删除基本块并释放内存
        auto it = C->block_map->find(blockId);
        if (it != C->block_map->end()) {
            delete it->second; // 假设 BasicBlock 是动态分配的
            C->block_map->erase(it);
        }
    }

    // Step 4: 更新邻接表，移除对已删除基本块的引用
    for (auto &[blockId, block] : *(C->block_map)) {
        // 更新 G（正向邻接表）
        std::vector<LLVMBlock> &succs = C->G[blockId];
        succs.erase(
            std::remove_if(succs.begin(), succs.end(),
                [&](LLVMBlock succ) {
                    return C->block_map->find(succ->block_id) == C->block_map->end();
                }),
            succs.end()
        );

        // 更新 invG（逆向邻接表）
        std::vector<LLVMBlock> &preds = C->invG[blockId];
        preds.erase(
            std::remove_if(preds.begin(), preds.end(),
                [&](LLVMBlock pred) {
                    return C->block_map->find(pred->block_id) == C->block_map->end();
                }),
            preds.end()
        );
    }

    // Step 5: 删除不可达的指令（即终止指令之后的指令）
    for (auto &[blockId, block] : *(C->block_map)) {
        auto &instructions = block->Instruction_list;

        bool terminate = false;
        auto it = instructions.begin();

        while (it != instructions.end()) {
            Instruction inst = *it;

            // 检查当前指令是否是终止指令
            bool isTerminate = false;
            switch (inst->GetOpcode()) {
                case BasicInstruction::BR_COND:
                case BasicInstruction::BR_UNCOND:
                case BasicInstruction::RET:
                    isTerminate = true;
                    break;
                default:
                    isTerminate = false;
                    break;
            }

            if (terminate) {
                // 当前指令是不可达的，删除它
                it = instructions.erase(it);
                delete inst; // 释放内存
                continue;
            }

            if (isTerminate) {
                terminate = true;
            }

            ++it;
        }
    }
    //TODO("EliminateUnreachedBlocksInsts"); 
}
