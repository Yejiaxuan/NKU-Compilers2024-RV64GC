#include "physical_register.h"
bool PhysicalRegistersAllocTools::OccupyReg(int phy_id, LiveInterval interval) {
    // 1. 检查物理寄存器是否已经被其他区间占用，且是否冲突
    for (auto other_interval : phy_occupied[phy_id]) {
        if (interval & other_interval) {  // 如果有冲突
            return false;  // 不能分配该寄存器，返回 false
        }
    }

    // 2. 没有冲突，可以分配该寄存器
    phy_occupied[phy_id].push_back(interval);
    return true;
}


bool PhysicalRegistersAllocTools::ReleaseReg(int phy_id, LiveInterval interval) { 
    //TODO("ReleaseReg"); 
    auto &occupied_list = phy_occupied[phy_id];  // 引用寄存器占用列表
    auto it = std::find(occupied_list.begin(), occupied_list.end(), interval);
    if (it != occupied_list.end()) {
        occupied_list.erase(it);  // 如果找到，则删除
        return true;
    }
    return false;  // 未找到对应的 interval
}

bool PhysicalRegistersAllocTools::OccupyMem(int offset, LiveInterval interval) {
    //TODO("OccupyMem");
    int required_size = offset + (interval.getReg().getDataWidth() / 4);
    if (required_size > mem_occupied.size()) {
        mem_occupied.resize(required_size);
    }

    // 将 interval 添加到对应的内存位置
    std::for_each(mem_occupied.begin() + offset, 
                  mem_occupied.begin() + required_size, 
                  [&](std::vector<LiveInterval> &slot) {
                      slot.push_back(interval);
                  });

    return true;
}

bool PhysicalRegistersAllocTools::ReleaseMem(int offset, LiveInterval interval) {
    //TODO("ReleaseMem");
    // 计算需要释放的内存范围
    int size = interval.getReg().getDataWidth() / 4;
    int end = offset + size;

    // 遍历内存范围并移除 interval
    std::for_each(mem_occupied.begin() + offset, 
                  mem_occupied.begin() + end, 
                  [&](std::vector<LiveInterval> &slot) {
                      auto it = std::find(slot.begin(), slot.end(), interval);
                      if (it != slot.end()) {
                          slot.erase(it);
                      }
                  });

    return true;
}

int PhysicalRegistersAllocTools::getIdleReg(LiveInterval interval) {
    //TODO("getIdleReg");
    // 遍历所有可能分配的寄存器
    for (auto reg_no : getValidRegs(interval)) {
        bool canAllocate = true;
        // 检查该寄存器及其别名冲突情况
        for (auto alias : getAliasRegs(reg_no)) {
            for (auto other_interval : phy_occupied[alias]) {
                if (interval & other_interval) {
                    canAllocate = false;
                    break;
                }
            }
            if (!canAllocate) break;
        }
        if (canAllocate) {
            return reg_no;
        }
    }
    // 未找到合适的寄存器返回 -1
    return -1;
}

int PhysicalRegistersAllocTools::getIdleMem(LiveInterval interval) { 
    //TODO("getIdleMem");
    int required_size = interval.getReg().getDataWidth() / 4; // 计算所需内存大小

    // 查找连续的空闲内存块
    auto it = std::search_n(mem_occupied.begin(), mem_occupied.end(), required_size, std::vector<LiveInterval>{},
                            [&](const std::vector<LiveInterval> &slot, const std::vector<LiveInterval> &) {
                                // 检查当前内存槽是否有冲突
                                return std::all_of(slot.begin(), slot.end(), [&](const LiveInterval &other_interval) {
                                    return !(interval & other_interval);
                                });
                            });

    // 如果找到合适的内存块，返回其起始位置；否则返回 mem_occupied 的大小（下一个空闲位置）
    return (it != mem_occupied.end()) ? std::distance(mem_occupied.begin(), it) : mem_occupied.size();
}

int PhysicalRegistersAllocTools::swapRegspill(int p_reg1, LiveInterval interval1, int offset_spill2, int size,
                                              LiveInterval interval2) {

    //TODO("swapRegspill");
    return 0;
}

std::vector<LiveInterval> PhysicalRegistersAllocTools::getConflictIntervals(LiveInterval interval) {
    std::vector<LiveInterval> result;
    for (auto phy_intervals : phy_occupied) {
        for (auto other_interval : phy_intervals) {
            if (interval.getReg().type == other_interval.getReg().type && (interval & other_interval)) {
                result.push_back(other_interval);
            }
        }
    }
    return result;
}

