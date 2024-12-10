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
    // 获取CFG中的基本块数量
    int num_blocks = C->block_map->size();
    if (num_blocks == 0) return;

    // 初始化支配集
    // dom[i] 表示基本块 i 的支配集，使用 block_id 表示
    std::vector<std::set<int>> dom(num_blocks, std::set<int>());
    for (int i = 0; i < num_blocks; ++i) {
        for (int j = 0; j < num_blocks; ++j) {
            dom[i].insert(j);
        }
    }

    // 假设入口块是 0 号基本块，支配集仅包含自身
    dom[0].clear();
    dom[0].insert(0);

    bool changed = true;
    while (changed) {
        changed = false;
        // 遍历所有基本块，除入口块外
        for (int n = 1; n < num_blocks; ++n) {
            // 获取基本块 n 的所有前驱块
            std::vector<LLVMBlock> predecessors = C->GetPredecessor(n);
            if (predecessors.empty()) continue; // 无前驱块，可能是孤立块

            // 计算前驱块的支配集的交集
            std::set<int> new_dom;
            if (!predecessors.empty()) {
                // 初始化为第一个前驱的支配集
                new_dom = dom[predecessors[0]->block_id];
                for (size_t i = 1; i < predecessors.size(); ++i) {
                    std::set<int> temp;
                    std::set_intersection(new_dom.begin(), new_dom.end(),
                                          dom[predecessors[i]->block_id].begin(), dom[predecessors[i]->block_id].end(),
                                          std::inserter(temp, temp.begin()));
                    new_dom = temp;
                }

                // 加上自身
                new_dom.insert(n);

                // 检查支配集是否变化
                if (new_dom != dom[n]) {
                    dom[n] = new_dom;
                    changed = true;
                }
            }
        }
    }

    // 计算立即支配者
    // idom[i] 表示基本块 i 的立即支配者的块指针
    idom.resize(num_blocks, nullptr);
    for (int n = 1; n < num_blocks; ++n) {
        std::set<int> dominators = dom[n];
        dominators.erase(n); // 移除自身
        // 寻找支配集中的最近支配者
        LLVMBlock immediate_dominator = nullptr;
        for (auto it = dominators.begin(); it != dominators.end(); ++it) {
            bool is_immediate = true;
            for (auto jt = dominators.begin(); jt != dominators.end(); ++jt) {
                if (*jt == *it) continue;
                if (dom[*it].find(*jt) != dom[*it].end() && *jt != *it) {
                    // 如果 *jt 也支配 *it，则 *it 不是立即支配者
                    is_immediate = false;
                    break;
                }
            }
            if (is_immediate) {
                immediate_dominator = (*C->block_map)[*it];
                break;
            }
        }
        idom[n] = immediate_dominator;
    }

    // 构建支配树的邻接表
    dom_tree.clear();
    dom_tree.resize(num_blocks, std::vector<LLVMBlock>());
    for (int n = 1; n < num_blocks; ++n) {
        if (idom[n] != nullptr) {
            // idom[n] 是块 n 的立即支配者
            dom_tree[idom[n]->block_id].push_back((*C->block_map)[n]);
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
    if (id1 == id2) return true;
    int current = id2;
    while (idom[current] != nullptr) {
        if (idom[current]->block_id == id1) return true;
        current = idom[current]->block_id;
    }
    return false;
    //TODO("IsDominate"); 
}

