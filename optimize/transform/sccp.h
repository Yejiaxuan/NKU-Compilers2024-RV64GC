#ifndef SCCP_H
#define SCCP_H
#include "../../include/cfg.h"
#include "../analysis/dominator_tree.h"
#include <assert.h>

class ConstLattice {
public:
    enum LatticeStatus {
        UNINIT = 0,
        CONST = 1,
        VAR = 2,
    } status;
    enum LatticeType {
        NONE = 0,
        I32 = 1,
        FLOAT = 2,
    } val_type;
    union {
        int I32Val;
        float FloatVal;
    } vals;
    ConstLattice() {
        status = UNINIT;
        val_type = NONE;
    }
    ConstLattice(LatticeStatus s, LatticeType type, int val) {
        status = s;
        val_type = type;
        vals.I32Val = val;
    }
    ConstLattice(LatticeStatus s, LatticeType type, float val) {
        status = s;
        val_type = type;
        vals.FloatVal = val;
    }
};

class SCCPPass : public IRPass {
private:
    // 构建 SSA 图
    void BuildSSAGraph(CFG *C);

    // 设置每条指令所属的基本块ID
    void SetInstructionBlockID(CFG *C);

    // 更新格点状态（用于phi指令）
    bool UpdateLatticeStatus(Instruction I, ConstLattice pre_lattice);

    // 访问操作（执行常量传播逻辑）
    void VisitOperation(Instruction I, std::set<Instruction> &SSAWorklist, 
                        std::map<std::pair<int, int>, int> &CFGedgeExec,
                        std::set<std::pair<int, int>> &CFGWorklist, 
                        std::map<int, Instruction> &regresult_map,
                        std::map<Instruction, std::vector<Instruction>> &SSA_G);

    // 替换寄存器为常量
    void ReplaceRegToConst(CFG *C);

    // 在SCCP后消除死块
    void DeadBlockEliminateAfterSCCP(CFG *C, std::map<std::pair<int, int>, int> &CFGedgeExec);

    // 消除常量指令
    void ConstInstructionEliminate(CFG *C);

    // 执行SCCP算法
    void PerformSCCP(CFG *C);

public:
    // 构造函数
    SCCPPass(LLVMIR *IR, DomAnalysis *dom) : IRPass(IR) {}
    
    // 执行SCCP优化
    void Execute();
};


#endif