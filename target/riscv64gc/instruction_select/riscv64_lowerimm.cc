// riscv64_lowerimm.cc
#include "riscv64_lowerimm.h"
#include "assert.h"

void RiscV64LowerImm::Execute() {
    for (auto func : unit->functions) {
        current_func = func;
        for (auto block : func->blocks) {
            cur_block = block;
            for (auto it = block->begin(); it != block->end(); ++it) {
                auto cur_ins = *it;
                if (cur_ins->arch == MachineBaseInstruction::RiscV) {
                    auto cur_rvins = static_cast<RiscV64Instruction *>(cur_ins);
                    if (NeedsImmediateLowering(cur_rvins)) {
                        if (OpTable[cur_rvins->getOpcode()].ins_formattype == RvOpInfo::I_type) {
                            LowerITypeInstruction(it, cur_rvins, block);
                        } else {
                            LowerLoadStoreInstruction(it, cur_rvins, block);
                        }
                    }
                }
            }
        }
    }
}

bool RiscV64LowerImm::NeedsImmediateLowering(RiscV64Instruction *ins) {
    if (ins->getImm() <= 2047 && ins->getImm() >= -2048)
        return false;
    if (ins->getUseLabel())
        return false;
    return (ins->getOpcode() == RISCV_LD || ins->getOpcode() == RISCV_SD ||
            ins->getOpcode() == RISCV_LW || ins->getOpcode() == RISCV_SW ||
            ins->getOpcode() == RISCV_FLW || ins->getOpcode() == RISCV_FSW) ||
           (OpTable[ins->getOpcode()].ins_formattype == RvOpInfo::I_type);
}

void RiscV64LowerImm::LowerLoadStoreInstruction(std::list<MachineBaseInstruction *>::iterator &it, RiscV64Instruction *ins, MachineBlock *block) {
    it = block->erase(it);
    auto imm_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
    auto addr_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
    block->insert(it, rvconstructor->ConstructCopyRegImmI(imm_reg, ins->getImm(), INT64));
    
    Register addrbase_reg;
    if (ins->getOpcode() == RISCV_LD || ins->getOpcode() == RISCV_LW ||
        ins->getOpcode() == RISCV_FLW) {
        addrbase_reg = ins->getRs1();
        ins->setRs1(addr_reg);
    } else {
        addrbase_reg = ins->getRs2();
        ins->setRs2(addr_reg);
    }
    
    block->insert(it, rvconstructor->ConstructR(RISCV_ADD, addr_reg, addrbase_reg, imm_reg));
    ins->setImm(0);
    block->insert(it, ins);
    --it;
}

void RiscV64LowerImm::LowerITypeInstruction(std::list<MachineBaseInstruction *>::iterator &it, RiscV64Instruction *ins, MachineBlock *block) {
    it = block->erase(it);
    auto imm_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
    block->insert(it, rvconstructor->ConstructCopyRegImmI(imm_reg, ins->getImm(), INT64));
    ins->setRs2(imm_reg);
    ins->setImm(0);
    
    switch (ins->getOpcode()) {
        case RISCV_SLLI:
            ins->setOpcode(RISCV_SLL, false);
            break;
        case RISCV_SRLI:
            ins->setOpcode(RISCV_SRL, false);
            break;
        case RISCV_SRAI:
            ins->setOpcode(RISCV_SRA, false);
            break;
        case RISCV_ADDI:
            ins->setOpcode(RISCV_ADD, false);
            break;
        case RISCV_XORI:
            ins->setOpcode(RISCV_XOR, false);
            break;
        case RISCV_ORI:
            ins->setOpcode(RISCV_OR, false);
            break;
        case RISCV_ANDI:
            ins->setOpcode(RISCV_AND, false);
            break;
        case RISCV_SLTI:
            ins->setOpcode(RISCV_SLT, false);
            break;
        case RISCV_SLTIU:
            ins->setOpcode(RISCV_SLTU, false);
            break;
        case RISCV_SLLIW:
            ins->setOpcode(RISCV_SLLW, false);
            break;
        case RISCV_SRLIW:
            ins->setOpcode(RISCV_SRLW, false);
            break;
        case RISCV_SRAIW:
            ins->setOpcode(RISCV_SRAW, false);
            break;
        case RISCV_ADDIW:
            ins->setOpcode(RISCV_ADDW, false);
            break;
        default:
            break;
    }
    
    block->insert(it, ins);
    --it;
}
