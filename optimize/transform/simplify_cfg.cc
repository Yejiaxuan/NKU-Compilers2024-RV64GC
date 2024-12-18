#include "simplify_cfg.h"

void SimplifyCFGPass::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        EliminateUnreachedBlocksInsts(cfg);
    }
}

// 删除从函数入口开始到达不了的基本块和指令
// 不需要考虑控制流为常量的情况，你只需要从函数入口基本块(0号基本块)开始DFS，将没有被访问到的基本块和指令删去即可
void SimplifyCFGPass::EliminateUnreachedBlocksInsts(CFG *C) { 
    auto &blocks = *(C->block_map);
    std::map<int, bool> visited;
    std::stack<int> s;
    s.push(0); // 从入口块（ID 为0）开始

    while(!s.empty()){
        int cur = s.top();
        s.pop();

        if (visited[cur]) continue;
        visited[cur] = true;

        // 修剪不可达指令，保留到最后一条有效指令
        TrimUnreachableInstructions(C, cur);

        // 获取基本块的最后一条指令并处理跳转逻辑
        ProcessBlockLastInstruction(C, visited, cur, s);
    }

    // 删除不可达的基本块
    RemoveUnreachableBlocks(C, visited);
    //TODO("EliminateUnreachedBlocksInsts"); 
}

void SimplifyCFGPass::TrimUnreachableInstructions(CFG *C, int block_id) {
    auto &blocks = *(C->block_map);
    auto &block = blocks[block_id];
    auto &blockList = block->Instruction_list;

    int ret_pos = blockList.size();
    for (int i = 0; i < blockList.size(); i++) {
        if (blockList[i]->GetOpcode() == BasicInstruction::RET) {
            ret_pos = i;
            break;
        }
    }

    while (blockList.size() > ret_pos + 1) {
        auto last_inst = blockList.back();
        if (last_inst->GetOpcode() == BasicInstruction::BR_UNCOND) {
            RemoveUnconditionalBranch(C, block_id, last_inst);
        } else if (last_inst->GetOpcode() == BasicInstruction::BR_COND) {
            RemoveConditionalBranch(C, block_id, last_inst);
        }
        blockList.pop_back();
    }
}

void SimplifyCFGPass::RemoveUnconditionalBranch(CFG *C, int block_id, Instruction last_inst) {
    auto bruncond = static_cast<BrUncondInstruction *>(last_inst);
    int target_block_no = static_cast<LabelOperand *>(bruncond->GetDestLabel())->GetLabelNo();
    RemoveEdgeFromGraph(C, block_id, target_block_no);
}

void SimplifyCFGPass::RemoveConditionalBranch(CFG *C, int block_id, Instruction last_inst) {
    auto brcond = static_cast<BrCondInstruction *>(last_inst);
    int true_block_no = static_cast<LabelOperand *>(brcond->GetTrueLabel())->GetLabelNo();
    int false_block_no = static_cast<LabelOperand *>(brcond->GetFalseLabel())->GetLabelNo();

    RemoveEdgeFromGraph(C, block_id, true_block_no);
    RemoveEdgeFromGraph(C, block_id, false_block_no);
}

void SimplifyCFGPass::RemoveEdgeFromGraph(CFG *C, int block_id, int target_block_no) {
    for (int i = 0; i < C->G[block_id].size(); i++) {
        if (C->G[block_id][i]->block_id == target_block_no) {
            C->G[block_id].erase(C->G[block_id].begin() + i);
            break;
        }
    }
    for (int i = 0; i < C->invG[target_block_no].size(); i++) {
        if (C->invG[target_block_no][i]->block_id == block_id) {
            C->invG[target_block_no].erase(C->invG[target_block_no].begin() + i);
            break;
        }
    }
}

void SimplifyCFGPass::ProcessBlockLastInstruction(CFG *C, std::map<int, bool> &visited, int block_id, std::stack<int> &s) {
    auto &blocks = *(C->block_map);
    auto &block = blocks[block_id];
    auto &blockList = block->Instruction_list;

    // 获取基本块的最后一条指令
    Instruction blocklast = blockList.empty() ? nullptr : blockList.back();

    // 根据最后一条指令处理跳转逻辑
    if (blocklast) {
        if (blocklast->GetOpcode() == BasicInstruction::BR_UNCOND) {
            auto bruncond = static_cast<BrUncondInstruction *>(blocklast);
            int target_block_no = static_cast<LabelOperand *>(bruncond->GetDestLabel())->GetLabelNo();
            if (!visited[target_block_no])
                s.push(target_block_no);
        } else if (blocklast->GetOpcode() == BasicInstruction::BR_COND) {
            auto brcond = static_cast<BrCondInstruction *>(blocklast);
            int true_block_no = static_cast<LabelOperand *>(brcond->GetTrueLabel())->GetLabelNo();
            int false_block_no = static_cast<LabelOperand *>(brcond->GetFalseLabel())->GetLabelNo();
            if (!visited[true_block_no])
                s.push(true_block_no);
            if (!visited[false_block_no])
                s.push(false_block_no);
        }
    }
}

void SimplifyCFGPass::RemoveUnreachableBlocks(CFG *C, std::map<int, bool> &visited) {
    auto &blocks = *(C->block_map);
    std::set<int> deadbb_set;
    std::queue<int> deadblocks;

    // 收集所有不可达的基本块
    for (const auto &id_block_pair : blocks) {
        if (!visited[id_block_pair.first]) {
            deadblocks.push(id_block_pair.first);
            deadbb_set.insert(id_block_pair.first);
        }
    }

    // 删除图中指向不可达基本块的边
    for (int i = 0; i < C->G.size(); i++) {
        if (deadbb_set.find(i) != deadbb_set.end()) {
            C->G[i].clear();
            continue;
        }
        for (int j = 0; j < C->G[i].size(); j++) {
            if (deadbb_set.find(C->G[i][j]->block_id) != deadbb_set.end()) {
                C->G[i].erase(C->G[i].begin() + j);
                j--;
            }
        }
    }

    for (int i = 0; i < C->invG.size(); i++) {
        if (deadbb_set.find(i) != deadbb_set.end()) {
            C->invG[i].clear();
            continue;
        }
        for (int j = 0; j < C->invG[i].size(); j++) {
            if (deadbb_set.find(C->invG[i][j]->block_id) != deadbb_set.end()) {
                C->invG[i].erase(C->invG[i].begin() + j);
                j--;
            }
        }
    }

    // 删除不可达的基本块
    while (!deadblocks.empty()) {
        blocks.erase(deadblocks.front());
        deadblocks.pop();
    }
}
