#include "basic_register_allocation.h"

void VirtualRegisterRewrite::Execute() {
    for (auto func : unit->functions) {
        current_func = func;
        ExecuteInFunc();
    }
}

void VirtualRegisterRewrite::ExecuteInFunc() {
    auto func = current_func;
    auto block_it = func->getMachineCFG()->getSeqScanIterator();
    block_it->open();
    while (block_it->hasNext()) {
        auto block = block_it->next()->Mblock;
        for (auto it = block->begin(); it != block->end(); ++it) {
            auto ins = *it;
            // 根据alloc_result将ins的虚拟寄存器重写为物理寄存器
            //TODO("VirtualRegisterRewrite");

            // 重写读寄存器
            RewriteRegisters(ins->GetReadReg(), func, /*isWrite=*/false);

            // 重写写寄存器
            RewriteRegisters(ins->GetWriteReg(), func, /*isWrite=*/true);
        }
    }
}

/**
 * @brief 重写指令的寄存器（读或写）
 * 
 * @param regs 寄存器列表
 * @param func 当前函数
 * @param isWrite 是否为写寄存器
 */
void VirtualRegisterRewrite::RewriteRegisters(const std::vector<Register*>& regs, 
                                              MachineFunction* func, 
                                              bool isWrite) {
    for (auto reg : regs) {
        if (!reg->is_virtual) {
            Assert(alloc_result.find(func) == alloc_result.end() ||
                   alloc_result.find(func)->second.find(*reg) == alloc_result.find(func)->second.end());
            continue;
        }
        auto it = alloc_result.find(func);
        if (it == alloc_result.end()) {
            continue;
        }
        auto reg_it = it->second.find(*reg);
        if (reg_it == it->second.end()) {
            continue;
        }
        auto result = reg_it->second;
        if (result.in_mem) {
            ERROR("Shouldn't reach here");
        } else {
            reg->is_virtual = false;
            reg->reg_no = result.phy_reg_no;
        }
    }
}

void SpillCodeGen::ExecuteInFunc(MachineFunction *function, std::map<Register, AllocResult> *alloc_result) {
    this->function = function;
    this->alloc_result = alloc_result;
    auto block_it = function->getMachineCFG()->getSeqScanIterator();
    block_it->open();
    while (block_it->hasNext()) {
        cur_block = block_it->next()->Mblock;
        for (auto it = cur_block->begin(); it != cur_block->end(); ++it) {
            auto ins = *it;
            // 根据alloc_result对ins溢出的寄存器生成溢出代码
            // 在溢出虚拟寄存器的read前插入load，在write后插入store
            // 注意load的结果寄存器是虚拟寄存器, 因为我们接下来要重新分配直到不再溢出
            //TODO("SpillCodeGen");
            if (ins->arch == MachineBaseInstruction::COPY) {
                HandleCopyInstruction(it, ins);
                continue;
            }
            // 处理读寄存器
            SpillRegisters(ins->GetReadReg(), it, /*isWrite=*/false);
            // 处理写寄存器
            SpillRegisters(ins->GetWriteReg(), it, /*isWrite=*/true);
        }
    }
}

/**
 * @brief 处理COPY指令的重写逻辑
 * 
 * @param it 指令迭代器
 * @param ins 当前指令
 */
void SpillCodeGen::HandleCopyInstruction(std::list<MachineBaseInstruction *>::iterator &it, MachineBaseInstruction* ins) {
    auto copy_ins = (MachineCopyInstruction *)ins;
    if (!copy_ins->GetReadReg().empty() && !copy_ins->GetWriteReg().empty()) {
        auto src = copy_ins->GetReadReg()[0];
        auto dst = copy_ins->GetWriteReg()[0];
        AllocResult src_p, dst_p;
        if (src->is_virtual) {
            src_p = alloc_result->find(*src)->second;
        }
        if (dst->is_virtual) {
            dst_p = alloc_result->find(*dst)->second;
        }
        if (*src == *dst) {
            it = cur_block->erase(it);
            --it;
        } else if (src_p.in_mem && dst_p.in_mem) {
            it = cur_block->erase(it);
            auto mid_reg = function->GetNewRegister(src->type.data_type, src->type.data_length);
            GenerateCopyFromStackCode(it, src_p.stack_offset * 4, mid_reg, src->type);
            GenerateCopyToStackCode(it, dst_p.stack_offset * 4, mid_reg, src->type);
            --it;
        } else if (src_p.in_mem) {
            it = cur_block->erase(it);
            GenerateCopyFromStackCode(it, src_p.stack_offset * 4, *dst, src->type);
            --it;
        } else if (dst_p.in_mem) {
            it = cur_block->erase(it);
            GenerateCopyToStackCode(it, dst_p.stack_offset * 4, *src, src->type);
            --it;
        }
    }
}

/**
 * @brief 生成溢出寄存器的load和store代码
 * 
 * @param regs 寄存器列表
 * @param it 指令迭代器
 * @param isWrite 是否为写寄存器
 */
void SpillCodeGen::SpillRegisters(const std::vector<Register*>& regs, 
                                  std::list<MachineBaseInstruction *>::iterator &it, 
                                  bool isWrite) {
    for (auto reg : regs) {
        if (!reg->is_virtual)
            continue;
        auto it_alloc = alloc_result->find(*reg);
        if (it_alloc == alloc_result->end())
            continue;
        auto result = it_alloc->second;
        if (result.in_mem) {
            if (isWrite) {
                // 溢出寄存器的写后插入store
                *reg = GenerateWriteCode(it, result.stack_offset * 4, reg->type);
            } else {
                // 溢出寄存器的读前插入load
                *reg = GenerateReadCode(it, result.stack_offset * 4, reg->type);
            }
        }
    }
}

