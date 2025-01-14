#ifndef LivEInterval_H
#define LivEInterval_H
#include "../machine_pass.h"
#include <assert.h>
#include <queue>
class LiveInterval {
private:
    Register reg;
    // 当begin和end不同时, 活跃区间为[begin,end), 即左闭右开
    // 当begin和end相同时, 表示[begin,end], 即一个单点 (这么做的原因是方便活跃区间计算)
    // 注意特殊判断begin和end相同时的情况
    struct LiveSegment {
        int begin;
        int end;
        bool inside(int pos) const {
            if (begin == end) return begin == pos;
            return begin <= pos && pos < end; 
        }
        bool operator&(const struct LiveSegment &that) const {
            return this->inside(that.begin) || this->inside(that.end - 1 > that.begin ? that.end - 1 : that.begin) ||
                   that.inside(this->begin) || that.inside(this->end - 1 > this->begin ? this->end - 1 : this->begin);
        }
        bool operator==(const struct LiveSegment &that) const {
            return this->begin == that.begin && this->end == that.end;
        }
    };
    std::list<LiveSegment> segments{};
    int reference_count;

public:
    // 检测两个活跃区间是否重叠
    // 保证两个活跃区间各个段各自都是不降序（升序）排列的
    bool operator&(const LiveInterval &that) const {
        //TODO("& operator in LiveInterval");
        // 如果任一方的 segments 为空，直接返回 false
        if (segments.empty() || that.segments.empty()) {
            return false;
        }

        // 遍历两个区间列表，检查是否有交集
        for (const auto& seg1 : segments) {
            for (const auto& seg2 : that.segments) {
                // 检查两个区间是否重叠
                if (seg1.begin < seg2.end && seg1.end > seg2.begin) {
                    return true;  // 如果发现交集，返回 true
                }
            }
        }

        return false;  // 如果没有交集，返回 false
    }

    bool operator==(const LiveInterval &that) const {
        // TODO : Judge if *this and that are equal
        if (reg == that.reg) {
            Assert(segments == that.segments);
            return true;
        } else {
            return false;
        }
        return reg == that.reg;    // && segments == that.segments;
    }
    LiveInterval operator|(const LiveInterval &that) const {
        LiveInterval ret(this->reg);
        ret.reference_count = this->reference_count + that.reference_count - 2;
        auto it = segments.begin();
        auto jt = that.segments.begin();
        while (1) {
            if (it == segments.end() && jt == that.segments.end()) {
                break;
            }
            if (it == segments.end()) {
                ret.segments.push_back(*jt);
                ++jt;
                continue;
            }
            if (jt == that.segments.end()) {
                ret.segments.push_back(*it);
                ++it;
                continue;
            }
            if (it->begin < jt->begin) {
                ret.segments.push_back(*it);
                ++it;
            } else {
                ret.segments.push_back(*jt);
                ++jt;
            }
        }
        return ret;
    }

    // 更新引用计数
    void IncreaseReferenceCount(int count) { reference_count += count; }
    int getReferenceCount() { return reference_count; }
    // 返回活跃区间长度
    int getIntervalLen() {
        int ret = 0;
        for (auto seg : segments) {
            ret += (seg.end - seg.begin + 1);
        }
        return ret;
    }
    Register getReg() { return reg; }
    LiveInterval() : reference_count(0) {}    // Temp
    LiveInterval(Register reg) : reg(reg), reference_count(0) {}

    void PushFront(int begin, int end) { segments.push_front({begin = begin, end = end}); }
    void SetMostBegin(int begin) { segments.begin()->begin = begin; }

    // 可以直接 for(auto segment : liveinterval)
    decltype(segments.begin()) begin() { return segments.begin(); }
    decltype(segments.end()) end() { return segments.end(); }
};

class Liveness {
private:
    MachineFunction *current_func;
    // 更新所有块DEF和USE集合
    void UpdateDefUse();
    // Key: Block_Number
    // 存储活跃变量分析的结果
    std::map<int, std::set<Register>> IN{}, OUT{}, DEF{}, USE{};

public:
    // 对所有块进行活跃变量分析并保存结果
    void Execute();
    Liveness(MachineFunction *mfun, bool calculate = true) : current_func(mfun) {
        if (calculate) {
            Execute();
        }
    }
    // 获取基本块的IN/OUT/DEF/USE集合
    std::set<Register> GetIN(int bid) { return IN[bid]; }
    std::set<Register> GetOUT(int bid) { return OUT[bid]; }
    std::set<Register> GetDef(int bid) { return DEF[bid]; }
    std::set<Register> GetUse(int bid) { return USE[bid]; }
};
#endif