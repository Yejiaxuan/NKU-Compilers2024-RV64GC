#include "riscv64_lowercopy.h"
#include "../riscv64.h"
#include <assert.h>

// 实现辅助函数：处理寄存器拷贝
void RiscV64LowerBase::HandleRegisterCopy(MachineCopyInstruction *m_copy,
                                          std::list<MachineBaseInstruction *>::iterator &it,
                                          MachineBlock *block,
                                          MachineFunction *function) {
    auto Reg = ((MachineRegister *)(m_copy->GetSrc()))->reg;
    if (Reg.type.data_type == MachineDataType::INT) {
        auto dst_reg = ((MachineRegister *)(m_copy)->GetDst())->reg;
        auto src_reg = ((MachineRegister *)(m_copy)->GetSrc())->reg;

        auto copy_addi_ins = rvconstructor->ConstructR(RISCV_ADD, dst_reg, src_reg, GetPhysicalReg(RISCV_x0));
        it = block->erase(it);
        if (dst_reg != src_reg) {
            block->insert(it, copy_addi_ins);
        }
        --it;
    } else if (Reg.type.data_type == MachineDataType::FLOAT) {
        auto dst_reg = ((MachineRegister *)(m_copy)->GetDst())->reg;
        auto src_reg = ((MachineRegister *)(m_copy)->GetSrc())->reg;

        it = block->erase(it);
        if (dst_reg != src_reg) {
            block->insert(it, rvconstructor->ConstructR2(RISCV_FMV_S, dst_reg, src_reg));
        }
        --it;
    }
}

// 实现辅助函数：处理立即数拷贝
void RiscV64LowerBase::HandleImmediateCopy(MachineCopyInstruction *m_copy,
                                         std::list<MachineBaseInstruction *>::iterator &it,
                                         MachineBlock *block,
                                         MachineFunction *function) {
    if (m_copy->GetSrc()->op_type == MachineBaseOperand::IMMI) {
        auto src_immi = (MachineImmediateInt *)m_copy->GetSrc();
        auto dst_reg = (MachineRegister *)m_copy->GetDst();
        auto mid_reg = function->GetNewReg(INT64);

        it = block->erase(it);
        block->insert(it, rvconstructor->ConstructUImm(RISCV_LUI, mid_reg, src_immi->imm32 + 0x800));
        block->insert(it, rvconstructor->ConstructIImm(RISCV_ADDI, dst_reg->reg, mid_reg,
                                                       ((src_immi->imm32) << 20) >> 20));
        --it;
    }
}

