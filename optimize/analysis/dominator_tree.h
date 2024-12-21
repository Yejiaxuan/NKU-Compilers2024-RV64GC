#ifndef DOMINATOR_TREE_H
#define DOMINATOR_TREE_H

#include "../../include/ir.h"
#include "../pass.h"
#include <set>
#include <vector>
#include <cassert>
#include <cstdint>
#include <map>
#include <iostream>

// ------------------- 辅助函数 -------------------

// 在位集 `bits` 的位置 `pos` 设置位为 `value`。
inline void set_bit(std::vector<uint64_t> &bits, int pos, bool value) {
    int word = pos / 64;
    int bit = pos % 64;
    if (word >= bits.size()) {
        bits.resize(word + 1, 0);
    }
    if (value) {
        bits[word] |= (1ULL << bit);
    } else {
        bits[word] &= ~(1ULL << bit);
    }
}

// 获取位集 `bits` 中位置 `pos` 的位值。
inline bool get_bit(const std::vector<uint64_t> &bits, int pos) {
    int word = pos / 64;
    int bit = pos % 64;
    if (word >= bits.size()) {
        return false;
    }
    return (bits[word] >> bit) & 1ULL;
}

// 对两个位集执行按位与操作。
inline std::vector<uint64_t> bit_and(const std::vector<uint64_t> &a, const std::vector<uint64_t> &b) {
    assert(a.size() == b.size());
    std::vector<uint64_t> result(a.size());
    for (size_t i = 0; i < a.size(); ++i) {
        result[i] = a[i] & b[i];
    }
    return result;
}

// 对两个位集执行按位或操作。
inline std::vector<uint64_t> bit_or(const std::vector<uint64_t> &a, const std::vector<uint64_t> &b) {
    assert(a.size() == b.size());
    std::vector<uint64_t> result(a.size());
    for (size_t i = 0; i < a.size(); ++i) {
        result[i] = a[i] | b[i];
    }
    return result;
}

// 对两个位集执行按位异或操作。
inline std::vector<uint64_t> bit_xor(const std::vector<uint64_t> &a, const std::vector<uint64_t> &b) {
    assert(a.size() == b.size());
    std::vector<uint64_t> result(a.size());
    for (size_t i = 0; i < a.size(); ++i) {
        result[i] = a[i] ^ b[i];
    }
    return result;
}

// 对两个位集执行差集操作 (a - b)。
inline std::vector<uint64_t> bit_diff(const std::vector<uint64_t> &a, const std::vector<uint64_t> &b) {
    assert(a.size() == b.size());
    std::vector<uint64_t> result(a.size());
    for (size_t i = 0; i < a.size(); ++i) {
        result[i] = a[i] & ~b[i];
    }
    return result;
}

// 计算位集中设置位的数量。
inline int count_bits(const std::vector<uint64_t> &bits) {
    int cnt = 0;
    for(auto word : bits) {
        cnt += __builtin_popcountll(word);
    }
    return cnt;
}

// 检查两个位集是否相等。
inline bool bits_equal(const std::vector<uint64_t> &a, const std::vector<uint64_t> &b) {
    if (a.size() != b.size()) return false;
    for(size_t i=0; i < a.size(); ++i){
        if(a[i] != b[i]) return false;
    }
    return true;
}

// ------------------- DominatorTree 类 -------------------

class DominatorTree {
public:
    CFG *C; // 指向控制流图的指针
    std::vector<std::vector<BasicBlock*>> dom_tree; // 支配树
    std::vector<BasicBlock*> idom; // 直接支配者

    // 构建支配树
    void BuildDominatorTree(bool reverse = false);

    // 获取一组节点的支配前沿
    std::set<int> GetDF(std::set<int> S);
    // 获取单个节点的支配前沿
    std::set<int> GetDF(int id);

    // 检查 `id1` 是否支配 `id2`
    bool IsDominate(int id1, int id2);

    std::vector<std::vector<uint64_t>> df; // 支配前沿
    std::vector<std::vector<uint64_t>> atdom; // 支配关系

    // TODO: 根据需要添加或修改函数和成员
};

// ------------------- DomAnalysis 类 -------------------

class DomAnalysis : public IRPass {
private:
    std::map<CFG *, DominatorTree> DomInfo; // 从 CFG 到 DominatorTree 的映射

public:
    DomAnalysis(LLVMIR *IR) : IRPass(IR) {}

    // 执行支配分析
    void Execute();

    // 获取特定 CFG 的 DominatorTree
    DominatorTree *GetDomTree(CFG *C) { return &DomInfo[C]; }

    // TODO: 根据需要添加更多函数和成员
};

// ------------------- DFS 后序函数 -------------------

// 用于构建支配树的深度优先搜索后序遍历
void dfs_postorder(int cur, const std::vector<std::vector<BasicBlock*>> &G, std::vector<int> &result,
                   std::vector<int> &vsd);

#endif // DOMINATOR_TREE_H
