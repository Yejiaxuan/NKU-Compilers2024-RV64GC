#include "riscv64_instSelect.h"
#include <sstream>

template <> void RiscV64Selector::ConvertAndAppend<LoadInstruction *>(LoadInstruction *ins) {
    if (ins->GetPointer()->GetOperandType() == BasicOperand::REG) {
        // Lazy("Deal with alloca later");

        auto ptr_op = (RegOperand *)ins->GetPointer();
        auto rd_op = (RegOperand *)ins->GetResult();

        if (ins->GetDataType() == BasicInstruction::I32) {
            Register rd = GetllvmReg(rd_op->GetRegNo(), INT64);
            if (llvm_rv_allocas.find(ptr_op->GetRegNo()) == llvm_rv_allocas.end()) {
                Register ptr = GetllvmReg(ptr_op->GetRegNo(), INT64);    // INT64 HERE
                auto lw_instr = rvconstructor->ConstructIImm(RISCV_LW, rd, ptr, 0);
                cur_block->push_back(lw_instr);
            } else {
                auto sp_offset = llvm_rv_allocas[ptr_op->GetRegNo()];
                auto lw_instr = rvconstructor->ConstructIImm(RISCV_LW, rd, GetPhysicalReg(RISCV_sp), sp_offset);
                ((RiscV64Function *)cur_func)->AddAllocaIns(lw_instr);
                cur_block->push_back(lw_instr);
            }
        } else if (ins->GetDataType() == BasicInstruction::FLOAT32) {
            Register rd = GetllvmReg(rd_op->GetRegNo(), FLOAT64);
            if (llvm_rv_allocas.find(ptr_op->GetRegNo()) == llvm_rv_allocas.end()) {
                Register ptr = GetllvmReg(ptr_op->GetRegNo(), INT64);    // INT64 HERE
                auto lw_instr = rvconstructor->ConstructIImm(RISCV_FLW, rd, ptr, 0);
                cur_block->push_back(lw_instr);
            } else {
                auto sp_offset = llvm_rv_allocas[ptr_op->GetRegNo()];
                auto lw_instr = rvconstructor->ConstructIImm(RISCV_FLW, rd, GetPhysicalReg(RISCV_sp), sp_offset);
                ((RiscV64Function *)cur_func)->AddAllocaIns(lw_instr);
                cur_block->push_back(lw_instr);
            }
        } else if (ins->GetDataType() == BasicInstruction::PTR) {
            Register rd = GetllvmReg(rd_op->GetRegNo(), INT64);
            if (llvm_rv_allocas.find(ptr_op->GetRegNo()) == llvm_rv_allocas.end()) {
                Register ptr = GetllvmReg(ptr_op->GetRegNo(), INT64);    // INT64 HERE
                auto lw_instr = rvconstructor->ConstructIImm(RISCV_LD, rd, ptr, 0);
                cur_block->push_back(lw_instr);
            } else {
                auto sp_offset = llvm_rv_allocas[ptr_op->GetRegNo()];
                auto lw_instr = rvconstructor->ConstructIImm(RISCV_LD, rd, GetPhysicalReg(RISCV_sp), sp_offset);
                ((RiscV64Function *)cur_func)->AddAllocaIns(lw_instr);
                cur_block->push_back(lw_instr);
            }
        } else {
            ERROR("Unexpected data type");
        }

    } else if (ins->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {
        // lui %r0, %hi(x)
        // lw  %rd, %lo(x)(%r0)
        auto global_op = (GlobalOperand *)ins->GetPointer();
        auto rd_op = (RegOperand *)ins->GetResult();

        Register addr_hi = GetNewReg(INT64);

        if (ins->GetDataType() == BasicInstruction::I32) {
            Register rd = GetllvmReg(rd_op->GetRegNo(), INT64);

            auto lui_instr = rvconstructor->ConstructULabel(RISCV_LUI, addr_hi, RiscVLabel(global_op->GetName(), true));
            auto lw_instr =
            rvconstructor->ConstructILabel(RISCV_LW, rd, addr_hi, RiscVLabel(global_op->GetName(), false));

            cur_block->push_back(lui_instr);
            cur_block->push_back(lw_instr);
        } else if (ins->GetDataType() == BasicInstruction::FLOAT32) {
            Register rd = GetllvmReg(rd_op->GetRegNo(), FLOAT64);

            auto lui_instr = rvconstructor->ConstructULabel(RISCV_LUI, addr_hi, RiscVLabel(global_op->GetName(), true));
            auto lw_instr =
            rvconstructor->ConstructILabel(RISCV_FLW, rd, addr_hi, RiscVLabel(global_op->GetName(), false));

            cur_block->push_back(lui_instr);
            cur_block->push_back(lw_instr);
        } else if (ins->GetDataType() == BasicInstruction::PTR) {
            Register rd = GetllvmReg(rd_op->GetRegNo(), INT64);

            auto lui_instr = rvconstructor->ConstructULabel(RISCV_LUI, addr_hi, RiscVLabel(global_op->GetName(), true));
            auto lw_instr =
            rvconstructor->ConstructILabel(RISCV_LD, rd, addr_hi, RiscVLabel(global_op->GetName(), false));

            cur_block->push_back(lui_instr);
            cur_block->push_back(lw_instr);
        } else {
            ERROR("Unexpected data type");
        }
    }
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<StoreInstruction *>(StoreInstruction *ins) {
    Register value_reg;
    if (ins->GetValue()->GetOperandType() == BasicOperand::IMMI32) {
        auto val_imm = (ImmI32Operand *)ins->GetValue();

        value_reg = GetNewReg(INT64);

        auto imm_copy_ins = rvconstructor->ConstructCopyRegImmI(value_reg, val_imm->GetIntImmVal(), INT64);
        cur_block->push_back(imm_copy_ins);

    } else if (ins->GetValue()->GetOperandType() == BasicOperand::REG) {
        auto val_reg = (RegOperand *)ins->GetValue();
        if (ins->GetDataType() == BasicInstruction::I32) {
            value_reg = GetllvmReg(val_reg->GetRegNo(), INT64);
        } else if (ins->GetDataType() == BasicInstruction::FLOAT32) {
            value_reg = GetllvmReg(val_reg->GetRegNo(), FLOAT64);
        } else {
            ERROR("Unexpected data type");
        }
    } else if (ins->GetValue()->GetOperandType() == BasicOperand::IMMF32) {
        auto val_imm = (ImmF32Operand *)ins->GetValue();

        value_reg = GetNewReg(FLOAT64);

        auto imm_copy_ins = rvconstructor->ConstructCopyRegImmF(value_reg, val_imm->GetFloatVal(), FLOAT64);
        float val = val_imm->GetFloatVal();
        cur_block->push_back(imm_copy_ins);
    } else {
        ERROR("Unexpected or unimplemented operand type");
    }

    if (ins->GetPointer()->GetOperandType() == BasicOperand::REG) {
        // Lazy("Deal with alloca later");
        auto reg_ptr_op = (RegOperand *)ins->GetPointer();

        if (ins->GetDataType() == BasicInstruction::I32) {
            if (llvm_rv_allocas.find(reg_ptr_op->GetRegNo()) == llvm_rv_allocas.end()) {
                auto ptr_reg = GetllvmReg(reg_ptr_op->GetRegNo(), INT64);

                auto store_instruction = rvconstructor->ConstructSImm(RISCV_SW, value_reg, ptr_reg, 0);
                cur_block->push_back(store_instruction);
            } else {
                auto sp_offset = llvm_rv_allocas[reg_ptr_op->GetRegNo()];

                auto store_instruction =
                rvconstructor->ConstructSImm(RISCV_SW, value_reg, GetPhysicalReg(RISCV_sp), sp_offset);
                ((RiscV64Function *)cur_func)->AddAllocaIns(store_instruction);
                cur_block->push_back(store_instruction);
            }
        } else if (ins->GetDataType() == BasicInstruction::FLOAT32) {
            if (llvm_rv_allocas.find(reg_ptr_op->GetRegNo()) == llvm_rv_allocas.end()) {
                auto ptr_reg = GetllvmReg(reg_ptr_op->GetRegNo(), INT64);

                auto store_instruction = rvconstructor->ConstructSImm(RISCV_FSW, value_reg, ptr_reg, 0);
                cur_block->push_back(store_instruction);
            } else {
                auto sp_offset = llvm_rv_allocas[reg_ptr_op->GetRegNo()];

                auto store_instruction =
                rvconstructor->ConstructSImm(RISCV_FSW, value_reg, GetPhysicalReg(RISCV_sp), sp_offset);
                ((RiscV64Function *)cur_func)->AddAllocaIns(store_instruction);
                cur_block->push_back(store_instruction);
            }
        } else {
            ERROR("Unexpected data type");
        }

    } else if (ins->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {
        auto global_op = (GlobalOperand *)ins->GetPointer();

        auto addr_hi = GetNewReg(INT64);

        auto lui_instruction =
        rvconstructor->ConstructULabel(RISCV_LUI, addr_hi, RiscVLabel(global_op->GetName(), true));
        cur_block->push_back(lui_instruction);

        if (ins->GetDataType() == BasicInstruction::I32) {
            auto store_instruction =
            rvconstructor->ConstructSLabel(RISCV_SW, value_reg, addr_hi, RiscVLabel(global_op->GetName(), false));
            cur_block->push_back(store_instruction);
        } else if (ins->GetDataType() == BasicInstruction::FLOAT32) {
            auto store_instruction =
            rvconstructor->ConstructSLabel(RISCV_FSW, value_reg, addr_hi, RiscVLabel(global_op->GetName(), false));
            cur_block->push_back(store_instruction);
        } else {
            ERROR("Unexpected data type");
        }
    }
    TODO("Implement this if you need");
}

struct Multiplier64 {
    Uint128 m;
    int l;
};

Multiplier64 chooseMultiplier(Uint64 d, int p) {
    constexpr int N = 64;
    int l = N - __builtin_clzll(d - 1);
    Uint128 low = (Uint128(1) << (N + l)) / d;
    Uint128 high = ((Uint128(1) << (N + l)) + (Uint128(1) << (N + l - p))) / d;
    while ((low >> 1) < (high >> 1) && l > 0)
        low >>= 1, high >>= 1, --l;
    return {high, l};
}

template <> void RiscV64Selector::ConvertAndAppend<ArithmeticInstruction *>(ArithmeticInstruction *ins) {
    if (ins->GetOpcode() == BasicInstruction::ADD) {
        if (ins->GetDataType() == BasicInstruction::I32) {
            // Imm + Imm
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                auto *imm1_op = (ImmI32Operand *)ins->GetOperand1();
                auto *imm2_op = (ImmI32Operand *)ins->GetOperand2();
                auto *rd_op = (RegOperand *)ins->GetResult();

                auto imm1 = imm1_op->GetIntImmVal();
                auto imm2 = imm2_op->GetIntImmVal();
                auto rd = GetllvmReg(rd_op->GetRegNo(), INT64);

                auto copy_imm_instr = rvconstructor->ConstructCopyRegImmI(rd, imm1 + imm2, INT64);
                cur_block->push_back(copy_imm_instr);
            }
            // Reg + Imm
            // May Generate COPY
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);

                auto *rd_op = (RegOperand *)ins->GetResult();
                auto *rs_op = (RegOperand *)ins->GetOperand1();
                auto *i_op = (ImmI32Operand *)ins->GetOperand2();

                auto rd = GetllvmReg(rd_op->GetRegNo(), INT64);
                auto rs = GetllvmReg(rs_op->GetRegNo(), INT64);
                auto imm = i_op->GetIntImmVal();

                if (imm != 0) {
                    auto addiw_instr = rvconstructor->ConstructIImm(RISCV_ADDIW, rd, rs, imm);
                    cur_block->push_back(addiw_instr);
                } else {
                    auto copy_instr = rvconstructor->ConstructCopyReg(rd, rs, INT64);
                    cur_block->push_back(copy_instr);
                }
            }
            // Reg + Reg
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::REG) {
                Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
                auto *rd_op = (RegOperand *)ins->GetResult();
                auto *rs_op = (RegOperand *)ins->GetOperand1();
                auto *rt_op = (RegOperand *)ins->GetOperand2();

                auto rd = GetllvmReg(rd_op->GetRegNo(), INT64);
                auto rs = GetllvmReg(rs_op->GetRegNo(), INT64);
                auto rt = GetllvmReg(rt_op->GetRegNo(), INT64);

                auto addw_instr = rvconstructor->ConstructR(RISCV_ADDW, rd, rs, rt);

                cur_block->push_back(addw_instr);
            }
            // Imm + Reg
            // May Generate COPY
            if (ins->GetOperand2()->GetOperandType() == BasicOperand::REG &&
                ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {
                Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);

                auto *rd_op = (RegOperand *)ins->GetResult();
                auto *rs_op = (RegOperand *)ins->GetOperand2();
                auto *i_op = (ImmI32Operand *)ins->GetOperand1();

                auto rd = GetllvmReg(rd_op->GetRegNo(), INT64);
                auto rs = GetllvmReg(rs_op->GetRegNo(), INT64);
                auto imm = i_op->GetIntImmVal();

                if (imm != 0) {
                    auto addiw_instr = rvconstructor->ConstructIImm(RISCV_ADDIW, rd, rs, imm);
                    cur_block->push_back(addiw_instr);
                } else {
                    auto copy_instr = rvconstructor->ConstructCopyReg(rd, rs, INT64);
                    cur_block->push_back(copy_instr);
                }
            }
        } else {
            TODO("RV InstSelect For DataType %d", ins->GetDataType());
        }
        // Imm-Imm
    } else if (ins->GetOpcode() == BasicInstruction::SUB) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmI32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmI32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetIntImmVal();
            auto imm2 = imm2_op->GetIntImmVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), INT64);

            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmI(rd, imm1 - imm2, INT64);
            cur_block->push_back(copy_imm_instr);
        } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG &&
                   ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), INT64);
            Register sub1 = GetllvmReg(((RegOperand *)ins->GetOperand1())->GetRegNo(), INT64);
            int imm = ((ImmI32Operand *)ins->GetOperand2())->GetIntImmVal();
            // if (imm == -2147483648) {
            // Log("Entered -INF");
            // Register sub2 = GetNewReg(INT64);
            // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(sub2, imm, INT64));
            // cur_block->push_back(rvconstructor->ConstructR(RISCV_SUBW, rd, sub1, sub2));
            // } else {
            // Log("Entered -%d", imm);
            cur_block->push_back(rvconstructor->ConstructIImm(RISCV_ADDIW, rd, sub1, -imm));
            // }
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), INT64);
            Register sub1 = ExtractOp2Reg(ins->GetOperand1(), INT64);
            Register sub2 = ExtractOp2Reg(ins->GetOperand2(), INT64);
            auto sub_instr = rvconstructor->ConstructR(RISCV_SUBW, rd, sub1, sub2);
            cur_block->push_back(sub_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::DIV) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmI32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmI32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetIntImmVal();
            auto imm2 = imm2_op->GetIntImmVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), INT64);

            Assert(imm2 != 0);
            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmI(rd, imm1 / imm2, INT64);
            cur_block->push_back(copy_imm_instr);
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), INT64);
            Register div1, div2;
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {
                div1 = GetNewReg(INT64);
                auto copy_imm_instr =
                rvconstructor->ConstructCopyRegImmI(div1, ((ImmI32Operand *)ins->GetOperand1())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG) {
                div1 = GetllvmReg(((RegOperand *)ins->GetOperand1())->GetRegNo(), INT64);
            } else {
                ERROR("Unexpected op type");
            }
            if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                div2 = GetNewReg(INT64);
                auto copy_imm_instr =
                rvconstructor->ConstructCopyRegImmI(div2, ((ImmI32Operand *)ins->GetOperand2())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand2()->GetOperandType() == BasicOperand::REG) {
                div2 = GetllvmReg(((RegOperand *)ins->GetOperand2())->GetRegNo(), INT64);
            } else {
                ERROR("Unexpected op type");
            }
            auto div_instr = rvconstructor->ConstructR(RISCV_DIVW, rd, div1, div2);
            cur_block->push_back(div_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::MUL) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmI32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmI32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetIntImmVal();
            auto imm2 = imm2_op->GetIntImmVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), INT64);

            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmI(rd, imm1 * imm2, INT64);
            cur_block->push_back(copy_imm_instr);
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), INT64);
            Register mul1, mul2;
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {
                mul1 = GetNewReg(INT64);
                auto copy_imm_instr =
                rvconstructor->ConstructCopyRegImmI(mul1, ((ImmI32Operand *)ins->GetOperand1())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG) {
                mul1 = GetllvmReg(((RegOperand *)ins->GetOperand1())->GetRegNo(), INT64);
            } else {
                ERROR("Unexpected op type");
            }
            if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                mul2 = GetNewReg(INT64);
                auto copy_imm_instr =
                rvconstructor->ConstructCopyRegImmI(mul2, ((ImmI32Operand *)ins->GetOperand2())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand2()->GetOperandType() == BasicOperand::REG) {
                mul2 = GetllvmReg(((RegOperand *)ins->GetOperand2())->GetRegNo(), INT64);
            } else {
                ERROR("Unexpected op type");
            }
            auto mul_instr = rvconstructor->ConstructR(RISCV_MULW, rd, mul1, mul2);
            cur_block->push_back(mul_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::MOD) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmI32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmI32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetIntImmVal();
            auto imm2 = imm2_op->GetIntImmVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), INT64);

            Assert(imm2 != 0);
            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmI(rd, imm1 % imm2, INT64);
            cur_block->push_back(copy_imm_instr);
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), INT64);
            Register rem1, rem2;
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {
                rem1 = GetNewReg(INT64);
                auto copy_imm_instr =
                rvconstructor->ConstructCopyRegImmI(rem1, ((ImmI32Operand *)ins->GetOperand1())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG) {
                rem1 = GetllvmReg(((RegOperand *)ins->GetOperand1())->GetRegNo(), INT64);
            } else {
                ERROR("Unexpected op type");
            }
            if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                rem2 = GetNewReg(INT64);
                auto copy_imm_instr =
                rvconstructor->ConstructCopyRegImmI(rem2, ((ImmI32Operand *)ins->GetOperand2())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand2()->GetOperandType() == BasicOperand::REG) {
                rem2 = GetllvmReg(((RegOperand *)ins->GetOperand2())->GetRegNo(), INT64);
            } else {
                ERROR("Unexpected op type");
            }
            auto rem_instr = rvconstructor->ConstructR(RISCV_REMW, rd, rem1, rem2);
            cur_block->push_back(rem_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::FADD) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmF32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmF32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetFloatVal();
            auto imm2 = imm2_op->GetFloatVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), FLOAT64);

            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(rd, imm1 + imm2, FLOAT64);
            cur_block->push_back(copy_imm_instr);
            // float val = imm1 + imm2;

            // auto inter_reg = GetNewReg(INT64);
            // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(inter_reg, *(int *)&val, INT64));
            // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, rd, inter_reg));
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), FLOAT64);
            Register fadd1 = ExtractOp2Reg(ins->GetOperand1(), FLOAT64);
            Register fadd2 = ExtractOp2Reg(ins->GetOperand2(), FLOAT64);
            auto fadd_instr = rvconstructor->ConstructR(RISCV_FADD_S, rd, fadd1, fadd2);
            cur_block->push_back(fadd_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::FSUB) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmF32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmF32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetFloatVal();
            auto imm2 = imm2_op->GetFloatVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), FLOAT64);

            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(rd, imm1 - imm2, FLOAT64);
            cur_block->push_back(copy_imm_instr);
            // float val = imm1 - imm2;

            // auto inter_reg = GetNewReg(INT64);
            // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(inter_reg, *(int *)&val, INT64));
            // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, rd, inter_reg));
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), FLOAT64);
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32) {
                float op1_val = ExtractOp2ImmF32(ins->GetOperand1());
                if (op1_val == 0) {
                    // Log("Neg");
                    Assert(ins->GetOperand2()->GetOperandType() == BasicOperand::REG);
                    cur_block->push_back(
                    rvconstructor->ConstructR2(RISCV_FNEG_S, rd, ExtractOp2Reg(ins->GetOperand2(), FLOAT64)));
                    return;
                }
            } else if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
                float op2_val = ExtractOp2ImmF32(ins->GetOperand2());
                if (op2_val == 0) {
                    // Log("sub 0");
                    Assert(ins->GetOperand1()->GetOperandType() == BasicOperand::REG);
                    cur_block->push_back(
                    rvconstructor->ConstructCopyReg(rd, ExtractOp2Reg(ins->GetOperand1(), FLOAT64), FLOAT64));
                    return;
                }
            }
            Register fsub1, fsub2;
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32) {
                fsub1 = GetNewReg(FLOAT64);
                auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(
                fsub1, ((ImmF32Operand *)ins->GetOperand1())->GetFloatVal(), FLOAT64);
                // float val = ((ImmF32Operand *)ins->GetOperand1())->GetFloatVal();

                // auto inter_reg = GetNewReg(INT64);
                // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(inter_reg, *(int *)&val, INT64));
                // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, fsub1, inter_reg));
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG) {
                fsub1 = GetllvmReg(((RegOperand *)ins->GetOperand1())->GetRegNo(), FLOAT64);
            } else {
                ERROR("Unexpected op type");
            }
            if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
                fsub2 = GetNewReg(FLOAT64);
                auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(
                fsub2, ((ImmF32Operand *)ins->GetOperand2())->GetFloatVal(), FLOAT64);
                // float val = ((ImmF32Operand *)ins->GetOperand2())->GetFloatVal();

                // auto inter_reg = GetNewReg(INT64);
                // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(inter_reg, *(int *)&val, INT64));
                // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, fsub2, inter_reg));
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand2()->GetOperandType() == BasicOperand::REG) {
                fsub2 = GetllvmReg(((RegOperand *)ins->GetOperand2())->GetRegNo(), FLOAT64);
            } else {
                ERROR("Unexpected op type");
            }
            auto fsub_instr = rvconstructor->ConstructR(RISCV_FSUB_S, rd, fsub1, fsub2);
            cur_block->push_back(fsub_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::FMUL) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmF32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmF32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetFloatVal();
            auto imm2 = imm2_op->GetFloatVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), FLOAT64);

            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(rd, imm1 * imm2, FLOAT64);
            cur_block->push_back(copy_imm_instr);
            // float val = imm1 * imm2;

            // auto inter_reg = GetNewReg(INT64);
            // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(inter_reg, *(int *)&val, INT64));
            // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, rd, inter_reg));
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), FLOAT64);
            Register fsub1, fsub2;
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32) {
                fsub1 = GetNewReg(FLOAT64);
                auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(
                fsub1, ((ImmF32Operand *)ins->GetOperand1())->GetFloatVal(), FLOAT64);
                // float val = ((ImmF32Operand *)ins->GetOperand1())->GetFloatVal();

                // auto inter_reg = GetNewReg(INT64);
                // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(inter_reg, *(int *)&val, INT64));
                // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, fsub1, inter_reg));
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG) {
                fsub1 = GetllvmReg(((RegOperand *)ins->GetOperand1())->GetRegNo(), FLOAT64);
            } else {
                ERROR("Unexpected op type");
            }
            if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
                fsub2 = GetNewReg(FLOAT64);
                auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(
                fsub2, ((ImmF32Operand *)ins->GetOperand2())->GetFloatVal(), FLOAT64);
                // float val = ((ImmF32Operand *)ins->GetOperand2())->GetFloatVal();

                // auto inter_reg = GetNewReg(INT64);
                // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(inter_reg, *(int *)&val, INT64));
                // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, fsub2, inter_reg));
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand2()->GetOperandType() == BasicOperand::REG) {
                fsub2 = GetllvmReg(((RegOperand *)ins->GetOperand2())->GetRegNo(), FLOAT64);
            } else {
                ERROR("Unexpected op type");
            }
            auto fsub_instr = rvconstructor->ConstructR(RISCV_FMUL_S, rd, fsub1, fsub2);
            cur_block->push_back(fsub_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::FDIV) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmF32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmF32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetFloatVal();
            auto imm2 = imm2_op->GetFloatVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), FLOAT64);

            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(rd, imm1 / imm2, FLOAT64);
            cur_block->push_back(copy_imm_instr);
            // float val = imm1 / imm2;

            // auto inter_reg = GetNewReg(INT64);
            // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(inter_reg, *(int *)&val, INT64));
            // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, rd, inter_reg));
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), FLOAT64);
            Register fsub1, fsub2;
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32) {
                fsub1 = GetNewReg(FLOAT64);
                auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(
                fsub1, ((ImmF32Operand *)ins->GetOperand1())->GetFloatVal(), FLOAT64);
                // float val = ((ImmF32Operand *)ins->GetOperand1())->GetFloatVal();

                // auto inter_reg = GetNewReg(INT64);
                // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(inter_reg, *(int *)&val, INT64));
                // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, fsub1, inter_reg));
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG) {
                fsub1 = GetllvmReg(((RegOperand *)ins->GetOperand1())->GetRegNo(), FLOAT64);
            } else {
                ERROR("Unexpected op type");
            }
            if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
                fsub2 = GetNewReg(FLOAT64);
                auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(
                fsub2, ((ImmF32Operand *)ins->GetOperand2())->GetFloatVal(), FLOAT64);
                // float val = ((ImmF32Operand *)ins->GetOperand2())->GetFloatVal();

                // auto inter_reg = GetNewReg(INT64);
                // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(inter_reg, *(int *)&val, INT64));
                // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, fsub2, inter_reg));
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand2()->GetOperandType() == BasicOperand::REG) {
                fsub2 = GetllvmReg(((RegOperand *)ins->GetOperand2())->GetRegNo(), FLOAT64);
            } else {
                ERROR("Unexpected op type");
            }
            auto fsub_instr = rvconstructor->ConstructR(RISCV_FDIV_S, rd, fsub1, fsub2);
            cur_block->push_back(fsub_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::LL_ADDMOD) {
        auto op1 = ins->GetOperand1();
        auto op2 = ins->GetOperand2();
        auto op3 = ins->GetOperand3();
        auto result_op = ins->GetResult();
        Assert(result_op->GetOperandType() == BasicOperand::REG);
        auto result_reg = GetllvmReg(((RegOperand *)result_op)->GetRegNo(), INT64);
        if (op1->GetOperandType() == BasicOperand::IMMI32 && op2->GetOperandType() == BasicOperand::IMMI32 &&
            op3->GetOperandType() == BasicOperand::IMMI32) {
            unsigned int val1 = ((ImmI32Operand *)op1)->GetIntImmVal();
            unsigned int val2 = ((ImmI32Operand *)op2)->GetIntImmVal();
            unsigned int val3 = ((ImmI32Operand *)op3)->GetIntImmVal();
            cur_block->push_back(rvconstructor->ConstructCopyRegImmI(result_reg, (1ll * val1 * val2) % val3, INT64));
        } else {
            Assert(op3->GetOperandType() == BasicOperand::IMMI32);
            Uint64 div = ((ImmI32Operand *)op3)->GetIntImmVal();
            Register r1 = ExtractOp2Reg(op1, INT64);
            Register r2 = ExtractOp2Reg(op2, INT64);
            Register r1_64 = GetNewReg(INT64);
            Register r2_64 = GetNewReg(INT64);
            Register r_n = GetNewReg(INT64);
            cur_block->push_back(rvconstructor->ConstructR2(RISCV_ZEXT_W, r1_64, r1));
            cur_block->push_back(rvconstructor->ConstructR2(RISCV_ZEXT_W, r2_64, r2));
            cur_block->push_back(rvconstructor->ConstructR(RISCV_MUL, r_n, r1_64, r2_64));
            Register r_n0 = r_n;
            if (__builtin_ctzll(div)) {
                r_n0 = GetNewReg(INT64);
                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_SRLI, r_n0, r_n, __builtin_ctzll(div)));
            }
            Multiplier64 mult = chooseMultiplier(div >> __builtin_ctzll(div), 64 - __builtin_ctzll(div));
            Uint128 mul_imm = mult.m;
            bool overflow = false;
            if (mul_imm >= Uint128(1) << 64) {
                mul_imm -= ((Uint128(1)) << 64);
                overflow = true;
            }
            Assert(mul_imm < Uint128(1) << 64);
            Register mulimm_r = GetNewReg(INT64);
            if (mul_imm < Uint128(1) << 32) {
                cur_block->push_back(rvconstructor->ConstructCopyRegImmI(mulimm_r, mul_imm, INT64));
            } else {
                std::string imm_name = ".IMM_" + std::to_string((Uint64)mul_imm);
                if (!global_imm_vsd[mul_imm]) {
                    dest->global_def.push_back(
                    new GlobalVarDefineInstruction(imm_name, BasicInstruction::I64, new ImmI64Operand(mul_imm)));
                    global_imm_vsd[mul_imm] = true;
                }
                Register lui_r = GetNewReg(INT64);
                cur_block->push_back(rvconstructor->ConstructULabel(RISCV_LUI, lui_r, RiscVLabel(imm_name, true)));
                cur_block->push_back(
                rvconstructor->ConstructILabel(RISCV_LD, mulimm_r, lui_r, RiscVLabel(imm_name, false)));
            }
            Register k_r = GetNewReg(INT64);
            cur_block->push_back(rvconstructor->ConstructR(RISCV_MULHU, k_r, r_n0, mulimm_r));
            Register div_r = GetNewReg(INT64);
            if (!overflow) {
                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_SRLI, div_r, k_r, mult.l));
            } else {
                Register sub_r = GetNewReg(INT64);
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SUB, sub_r, r_n0, k_r));
                Register sub_d2 = GetNewReg(INT64);
                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_SRLI, sub_d2, sub_r, 1));
                Register add_r = GetNewReg(INT64);
                cur_block->push_back(rvconstructor->ConstructR(RISCV_ADD, add_r, sub_d2, k_r));
                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_SRLI, div_r, add_r, mult.l - 1));
            }
            Register prod_part_r = GetNewReg(INT64);
            Register divisor_r = GetNewReg(INT64);
            cur_block->push_back(rvconstructor->ConstructCopyRegImmI(divisor_r, div, INT64));
            cur_block->push_back(rvconstructor->ConstructR(RISCV_MUL, prod_part_r, div_r, divisor_r));
            cur_block->push_back(rvconstructor->ConstructR(RISCV_SUB, result_reg, r_n, prod_part_r));
        }
    } else if (ins->GetOpcode() == BasicInstruction::UMIN_I32) {
        TODO("UMIN");
    } else if (ins->GetOpcode() == BasicInstruction::UMAX_I32) {
        TODO("UMAX");
    } else if (ins->GetOpcode() == BasicInstruction::SMIN_I32) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmI32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmI32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetIntImmVal();
            auto imm2 = imm2_op->GetIntImmVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), INT64);

            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmI(rd, imm1 < imm2 ? imm1 : imm2, INT64);
            cur_block->push_back(copy_imm_instr);
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), INT64);
            Register min1, min2;
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {
                min1 = GetNewReg(INT64);
                auto copy_imm_instr =
                rvconstructor->ConstructCopyRegImmI(min1, ((ImmI32Operand *)ins->GetOperand1())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG) {
                min1 = GetllvmReg(((RegOperand *)ins->GetOperand1())->GetRegNo(), INT64);
            } else {
                ERROR("Unexpected op type");
            }
            if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                min2 = GetNewReg(INT64);
                auto copy_imm_instr =
                rvconstructor->ConstructCopyRegImmI(min2, ((ImmI32Operand *)ins->GetOperand2())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand2()->GetOperandType() == BasicOperand::REG) {
                min2 = GetllvmReg(((RegOperand *)ins->GetOperand2())->GetRegNo(), INT64);
            } else {
                ERROR("Unexpected op type");
            }
            auto min_instr = rvconstructor->ConstructR(RISCV_MIN, rd, min1, min2);
            cur_block->push_back(min_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::SMAX_I32) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmI32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmI32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetIntImmVal();
            auto imm2 = imm2_op->GetIntImmVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), INT64);

            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmI(rd, imm1 > imm2 ? imm1 : imm2, INT64);
            cur_block->push_back(copy_imm_instr);
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), INT64);
            Register max1, max2;
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {
                max1 = GetNewReg(INT64);
                auto copy_imm_instr =
                rvconstructor->ConstructCopyRegImmI(max1, ((ImmI32Operand *)ins->GetOperand1())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG) {
                max1 = GetllvmReg(((RegOperand *)ins->GetOperand1())->GetRegNo(), INT64);
            } else {
                ERROR("Unexpected op type");
            }
            if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                max2 = GetNewReg(INT64);
                auto copy_imm_instr =
                rvconstructor->ConstructCopyRegImmI(max2, ((ImmI32Operand *)ins->GetOperand2())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else if (ins->GetOperand2()->GetOperandType() == BasicOperand::REG) {
                max2 = GetllvmReg(((RegOperand *)ins->GetOperand2())->GetRegNo(), INT64);
            } else {
                ERROR("Unexpected op type");
            }
            auto max_instr = rvconstructor->ConstructR(RISCV_MAX, rd, max1, max2);
            cur_block->push_back(max_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::FMIN_F32) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmF32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmF32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetFloatVal();
            auto imm2 = imm2_op->GetFloatVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), FLOAT64);

            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(rd, imm1 < imm2 ? imm1 : imm2, FLOAT64);
            cur_block->push_back(copy_imm_instr);
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), FLOAT64);
            Register max1 = ExtractOp2Reg(ins->GetOperand1(), FLOAT64);
            Register max2 = ExtractOp2Reg(ins->GetOperand2(), FLOAT64);
            auto min_instr = rvconstructor->ConstructR(RISCV_FMIN_S, rd, max1, max2);
            cur_block->push_back(min_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::FMAX_F32) {
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            auto *rd_op = (RegOperand *)ins->GetResult();
            auto *imm1_op = (ImmF32Operand *)ins->GetOperand1();
            auto *imm2_op = (ImmF32Operand *)ins->GetOperand2();

            auto imm1 = imm1_op->GetFloatVal();
            auto imm2 = imm2_op->GetFloatVal();
            auto rd = GetllvmReg(rd_op->GetRegNo(), FLOAT64);

            auto copy_imm_instr = rvconstructor->ConstructCopyRegImmF(rd, imm1 > imm2 ? imm1 : imm2, FLOAT64);
            cur_block->push_back(copy_imm_instr);
        } else {
            Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
            Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), FLOAT64);
            Register max1 = ExtractOp2Reg(ins->GetOperand1(), FLOAT64);
            Register max2 = ExtractOp2Reg(ins->GetOperand2(), FLOAT64);
            auto max_instr = rvconstructor->ConstructR(RISCV_FMAX_S, rd, max1, max2);
            cur_block->push_back(max_instr);
        }
    } else if (ins->GetOpcode() == BasicInstruction::BITAND) {
        Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), INT64);
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
            int imm1 = ExtractOp2ImmI32(ins->GetOperand1());
            int imm2 = ExtractOp2ImmI32(ins->GetOperand2());
            cur_block->push_back(rvconstructor->ConstructCopyRegImmI(rd, imm1 & imm2, INT64));
        } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 ||
                   ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
            int imm = ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32
                      ? ExtractOp2ImmI32(ins->GetOperand1())
                      : ExtractOp2ImmI32(ins->GetOperand2());
            Register rs = ins->GetOperand1()->GetOperandType() == BasicOperand::REG
                          ? ExtractOp2Reg(ins->GetOperand1(), INT64)
                          : ExtractOp2Reg(ins->GetOperand2(), INT64);
            cur_block->push_back(rvconstructor->ConstructIImm(RISCV_ANDI, rd, rs, imm));
        } else {
            cur_block->push_back(rvconstructor->ConstructR(RISCV_AND, rd, ExtractOp2Reg(ins->GetOperand1(), INT64),
                                                           ExtractOp2Reg(ins->GetOperand2(), INT64)));
        }
    } else if (ins->GetOpcode() == BasicInstruction::BITXOR) {
        Register rd = GetllvmReg(((RegOperand *)ins->GetResult())->GetRegNo(), INT64);
        if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
            ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
            int imm1 = ExtractOp2ImmI32(ins->GetOperand1());
            int imm2 = ExtractOp2ImmI32(ins->GetOperand2());
            cur_block->push_back(rvconstructor->ConstructCopyRegImmI(rd, imm1 ^ imm2, INT64));
        } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 ||
                   ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
            int imm = ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32
                      ? ExtractOp2ImmI32(ins->GetOperand1())
                      : ExtractOp2ImmI32(ins->GetOperand2());
            Register rs = ins->GetOperand1()->GetOperandType() == BasicOperand::REG
                          ? ExtractOp2Reg(ins->GetOperand1(), INT64)
                          : ExtractOp2Reg(ins->GetOperand2(), INT64);
            cur_block->push_back(rvconstructor->ConstructIImm(RISCV_XORI, rd, rs, imm));
        } else {
            cur_block->push_back(rvconstructor->ConstructR(RISCV_XOR, rd, ExtractOp2Reg(ins->GetOperand1(), INT64),
                                                           ExtractOp2Reg(ins->GetOperand2(), INT64)));
        }
    } else {
        Log("RV InstSelect For Opcode %d", ins->GetOpcode());
    }
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<IcmpInstruction *>(IcmpInstruction *ins) {
    Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
    auto res_op = (RegOperand *)ins->GetResult();
    auto res_reg = GetllvmReg(res_op->GetRegNo(), INT64);
    cmp_context[res_reg] = ins;
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<FcmpInstruction *>(FcmpInstruction *ins) {
    Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
    auto res_op = (RegOperand *)ins->GetResult();
    auto res_reg = GetllvmReg(res_op->GetRegNo(), INT64);
    cmp_context[res_reg] = ins;
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<AllocaInstruction *>(AllocaInstruction *ins) {
    Assert(ins->GetResult()->GetOperandType() == BasicOperand::REG);
    auto reg_op = (RegOperand *)ins->GetResult();
    int byte_size = ins->GetAllocaSize() << 2;
    // Log("Alloca size %d", byte_size);
    llvm_rv_allocas[reg_op->GetRegNo()] = cur_offset;
    cur_offset += byte_size;
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<BrCondInstruction *>(BrCondInstruction *ins) {
    // Log("RV InstSelect");
    // Lazy("Testing RV InstSelect");
    Assert(ins->GetCond()->GetOperandType() == BasicOperand::REG);
    auto cond_reg = (RegOperand *)ins->GetCond();
    auto br_reg = GetllvmReg(cond_reg->GetRegNo(), INT64);
    auto cmp_ins = cmp_context[br_reg];
    // Assert(br_reg == cmpInfo.cmp_result);
    int opcode;
    Register cmp_rd, cmp_op1, cmp_op2;
    if (cmp_ins->GetOpcode() == BasicInstruction::ICMP) {
        auto icmp_ins = (IcmpInstruction *)cmp_ins;
        if (icmp_ins->GetOp1()->GetOperandType() == BasicOperand::REG) {
            cmp_op1 = GetllvmReg(((RegOperand *)icmp_ins->GetOp1())->GetRegNo(), INT64);
        } else if (icmp_ins->GetOp1()->GetOperandType() == BasicOperand::IMMI32) {
            // volatile int temp = ((ImmI32Operand *)icmp_ins->GetOp1())->GetIntImmVal();
            if (((ImmI32Operand *)icmp_ins->GetOp1())->GetIntImmVal() != 0) {
                cmp_op1 = GetNewReg(INT64);
                auto copy_imm_instr = rvconstructor->ConstructCopyRegImmI(
                cmp_op1, ((ImmI32Operand *)icmp_ins->GetOp1())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else {
                cmp_op1 = GetPhysicalReg(RISCV_x0);
            }
        } else {
            ERROR("Unexpected ICMP op1 type");
        }
        if (icmp_ins->GetOp2()->GetOperandType() == BasicOperand::REG) {
            cmp_op2 = GetllvmReg(((RegOperand *)icmp_ins->GetOp2())->GetRegNo(), INT64);
        } else if (icmp_ins->GetOp2()->GetOperandType() == BasicOperand::IMMI32) {
            if (((ImmI32Operand *)icmp_ins->GetOp2())->GetIntImmVal() != 0) {
                cmp_op2 = GetNewReg(INT64);
                auto copy_imm_instr = rvconstructor->ConstructCopyRegImmI(
                cmp_op2, ((ImmI32Operand *)icmp_ins->GetOp2())->GetIntImmVal(), INT64);
                cur_block->push_back(copy_imm_instr);
            } else {
                cmp_op2 = GetPhysicalReg(RISCV_x0);
            }
        } else {
            ERROR("Unexpected ICMP op2 type");
        }
        switch (icmp_ins->GetCond()) {
        case BasicInstruction::eq:    // beq
            opcode = RISCV_BEQ;
            break;
        case BasicInstruction::ne:    // bne
            opcode = RISCV_BNE;
            break;
        case BasicInstruction::ugt:    // bgtu --p
            opcode = RISCV_BGTU;
            break;
        case BasicInstruction::uge:    // bgeu
            opcode = RISCV_BGEU;
            break;
        case BasicInstruction::ult:    // bltu
            opcode = RISCV_BLTU;
            break;
        case BasicInstruction::ule:    // bleu --p
            opcode = RISCV_BLEU;
            break;
        case BasicInstruction::sgt:    // bgt --p
            opcode = RISCV_BGT;
            break;
        case BasicInstruction::sge:    // bge
            opcode = RISCV_BGE;
            break;
        case BasicInstruction::slt:    // blt
            opcode = RISCV_BLT;
            break;
        case BasicInstruction::sle:    // ble --p
            opcode = RISCV_BLE;
            break;
        default:
            ERROR("Unexpected ICMP cond");
        }
    } else if (cmp_ins->GetOpcode() == BasicInstruction::FCMP) {
        auto fcmp_ins = (FcmpInstruction *)cmp_ins;
        if (fcmp_ins->GetOp1()->GetOperandType() == BasicOperand::REG) {
            cmp_op1 = GetllvmReg(((RegOperand *)fcmp_ins->GetOp1())->GetRegNo(), FLOAT64);
        } else if (fcmp_ins->GetOp1()->GetOperandType() == BasicOperand::IMMF32) {
            cmp_op1 = GetNewReg(FLOAT64);
            auto cmp_oppre = GetNewReg(INT64);
            // float float_val = ((ImmF32Operand *)fcmp_ins->GetOp1())->GetFloatVal();
            // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(cmp_oppre, *(int *)&float_val, INT64));
            // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, cmp_op1, cmp_oppre));
            auto copy_imm_instr =
            rvconstructor->ConstructCopyRegImmF(cmp_op1, ((ImmF32Operand *)fcmp_ins->GetOp1())->GetFloatVal(), FLOAT64);
            cur_block->push_back(copy_imm_instr);
        } else {
            ERROR("Unexpected FCMP op1 type");
        }
        if (fcmp_ins->GetOp2()->GetOperandType() == BasicOperand::REG) {
            cmp_op2 = GetllvmReg(((RegOperand *)fcmp_ins->GetOp2())->GetRegNo(), FLOAT64);
        } else if (fcmp_ins->GetOp2()->GetOperandType() == BasicOperand::IMMF32) {
            cmp_op2 = GetNewReg(FLOAT64);
            auto cmp_oppre = GetNewReg(INT64);
            // float float_val = ((ImmF32Operand *)fcmp_ins->GetOp2())->GetFloatVal();
            // cur_block->push_back(rvconstructor->ConstructCopyRegImmI(cmp_oppre, *(int *)&float_val, INT64));
            // cur_block->push_back(rvconstructor->ConstructR2(RISCV_FMV_W_X, cmp_op2, cmp_oppre));
            auto copy_imm_instr =
            rvconstructor->ConstructCopyRegImmF(cmp_op2, ((ImmF32Operand *)fcmp_ins->GetOp2())->GetFloatVal(), FLOAT64);
            cur_block->push_back(copy_imm_instr);
        } else {
            ERROR("Unexpected FCMP op2 type");
        }
        cmp_rd = GetNewReg(INT64);
        switch (fcmp_ins->GetCond()) {
        case BasicInstruction::OEQ:
        case BasicInstruction::UEQ:
            // FEQ.S
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FEQ_S, cmp_rd, cmp_op1, cmp_op2));
            opcode = RISCV_BNE;
            break;
        case BasicInstruction::OGT:
        case BasicInstruction::UGT:
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FLT_S, cmp_rd, cmp_op2, cmp_op1));
            opcode = RISCV_BNE;
            break;
        case BasicInstruction::OGE:
        case BasicInstruction::UGE:
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FLE_S, cmp_rd, cmp_op2, cmp_op1));
            opcode = RISCV_BNE;
            break;
        case BasicInstruction::OLT:
        case BasicInstruction::ULT:
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FLT_S, cmp_rd, cmp_op1, cmp_op2));
            opcode = RISCV_BNE;
            break;
        case BasicInstruction::OLE:
        case BasicInstruction::ULE:
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FLE_S, cmp_rd, cmp_op1, cmp_op2));
            opcode = RISCV_BNE;
            break;
        case BasicInstruction::ONE:
        case BasicInstruction::UNE:
            cur_block->push_back(rvconstructor->ConstructR(RISCV_FEQ_S, cmp_rd, cmp_op1, cmp_op2));
            opcode = RISCV_BEQ;
            break;
        case BasicInstruction::ORD:
        case BasicInstruction::UNO:
        case BasicInstruction::TRUE:
        case BasicInstruction::FALSE:
        default:
            ERROR("Unexpected FCMP cond");
        }
        cmp_op1 = cmp_rd;
        cmp_op2 = GetPhysicalReg(RISCV_x0);
    } else {
        ERROR("No Cmp Before Br");
    }
    Assert(ins->GetTrueLabel()->GetOperandType() == BasicOperand::LABEL);
    Assert(ins->GetFalseLabel()->GetOperandType() == BasicOperand::LABEL);
    auto true_label = (LabelOperand *)ins->GetTrueLabel();
    auto false_label = (LabelOperand *)ins->GetFalseLabel();

    auto br_ins = rvconstructor->ConstructBLabel(opcode, cmp_op1, cmp_op2,
                                                 RiscVLabel(true_label->GetLabelNo()));
    cur_block->push_back(br_ins);
    auto br_else_ins =
    rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), RiscVLabel(false_label->GetLabelNo()));
    cur_block->push_back(br_else_ins);
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<BrUncondInstruction *>(BrUncondInstruction *ins) {
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<CallInstruction *>(CallInstruction *ins) {
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<RetInstruction *>(RetInstruction *ins) {
    if (ins->GetRetVal() != NULL) {
        if (ins->GetRetVal()->GetOperandType() == BasicOperand::IMMI32) {
            auto retimm_op = (ImmI32Operand *)ins->GetRetVal();
            auto imm = retimm_op->GetIntImmVal();

            auto retcopy_instr = rvconstructor->ConstructUImm(RISCV_LI, GetPhysicalReg(RISCV_a0), imm);
            cur_block->push_back(retcopy_instr);
        } else if (ins->GetRetVal()->GetOperandType() == BasicOperand::IMMF32) {
            TODO("Implement this if you need");
        } else if (ins->GetRetVal()->GetOperandType() == BasicOperand::REG) {
            TODO("Implement this if you need");
        }
    }

    auto ret_instr = rvconstructor->ConstructIImm(RISCV_JALR, GetPhysicalReg(RISCV_x0), GetPhysicalReg(RISCV_ra), 0);
    if (ins->GetType() == BasicInstruction::I32) {
        ret_instr->setRetType(1);
    } else if (ins->GetType() == BasicInstruction::FLOAT32) {
        ret_instr->setRetType(2);
    } else {
        ret_instr->setRetType(0);
    }
    cur_block->push_back(ret_instr);
}

template <> void RiscV64Selector::ConvertAndAppend<FptosiInstruction *>(FptosiInstruction *ins) {
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<SitofpInstruction *>(SitofpInstruction *ins) {
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<ZextInstruction *>(ZextInstruction *ins) {
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<GetElementptrInstruction *>(GetElementptrInstruction *ins) {
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<PhiInstruction *>(PhiInstruction *ins) {
    TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<Instruction>(Instruction inst) {
    switch (inst->GetOpcode()) {
    case BasicInstruction::LOAD:
        ConvertAndAppend<LoadInstruction *>((LoadInstruction *)inst);
        break;
    case BasicInstruction::STORE:
        ConvertAndAppend<StoreInstruction *>((StoreInstruction *)inst);
        break;
    case BasicInstruction::ADD:
    case BasicInstruction::SUB:
    case BasicInstruction::MUL:
    case BasicInstruction::DIV:
    case BasicInstruction::FADD:
    case BasicInstruction::FSUB:
    case BasicInstruction::FMUL:
    case BasicInstruction::FDIV:
    case BasicInstruction::MOD:
    case BasicInstruction::SHL:
    case BasicInstruction::BITXOR:
        ConvertAndAppend<ArithmeticInstruction *>((ArithmeticInstruction *)inst);
        break;
    case BasicInstruction::ICMP:
        ConvertAndAppend<IcmpInstruction *>((IcmpInstruction *)inst);
        break;
    case BasicInstruction::FCMP:
        ConvertAndAppend<FcmpInstruction *>((FcmpInstruction *)inst);
        break;
    case BasicInstruction::ALLOCA:
        ConvertAndAppend<AllocaInstruction *>((AllocaInstruction *)inst);
        break;
    case BasicInstruction::BR_COND:
        ConvertAndAppend<BrCondInstruction *>((BrCondInstruction *)inst);
        break;
    case BasicInstruction::BR_UNCOND:
        ConvertAndAppend<BrUncondInstruction *>((BrUncondInstruction *)inst);
        break;
    case BasicInstruction::RET:
        ConvertAndAppend<RetInstruction *>((RetInstruction *)inst);
        break;
    case BasicInstruction::ZEXT:
        ConvertAndAppend<ZextInstruction *>((ZextInstruction *)inst);
        break;
    case BasicInstruction::FPTOSI:
        ConvertAndAppend<FptosiInstruction *>((FptosiInstruction *)inst);
        break;
    case BasicInstruction::SITOFP:
        ConvertAndAppend<SitofpInstruction *>((SitofpInstruction *)inst);
        break;
    case BasicInstruction::GETELEMENTPTR:
        ConvertAndAppend<GetElementptrInstruction *>((GetElementptrInstruction *)inst);
        break;
    case BasicInstruction::CALL:
        ConvertAndAppend<CallInstruction *>((CallInstruction *)inst);
        break;
    case BasicInstruction::PHI:
        ConvertAndAppend<PhiInstruction *>((PhiInstruction *)inst);
        break;
    default:
        ERROR("Unknown LLVM IR instruction");
    }
}

void RiscV64Selector::SelectInstructionAndBuildCFG() {
    // 与中间代码生成一样, 如果你完全无从下手, 可以先看看输出是怎么写的
    // 即riscv64gc/instruction_print/*  common/machine_passes/machine_printer.h

    // 指令选择除了一些函数调用约定必须遵守的情况需要物理寄存器，其余情况必须均为虚拟寄存器
    dest->global_def = IR->global_def;
    // 遍历每个LLVM IR函数
    for (auto [defI,cfg] : IR->llvm_cfg) {
        if(cfg == nullptr){
            ERROR("LLVMIR CFG is Empty, you should implement BuildCFG in MidEnd first");
        }
        std::string name = cfg->function_def->GetFunctionName();

        cur_func = new RiscV64Function(name);
        cur_func->SetParent(dest);
        // 你可以使用cur_func->GetNewRegister来获取新的虚拟寄存器
        dest->functions.push_back(cur_func);

        auto cur_mcfg = new MachineCFG;
        cur_func->SetMachineCFG(cur_mcfg);

        // 清空指令选择状态(可能需要自行添加初始化操作)
        ClearFunctionSelectState();

        // TODO: 添加函数参数(推荐先阅读一下riscv64_lowerframe.cc中的代码和注释)
        // See MachineFunction::AddParameter()
        TODO("Add function parameter if you need");

        // 遍历每个LLVM IR基本块
        for (auto [id, block] : *(cfg->block_map)) {
            cur_block = new RiscV64Block(id);
            // 将新块添加到Machine CFG中
            cur_mcfg->AssignEmptyNode(id, cur_block);
            cur_func->UpdateMaxLabel(id);

            cur_block->setParent(cur_func);
            cur_func->blocks.push_back(cur_block);

            // 指令选择主要函数, 请注意指令选择时需要维护变量cur_offset
            for (auto instruction : block->Instruction_list) {
                // Log("Selecting Instruction");
                ConvertAndAppend<Instruction>(instruction);
            }
        }

        // RISCV 8字节对齐（）
        if (cur_offset % 8 != 0) {
            cur_offset = ((cur_offset + 7) / 8) * 8;
        }
        cur_func->SetStackSize(cur_offset + cur_func->GetParaSize());

        // 控制流图连边
        for (int i = 0; i < cfg->G.size(); i++) {
            const auto &arcs = cfg->G[i];
            for (auto arc : arcs) {
                cur_mcfg->MakeEdge(i, arc->block_id);
            }
        }
    }
}

void RiscV64Selector::ClearFunctionSelectState() { 
    llvm_rv_regtable.clear();
    llvm_rv_allocas.clear();
    cmp_context.clear();
    cur_offset = 0; 
}

Register RiscV64Selector::GetllvmReg(int ir_reg, MachineDataType type) {
    if (llvm_rv_regtable.find(ir_reg) == llvm_rv_regtable.end()) {
        llvm_rv_regtable[ir_reg] = GetNewReg(type);
    }
    Assert(llvm_rv_regtable[ir_reg].type == type);
    return llvm_rv_regtable[ir_reg];
}

Register RiscV64Selector::GetNewReg(MachineDataType type) {
    return cur_func->GetNewRegister(type.data_type, type.data_length);
}

Register RiscV64Selector::ExtractOp2Reg(BasicOperand *op, MachineDataType type) {
    if (op->GetOperandType() == BasicOperand::IMMI32) {
        Assert(type == INT64);
        Register ret = GetNewReg(INT64);
        cur_block->push_back(rvconstructor->ConstructCopyRegImmI(ret, ((ImmI32Operand *)op)->GetIntImmVal(), INT64));
        return ret;
    } else if (op->GetOperandType() == BasicOperand::IMMF32) {
        Assert(type == FLOAT64);
        Register ret = GetNewReg(FLOAT64);
        cur_block->push_back(rvconstructor->ConstructCopyRegImmF(ret, ((ImmF32Operand *)op)->GetFloatVal(), FLOAT64));
        return ret;
    } else if (op->GetOperandType() == BasicOperand::REG) {
        return GetllvmReg(((RegOperand *)op)->GetRegNo(), type);
    } else {
        ERROR("Unexpected op type");
    }
    return Register();
}

int RiscV64Selector::ExtractOp2ImmI32(BasicOperand *op) {
    Assert(op->GetOperandType() == BasicOperand::IMMI32);
    return ((ImmI32Operand *)op)->GetIntImmVal();
}

float RiscV64Selector::ExtractOp2ImmF32(BasicOperand *op) {
    Assert(op->GetOperandType() == BasicOperand::IMMF32);
    return ((ImmF32Operand *)op)->GetFloatVal();
}
