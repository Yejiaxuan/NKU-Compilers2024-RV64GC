#include "simple_alloca.h"

SimpleAlloca::SimpleAlloca(MachineUnit *unit, PhysicalRegistersAllocTools *phy, SpillCodeGen *spiller)
    : RegisterAllocation(unit, phy, spiller){}

bool SimpleAlloca::DoAllocInCurrentFunc() {
    bool spilled = false;
    auto mfun = current_func;

    // 清空物理寄存器的占用状态
    phy_regs_tools->clear();

    // 首先将所有预先占有的物理寄存器（非虚拟寄存器）占用
    for (auto interval_pair : intervals) {
        Register reg = interval_pair.first;
        LiveInterval interval = interval_pair.second;
        if (!reg.is_virtual) {
            // 对于已分配物理寄存器，直接占用对应的物理寄存器编号
            phy_regs_tools->OccupyReg(reg.reg_no, interval);
        }
    }

    // 遍历所有虚拟寄存器的活跃区间，进行简单分配
    for (auto interval_pair : intervals) {
        Register reg = interval_pair.first;
        LiveInterval interval = interval_pair.second;
        if (!reg.is_virtual)
            continue; // 非虚拟寄存器已处理

        // 此处不使用偏好寄存器，直接尝试获取一个空闲寄存器
        int phy_reg = phy_regs_tools->getIdleReg(interval, std::vector<int>{}, std::vector<int>{});
        if (phy_reg >= 0) {
            // 分配物理寄存器：记录分配结果并更新物理寄存器状态
            AllocPhyReg(mfun, reg, phy_reg);
            phy_regs_tools->OccupyReg(phy_reg, interval);
        } else {
            // 若无可用寄存器，则溢出到内存
            spilled = true;
            int mem = phy_regs_tools->getIdleMem(interval);
            phy_regs_tools->OccupyMem(mem, reg.getDataWidth(), interval);
            AllocStack(mfun, reg, mem);
        }
    }

    return spilled;
}

void SimpleAlloca::CoalesceInCurrentFunc() {
    // 无合并策略
}