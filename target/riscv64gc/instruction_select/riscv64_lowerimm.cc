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
    // Check if opcode is load/store
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

Multiplier RiscV64AlgStrengthReduce::chooseMultiplier(Uint32 d, int p) {
    constexpr int N = 32;
    int l = N - __builtin_clz(d - 1);
    Uint64 low = (Uint64(1) << (N + l)) / d;
    Uint64 high = ((Uint64(1) << (N + l)) + (Uint64(1) << (N + l - p))) / d;
    while ((low >> 1) < (high >> 1) && l > 0)
        low >>= 1, high >>= 1, --l;
    return {high, l};
}

void RiscV64AlgStrengthReduce::Execute() {
    for (auto func : unit->functions) {
        current_func = func;
        std::map<int, int> IntConstPool;
        // First pass: collect constant pools
        for (auto block : func->blocks) {
            cur_block = block;
            for (auto it = block->begin(); it != block->end(); ++it) {
                auto cur_ins = *it;
                if (cur_ins->arch == MachineBaseInstruction::COPY) {
                    auto cur_copyins = static_cast<MachineCopyInstruction *>(cur_ins);
                    if (cur_copyins->GetCopyType() == INT64) {
                        if (cur_copyins->GetSrc()->op_type == MachineBaseOperand::IMMI) {
                            assert(cur_copyins->GetDst()->op_type == MachineBaseOperand::REG);
                            auto dst_reg = static_cast<MachineRegister *>(cur_copyins->GetDst())->reg;
                            if (dst_reg.is_virtual) {
                                IntConstPool[dst_reg.reg_no] = static_cast<MachineImmediateInt *>(cur_copyins->GetSrc())->imm32;
                            }
                        }
                    }
                }
            }
            // Second pass: optimize instructions
            for (auto it = block->begin(); it != block->end(); ++it) {
                auto cur_ins = *it;
                if (cur_ins->arch == MachineBaseInstruction::RiscV) {
                    auto cur_rvins = static_cast<RiscV64Instruction *>(cur_ins);
                    auto cur_op = cur_rvins->getOpcode();
                    if (cur_op == RISCV_MUL || cur_op == RISCV_MULW) {
                        OptimizeMultiply(it, cur_rvins, block, IntConstPool);
                    } else if (cur_op == RISCV_DIV || cur_op == RISCV_DIVW ||
                               cur_op == RISCV_REM || cur_op == RISCV_REMW) {
                        OptimizeDivide(it, cur_rvins, block, IntConstPool);
                    }
                }
            }
        }
    }
}

void RiscV64AlgStrengthReduce::OptimizeMultiply(std::list<MachineBaseInstruction *>::iterator &it, RiscV64Instruction *ins, MachineBlock *block, std::map<int, int> &constPool) {
    auto reg1 = ins->getRs1();
    auto reg2 = ins->getRs2();
    if (reg1.is_virtual && constPool.find(reg1.reg_no) != constPool.end()) {
        std::swap(reg1, reg2);
    }
    if (reg2.is_virtual && constPool.find(reg2.reg_no) != constPool.end()) {
        auto const_val = constPool[reg2.reg_no];
        if (const_val == 1) {
            it = block->erase(it);
            block->insert(it, rvconstructor->ConstructCopyReg(ins->getRd(), reg1, INT64));
            --it;
        } else if (const_val == -1) {
            it = block->erase(it);
            auto op = (ins->getOpcode() == RISCV_MUL) ? RISCV_SUB : RISCV_SUBW;
            block->insert(it, rvconstructor->ConstructR(op, ins->getRd(), GetPhysicalReg(RISCV_x0), reg1));
            --it;
        } else if (__builtin_popcount(const_val) == 1) {
            it = block->erase(it);
            auto op = (ins->getOpcode() == RISCV_MUL) ? RISCV_SLLI : RISCV_SLLIW;
            int shift = __builtin_ctz(const_val);
            block->insert(it, rvconstructor->ConstructIImm(op, ins->getRd(), reg1, shift));
            --it;
        } else if (__builtin_popcount(const_val + 1) == 1) {
            it = block->erase(it);
            auto op1 = (ins->getOpcode() == RISCV_MUL) ? RISCV_SLLI : RISCV_SLLIW;
            auto op2 = (ins->getOpcode() == RISCV_MUL) ? RISCV_SUB : RISCV_SUBW;
            auto mid_reg = current_func->GetNewReg(INT64);
            block->insert(it, rvconstructor->ConstructIImm(op1, mid_reg, reg1, __builtin_ctz(const_val + 1)));
            block->insert(it, rvconstructor->ConstructR(op2, ins->getRd(), mid_reg, reg1));
            --it;
        } else if (__builtin_popcount(const_val - 1) == 1) {
            it = block->erase(it);
            auto op1 = (ins->getOpcode() == RISCV_MUL) ? RISCV_SLLI : RISCV_SLLIW;
            auto op2 = (ins->getOpcode() == RISCV_MUL) ? RISCV_ADD : RISCV_ADDW;
            auto mid_reg = current_func->GetNewReg(INT64);
            block->insert(it, rvconstructor->ConstructIImm(op1, mid_reg, reg1, __builtin_ctz(const_val - 1)));
            block->insert(it, rvconstructor->ConstructR(op2, ins->getRd(), mid_reg, reg1));
            --it;
        }
    }
}

