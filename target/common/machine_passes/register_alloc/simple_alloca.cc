#include "simple_alloca.h"

SimpleAlloca::SimpleAlloca(MachineUnit *unit, PhysicalRegistersAllocTools *phy, SpillCodeGen *spiller)
    : RegisterAllocation(unit, phy, spiller){}

bool SimpleAlloca::DoAllocInCurrentFunc() {
    // 表示是否有虚拟寄存器溢出到内存
    bool spilledFlag = false;
    auto mfun = current_func;

    // 首先清除所有物理寄存器的占用状态
    phy_regs_tools->clear();

    // 定义辅助 lambda：处理已分配物理寄存器（非虚拟寄存器）的占用
    auto occupyPhysicalRegs = [this](const Register &reg, const LiveInterval &interval) {
        // 直接占用该物理寄存器编号
        phy_regs_tools->OccupyReg(reg.reg_no, interval);
    };

    // 定义辅助 lambda：处理虚拟寄存器的分配，如果无法分配物理寄存器则溢出到内存
    auto allocateVirtualRegs = [this, mfun, &spilledFlag](const Register &reg, const LiveInterval &interval) {
        // 尝试分配一个空闲的物理寄存器
        int idleReg = phy_regs_tools->getIdleReg(interval);
        if (idleReg >= 0) {
            // 分配成功，记录分配结果并更新寄存器状态
            AllocPhyReg(mfun, reg, idleReg);
            phy_regs_tools->OccupyReg(idleReg, interval);
        } else {
            // 无可用寄存器，则溢出到内存
            spilledFlag = true;
            int idleMem = phy_regs_tools->getIdleMem(interval);
            phy_regs_tools->OccupyMem(idleMem, interval);
            AllocStack(mfun, reg, idleMem);
        }
    };

    // 1. 先处理所有非虚拟寄存器，直接占用预先分配的物理寄存器
    for (const auto &pair : intervals) {
        const auto &reg = pair.first;
        const auto &interval = pair.second;
        if (!reg.is_virtual) {
            occupyPhysicalRegs(reg, interval);
        }
    }

    // 2. 再处理所有虚拟寄存器，进行寄存器分配或内存溢出
    for (const auto &pair : intervals) {
        const auto &reg = pair.first;
        const auto &interval = pair.second;
        if (reg.is_virtual) {
            allocateVirtualRegs(reg, interval);
        }
    }

    return spilledFlag;
}



