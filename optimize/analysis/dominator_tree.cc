#include "dominator_tree.h"
#include "../../include/ir.h"

void DomAnalysis::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        DominatorTree &dom_tree_instance = DomInfo[cfg];
        dom_tree_instance.C = cfg;
        dom_tree_instance.BuildDominatorTree();
    }
}

void DominatorTree::BuildDominatorTree(bool reverse) { 
    // 获取CFG中的所有基本块
    if (!C || !C->block_map) {
        return;
    }

    // 首先找到最大 block_id 以确定向量的大小
    int max_block_id = -1;
    for (const auto& [id, block] : *(C->block_map)) {
        if (id > max_block_id) {
            max_block_id = id;
        }
    }

    if (max_block_id == -1) {
        return;
    }

    // 为了处理任意的 block_id，我们使用一个映射将 block_id 映射到向量的索引
    std::map<int, int> block_id_to_index;
    std::map<int, int> index_to_block_id;
    int index = 0;
    for (const auto& [id, block] : *(C->block_map)) {
        block_id_to_index[id] = index;
        index_to_block_id[index] = id;
        index++;
    }
    int num_blocks = index; // 实际的基本块数量

    // 初始化支配集
    // dom[i] 表示基本块索引 i 的支配集（存储 block_id）
    std::vector<std::set<int>> dom(num_blocks, std::set<int>());
    for (int i = 0; i < num_blocks; ++i) {
        for (int j = 0; j < num_blocks; ++j) {
            dom[i].insert(index_to_block_id[j]);
        }
    }

    // 假设入口块是 block_id 最小的基本块，通常为 0
    // 如果入口块的 block_id 不是 0，可以根据实际情况调整
    int entry_block_id = 0; // 您可以根据需要更改入口块的 ID
    if (C->block_map->find(entry_block_id) == C->block_map->end()) {
        // 如果 0 号块不存在，选择最小的 block_id 作为入口块
        entry_block_id = std::min_element(
            C->block_map->begin(),
            C->block_map->end(),
            [](const std::pair<int, LLVMBlock>& a, const std::pair<int, LLVMBlock>& b) {
                return a.first < b.first;
            }
        )->first;
    }

    // 获取入口块的索引
    int entry_index = block_id_to_index[entry_block_id];

    // 设置入口块的支配集仅包含自身
    dom[entry_index].clear();
    dom[entry_index].insert(entry_block_id);

    bool changed = true;
    while (changed) {
        changed = false;
        // 遍历所有基本块，除入口块外
        for (int i = 0; i < num_blocks; ++i) {
            if (i == entry_index) continue; // 跳过入口块

            int current_block_id = index_to_block_id[i];
            std::vector<LLVMBlock> predecessors = C->GetPredecessor(current_block_id);

            if (predecessors.empty()) continue; // 无前驱块，可能是孤立块

            // 计算前驱块的支配集的交集
            std::set<int> new_dom;
            bool first_pred = true;
            for (const auto& pred_block : predecessors) {
                if (!pred_block) {
                    continue;
                }
                int pred_block_id = pred_block->block_id;
                if (block_id_to_index.find(pred_block_id) == block_id_to_index.end()) {
                    continue;
                }
                int pred_index = block_id_to_index[pred_block_id];
                if (first_pred) {
                    new_dom = dom[pred_index];
                    first_pred = false;
                } else {
                    std::set<int> temp;
                    std::set_intersection(new_dom.begin(), new_dom.end(),
                                          dom[pred_index].begin(), dom[pred_index].end(),
                                          std::inserter(temp, temp.begin()));
                    new_dom = temp;
                }
            }

            // 加上自身
            new_dom.insert(current_block_id);

            // 检查支配集是否变化
            if (new_dom != dom[i]) {
                dom[i] = new_dom;
                changed = true;
            }
        }
    }

    // 计算立即支配者
    // idom[i] 表示基本块索引 i 的立即支配者的 block_id
    idom.resize(num_blocks, nullptr);
    for (int i = 0; i < num_blocks; ++i) {
        if (i == entry_index) {
            idom[i] = nullptr; // 入口块没有立即支配者
            continue;
        }

        int current_block_id = index_to_block_id[i];
        std::set<int> dominators = dom[i];
        dominators.erase(current_block_id); // 移除自身

        // 寻找支配集中的最近支配者
        LLVMBlock immediate_dominator = nullptr;
        for (int dom_id : dominators) {
            bool is_immediate = true;
            for (int other_dom_id : dominators) {
                if (dom_id == other_dom_id) continue;
                if (dom[block_id_to_index[dom_id]].find(other_dom_id) != dom[block_id_to_index[dom_id]].end()) {
                    // 如果 other_dom_id 也支配 dom_id，则 dom_id 不是立即支配者
                    is_immediate = false;
                    break;
                }
            }
            if (is_immediate) {
                immediate_dominator = (*(C->block_map))[dom_id];
                break;
            }
        }
        idom[i] = immediate_dominator;
    }

    // 构建支配树的邻接表
    dom_tree.clear();
    dom_tree.resize(num_blocks, std::vector<LLVMBlock>());
    for (int i = 0; i < num_blocks; ++i) {
        if (idom[i] != nullptr) {
            int idom_block_id = idom[i]->block_id;
            if (block_id_to_index.find(idom_block_id) != block_id_to_index.end()) {
                int idom_index = block_id_to_index[idom_block_id];
                int current_block_id = index_to_block_id[i];
                if (block_id_to_index.find(current_block_id) != block_id_to_index.end()) {
                    int current_index = block_id_to_index[current_block_id];
                    dom_tree[idom_index].push_back((*(C->block_map))[current_block_id]);
                }
            }
        }
    }
    //TODO("BuildDominatorTree"); 
}

std::set<int> DominatorTree::GetDF(std::set<int> S) {
    std::set<int> df;
    for (int id : S) {
        if (id >= 0 && id < idom.size()) {
            LLVMBlock idominator = idom[id];
            if (idominator != nullptr) {
                df.insert(idominator->block_id);
            }
        }
    }
    return df;
    //TODO("GetDF"); 
}

std::set<int> DominatorTree::GetDF(int id) {
    std::set<int> s;
    s.insert(id);
    return GetDF(s);
    //TODO("GetDF"); 
}

bool DominatorTree::IsDominate(int id1, int id2) {
    if (id1 == id2) {
        return true;
    }
    int current = id2;
    while (idom[current] != nullptr) {
        if (idom[current]->block_id == id1) {
            return true;
        }
        current = idom[current]->block_id;
    }
    return false;
    //TODO("IsDominate"); 
}