// 实现RiscV64LowerCopy::Execute
void RiscV64LowerCopy::Execute() {
    for (auto function : unit->functions) {
        auto block_it = function->getMachineCFG()->getSeqScanIterator();
        block_it->open();
        while (block_it->hasNext()) {
            auto block = block_it->next()->Mblock;
            for (auto it = block->begin(); it != block->end(); ++it) {
                auto &ins = *it;
                if (ins->arch == MachineBaseInstruction::COPY) {
                    auto m_copy = (MachineCopyInstruction *)ins;
                    Assert(m_copy->GetDst()->op_type == MachineBaseOperand::REG);
                    
                    if (m_copy->GetSrc()->op_type == MachineBaseOperand::IMMI || 
                        m_copy->GetSrc()->op_type == MachineBaseOperand::IMMF ||
                        m_copy->GetSrc()->op_type == MachineBaseOperand::REG) {
                        if (m_copy->GetSrc()->op_type == MachineBaseOperand::REG) {
                            HandleRegisterCopy(m_copy, it, block, function);
                        } else {
                            HandleImmediateCopy(m_copy, it, block, function);
                        }
                    }
                }
            }
        }
    }
}
// Reference: https://github.com/yuhuifishash/SysY/blob/master/target/riscv64gc/instruction_select/riscv64_lowercopy.cc line64-line173
// 实现RiscV64LowerFImmCopy::Execute
void RiscV64LowerFImmCopy::Execute() {
    for (auto function : unit->functions) {
        auto block_it = function->getMachineCFG()->getSeqScanIterator();
        block_it->open();
        while (block_it->hasNext()) {
            auto block = block_it->next()->Mblock;
            for (auto it = block->begin(); it != block->end(); ++it) {
                auto &ins = *it;
                if (ins->arch == MachineBaseInstruction::COPY) {
                    auto m_copy = (MachineCopyInstruction *)ins;
                    Assert(m_copy->GetDst()->op_type == MachineBaseOperand::REG);
                    
                    if (m_copy->GetSrc()->op_type == MachineBaseOperand::IMMF) {
                        auto src_immf = (MachineImmediateFloat *)m_copy->GetSrc();
                        auto dst_reg = (MachineRegister *)m_copy->GetDst();
                        auto immf = src_immf->fimm32;
                        if (immf == 0.0) {
                            it = block->erase(it);
                            block->insert(
                                it, rvconstructor->ConstructR2(RISCV_FMV_W_X, dst_reg->reg, GetPhysicalReg(RISCV_x0)));
                            --it;
                        } else {
                            it = block->erase(it);
                            auto mid_reg = function->GetNewRegister(INT64.data_type, INT64.data_length);
                            block->insert(it, rvconstructor->ConstructCopyRegImmI(mid_reg, *(int *)&immf, INT64));
                            block->insert(it, rvconstructor->ConstructR2(RISCV_FMV_W_X, dst_reg->reg, mid_reg));
                            --it;
                        }
                    }
                }
            }
        }
    }
}

// 实现RiscV64LowerIImmCopy::Execute
void RiscV64LowerIImmCopy::Execute() {
    for (auto function : unit->functions) {
        auto block_it = function->getMachineCFG()->getSeqScanIterator();
        block_it->open();
        while (block_it->hasNext()) {
            auto block = block_it->next()->Mblock;
            for (auto it = block->begin(); it != block->end(); ++it) {
                auto &ins = *it;
                if (ins->arch == MachineBaseInstruction::COPY) {
                    auto m_copy = (MachineCopyInstruction *)ins;
                    Assert(m_copy->GetDst()->op_type == MachineBaseOperand::REG);
                    
                    if (m_copy->GetSrc()->op_type == MachineBaseOperand::IMMI) {
                        auto src_immi = (MachineImmediateInt *)m_copy->GetSrc();
                        auto dst_reg = (MachineRegister *)m_copy->GetDst();
                        if (src_immi != 0) {
                            if (src_immi->imm32 <= 2047 && src_immi->imm32 >= -2048) {
                                it = block->erase(it);
                                block->insert(it,
                                              rvconstructor->ConstructIImm(RISCV_ADDIW, dst_reg->reg,
                                                                           GetPhysicalReg(RISCV_x0), src_immi->imm32));
                                --it;
                            } else if ((src_immi->imm32 & 0xFFF) == 0) {
                                it = block->erase(it);
                                block->insert(it, rvconstructor->ConstructUImm(RISCV_LUI, dst_reg->reg,
                                                                               ((unsigned int)src_immi->imm32) >> 12));
                                --it;
                            } else {
                                auto mid_reg = function->GetNewReg(INT64);
                                it = block->erase(it);
                                block->insert(it, rvconstructor->ConstructUImm(
                                                  RISCV_LUI, mid_reg, ((unsigned int)(src_immi->imm32 + 0x800)) >> 12));
                                block->insert(it, rvconstructor->ConstructIImm(RISCV_ADDIW, dst_reg->reg, mid_reg,
                                                                               ((src_immi->imm32) << 20) >> 20));
                                --it;
                            }
                        } else {
                            it = block->erase(it);
                            block->insert(
                                it, rvconstructor->ConstructCopyReg(dst_reg->reg, GetPhysicalReg(RISCV_x0), INT64));
                            --it;
                        }
                    }
                }
            }
        }
    }
}
