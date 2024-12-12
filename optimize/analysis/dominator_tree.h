#ifndef DOMINATOR_TREE_H
#define DOMINATOR_TREE_H
#include "../../include/ir.h"
#include "../pass.h"
#include <set>
#include <vector>
#include <assert.h>
#include <iostream>

// Reference: https://github.com/yuhuifishash/SysY/blob/master/include/dynamic_bitset.h line4-line57
class DynamicBitset {
private:
    int bit_width;
    int bit_array_length;
    long *bits;

public:
    DynamicBitset() : bit_width(0), bit_array_length(0), bits(nullptr) {}
    void remake(int bit_width) {
        this->bit_width = bit_width;
        if (bits != nullptr) {
            delete[] bits;
        }
        bit_array_length = (bit_width + sizeof(long) - 1) / sizeof(long);
        bits = new long[bit_array_length];
        for (int i = 0; i < bit_array_length; i++) {
            bits[i] = 0;
        }
    }
    DynamicBitset(int bit_width)
        : bit_width(bit_width), bit_array_length((bit_width + sizeof(long) - 1) / sizeof(long)),
          bits(new long[bit_array_length]) {
        for (int i = 0; i < bit_array_length; i++) {
            bits[i] = 0;
        }
    }
    ~DynamicBitset() {
        if (bits != nullptr)
            delete[] bits;
    }
    // Reference: https://github.com/yuhuifishash/SysY/blob/master/utils/dynamic_bitset.cc line5-line111
    int count()
    {
        assert(bit_width);
        int result = 0;
        for (int i = 0; i < bit_array_length; i++) {
            result += __builtin_popcount(bits[i]);
        }
        return result;
    }

    // bool& operator[](int);// assert(this->bit_width > int)
    void setbit(int pos, bool value)
    {
        assert(pos < bit_width);
        int array_idx = pos / sizeof(long);
        int bit_idx = pos % sizeof(long);
        if (value) {
            bits[array_idx] |= (1 << bit_idx);
        } else {
            bits[array_idx] &= ~(1 << bit_idx);
        }
    }
    bool getbit(int pos)
    {
        assert(pos < bit_width);
        int array_idx = pos / sizeof(long);
        int bit_idx = pos % sizeof(long);
        return (bits[array_idx] >> bit_idx) & 1;
    }

    DynamicBitset operator&(DynamicBitset other)    // assert(this->bit_width == other.bit_width)
    {
        assert(bit_width && other.bit_width);
        assert(this->bit_width == other.bit_width);
        DynamicBitset result(bit_width);
        for (int i = 0; i < bit_array_length; i++) {
            result.bits[i] = this->bits[i] & other.bits[i];
        }
        return result;
    }
    DynamicBitset operator|(DynamicBitset other)
    {
        assert(bit_width && other.bit_width);
        assert(this->bit_width == other.bit_width);
        DynamicBitset result(bit_width);
        for (int i = 0; i < bit_array_length; i++) {
            result.bits[i] = this->bits[i] | other.bits[i];
        }
        return result;
    }
    DynamicBitset operator^(DynamicBitset other)
    {
        assert(bit_width && other.bit_width);
        assert(this->bit_width == other.bit_width);
        DynamicBitset result(bit_width);
        for (int i = 0; i < bit_array_length; i++) {
            result.bits[i] = this->bits[i] ^ other.bits[i];
        }
        return result;
    }
    DynamicBitset operator-(DynamicBitset other)
    {
        assert(bit_width && other.bit_width);
        assert(this->bit_width == other.bit_width);
        DynamicBitset result(bit_width);
        for (int i = 0; i < bit_array_length; i++) {
            result.bits[i] = this->bits[i] & (this->bits[i] ^ other.bits[i]);
        }
        return result;
    }

    DynamicBitset operator=(DynamicBitset other)    // Deep Copy
    {
        assert(bit_width && other.bit_width);
        assert(this->bit_width == other.bit_width);
        DynamicBitset result(bit_width);
        for (int i = 0; i < bit_array_length; i++) {
            result.bits[i] = this->bits[i] = other.bits[i];
        }
        return result;
    }

    bool operator==(DynamicBitset other)    // Deep Compare
    {
        assert(bit_width && other.bit_width);
        assert(this->bit_width == other.bit_width);
        for (int i = 0; i < bit_array_length; i++) {
            if (this->bits[i] != other.bits[i]) {
                return false;
            }
        }
        return true;
    }
    bool operator!=(DynamicBitset other)    // return !(*this == other)
    {
        assert(bit_width && other.bit_width);
        assert(this->bit_width == other.bit_width);
        for (int i = 0; i < bit_array_length; i++) {
            if (this->bits[i] != other.bits[i]) {
                return true;
            }
        }
        return false;
    }

    DynamicBitset(const DynamicBitset &other)
    {
        this->bit_width = other.bit_width;
        this->bit_array_length = other.bit_array_length;
        this->bits = new long[bit_array_length];
        for (int i = 0; i < bit_array_length; i++) {
            this->bits[i] = other.bits[i];
        }
    }
};

class DominatorTree {
public:
    CFG *C;
    std::vector<std::vector<LLVMBlock>> dom_tree{};
    std::vector<LLVMBlock> idom{};

    void BuildDominatorTree(bool reverse = false);    // build the dominator tree of CFG* C
    void BuildPostDominatorTree();
    std::set<int> GetDF(std::set<int> S);             // return DF(S)  S = {id1,id2,id3,...}
    std::set<int> GetDF(int id);                      // return DF(id)
    bool IsDominate(int id1, int id2);                // if blockid1 dominate blockid2, return true, else return false

    std::vector<DynamicBitset> df;
    std::vector<DynamicBitset> atdom;
    
    // TODO(): add or modify functions and members if you need
};

class DomAnalysis : public IRPass {
private:
    std::map<CFG *, DominatorTree> DomInfo;

public:
    DomAnalysis(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
    DominatorTree *GetDomTree(CFG *C) { return &DomInfo[C]; }
    // TODO(): add more functions and members if you need
};
#endif
