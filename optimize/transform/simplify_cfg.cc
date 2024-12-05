#include "simplify_cfg.h"

void SimplifyCFGPass::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        EliminateUnreachedBlocksInsts(cfg);
    }
}

// 删除从函数入口开始到达不了的基本块和指令
// 不需要考虑控制流为常量的情况，你只需要从函数入口基本块(0号基本块)开始DFS，将没有被访问到的基本块和指令删去即可
void SimplifyCFGPass::EliminateUnreachedBlocksInsts(CFG *C) { 
    // 步骤 1：执行深度优先搜索（DFS）以找出可达的基本块
    std::vector<bool> visited(C->G.size(), false);
    std::stack<int> s; // DFS 使用的栈
    s.push(0); // 从入口块（ID 为0）开始

    while(!s.empty()){
        int cur = s.top();
        s.pop();

        if(visited[cur]){
            continue; // 如果当前基本块已访问，跳过
        }

        visited[cur] = true;

        for(auto &nxt : C->G[cur]){
            if(!visited[nxt->block_id]){
                s.push(nxt->block_id);
            }
        }
    }

    // 步骤 2：收集不可达的基本块
    std::vector<int> unreachable_block_ids;
    for(int i = 0; i < static_cast<int>(visited.size()); i++){
        if(!visited[i]){
            unreachable_block_ids.push_back(i);
        }
    }

    // 步骤 3：删除不可达的基本块和指令
    for(int block_id : unreachable_block_ids){
        auto it = C->block_map->find(block_id);
        if(it != C->block_map->end()){
            LLVMBlock unreachable_block = it->second;

            // 删除基本块中的指令
            for(auto instr : unreachable_block->Instruction_list){
                delete instr; // 假设指令是动态分配的内存
            }
            unreachable_block->Instruction_list.clear();

            // 从 block_map 中删除该基本块
            delete unreachable_block; // 假设基本块是动态分配的
            C->block_map->erase(it);
        }

        // 清空 G 和 invG 中的连接
        if(block_id < static_cast<int>(C->G.size())){
            C->G[block_id].clear();
        }
        if(block_id < static_cast<int>(C->invG.size())){
            C->invG[block_id].clear();
        }
    }

    // 步骤 4：更新 G 和 invG，移除指向不可达块的边
    for(int i = 0; i < static_cast<int>(C->G.size()); i++){
        if(visited[i]){
            std::vector<LLVMBlock> newSuccessors;
            for(auto &succ_block : C->G[i]){
                if(visited[succ_block->block_id]){
                    newSuccessors.push_back(succ_block);
                }
            }
            C->G[i] = newSuccessors;
        }
    }

    for(int i = 0; i < static_cast<int>(C->invG.size()); i++){
        if(visited[i]){
            std::vector<LLVMBlock> newPredecessors;
            for(auto &pred_block : C->invG[i]){
                if(visited[pred_block->block_id]){
                    newPredecessors.push_back(pred_block);
                }
            }
            C->invG[i] = newPredecessors;
        }
    }

    // 步骤 5：处理可达块中的指令，删除跳转或返回指令后的所有指令
    for(auto &[block_id, block] : *C->block_map){
        // 使用索引遍历，以便安全删除后续指令
        for(int i = 0; i < static_cast<int>(block->Instruction_list.size()); i++){
            BasicInstruction* instr = block->Instruction_list[i];
            NodeAttribute::opcode opcode = static_cast<NodeAttribute::opcode>(instr->GetOpcode());

            if(opcode == BasicInstruction::BR_COND ||
               opcode == BasicInstruction::BR_UNCOND ||
               opcode == BasicInstruction::RET){

                // 删除当前指令后的所有指令
                for(int j = i + 1; j < static_cast<int>(block->Instruction_list.size()); j++){
                    delete block->Instruction_list[j]; // 假设指令是动态分配的内存
                }

                // 截断指令列表到当前指令位置
                block->Instruction_list.erase(block->Instruction_list.begin() + i + 1, block->Instruction_list.end());

                // 跳转或返回指令后面的指令已删除，退出当前指令列表的遍历
                break;
            }
        }
    }
    //TODO("EliminateUnreachedBlocksInsts"); 
}
