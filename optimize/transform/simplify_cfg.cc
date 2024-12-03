#include "simplify_cfg.h"

void SimplifyCFGPass::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        EliminateUnreachedBlocksInsts(cfg);
    }
}

// 删除从函数入口开始到达不了的基本块和指令
// 不需要考虑控制流为常量的情况，你只需要从函数入口基本块(0号基本块)开始dfs，将没有被访问到的基本块和指令删去即可
void SimplifyCFGPass::EliminateUnreachedBlocksInsts(CFG *C) { 
    // Step 1: Perform DFS to mark reachable blocks
    std::set<int> visited;
    std::stack<int> stack;

    // Start DFS from the entry block (assumed to be block 0)
    int entryBlockId = 0;
    stack.push(entryBlockId);
    visited.insert(entryBlockId);

    while (!stack.empty()) {
        int currentBlockId = stack.top();
        stack.pop();

        // Get the successors of the current block
        std::vector<LLVMBlock> successors = C->GetSuccessor(currentBlockId);

        for (LLVMBlock succBlock : successors) {
            int succBlockId = succBlock->block_id;
            if (visited.find(succBlockId) == visited.end()) {
                visited.insert(succBlockId);
                stack.push(succBlockId);
            }
        }
    }

    // Step 2: Collect unreachable blocks
    std::vector<int> unreachableBlocks;
    for (const auto &[blockId, block] : *(C->block_map)) {
        if (visited.find(blockId) == visited.end()) {
            // This block is unreachable
            unreachableBlocks.push_back(blockId);
        }
    }

    // Step 3: Remove unreachable blocks and instructions
    for (int blockId : unreachableBlocks) {
        // Erase the block from block_map
        C->block_map->erase(blockId);

        // Remove from function_block_map if necessary
        // Assuming function_block_map is a member of llvmIR and maps function definitions to block maps
        // Remove edges related to this block in G and invG
        // First, remove outgoing edges from G
        if (blockId < C->G.size()) {
            C->G[blockId].clear();
        }
        // Then, remove incoming edges to this block in invG
        if (blockId < C->invG.size()) {
            C->invG[blockId].clear();
        }
    }

    // Step 4: Rebuild the CFG's adjacency lists to remove references to the deleted blocks
    // For each block, remove successors that are no longer in block_map
    for (auto &[blockId, block] : *(C->block_map)) {
        std::vector<LLVMBlock> &succs = C->G[blockId];
        std::vector<LLVMBlock> newSuccs;
        for (LLVMBlock succBlock : succs) {
            int succBlockId = succBlock->block_id;
            if (C->block_map->find(succBlockId) != C->block_map->end()) {
                newSuccs.push_back(succBlock);
            }
        }
        succs = newSuccs;
    }

    // Similarly, update invG
    for (auto &[blockId, block] : *(C->block_map)) {
        std::vector<LLVMBlock> &preds = C->invG[blockId];
        std::vector<LLVMBlock> newPreds;
        for (LLVMBlock predBlock : preds) {
            int predBlockId = predBlock->block_id;
            if (C->block_map->find(predBlockId) != C->block_map->end()) {
                newPreds.push_back(predBlock);
            }
        }
        preds = newPreds;
    }
    //TODO("EliminateUnreachedBlocksInsts"); 
}