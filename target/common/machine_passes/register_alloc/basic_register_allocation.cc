#include "basic_register_allocation.h"

void RegisterAllocation::Execute() {
    // 初始化分配队列
    InitializeAllocationQueue();

    // 处理分配队列中的函数
    while (!not_allocated_funcs.empty()) {
        current_func = not_allocated_funcs.front();
        not_allocated_funcs.pop();

        // 初始化当前函数的分配
        InitializeAllocation(current_func);

        // 尝试进行寄存器分配
        if (AttemptAllocation(current_func)) {
            // 如果发生溢出，处理溢出
            HandleSpillingIfNeeded(current_func);
        }
    }

    // 重写虚拟寄存器，全部转换为物理寄存器
    FinalizeAllocation();
}

void RegisterAllocation::InitializeAllocationQueue() {
    // 将所有函数加入待分配队列
    for (auto func : unit->functions) {
        not_allocated_funcs.push(func);
    }
}

void RegisterAllocation::InitializeAllocation(MachineFunction *mfun) {
    numbertoins.clear();
    // 对每条指令进行编号
    InstructionNumber(unit, numbertoins).ExecuteInFunc(mfun);

    // 清除之前分配的结果
    ClearPreviousAllocResult(mfun);

    // 计算活跃区间
    UpdateIntervalsInCurrentFunc();
}

void RegisterAllocation::ClearPreviousAllocResult(MachineFunction *mfun) {
    // 清空之前的分配结果
    alloc_result[mfun].clear();
}

bool RegisterAllocation::AttemptAllocation(MachineFunction *mfun) {
    // 尝试在当前函数中进行寄存器分配
    return DoAllocInCurrentFunc();
}

void RegisterAllocation::HandleSpillingIfNeeded(MachineFunction *mfun) {
    // 如果需要溢出处理，则生成溢出代码
    spiller->ExecuteInFunc(mfun, &alloc_result[mfun]);

    // 调整栈的大小以容纳溢出的变量
    mfun->AddStackSize(phy_regs_tools->getSpillSize());

    // 将函数重新加入分配队列以进行重新分配
    not_allocated_funcs.push(mfun);
}

void RegisterAllocation::FinalizeAllocation() {
    // 将虚拟寄存器转换为物理寄存器
    VirtualRegisterRewrite rewrite_pass(unit, alloc_result);
    rewrite_pass.Execute();
}

void InstructionNumber::Execute() {
    // 遍历所有函数，执行编号
    for (auto func : unit->functions) {
        ExecuteInFunc(func);
    }
}

void InstructionNumber::ExecuteInFunc(MachineFunction *func) {
    // 为当前函数中的每个指令分配编号
    int count_begin = 0;
    current_func = func;
    auto it = func->getMachineCFG()->getBFSIterator();
    it->open();
    while (it->hasNext()) {
        auto mcfg_node = it->next();
        auto mblock = mcfg_node->Mblock;
        // 每个基本块开头会占据一个编号
        this->numbertoins[count_begin] = InstructionNumberEntry(nullptr, true);
        count_begin++;
        for (auto ins : *mblock) {
            // 为每条指令分配编号
            this->numbertoins[count_begin] = InstructionNumberEntry(ins, false);
            ins->setNumber(count_begin++);
        }
    }
}
// Reference: https://github.com/yuhuifishash/SysY/blob/master/target/common/machine_passes/register_alloc/basic_register_allocation.cc line72-line166
void RegisterAllocation::UpdateIntervalsInCurrentFunc() {
    intervals.clear();
    copy_sources.clear();
    auto mfun = current_func;
    auto mcfg = mfun->getMachineCFG();

    Liveness liveness(mfun);

    auto it = mcfg->getReverseIterator(mcfg->getBFSIterator());
    it->open();

    std::map<Register, int> last_def, last_use;

    while (it->hasNext()) {
        auto mcfg_node = it->next();
        auto mblock = mcfg_node->Mblock;
        auto cur_id = mblock->getLabelId();
        // 处理活跃区间
        for (auto reg : liveness.GetOUT(cur_id)) {
            if (intervals.find(reg) == intervals.end()) {
                intervals[reg] = LiveInterval(reg);
            }
            // 扩展或新增区间
            if (last_use.find(reg) == last_use.end()) {
                intervals[reg].PushFront(mblock->getBlockInNumber(), mblock->getBlockOutNumber());
            } else {
                intervals[reg].PushFront(mblock->getBlockInNumber(), mblock->getBlockOutNumber());
            }
            last_use[reg] = mblock->getBlockOutNumber();
        }
        // 处理每个指令的读取和写入寄存器
        for (auto reverse_it = mblock->ReverseBegin(); reverse_it != mblock->ReverseEnd(); ++reverse_it) {
            auto ins = *reverse_it;
            if (ins->arch == MachineBaseInstruction::COPY) {
                // 处理复制指令中的寄存器
                for (auto reg_w : ins->GetWriteReg()) {
                    for (auto reg_r : ins->GetReadReg()) {
                        copy_sources[*reg_w].push_back(*reg_r);
                        copy_sources[*reg_r].push_back(*reg_w);
                    }
                }
            }
            for (auto reg : ins->GetWriteReg()) {
                // 更新寄存器的最后定义位置
                last_def[*reg] = ins->getNumber();

                if (intervals.find(*reg) == intervals.end()) {
                    intervals[*reg] = LiveInterval(*reg);
                }

                // 有最后一次使用，切割区间
                if (last_use.find(*reg) != last_use.end()) {
                    last_use.erase(*reg);
                    intervals[*reg].SetMostBegin(ins->getNumber());
                } else {
                    // 没有最后一次使用，新增区间
                    intervals[*reg].PushFront(ins->getNumber(), ins->getNumber());
                }
                intervals[*reg].IncreaseReferenceCount(1);
            }
            for (auto reg : ins->GetReadReg()) {
                // 更新寄存器的最后使用位置
                if (intervals.find(*reg) == intervals.end()) {
                    intervals[*reg] = LiveInterval(*reg);
                }

                if (last_use.find(*reg) == last_use.end()) {
                    // 没有最后一次使用，新增区间
                    intervals[*reg].PushFront(mblock->getBlockInNumber(), ins->getNumber());
                }
                last_use[*reg] = ins->getNumber();

                intervals[*reg].IncreaseReferenceCount(1);
            }
        }
        // 清空临时记录
        last_use.clear();
        last_def.clear();
    }
}


