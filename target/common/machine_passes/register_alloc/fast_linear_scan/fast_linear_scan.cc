#include "fast_linear_scan.h"
bool IntervalsPrioCmp(LiveInterval a, LiveInterval b) { return a.begin()->begin > b.begin()->begin; }
FastLinearScan::FastLinearScan(MachineUnit *unit, PhysicalRegistersAllocTools *phy, SpillCodeGen *spiller)
    : RegisterAllocation(unit, phy, spiller), unalloc_queue(IntervalsPrioCmp) {}
bool FastLinearScan::DoAllocInCurrentFunc() {
    bool spilled = false;
    auto mfun = current_func;
    PRINT("FastLinearScan: %s", mfun->getFunctionName().c_str());
    // std::cerr<<"FastLinearScan: "<<mfun->getFunctionName()<<"\n";
    phy_regs_tools->clear();
    for (auto interval : intervals) {
        Assert(interval.first == interval.second.getReg());
        if (interval.first.is_virtual) {
            // 需要分配的虚拟寄存器
            unalloc_queue.push(interval.second);
        } else {
            // Log("Pre Occupy Physical Reg %d",interval.first.reg_no);
            // 预先占用已经存在的物理寄存器
            phy_regs_tools->OccupyReg(interval.first.reg_no, interval.second);
        }
    }
    // //TODO: 进行线性扫描寄存器分配, 为每个虚拟寄存器选择合适的物理寄存器或者将其溢出到合适的栈地址中
    // 该函数中只需正确设置alloc_result，并不需要实际生成溢出代码
    //TODO("LinearScan");

    while (!unalloc_queue.empty()) {
        auto interval = unalloc_queue.top();
        unalloc_queue.pop();
        auto cur_vreg = interval.getReg();
        std::vector<int> prefered_regs, noprefer_regs;
        for (auto reg : copy_sources[cur_vreg]) {
            if (reg.is_virtual) {
                if (alloc_result[mfun].find(reg) != alloc_result[mfun].end()) {
                    if (alloc_result[mfun][reg].in_mem == false) {
                        prefered_regs.push_back(alloc_result[mfun][reg].phy_reg_no);
                    }
                }
            } else {
                prefered_regs.push_back(reg.reg_no);
            }
        }
#ifdef ENABLE_WAW_ELIMATE
        for (auto seg : interval) {
            int def = seg.begin;
            Assert(numbertoins.find(def) != numbertoins.end());
            auto cur_ins = numbertoins[def].ins;
            if (cur_ins == nullptr)
                continue;
            const int pre_len = 10;
            for (int i = 1; i < pre_len; i++) {
                int pre_no = def - i;
                if (numbertoins[pre_no].is_block_begin) {
                    break;
                }
                auto pre_ins = numbertoins[pre_no].ins;
                int pre_latency = pre_ins->GetLatency();
                if (pre_latency < i)
                    continue;
                if (pre_ins->GetWriteReg().size() == 1) {
                    auto reg = pre_ins->GetWriteReg()[0];
                    if (reg->is_virtual) {
                        if (alloc_result[mfun].find(*reg) != alloc_result[mfun].end()) {
                            if (alloc_result[mfun][*reg].in_mem == false) {
                                Assert(alloc_result[mfun][*reg].phy_reg_no != 0);
                                noprefer_regs.push_back(alloc_result[mfun][*reg].phy_reg_no);
                            }
                        }
                    } else {
                        if (reg->reg_no != 0) {
                            noprefer_regs.push_back(reg->reg_no);
                        }
                    }
                }
            }
            for (int i = 1; i <= cur_ins->GetLatency(); i++) {
                int after_no = def + i;
                if (numbertoins[after_no].is_block_begin) {
                    break;
                }
                auto after_ins = numbertoins[after_no].ins;
                if (after_ins->GetWriteReg().size() == 1) {
                    auto reg = after_ins->GetWriteReg()[0];
                    if (reg->is_virtual) {
                        if (alloc_result[mfun].find(*reg) != alloc_result[mfun].end()) {
                            if (alloc_result[mfun][*reg].in_mem == false) {
                                Assert(alloc_result[mfun][*reg].phy_reg_no != 0);
                                noprefer_regs.push_back(alloc_result[mfun][*reg].phy_reg_no);
                            }
                        }
                    } else {
                        if (reg->reg_no != 0) {
                            noprefer_regs.push_back(reg->reg_no);
                        }
                    }
                }
            }
        }
#endif
        int phy_reg_id = phy_regs_tools->getIdleReg(interval, prefered_regs, noprefer_regs);
        if (phy_reg_id >= 0) {
            phy_regs_tools->OccupyReg(phy_reg_id, interval);
            AllocPhyReg(mfun, cur_vreg, phy_reg_id);
        } else {
            spilled = true;

            int mem = phy_regs_tools->getIdleMem(interval);
            phy_regs_tools->OccupyMem(mem, cur_vreg.getDataWidth(), interval);
            // volatile int mem_ = mem;
            // volatile int mem__ = mem_+current_func->GetStackOffset();
            AllocStack(mfun, cur_vreg, mem);

            double spill_weight = CalculateSpillWeight(interval);
            auto spill_interval = interval;
            for (auto other : phy_regs_tools->getConflictIntervals(interval)) {
                double other_weight = CalculateSpillWeight(other);
                if (spill_weight > other_weight && other.getReg().is_virtual) {
                    spill_weight = other_weight;
                    spill_interval = other;
                }
            }

            if (!(interval == spill_interval)) {
                phy_regs_tools->swapRegspill(getAllocResultInReg(mfun, spill_interval.getReg()), spill_interval, mem,
                                       cur_vreg.getDataWidth(), interval);
                swapAllocResult(mfun, interval.getReg(), spill_interval.getReg());
                // alloc_result[mfun].erase(spill_interval.getReg());
                // unalloc_queue.push(spill_interval);
                int spill_mem = phy_regs_tools->getIdleMem(spill_interval);
                phy_regs_tools->OccupyMem(spill_mem, spill_interval.getReg().getDataWidth(), spill_interval);
                AllocStack(mfun, spill_interval.getReg(), spill_mem);
            }
        }
    }

    // 返回是否发生溢出
    return spilled;
}

// 计算溢出权重
double FastLinearScan::CalculateSpillWeight(LiveInterval interval) {
    return (double)interval.getReferenceCount() / interval.getIntervalLen();
}
