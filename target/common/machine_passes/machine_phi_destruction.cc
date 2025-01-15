#include "machine_phi_destruction.h"
#include "assert.h"

// 执行 Phi 销毁过程，针对单元中的所有函数
void MachinePhiDestruction::Execute() {
    for (auto func : unit->functions) {
        current_func = func;
        PhiDestructionInCurrentFunction();
    }
}

// 检查给定的基本块是否包含任何 PHI 指令
bool MachinePhiDestruction::BlockHasPhi(MachineBlock *block) {
    for (auto ins : *block) {
        if (ins->arch == MachineBaseInstruction::PHI) {
            return true;
        }
    }
    return false;
}

// 在前驱块和当前块之间插入一个仅包含分支的基本块
void MachinePhiDestruction::InsertBranchOnlyBlock(int predecessor_id, int current_block_id) {
    // 在前驱块和当前块之间插入一个新的仅包含分支的基本块
    auto new_block = current_func->InsertNewBranchOnlyBlockBetweenEdge(predecessor_id, current_block_id);
    // 可选择添加日志或额外的处理
}

// 处理给定基本块中的所有 PHI 指令
void MachinePhiDestruction::ProcessPhiInstructions(MachineBlock *block) {
    for (auto it = block->begin(); it != block->end(); ++it) {
        auto ins = *it;
        if (ins->arch != MachineBaseInstruction::PHI) {
            break;
        }
        auto phi_Ins = static_cast<MachinePhiInstruction *>(ins);
        it = block->erase(it);
        --it;
        for (auto &[phi_labelid, phi_operand] : phi_Ins->GetPhiList()) {
            assert(phi_operand != nullptr);
            // TODO: 插入到并行拷贝列表
            auto B = current_func->getMachineCFG()->GetNodeByBlockId(phi_labelid)->Mblock;
            if (phi_operand->op_type == MachineBaseOperand::REG) {
                if (static_cast<MachineRegister *>(phi_operand)->reg == phi_Ins->GetResult()) {
                    continue;
                }
            }
            B->InsertParallelCopyList(phi_Ins->GetResult(), phi_operand);
        }
    }
}

// 处理给定基本块的并行拷贝列表
void MachinePhiDestruction::HandleParallelCopyList(MachineBlock *block) {
    auto &copylist = block->GetParallelCopyList();
    if (copylist.empty())
        return;

    std::map<Register, int> src_ref_count;
    for (auto &[dst, src] : copylist) {
        src_ref_count[dst] = src_ref_count[dst];
        if (src->op_type == MachineBaseOperand::REG) {
            auto reg = static_cast<MachineRegister *>(src)->reg;
            assert(reg.is_virtual);
            src_ref_count[reg]++;
        }
    }

    std::queue<Register> can_seq;
    for (auto &[reg, srcref] : src_ref_count) {
        if (srcref == 0) {
            can_seq.push(reg);
        }
    }

    auto insert_it = block->getInsertBeforeBrIt();
    while (!copylist.empty()) {
        if (!can_seq.empty()) {
            auto dstreg = can_seq.front();
            can_seq.pop();
            if (copylist.find(dstreg) != copylist.end()) {
                auto srcop = copylist[dstreg];
                copylist.erase(dstreg);
                if (srcop->op_type == MachineBaseOperand::REG) {
                    auto srcreg = static_cast<MachineRegister *>(srcop)->reg;
                    src_ref_count[srcreg]--;
                    if (src_ref_count[srcreg] == 0) {
                        can_seq.push(srcreg);
                    }
                }

                auto copy_instr = new MachineCopyInstruction(srcop, new MachineRegister(dstreg), dstreg.type);
                block->insert(insert_it, copy_instr);
            }
        } else {
            auto pair = *copylist.begin();
            auto mid_reg = current_func->GetNewRegister(pair.first.type.data_type, pair.first.type.data_length);
            auto dst_reg = pair.first;
            auto src_op = pair.second;

            auto copy_instr = new MachineCopyInstruction(src_op, new MachineRegister(mid_reg), dst_reg.type);
            block->insert(insert_it, copy_instr);
            if (src_op->op_type == MachineBaseOperand::REG) {
                auto src_reg = static_cast<MachineRegister *>(src_op)->reg;
                src_ref_count[src_reg]--;
                if (src_ref_count[src_reg] == 0) {
                    can_seq.push(src_reg);
                }
            }
            src_ref_count[mid_reg]++;
            block->InsertParallelCopyList(dst_reg, new MachineRegister(mid_reg));
        }
    }
}

// 对当前函数执行 Phi 销毁过程
void MachinePhiDestruction::PhiDestructionInCurrentFunction() {
    auto block_it = current_func->getMachineCFG()->getSeqScanIterator();
    block_it->open();
    while (block_it->hasNext()) {
        auto block = block_it->next()->Mblock;
        if (!BlockHasPhi(block))
            continue;

        // 对每个前驱块进行处理
        for (auto predecessor : current_func->getMachineCFG()->GetPredecessorsByBlockId(block->getLabelId())) {
            if (current_func->getMachineCFG()->GetSuccessorsByBlockId(predecessor->Mblock->getLabelId()).size() > 1) {
                InsertBranchOnlyBlock(predecessor->Mblock->getLabelId(), block->getLabelId());
            }
        }

        // 处理基本块中的 PHI 指令
        ProcessPhiInstructions(block);
    }

    // 第二遍处理并行拷贝列表
    block_it->rewind();
    block_it->open();
    while (block_it->hasNext()) {
        auto block = block_it->next()->Mblock;
        HandleParallelCopyList(block);
    }
}

