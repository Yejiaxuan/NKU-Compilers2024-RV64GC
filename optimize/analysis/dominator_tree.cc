#include "dominator_tree.h"
#include "../../include/ir.h"
#include <algorithm>
#include <cassert>
#include <iostream>

// ------------------- DomAnalysis::Execute -------------------

void DomAnalysis::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        DominatorTree &dom_tree_instance = DomInfo[cfg];
        dom_tree_instance.C = cfg;
        dom_tree_instance.BuildDominatorTree();
    }
}

// ------------------- DFS 后序遍历 -------------------

void dfs_postorder(int cur, const std::vector<std::vector<BasicBlock*>> &G, std::vector<int> &result,
                   std::vector<int> &vsd) {
    vsd[cur] = 1;
    for (auto next_block : G[cur]) {
        if (vsd[next_block->block_id] == 0) {
            dfs_postorder(next_block->block_id, G, result, vsd);
        }
    }
    result.push_back(cur);
}

// ------------------- DominatorTree::BuildDominatorTree -------------------

void DominatorTree::BuildDominatorTree(bool reverse) { 
    std::cout << "开始构建支配树 (BuildDominatorTree)，reverse=" << reverse << std::endl;
    const std::vector<std::vector<BasicBlock*>>* G = &(C->G);
    const std::vector<std::vector<BasicBlock*>>* invG = &(C->invG);
    int begin_id = 0;

    if (reverse) {
        // 交换 G 和 invG 以构建反向支配树
        std::swap(const_cast<std::vector<std::vector<BasicBlock*>>*&>(G),
                  const_cast<std::vector<std::vector<BasicBlock*>>*&>(invG));
        assert(C->ret_block != nullptr);
        begin_id = C->ret_block->block_id;
        std::cout << "反向支配树，起始节点 ID: " << begin_id << std::endl;
    } else {
        std::cout << "正向支配树，起始节点 ID: " << begin_id << std::endl;
    }

    int block_num = C->max_label + 1;

    std::vector<int> PostOrder_id;
    std::vector<int> vsd(block_num, 0);

    // 初始化支配树结构
    dom_tree.clear();
    dom_tree.resize(block_num);
    idom.clear();
    idom.resize(block_num, nullptr); // 初始化为 nullptr
    atdom.clear();

    // 初始化 df 和 atdom
    df.resize(block_num, std::vector<uint64_t>((block_num +63)/64, 0));
    atdom.resize(block_num, std::vector<uint64_t>((block_num +63)/64, ~0ULL));

    // 只有 begin_id 自己支配自己
    std::fill(atdom[begin_id].begin(), atdom[begin_id].end(), 0);
    set_bit(atdom[begin_id], begin_id, true);
    // 其他节点初始时全部位设置为 1（已通过 ~0ULL 实现）

    // 执行后序遍历
    dfs_postorder(begin_id, (*G), PostOrder_id, vsd);
    std::cout << "后序遍历完成，节点数: " << PostOrder_id.size() << std::endl;

    // 迭代算法计算支配者
    bool changed = true;
    int iteration = 0;
    while (changed) {
        changed = false;
        iteration++;
        std::cout << "支配集迭代 " << iteration << " 开始。" << std::endl;
        // 按逆后序遍历顺序迭代
        for (auto it = PostOrder_id.rbegin(); it != PostOrder_id.rend(); ++it) {
            int u = *it;
            if (u == begin_id) continue;

            // 计算 new_dom_u = 所有前驱 p 的 atdom[p] 的交集
            std::vector<uint64_t> new_dom_u((block_num +63)/64, ~0ULL);
            bool first_pred = true;
            if (!(*invG)[u].empty()) {
                for (auto pred_block : (*invG)[u]) {
                    int p = pred_block->block_id;
                    if (first_pred) {
                        new_dom_u = atdom[p];
                        first_pred = false;
                    } else {
                        new_dom_u = bit_and(new_dom_u, atdom[p]);
                    }
                }
            }
            // 将 u 添加到其支配集
            set_bit(new_dom_u, u, true);

            // 检查 atdom[u] 是否发生变化
            if (!bits_equal(new_dom_u, atdom[u])) {
                atdom[u] = new_dom_u;
                changed = true;
                std::cout << "节点 " << u << " 的支配集发生变化。" << std::endl;
            }
        }
        std::cout << "支配集迭代 " << iteration << " 结束。" << std::endl;
    }

    std::cout << "支配集稳定，开始计算直接支配者 (idom)." << std::endl;

    // 计算直接支配者
    for (int u = 0; u <= C->max_label; u++) {
        if (u == begin_id) continue;
        for (int v = 0; v <= C->max_label; v++) {
            if (get_bit(atdom[u], v)) {
                std::vector<uint64_t> tmp = bit_diff(atdom[u], atdom[v]);
                if (count_bits(tmp) == 1 && get_bit(tmp, u)) {
                    // 确保 v 存在于 block_map 中
                    if (C->block_map->find(v) != C->block_map->end()) {
                        idom[u] = (*C->block_map)[v];
                        dom_tree[v].push_back((*C->block_map)[u]);
                        std::cout << "节点 " << u << " 的直接支配者是节点 " << v << "。" << std::endl;
                    } else {
                        std::cerr << "错误: 块 ID " << v << " 未在 block_map 中找到。" << std::endl;
                    }
                }
            }
        }
    }

    std::cout << "直接支配者计算完成，开始构建支配前沿 (DF)." << std::endl;

    // 构建支配前沿
    df.clear();
    df.resize(block_num, std::vector<uint64_t>((block_num +63)/64, 0));

    for (int i = 0; i < G->size(); i++) {
        for (auto edg_end : (*G)[i]) {
            int a = i;
            int b = edg_end->block_id;
            int x = a;
            while (x == b || !IsDominate(x, b)) {
                set_bit(df[x], b, true);
                std::cout << "设置支配前沿: " << x << " -> " << b << std::endl;
                if (idom[x] != nullptr) {
                    x = idom[x]->block_id;
                } else {
                    break;
                }
            }
        }
    }

    std::cout << "支配前沿构建完成。" << std::endl;

    // 输出支配树结构
    std::cout << "支配树结构:" << std::endl;
    for (int v = 0; v < block_num; ++v) {
        if (dom_tree[v].empty()) continue; // 无子节点
        std::cout << "节点 " << v << " 支配的节点: ";
        for (auto &child : dom_tree[v]) {
            std::cout << child->block_id << " ";
        }
        std::cout << std::endl;
    }

    std::cout << "支配树构建完成 (BuildDominatorTree)." << std::endl;
    //TODO("BuildDominatorTree"); 
}

// ------------------- DominatorTree::GetDF -------------------

// 获取一组节点的支配前沿
std::set<int> DominatorTree::GetDF(std::set<int> S) {
    std::vector<uint64_t> result((C->max_label +1 +63)/64, 0);
    for (auto node : S) {
        std::vector<uint64_t> tmp = df[node];
        // 按位或
        for (size_t i=0; i < result.size(); ++i){
            result[i] |= tmp[i];
        }
    }
    std::set<int> ret;
    for (int i = 0; i <= C->max_label; ++i) {
        if (get_bit(result, i)) {
            ret.insert(i);
        }
    }
    return ret;
    //TODO("GetDF"); 
}

// 获取单个节点的支配前沿
std::set<int> DominatorTree::GetDF(int id) {
    std::set<int> ret;
    for (int i = 0; i <= C->max_label; ++i) {
        if (get_bit(df[id], i)) {
            ret.insert(i);
        }
    }
    return ret;
    //TODO("GetDF"); 
}

// ------------------- DominatorTree::IsDominate -------------------

// 检查 `id1` 是否支配 `id2`
bool DominatorTree::IsDominate(int id1, int id2) {
    return get_bit(atdom[id2], id1);
    //TODO("IsDominate"); 
}