void RiscV64AlgStrengthReduce::OptimizeDivide(std::list<MachineBaseInstruction *>::iterator &it, RiscV64Instruction *ins, MachineBlock *block, std::map<int, int> &constPool) {
    auto reg1 = ins->getRs1();
    auto reg2 = ins->getRs2();
    if (reg2.is_virtual && constPool.find(reg2.reg_no) != constPool.end()) {
        auto const_val = constPool[reg2.reg_no];
        bool negative = false;
        auto result_reg = ins->getRd();
        if (ins->getOpcode() == RISCV_DIV || ins->getOpcode() == RISCV_DIVW) {
            if (const_val < 0) {
                const_val = -const_val;
                negative = true;
                result_reg = current_func->GetNewReg(INT64);
            }
            if (const_val == 1) {
                it = block->erase(it);
                block->insert(it, rvconstructor->ConstructCopyReg(result_reg, reg1, INT64));
            } else if (const_val == -1) {
                it = block->erase(it);
                auto op = (ins->getOpcode() == RISCV_DIV) ? RISCV_SUB : RISCV_SUBW;
                block->insert(it, rvconstructor->ConstructR(op, result_reg, GetPhysicalReg(RISCV_x0), reg1));
            } else if (const_val == 2) {
                it = block->erase(it);
                auto mid_reg = current_func->GetNewReg(INT64);
                auto mid2_reg = current_func->GetNewReg(INT64);
                block->insert(it, rvconstructor->ConstructIImm(RISCV_SRLIW, mid_reg, reg1, 31));
                block->insert(it, rvconstructor->ConstructR(RISCV_ADD, mid2_reg, reg1, mid_reg));
                block->insert(it, rvconstructor->ConstructIImm(RISCV_SRAIW, result_reg, mid2_reg, 1));
            } else if (__builtin_popcount(const_val) == 1) {
                int log = __builtin_ctz(const_val);
                it = block->erase(it);
                auto mid_reg = current_func->GetNewReg(INT64);
                auto mid2_reg = current_func->GetNewReg(INT64);
                auto mid3_reg = current_func->GetNewReg(INT64);
                block->insert(it, rvconstructor->ConstructIImm(RISCV_SLLI, mid_reg, reg1, 1));
                block->insert(it, rvconstructor->ConstructIImm(RISCV_SRLI, mid2_reg, mid_reg, 64 - log));
                block->insert(it, rvconstructor->ConstructR(RISCV_ADD, mid3_reg, reg1, mid2_reg));
                block->insert(it, rvconstructor->ConstructIImm(RISCV_SRAIW, result_reg, mid3_reg, log));
            } else {
                auto multiplier = chooseMultiplier(const_val, 31);
                int mult = multiplier.m & 0xFFFFFFFF;
                auto imm_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                auto mul_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                it = block->erase(it);
                block->insert(it, rvconstructor->ConstructCopyRegImmI(imm_reg, mult, INT64));
                block->insert(it, rvconstructor->ConstructR(RISCV_MUL, mul_reg, reg1, imm_reg));
                
                if (mult > 0) {
                    auto sign_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    auto preres_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    block->insert(it, rvconstructor->ConstructIImm(RISCV_SRLI, sign_reg, mul_reg, 63));
                    auto op = (multiplier.l == 0) ? RISCV_SRLI : RISCV_SRAI;
                    block->insert(it, rvconstructor->ConstructIImm(op, preres_reg, mul_reg, 32 + multiplier.l));
                    block->insert(it, rvconstructor->ConstructR(RISCV_ADD, result_reg, preres_reg, sign_reg));
                } else {
                    auto premul_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    auto realmul_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    auto sign_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    auto preres_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    block->insert(it, rvconstructor->ConstructIImm(RISCV_SRLI, premul_reg, mul_reg, 32));
                    block->insert(it, rvconstructor->ConstructR(RISCV_ADD, realmul_reg, reg1, premul_reg));
                    block->insert(it, rvconstructor->ConstructIImm(RISCV_SRLIW, sign_reg, realmul_reg, 31));
                    block->insert(it, rvconstructor->ConstructIImm(RISCV_SRAIW, preres_reg, realmul_reg, multiplier.l));
                    block->insert(it, rvconstructor->ConstructR(RISCV_ADD, result_reg, preres_reg, sign_reg));
                }
                
                if (negative) {
                    block->insert(it, rvconstructor->ConstructR(RISCV_SUBW, ins->getRd(), GetPhysicalReg(RISCV_x0), result_reg));
                }
            }
            --it;
        } else if (ins->getOpcode() == RISCV_REM || ins->getOpcode() == RISCV_REMW) {
            if (const_val < 0) {
                const_val = -const_val;
            }
            if (const_val == 1) {
                it = block->erase(it);
                block->insert(it, rvconstructor->ConstructCopyRegImmI(ins->getRd(), 0, INT64));
            } else if (const_val == 2) {
                it = block->erase(it);
                auto mid1_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                auto mid2_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                auto mid3_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                block->insert(it, rvconstructor->ConstructIImm(RISCV_SRLIW, mid1_reg, reg1, 31));
                block->insert(it, rvconstructor->ConstructR(RISCV_ADD, mid2_reg, mid1_reg, reg1));
                block->insert(it, rvconstructor->ConstructIImm(RISCV_ANDI, mid3_reg, mid2_reg, -2));
                block->insert(it, rvconstructor->ConstructR(RISCV_SUBW, ins->getRd(), reg1, mid3_reg));
            } else if (__builtin_popcount(const_val) == 1) {
                int log = __builtin_ctz(const_val);
                it = block->erase(it);
                auto mid1_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                auto mid2_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                auto mid3_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                auto mid4_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                block->insert(it, rvconstructor->ConstructIImm(RISCV_SLLI, mid1_reg, reg1, 1));
                block->insert(it, rvconstructor->ConstructIImm(RISCV_SRLI, mid2_reg, mid1_reg, 64 - log));
                block->insert(it, rvconstructor->ConstructR(RISCV_ADD, mid3_reg, mid2_reg, reg1));
                block->insert(it, rvconstructor->ConstructIImm(RISCV_ANDI, mid4_reg, mid3_reg, -const_val));
                block->insert(it, rvconstructor->ConstructR(RISCV_SUBW, ins->getRd(), reg1, mid4_reg));
            } else {
                auto multiplier = chooseMultiplier(const_val, 31);
                int mult = multiplier.m & 0xFFFFFFFF;
                auto imm_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                auto mul_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                auto div_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                it = block->erase(it);
                block->insert(it, rvconstructor->ConstructCopyRegImmI(imm_reg, mult, INT64));
                block->insert(it, rvconstructor->ConstructR(RISCV_MUL, mul_reg, reg1, imm_reg));
                
                if (mult > 0) {
                    auto sign_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    auto preres_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    block->insert(it, rvconstructor->ConstructIImm(RISCV_SRLI, sign_reg, mul_reg, 63));
                    auto op = (multiplier.l == 0) ? RISCV_SRLI : RISCV_SRAI;
                    block->insert(it, rvconstructor->ConstructIImm(op, preres_reg, mul_reg, 32 + multiplier.l));
                    block->insert(it, rvconstructor->ConstructR(RISCV_ADD, div_reg, preres_reg, sign_reg));
                } else {
                    auto premul_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    auto realmul_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    auto sign_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    auto preres_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                    block->insert(it, rvconstructor->ConstructIImm(RISCV_SRLI, premul_reg, mul_reg, 32));
                    block->insert(it, rvconstructor->ConstructR(RISCV_ADD, realmul_reg, reg1, premul_reg));
                    block->insert(it, rvconstructor->ConstructIImm(RISCV_SRLIW, sign_reg, realmul_reg, 31));
                    block->insert(it, rvconstructor->ConstructIImm(RISCV_SRAIW, preres_reg, realmul_reg, multiplier.l));
                    block->insert(it, rvconstructor->ConstructR(RISCV_ADD, div_reg, preres_reg, sign_reg));
                }
                
                // result_reg = reg1 - div_reg * const_val
                auto mul_imm = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                auto product_reg = current_func->GetNewRegister(INT64.data_type, INT64.data_length);
                block->insert(it, rvconstructor->ConstructCopyRegImmI(mul_imm, const_val, INT64));
                block->insert(it, rvconstructor->ConstructR(RISCV_MUL, product_reg, div_reg, mul_imm));
                block->insert(it, rvconstructor->ConstructR(RISCV_SUBW, ins->getRd(), reg1, product_reg));
                
                --it;
            }
        }
    }
}


