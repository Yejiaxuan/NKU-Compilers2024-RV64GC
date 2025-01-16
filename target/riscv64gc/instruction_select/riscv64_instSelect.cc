#include "riscv64_instSelect.h"
#include <sstream>

template <>
void RiscV64Selector::ConvertAndAppend<LoadInstruction *>(LoadInstruction *ins) {
    if (ins->GetPointer()->GetOperandType() == BasicOperand::REG) {

        auto ptr_op = (RegOperand *)ins->GetPointer();
        auto rd_op = (RegOperand *)ins->GetResult();

        switch (ins->GetDataType()) {
            case BasicInstruction::I32: {
                Register rd = GetllvmReg(rd_op->GetRegNo(), INT64);
                if (llvm_rv_allocas.find(ptr_op->GetRegNo()) == llvm_rv_allocas.end()) {
                    Register ptr = GetllvmReg(ptr_op->GetRegNo(), INT64);
                    auto lw_instr = rvconstructor->ConstructIImm(RISCV_LW, rd, ptr, 0);
                    cur_block->push_back(lw_instr);
                } else {
                    auto sp_offset = llvm_rv_allocas[ptr_op->GetRegNo()];
                    auto lw_instr = rvconstructor->ConstructIImm(RISCV_LW, rd, GetPhysicalReg(RISCV_sp), sp_offset);
                    ((RiscV64Function *)cur_func)->AddAllocaIns(lw_instr);
                    cur_block->push_back(lw_instr);
                }
                break;
            }
            case BasicInstruction::FLOAT32: {
                Register rd = GetllvmReg(rd_op->GetRegNo(), FLOAT64);
                if (llvm_rv_allocas.find(ptr_op->GetRegNo()) == llvm_rv_allocas.end()) {
                    Register ptr = GetllvmReg(ptr_op->GetRegNo(), INT64);
                    auto lw_instr = rvconstructor->ConstructIImm(RISCV_FLW, rd, ptr, 0);
                    cur_block->push_back(lw_instr);
                } else {
                    auto sp_offset = llvm_rv_allocas[ptr_op->GetRegNo()];
                    auto lw_instr = rvconstructor->ConstructIImm(RISCV_FLW, rd, GetPhysicalReg(RISCV_sp), sp_offset);
                    ((RiscV64Function *)cur_func)->AddAllocaIns(lw_instr);
                    cur_block->push_back(lw_instr);
                }
                break;
            }
            case BasicInstruction::PTR: {
                Register rd = GetllvmReg(rd_op->GetRegNo(), INT64);
                if (llvm_rv_allocas.find(ptr_op->GetRegNo()) == llvm_rv_allocas.end()) {
                    Register ptr = GetllvmReg(ptr_op->GetRegNo(), INT64);
                    auto lw_instr = rvconstructor->ConstructIImm(RISCV_LD, rd, ptr, 0);
                    cur_block->push_back(lw_instr);
                } else {
                    auto sp_offset = llvm_rv_allocas[ptr_op->GetRegNo()];
                    auto lw_instr = rvconstructor->ConstructIImm(RISCV_LD, rd, GetPhysicalReg(RISCV_sp), sp_offset);
                    ((RiscV64Function *)cur_func)->AddAllocaIns(lw_instr);
                    cur_block->push_back(lw_instr);
                }
                break;
            }
        }

    } else if (ins->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {
        auto global_op = (GlobalOperand *)ins->GetPointer();
        auto rd_op = (RegOperand *)ins->GetResult();

        Register addr_hi = GetNewReg(INT64);

        switch (ins->GetDataType()) {
            case BasicInstruction::I32: {
                Register rd = GetllvmReg(rd_op->GetRegNo(), INT64);

                auto lui_instr = rvconstructor->ConstructULabel(RISCV_LUI, addr_hi, RiscVLabel(global_op->GetName(), true));
                auto lw_instr =
                    rvconstructor->ConstructILabel(RISCV_LW, rd, addr_hi, RiscVLabel(global_op->GetName(), false));

                cur_block->push_back(lui_instr);
                cur_block->push_back(lw_instr);
                break;
            }
            case BasicInstruction::FLOAT32: {
                Register rd = GetllvmReg(rd_op->GetRegNo(), FLOAT64);

                auto lui_instr = rvconstructor->ConstructULabel(RISCV_LUI, addr_hi, RiscVLabel(global_op->GetName(), true));
                auto lw_instr =
                    rvconstructor->ConstructILabel(RISCV_FLW, rd, addr_hi, RiscVLabel(global_op->GetName(), false));

                cur_block->push_back(lui_instr);
                cur_block->push_back(lw_instr);
                break;
            }
            case BasicInstruction::PTR: {
                Register rd = GetllvmReg(rd_op->GetRegNo(), INT64);

                auto lui_instr = rvconstructor->ConstructULabel(RISCV_LUI, addr_hi, RiscVLabel(global_op->GetName(), true));
                auto lw_instr =
                    rvconstructor->ConstructILabel(RISCV_LD, rd, addr_hi, RiscVLabel(global_op->GetName(), false));

                cur_block->push_back(lui_instr);
                cur_block->push_back(lw_instr);
                break;
            }
        }
    }
    // TODO("Implement this if you need");
}


template <>
void RiscV64Selector::ConvertAndAppend<StoreInstruction *>(StoreInstruction *ins) {
    Register value_reg;
    switch (ins->GetValue()->GetOperandType()) {
        case BasicOperand::IMMI32: {
            auto val_imm = (ImmI32Operand *)ins->GetValue();
            value_reg = GetNewReg(INT64);

            auto imm_copy_ins = rvconstructor->ConstructCopyRegImmI(value_reg, val_imm->GetIntImmVal(), INT64);
            cur_block->push_back(imm_copy_ins);
            break;
        }
        case BasicOperand::REG: {
            auto val_reg = (RegOperand *)ins->GetValue();
            if (ins->GetDataType() == BasicInstruction::I32) {
                value_reg = GetllvmReg(val_reg->GetRegNo(), INT64);
            } else if (ins->GetDataType() == BasicInstruction::FLOAT32) {
                value_reg = GetllvmReg(val_reg->GetRegNo(), FLOAT64);
            }
            break;
        }
        case BasicOperand::IMMF32: {
            auto val_imm = (ImmF32Operand *)ins->GetValue();
            value_reg = GetNewReg(FLOAT64);

            auto imm_copy_ins = rvconstructor->ConstructCopyRegImmF(value_reg, val_imm->GetFloatVal(), FLOAT64);
            cur_block->push_back(imm_copy_ins);
            break;
        }
    }

    switch (ins->GetPointer()->GetOperandType()) {
        case BasicOperand::REG: {
            auto reg_ptr_op = (RegOperand *)ins->GetPointer();

            switch (ins->GetDataType()) {
                case BasicInstruction::I32: {
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
                    break;
                }
                case BasicInstruction::FLOAT32: {
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
                    break;
                }
            }
            break;
        }
        case BasicOperand::GLOBAL: {
            auto global_op = (GlobalOperand *)ins->GetPointer();
            auto addr_hi = GetNewReg(INT64);

            auto lui_instruction =
                rvconstructor->ConstructULabel(RISCV_LUI, addr_hi, RiscVLabel(global_op->GetName(), true));
            cur_block->push_back(lui_instruction);

            switch (ins->GetDataType()) {
                case BasicInstruction::I32: {
                    auto store_instruction =
                        rvconstructor->ConstructSLabel(RISCV_SW, value_reg, addr_hi, RiscVLabel(global_op->GetName(), false));
                    cur_block->push_back(store_instruction);
                    break;
                }
                case BasicInstruction::FLOAT32: {
                    auto store_instruction =
                        rvconstructor->ConstructSLabel(RISCV_FSW, value_reg, addr_hi, RiscVLabel(global_op->GetName(), false));
                    cur_block->push_back(store_instruction);
                    break;
                }
            }
            break;
        }
    }
    // TODO("Implement this if you need");
}

Register RiscV64Selector::ExtractOp2Reg(BasicOperand *op, MachineDataType type) {
    if (op->GetOperandType() == BasicOperand::IMMI32) {
        Register ret = GetNewReg(INT64);
        cur_block->push_back(rvconstructor->ConstructCopyRegImmI(ret, ((ImmI32Operand *)op)->GetIntImmVal(), INT64));
        return ret;
    } else if (op->GetOperandType() == BasicOperand::IMMF32) {
        Register ret = GetNewReg(FLOAT64);
        cur_block->push_back(rvconstructor->ConstructCopyRegImmF(ret, ((ImmF32Operand *)op)->GetFloatVal(), FLOAT64));
        return ret;
    } else if (op->GetOperandType() == BasicOperand::REG) {
        return GetllvmReg(((RegOperand *)op)->GetRegNo(), type);
    }
    return Register();
}

template <>
void RiscV64Selector::ConvertAndAppend<ArithmeticInstruction *>(ArithmeticInstruction *ins) {
    auto opcode = ins->GetOpcode();
    switch (opcode) {
        case BasicInstruction::ADD: {
            if (ins->GetDataType() == BasicInstruction::I32) {
                // 立即数与立即数相加
                if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
                    ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                    auto *immOp1 = static_cast<ImmI32Operand *>(ins->GetOperand1());
                    auto *immOp2 = static_cast<ImmI32Operand *>(ins->GetOperand2());
                    auto *rdOp   = static_cast<RegOperand *>(ins->GetResult());

                    int immVal1 = immOp1->GetIntImmVal();
                    int immVal2 = immOp2->GetIntImmVal();
                    Register rd = GetllvmReg(rdOp->GetRegNo(), INT64);

                    auto copyImmInstr = rvconstructor->ConstructCopyRegImmI(rd, immVal1 + immVal2, INT64);
                    cur_block->push_back(copyImmInstr);
                }
                // 寄存器与立即数相加，可能会生成COPY指令
                if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG &&
                    ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                    auto *rdOp = static_cast<RegOperand *>(ins->GetResult());
                    auto *rsOp = static_cast<RegOperand *>(ins->GetOperand1());
                    auto *immOp = static_cast<ImmI32Operand *>(ins->GetOperand2());

                    Register rd = GetllvmReg(rdOp->GetRegNo(), INT64);
                    Register rs = GetllvmReg(rsOp->GetRegNo(), INT64);
                    int immValue = immOp->GetIntImmVal();

                    if (immValue != 0) {
                        auto addiwInstr = rvconstructor->ConstructIImm(RISCV_ADDIW, rd, rs, immValue);
                        cur_block->push_back(addiwInstr);
                    } else {
                        auto cpInstr = rvconstructor->ConstructCopyReg(rd, rs, INT64);
                        cur_block->push_back(cpInstr);
                    }
                }
                // 寄存器与寄存器相加
                if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG &&
                    ins->GetOperand2()->GetOperandType() == BasicOperand::REG) {

                    auto *rdOp = static_cast<RegOperand *>(ins->GetResult());
                    auto *rsOp = static_cast<RegOperand *>(ins->GetOperand1());
                    auto *rtOp = static_cast<RegOperand *>(ins->GetOperand2());

                    Register rd = GetllvmReg(rdOp->GetRegNo(), INT64);
                    Register rs = GetllvmReg(rsOp->GetRegNo(), INT64);
                    Register rt = GetllvmReg(rtOp->GetRegNo(), INT64);

                    auto addwInstr = rvconstructor->ConstructR(RISCV_ADDW, rd, rs, rt);
                    cur_block->push_back(addwInstr);
                }
                // 立即数加寄存器（顺序不同），也可能生成COPY
                if (ins->GetOperand2()->GetOperandType() == BasicOperand::REG &&
                    ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {

                    auto *rdOp = static_cast<RegOperand *>(ins->GetResult());
                    auto *rsOp = static_cast<RegOperand *>(ins->GetOperand2());
                    auto *immOp = static_cast<ImmI32Operand *>(ins->GetOperand1());

                    Register rd = GetllvmReg(rdOp->GetRegNo(), INT64);
                    Register rs = GetllvmReg(rsOp->GetRegNo(), INT64);
                    int immValue = immOp->GetIntImmVal();

                    if (immValue != 0) {
                        auto addiwInstr = rvconstructor->ConstructIImm(RISCV_ADDIW, rd, rs, immValue);
                        cur_block->push_back(addiwInstr);
                    } else {
                        auto cpInstr = rvconstructor->ConstructCopyReg(rd, rs, INT64);
                        cur_block->push_back(cpInstr);
                    }
                }
            }
            break;
        }
        case BasicInstruction::SUB: {
            // 两个立即数相减
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                auto *rdOp   = static_cast<RegOperand *>(ins->GetResult());
                auto *immOp1 = static_cast<ImmI32Operand *>(ins->GetOperand1());
                auto *immOp2 = static_cast<ImmI32Operand *>(ins->GetOperand2());

                int val1 = immOp1->GetIntImmVal();
                int val2 = immOp2->GetIntImmVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), INT64);

                auto cpImmInstr = rvconstructor->ConstructCopyRegImmI(rd, val1 - val2, INT64);
                cur_block->push_back(cpImmInstr);
            }
            // 寄存器减立即数
            else if (ins->GetOperand1()->GetOperandType() == BasicOperand::REG &&
                     ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) 
            {
                Register rd   = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), INT64);
                Register sub1 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand1())->GetRegNo(), INT64);
                int immVal = static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal();

                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_ADDIW, rd, sub1, -immVal));
            }
            // 寄存器减寄存器
            else {
                Register rd   = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), INT64);
                Register sub1 = ExtractOp2Reg(ins->GetOperand1(), INT64);
                Register sub2 = ExtractOp2Reg(ins->GetOperand2(), INT64);
                auto subInstr = rvconstructor->ConstructR(RISCV_SUBW, rd, sub1, sub2);
                cur_block->push_back(subInstr);
            }
            break;
        }
        case BasicInstruction::DIV: {
            // 两个立即数相除
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                auto *rdOp   = static_cast<RegOperand *>(ins->GetResult());
                auto *immOp1 = static_cast<ImmI32Operand *>(ins->GetOperand1());
                auto *immOp2 = static_cast<ImmI32Operand *>(ins->GetOperand2());

                int val1 = immOp1->GetIntImmVal();
                int val2 = immOp2->GetIntImmVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), INT64);

                auto cpImmInstr = rvconstructor->ConstructCopyRegImmI(rd, val1 / val2, INT64);
                cur_block->push_back(cpImmInstr);
            }
            // 至少有一个操作数是寄存器
            else {
                Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), INT64);
                Register div1, div2;

                if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {
                    div1 = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmI(div1,
                        static_cast<ImmI32Operand *>(ins->GetOperand1())->GetIntImmVal(), INT64));
                } else { // REG
                    div1 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand1())->GetRegNo(), INT64);
                }

                if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                    div2 = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmI(div2,
                        static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal(), INT64));
                } else {
                    div2 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand2())->GetRegNo(), INT64);
                }

                auto divInstr = rvconstructor->ConstructR(RISCV_DIVW, rd, div1, div2);
                cur_block->push_back(divInstr);
            }
            break;
        }
        case BasicInstruction::MUL: {
            // 两个立即数相乘
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                auto *rdOp   = static_cast<RegOperand *>(ins->GetResult());
                auto *immOp1 = static_cast<ImmI32Operand *>(ins->GetOperand1());
                auto *immOp2 = static_cast<ImmI32Operand *>(ins->GetOperand2());

                int val1 = immOp1->GetIntImmVal();
                int val2 = immOp2->GetIntImmVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), INT64);

                auto cpImmInstr = rvconstructor->ConstructCopyRegImmI(rd, val1 * val2, INT64);
                cur_block->push_back(cpImmInstr);
            }
            // 至少一个操作数为立即数（或两个均为寄存器）
            else {
                Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), INT64);
                Register mul1, mul2;

                if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {
                    mul1 = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmI(mul1,
                        static_cast<ImmI32Operand *>(ins->GetOperand1())->GetIntImmVal(), INT64));
                } else {
                    mul1 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand1())->GetRegNo(), INT64);
                }
                if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                    mul2 = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmI(mul2,
                        static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal(), INT64));
                } else {
                    mul2 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand2())->GetRegNo(), INT64);
                }

                auto mulInstr = rvconstructor->ConstructR(RISCV_MULW, rd, mul1, mul2);
                cur_block->push_back(mulInstr);
            }
            break;
        }
        case BasicInstruction::MOD: {
            // 两个立即数取余
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                auto *rdOp   = static_cast<RegOperand *>(ins->GetResult());
                auto *immOp1 = static_cast<ImmI32Operand *>(ins->GetOperand1());
                auto *immOp2 = static_cast<ImmI32Operand *>(ins->GetOperand2());

                int val1 = immOp1->GetIntImmVal();
                int val2 = immOp2->GetIntImmVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), INT64);

                auto cpImmInstr = rvconstructor->ConstructCopyRegImmI(rd, val1 % val2, INT64);
                cur_block->push_back(cpImmInstr);
            }
            // 至少一个操作数是立即数或两个都是寄存器
            else {
                Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), INT64);
                Register rem1, rem2;

                if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {
                    rem1 = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmI(rem1,
                        static_cast<ImmI32Operand *>(ins->GetOperand1())->GetIntImmVal(), INT64));
                } else {
                    rem1 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand1())->GetRegNo(), INT64);
                }

                if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                    rem2 = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmI(rem2,
                        static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal(), INT64));
                } else {
                    rem2 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand2())->GetRegNo(), INT64);
                }

                auto remInstr = rvconstructor->ConstructR(RISCV_REMW, rd, rem1, rem2);
                cur_block->push_back(remInstr);
            }
            break;
        }
        case BasicInstruction::FADD: {
            // 单精度浮点立即数相加
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {

                auto *rdOp   = static_cast<RegOperand *>(ins->GetResult());
                auto *immOp1 = static_cast<ImmF32Operand *>(ins->GetOperand1());
                auto *immOp2 = static_cast<ImmF32Operand *>(ins->GetOperand2());

                float fval1 = immOp1->GetFloatVal();
                float fval2 = immOp2->GetFloatVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), FLOAT64);

                auto cpImmFInstr = rvconstructor->ConstructCopyRegImmF(rd, fval1 + fval2, FLOAT64);
                cur_block->push_back(cpImmFInstr);
            }
            else {
                Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), FLOAT64);
                Register fop1 = ExtractOp2Reg(ins->GetOperand1(), FLOAT64);
                Register fop2 = ExtractOp2Reg(ins->GetOperand2(), FLOAT64);
                auto faddInstr = rvconstructor->ConstructR(RISCV_FADD_S, rd, fop1, fop2);
                cur_block->push_back(faddInstr);
            }
            break;
        }
        case BasicInstruction::FSUB: {
            // 浮点立即数相减
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {

                auto *rdOp   = static_cast<RegOperand *>(ins->GetResult());
                auto *immOp1 = static_cast<ImmF32Operand *>(ins->GetOperand1());
                auto *immOp2 = static_cast<ImmF32Operand *>(ins->GetOperand2());

                float fval1 = immOp1->GetFloatVal();
                float fval2 = immOp2->GetFloatVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), FLOAT64);

                auto cpImmFInstr = rvconstructor->ConstructCopyRegImmF(rd, fval1 - fval2, FLOAT64);
                cur_block->push_back(cpImmFInstr);
            }
            else {
                Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), FLOAT64);
                // 如果某操作数为0，则直接使用对应指令
                if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32) {
                    float opVal = static_cast<ImmF32Operand *>(ins->GetOperand1())->GetFloatVal();
                    if (opVal == 0) {
                        cur_block->push_back(rvconstructor->ConstructR2(RISCV_FNEG_S, rd, ExtractOp2Reg(ins->GetOperand2(), FLOAT64)));
                        break;
                    }
                }
                else if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
                    float opVal = static_cast<ImmF32Operand *>(ins->GetOperand2())->GetFloatVal();
                    if (opVal == 0) {
                        cur_block->push_back(rvconstructor->ConstructCopyReg(rd, ExtractOp2Reg(ins->GetOperand1(), FLOAT64), FLOAT64));
                        break;
                    }
                }

                Register fsub1, fsub2;
                if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32) {
                    fsub1 = GetNewReg(FLOAT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmF(fsub1,
                        static_cast<ImmF32Operand *>(ins->GetOperand1())->GetFloatVal(), FLOAT64));
                } else {
                    fsub1 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand1())->GetRegNo(), FLOAT64);
                }

                if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
                    fsub2 = GetNewReg(FLOAT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmF(fsub2,
                        static_cast<ImmF32Operand *>(ins->GetOperand2())->GetFloatVal(), FLOAT64));
                } else {
                    fsub2 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand2())->GetRegNo(), FLOAT64);
                }
                auto fsubInstr = rvconstructor->ConstructR(RISCV_FSUB_S, rd, fsub1, fsub2);
                cur_block->push_back(fsubInstr);
            }
            break;
        }
        case BasicInstruction::FMUL: {
            // 浮点立即数相乘
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {

                auto *rdOp   = static_cast<RegOperand *>(ins->GetResult());
                auto *immOp1 = static_cast<ImmF32Operand *>(ins->GetOperand1());
                auto *immOp2 = static_cast<ImmF32Operand *>(ins->GetOperand2());

                float fval1 = immOp1->GetFloatVal();
                float fval2 = immOp2->GetFloatVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), FLOAT64);

                auto cpImmFInstr = rvconstructor->ConstructCopyRegImmF(rd, fval1 * fval2, FLOAT64);
                cur_block->push_back(cpImmFInstr);
            }
            else {
                Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), FLOAT64);
                Register fmul1, fmul2;

                if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32) {
                    fmul1 = GetNewReg(FLOAT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmF(fmul1,
                        static_cast<ImmF32Operand *>(ins->GetOperand1())->GetFloatVal(), FLOAT64));
                } else {
                    fmul1 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand1())->GetRegNo(), FLOAT64);
                }
                if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
                    fmul2 = GetNewReg(FLOAT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmF(fmul2,
                        static_cast<ImmF32Operand *>(ins->GetOperand2())->GetFloatVal(), FLOAT64));
                } else {
                    fmul2 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand2())->GetRegNo(), FLOAT64);
                }
                auto fmulInstr = rvconstructor->ConstructR(RISCV_FMUL_S, rd, fmul1, fmul2);
                cur_block->push_back(fmulInstr);
            }
            break;
        }
        case BasicInstruction::FDIV: {
            // 浮点立即数相除
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {

                auto *rdOp   = static_cast<RegOperand *>(ins->GetResult());
                auto *immOp1 = static_cast<ImmF32Operand *>(ins->GetOperand1());
                auto *immOp2 = static_cast<ImmF32Operand *>(ins->GetOperand2());

                float fval1 = immOp1->GetFloatVal();
                float fval2 = immOp2->GetFloatVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), FLOAT64);

                auto cpImmFInstr = rvconstructor->ConstructCopyRegImmF(rd, fval1 / fval2, FLOAT64);
                cur_block->push_back(cpImmFInstr);
            }
            else {
                Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), FLOAT64);
                Register fdiv1, fdiv2;

                if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32) {
                    fdiv1 = GetNewReg(FLOAT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmF(fdiv1,
                        static_cast<ImmF32Operand *>(ins->GetOperand1())->GetFloatVal(), FLOAT64));
                } else {
                    fdiv1 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand1())->GetRegNo(), FLOAT64);
                }
                if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {
                    fdiv2 = GetNewReg(FLOAT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmF(fdiv2,
                        static_cast<ImmF32Operand *>(ins->GetOperand2())->GetFloatVal(), FLOAT64));
                } else {
                    fdiv2 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand2())->GetRegNo(), FLOAT64);
                }
                auto fdivInstr = rvconstructor->ConstructR(RISCV_FDIV_S, rd, fdiv1, fdiv2);
                cur_block->push_back(fdivInstr);
            }
            break;
        }
        case BasicInstruction::LL_ADDMOD: {
            // Low-level ADDMOD指令
            auto *op1       = ins->GetOperand1();
            auto *op2       = ins->GetOperand2();
            auto *op3       = ins->GetOperand3();
            auto *resultOp  = ins->GetResult();
            Register resReg = GetllvmReg(static_cast<RegOperand *>(resultOp)->GetRegNo(), INT64);

            if (op1->GetOperandType() == BasicOperand::IMMI32 &&
                op2->GetOperandType() == BasicOperand::IMMI32 &&
                op3->GetOperandType() == BasicOperand::IMMI32) {

                unsigned int val1 = static_cast<ImmI32Operand *>(op1)->GetIntImmVal();
                unsigned int val2 = static_cast<ImmI32Operand *>(op2)->GetIntImmVal();
                unsigned int div  = static_cast<ImmI32Operand *>(op3)->GetIntImmVal();

                cur_block->push_back(rvconstructor->ConstructCopyRegImmI(resReg, (1ll * val1 * val2) % div, INT64));
            } else {
                // 计算 (r1 * r2) % div
                unsigned int divValue = static_cast<ImmI32Operand *>(op3)->GetIntImmVal();
                Register r1 = ExtractOp2Reg(op1, INT64);
                Register r2 = ExtractOp2Reg(op2, INT64);
                Register rProd = GetNewReg(INT64);

                cur_block->push_back(rvconstructor->ConstructR(RISCV_MUL, rProd, r1, r2));

                Register rDivisor = GetNewReg(INT64);
                cur_block->push_back(rvconstructor->ConstructCopyRegImmI(rDivisor, divValue, INT64));

                Register quotient = GetNewReg(INT64);
                cur_block->push_back(rvconstructor->ConstructR(RISCV_DIVU, quotient, rProd, rDivisor));

                Register prodDiv = GetNewReg(INT64);
                cur_block->push_back(rvconstructor->ConstructR(RISCV_MUL, prodDiv, quotient, rDivisor));

                cur_block->push_back(rvconstructor->ConstructR(RISCV_SUB, resReg, rProd, prodDiv));
            }
            break;
        }
        case BasicInstruction::SMIN_I32: {
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                auto *rdOp = static_cast<RegOperand *>(ins->GetResult());
                int imm1 = static_cast<ImmI32Operand *>(ins->GetOperand1())->GetIntImmVal();
                int imm2 = static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), INT64);

                cur_block->push_back(rvconstructor->ConstructCopyRegImmI(rd, (imm1 < imm2 ? imm1 : imm2), INT64));
            } else {
                Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), INT64);
                Register opVal1, opVal2;

                if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {
                    opVal1 = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmI(opVal1,
                        static_cast<ImmI32Operand *>(ins->GetOperand1())->GetIntImmVal(), INT64));
                } else {
                    opVal1 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand1())->GetRegNo(), INT64);
                }
                if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                    opVal2 = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmI(opVal2,
                        static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal(), INT64));
                } else {
                    opVal2 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand2())->GetRegNo(), INT64);
                }
                auto minInstr = rvconstructor->ConstructR(RISCV_MIN, rd, opVal1, opVal2);
                cur_block->push_back(minInstr);
            }
            break;
        }
        case BasicInstruction::SMAX_I32: {
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                auto *rdOp = static_cast<RegOperand *>(ins->GetResult());
                int imm1 = static_cast<ImmI32Operand *>(ins->GetOperand1())->GetIntImmVal();
                int imm2 = static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), INT64);

                cur_block->push_back(rvconstructor->ConstructCopyRegImmI(rd, (imm1 > imm2 ? imm1 : imm2), INT64));
            } else {
                Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), INT64);
                Register opVal1, opVal2;
                if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32) {
                    opVal1 = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmI(opVal1,
                        static_cast<ImmI32Operand *>(ins->GetOperand1())->GetIntImmVal(), INT64));
                } else {
                    opVal1 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand1())->GetRegNo(), INT64);
                }
                if (ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {
                    opVal2 = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructCopyRegImmI(opVal2,
                        static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal(), INT64));
                } else {
                    opVal2 = GetllvmReg(static_cast<RegOperand *>(ins->GetOperand2())->GetRegNo(), INT64);
                }
                auto maxInstr = rvconstructor->ConstructR(RISCV_MAX, rd, opVal1, opVal2);
                cur_block->push_back(maxInstr);
            }
            break;
        }
        case BasicInstruction::FMIN_F32: {
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {

                auto *rdOp = static_cast<RegOperand *>(ins->GetResult());
                float f1 = static_cast<ImmF32Operand *>(ins->GetOperand1())->GetFloatVal();
                float f2 = static_cast<ImmF32Operand *>(ins->GetOperand2())->GetFloatVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), FLOAT64);

                cur_block->push_back(rvconstructor->ConstructCopyRegImmF(rd, (f1 < f2 ? f1 : f2), FLOAT64));
            } else {
                Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), FLOAT64);
                Register opF1 = ExtractOp2Reg(ins->GetOperand1(), FLOAT64);
                Register opF2 = ExtractOp2Reg(ins->GetOperand2(), FLOAT64);
                auto fminInstr = rvconstructor->ConstructR(RISCV_FMIN_S, rd, opF1, opF2);
                cur_block->push_back(fminInstr);
            }
            break;
        }
        case BasicInstruction::FMAX_F32: {
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMF32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMF32) {

                auto *rdOp = static_cast<RegOperand *>(ins->GetResult());
                float f1 = static_cast<ImmF32Operand *>(ins->GetOperand1())->GetFloatVal();
                float f2 = static_cast<ImmF32Operand *>(ins->GetOperand2())->GetFloatVal();
                Register rd = GetllvmReg(rdOp->GetRegNo(), FLOAT64);

                cur_block->push_back(rvconstructor->ConstructCopyRegImmF(rd, (f1 > f2 ? f1 : f2), FLOAT64));
            } else {
                Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), FLOAT64);
                Register opF1 = ExtractOp2Reg(ins->GetOperand1(), FLOAT64);
                Register opF2 = ExtractOp2Reg(ins->GetOperand2(), FLOAT64);
                auto fmaxInstr = rvconstructor->ConstructR(RISCV_FMAX_S, rd, opF1, opF2);
                cur_block->push_back(fmaxInstr);
            }
            break;
        }
        case BasicInstruction::BITAND: {
            Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), INT64);
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                int val1 = static_cast<ImmI32Operand *>(ins->GetOperand1())->GetIntImmVal();
                int val2 = static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal();
                cur_block->push_back(rvconstructor->ConstructCopyRegImmI(rd, val1 & val2, INT64));
            } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 ||
                       ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                int immVal = (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32)
                               ? static_cast<ImmI32Operand *>(ins->GetOperand1())->GetIntImmVal()
                               : static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal();
                Register rs = (ins->GetOperand1()->GetOperandType() == BasicOperand::REG)
                                ? ExtractOp2Reg(ins->GetOperand1(), INT64)
                                : ExtractOp2Reg(ins->GetOperand2(), INT64);
                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_ANDI, rd, rs, immVal));
            } else {
                cur_block->push_back(rvconstructor->ConstructR(RISCV_AND, rd,
                    ExtractOp2Reg(ins->GetOperand1(), INT64),
                    ExtractOp2Reg(ins->GetOperand2(), INT64)));
            }
            break;
        }
        case BasicInstruction::BITXOR: {
            Register rd = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), INT64);
            if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 &&
                ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                int val1 = static_cast<ImmI32Operand *>(ins->GetOperand1())->GetIntImmVal();
                int val2 = static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal();
                cur_block->push_back(rvconstructor->ConstructCopyRegImmI(rd, val1 ^ val2, INT64));
            } else if (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32 ||
                       ins->GetOperand2()->GetOperandType() == BasicOperand::IMMI32) {

                int immVal = (ins->GetOperand1()->GetOperandType() == BasicOperand::IMMI32)
                               ? static_cast<ImmI32Operand *>(ins->GetOperand1())->GetIntImmVal()
                               : static_cast<ImmI32Operand *>(ins->GetOperand2())->GetIntImmVal();
                Register rs = (ins->GetOperand1()->GetOperandType() == BasicOperand::REG)
                                ? ExtractOp2Reg(ins->GetOperand1(), INT64)
                                : ExtractOp2Reg(ins->GetOperand2(), INT64);
                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_XORI, rd, rs, immVal));
            } else {
                cur_block->push_back(rvconstructor->ConstructR(RISCV_XOR, rd,
                    ExtractOp2Reg(ins->GetOperand1(), INT64),
                    ExtractOp2Reg(ins->GetOperand2(), INT64)));
            }
            break;
        }
        default:
            break;
    }
}



template <> void RiscV64Selector::ConvertAndAppend<IcmpInstruction *>(IcmpInstruction *ins) {
    auto res_op = (RegOperand *)ins->GetResult();
    auto res_reg = GetllvmReg(res_op->GetRegNo(), INT64);
    cmp_context[res_reg] = ins;
    //TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<FcmpInstruction *>(FcmpInstruction *ins) {
    auto res_op = (RegOperand *)ins->GetResult();
    auto res_reg = GetllvmReg(res_op->GetRegNo(), INT64);
    cmp_context[res_reg] = ins;
    //TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<AllocaInstruction *>(AllocaInstruction *ins) {
    auto reg_op = (RegOperand *)ins->GetResult();
    int byte_size = ins->GetAllocaSize() * 4;
    llvm_rv_allocas[reg_op->GetRegNo()] = cur_offset;
    cur_offset += byte_size;
    //TODO("Implement this if you need");
}

template <>
void RiscV64Selector::ConvertAndAppend<BrCondInstruction *>(BrCondInstruction *ins) {
    auto cond_reg = (RegOperand *)ins->GetCond();
    auto br_reg = GetllvmReg(cond_reg->GetRegNo(), INT64);
    auto cmp_ins = cmp_context[br_reg];
    int opcode;
    Register cmp_rd, cmp_op1, cmp_op2;

    switch (cmp_ins->GetOpcode()) {
        case BasicInstruction::ICMP: {
            auto icmp_ins = (IcmpInstruction *)cmp_ins;

            switch (icmp_ins->GetOp1()->GetOperandType()) {
                case BasicOperand::REG:
                    cmp_op1 = GetllvmReg(((RegOperand *)icmp_ins->GetOp1())->GetRegNo(), INT64);
                    break;
                case BasicOperand::IMMI32: {
                    int imm_val = ((ImmI32Operand *)icmp_ins->GetOp1())->GetIntImmVal();
                    if (imm_val != 0) {
                        cmp_op1 = GetNewReg(INT64);
                        auto copy_imm_instr = rvconstructor->ConstructCopyRegImmI(cmp_op1, imm_val, INT64);
                        cur_block->push_back(copy_imm_instr);
                    } else {
                        cmp_op1 = GetPhysicalReg(RISCV_x0);
                    }
                    break;
                }
                default:
                    break;
            }

            switch (icmp_ins->GetOp2()->GetOperandType()) {
                case BasicOperand::REG:
                    cmp_op2 = GetllvmReg(((RegOperand *)icmp_ins->GetOp2())->GetRegNo(), INT64);
                    break;
                case BasicOperand::IMMI32: {
                    int imm_val = ((ImmI32Operand *)icmp_ins->GetOp2())->GetIntImmVal();
                    if (imm_val != 0) {
                        cmp_op2 = GetNewReg(INT64);
                        auto copy_imm_instr = rvconstructor->ConstructCopyRegImmI(cmp_op2, imm_val, INT64);
                        cur_block->push_back(copy_imm_instr);
                    } else {
                        cmp_op2 = GetPhysicalReg(RISCV_x0);
                    }
                    break;
                }
                default:
                    break;
            }

            switch (icmp_ins->GetCond()) {
                case BasicInstruction::eq:
                    opcode = RISCV_BEQ;
                    break;
                case BasicInstruction::ne:
                    opcode = RISCV_BNE;
                    break;
                case BasicInstruction::ugt:
                    opcode = RISCV_BGTU;
                    break;
                case BasicInstruction::uge:
                    opcode = RISCV_BGEU;
                    break;
                case BasicInstruction::ult:
                    opcode = RISCV_BLTU;
                    break;
                case BasicInstruction::ule:
                    opcode = RISCV_BLEU;
                    break;
                case BasicInstruction::sgt:
                    opcode = RISCV_BGT;
                    break;
                case BasicInstruction::sge:
                    opcode = RISCV_BGE;
                    break;
                case BasicInstruction::slt:
                    opcode = RISCV_BLT;
                    break;
                case BasicInstruction::sle:
                    opcode = RISCV_BLE;
                    break;
                default:
                    break;
            }
            break;
        }
        case BasicInstruction::FCMP: {
            auto fcmp_ins = (FcmpInstruction *)cmp_ins;

            switch (fcmp_ins->GetOp1()->GetOperandType()) {
                case BasicOperand::REG:
                    cmp_op1 = GetllvmReg(((RegOperand *)fcmp_ins->GetOp1())->GetRegNo(), FLOAT64);
                    break;
                case BasicOperand::IMMF32: {
                    cmp_op1 = GetNewReg(FLOAT64);
                    auto copy_imm_instr =
                        rvconstructor->ConstructCopyRegImmF(cmp_op1, ((ImmF32Operand *)fcmp_ins->GetOp1())->GetFloatVal(), FLOAT64);
                    cur_block->push_back(copy_imm_instr);
                    break;
                }
                default:
                    break;
            }

            switch (fcmp_ins->GetOp2()->GetOperandType()) {
                case BasicOperand::REG:
                    cmp_op2 = GetllvmReg(((RegOperand *)fcmp_ins->GetOp2())->GetRegNo(), FLOAT64);
                    break;
                case BasicOperand::IMMF32: {
                    cmp_op2 = GetNewReg(FLOAT64);
                    auto copy_imm_instr =
                        rvconstructor->ConstructCopyRegImmF(cmp_op2, ((ImmF32Operand *)fcmp_ins->GetOp2())->GetFloatVal(), FLOAT64);
                    cur_block->push_back(copy_imm_instr);
                    break;
                }
                default:
                    break;
            }

            cmp_rd = GetNewReg(INT64);

            switch (fcmp_ins->GetCond()) {
                case BasicInstruction::OEQ:
                case BasicInstruction::UEQ:
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
                default:
                    break;
            }

            cmp_op1 = cmp_rd;
            cmp_op2 = GetPhysicalReg(RISCV_x0);
            break;
        }
        default:
            break;
    }

    auto true_label = (LabelOperand *)ins->GetTrueLabel();
    auto false_label = (LabelOperand *)ins->GetFalseLabel();

    auto br_ins = rvconstructor->ConstructBLabel(opcode, cmp_op1, cmp_op2, RiscVLabel(true_label->GetLabelNo()));
    cur_block->push_back(br_ins);

    auto br_else_ins = rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), RiscVLabel(false_label->GetLabelNo()));
    cur_block->push_back(br_else_ins);

    // TODO: Implement additional logic if needed
}


template <> void RiscV64Selector::ConvertAndAppend<BrUncondInstruction *>(BrUncondInstruction *ins) {
    auto dest_label = RiscVLabel(((LabelOperand *)ins->GetDestLabel())->GetLabelNo());

    auto jal_instr = rvconstructor->ConstructJLabel(RISCV_JAL, GetPhysicalReg(RISCV_x0), dest_label);

    cur_block->push_back(jal_instr);
    //TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<CallInstruction *>(CallInstruction *ins) {

    int ireg_cnt = 0;
    int freg_cnt = 0;
    int stkpara_cnt = 0;

    if (ins->GetFunctionName() == std::string("llvm.memset.p0.i32")) {

        // parameter 0
        {
            int ptrreg_no = ((RegOperand *)ins->GetParameterList()[0].second)->GetRegNo();
            if (llvm_rv_allocas.find(ptrreg_no) == llvm_rv_allocas.end()) {
                cur_block->push_back(
                rvconstructor->ConstructCopyReg(GetPhysicalReg(RISCV_a0), GetllvmReg(ptrreg_no, INT64), INT64));
            } else {
                auto sp_offset = llvm_rv_allocas[ptrreg_no];
                auto ld_alloca =
                rvconstructor->ConstructIImm(RISCV_ADDI, GetPhysicalReg(RISCV_a0), GetPhysicalReg(RISCV_sp), sp_offset);
                ((RiscV64Function *)cur_func)->AddAllocaIns(ld_alloca);
                cur_block->push_back(ld_alloca);
            }
        }

        // parameters 1
        {
            auto imm_op = (ImmI32Operand *)(ins->GetParameterList()[1].second);
            cur_block->push_back(
            rvconstructor->ConstructCopyRegImmI(GetPhysicalReg(RISCV_a1), imm_op->GetIntImmVal(), INT64));
        }

        // paramters 2
        {
            if (ins->GetParameterList()[2].second->GetOperandType() == BasicOperand::IMMI32) {
                int arr_sz = ((ImmI32Operand *)ins->GetParameterList()[2].second)->GetIntImmVal();
                cur_block->push_back(rvconstructor->ConstructCopyRegImmI(GetPhysicalReg(RISCV_a2), arr_sz, INT64));
            } else {
                int sizereg_no = ((RegOperand *)ins->GetParameterList()[2].second)->GetRegNo();
                if (llvm_rv_allocas.find(sizereg_no) == llvm_rv_allocas.end()) {
                    cur_block->push_back(
                    rvconstructor->ConstructCopyReg(GetPhysicalReg(RISCV_a2), GetllvmReg(sizereg_no, INT64), INT64));
                } else {
                    auto sp_offset = llvm_rv_allocas[sizereg_no];
                    auto ld_alloca = rvconstructor->ConstructIImm(RISCV_ADDI, GetPhysicalReg(RISCV_a2),
                                                                  GetPhysicalReg(RISCV_sp), sp_offset);
                    ((RiscV64Function *)cur_func)->AddAllocaIns(ld_alloca);
                    cur_block->push_back(ld_alloca);
                }
            }
        }

        // call
        cur_block->push_back(rvconstructor->ConstructCall(RISCV_CALL, "memset", 3, 0));
        return;
    }

    // Parameters
    for (auto [type, arg_op] : ins->GetParameterList()) {
        if (type == BasicInstruction::I32 || type == BasicInstruction::PTR) {
            if (ireg_cnt < 8) {
                if (arg_op->GetOperandType() == BasicOperand::REG) {
                    auto arg_regop = (RegOperand *)arg_op;
                    auto arg_reg = GetllvmReg(arg_regop->GetRegNo(), INT64);
                    if (llvm_rv_allocas.find(arg_regop->GetRegNo()) == llvm_rv_allocas.end()) {
                        auto arg_copy_instr =
                        rvconstructor->ConstructCopyReg(GetPhysicalReg(RISCV_a0 + ireg_cnt), arg_reg, INT64);
                        cur_block->push_back(arg_copy_instr);
                    } else {
                        auto sp_offset = llvm_rv_allocas[arg_regop->GetRegNo()];
                        cur_block->push_back(rvconstructor->ConstructIImm(
                        RISCV_ADDI, GetPhysicalReg(RISCV_a0 + ireg_cnt), GetPhysicalReg(RISCV_sp), sp_offset));
                    }
                } else if (arg_op->GetOperandType() == BasicOperand::IMMI32) {
                    auto arg_immop = (ImmI32Operand *)arg_op;
                    auto arg_imm = arg_immop->GetIntImmVal();
                    auto arg_copy_instr =
                    rvconstructor->ConstructCopyRegImmI(GetPhysicalReg(RISCV_a0 + ireg_cnt), arg_imm, INT64);
                    cur_block->push_back(arg_copy_instr);
                } else if (arg_op->GetOperandType() == BasicOperand::GLOBAL) {
                    auto mid_reg = GetNewReg(INT64);
                    auto arg_global = (GlobalOperand *)arg_op;
                    cur_block->push_back(
                    rvconstructor->ConstructULabel(RISCV_LUI, mid_reg, RiscVLabel(arg_global->GetName(), true)));
                    cur_block->push_back(rvconstructor->ConstructILabel(RISCV_ADDI, GetPhysicalReg(RISCV_a0 + ireg_cnt),
                                                                        mid_reg,
                                                                        RiscVLabel(arg_global->GetName(), false)));
                }
            }
            ireg_cnt++;
        } else if (type == BasicInstruction::FLOAT32) {
            if (freg_cnt < 8) {
                if (arg_op->GetOperandType() == BasicOperand::REG) {
                    auto arg_regop = (RegOperand *)arg_op;
                    auto arg_reg = GetllvmReg(arg_regop->GetRegNo(), FLOAT64);
                    auto arg_copy_instr =
                    rvconstructor->ConstructCopyReg(GetPhysicalReg(RISCV_fa0 + freg_cnt), arg_reg, FLOAT64);
                    cur_block->push_back(arg_copy_instr);
                } else if (arg_op->GetOperandType() == BasicOperand::IMMF32) {
                    auto arg_immop = (ImmF32Operand *)arg_op;
                    auto arg_imm = arg_immop->GetFloatVal();
                    auto arg_copy_instr =
                    rvconstructor->ConstructCopyRegImmF(GetPhysicalReg(RISCV_fa0 + freg_cnt), arg_imm, FLOAT64);
                    cur_block->push_back(arg_copy_instr);
                }
            }
            freg_cnt++;
        } else if (type == BasicInstruction::DOUBLE) {
            if (ireg_cnt < 8) {
                if (arg_op->GetOperandType() == BasicOperand::REG) {
                    auto arg_regop = (RegOperand *)arg_op;
                    auto arg_reg = GetllvmReg(arg_regop->GetRegNo(), FLOAT64);
                    cur_block->push_back(
                    rvconstructor->ConstructR2(RISCV_FMV_X_D, GetPhysicalReg(RISCV_a0 + ireg_cnt), arg_reg));
                }
            }
            ireg_cnt++;
        }
    }

    if (ireg_cnt - 8 > 0)
        stkpara_cnt += (ireg_cnt - 8);
    if (freg_cnt - 8 > 0)
        stkpara_cnt += (freg_cnt - 8);

    if (stkpara_cnt != 0) {
        ireg_cnt = freg_cnt = 0;
        int arg_off = 0;
        for (auto [type, arg_op] : ins->GetParameterList()) {
            if (type == BasicInstruction::I32 || type == BasicInstruction::PTR) {
                if (ireg_cnt < 8) {
                } else {
                    if (arg_op->GetOperandType() == BasicOperand::REG) {
                        auto arg_regop = (RegOperand *)arg_op;
                        auto arg_reg = GetllvmReg(arg_regop->GetRegNo(), INT64);
                        if (llvm_rv_allocas.find(arg_regop->GetRegNo()) == llvm_rv_allocas.end()) {
                            cur_block->push_back(
                            rvconstructor->ConstructSImm(RISCV_SD, arg_reg, GetPhysicalReg(RISCV_sp), arg_off));
                        } else {
                            auto sp_offset = llvm_rv_allocas[arg_regop->GetRegNo()];
                            auto mid_reg = GetNewReg(INT64);
                            cur_block->push_back(
                            rvconstructor->ConstructIImm(RISCV_ADDI, mid_reg, GetPhysicalReg(RISCV_sp), sp_offset));
                            cur_block->push_back(
                            rvconstructor->ConstructSImm(RISCV_SD, mid_reg, GetPhysicalReg(RISCV_sp), arg_off));
                        }
                    } else if (arg_op->GetOperandType() == BasicOperand::IMMI32) {
                        auto arg_immop = (ImmI32Operand *)arg_op;
                        auto arg_imm = arg_immop->GetIntImmVal();
                        auto imm_reg = GetNewReg(INT64);
                        cur_block->push_back(rvconstructor->ConstructCopyRegImmI(imm_reg, arg_imm, INT64));
                        cur_block->push_back(
                        rvconstructor->ConstructSImm(RISCV_SD, imm_reg, GetPhysicalReg(RISCV_sp), arg_off));
                    } else if (arg_op->GetOperandType() == BasicOperand::GLOBAL) {
                        auto glb_reg1 = GetNewReg(INT64);
                        auto glb_reg2 = GetNewReg(INT64);
                        auto arg_glbop = (GlobalOperand *)arg_op;
                        cur_block->push_back(
                        rvconstructor->ConstructULabel(RISCV_LUI, glb_reg1, RiscVLabel(arg_glbop->GetName(), true)));
                        cur_block->push_back(rvconstructor->ConstructILabel(RISCV_ADDI, glb_reg2, glb_reg1,
                                                                            RiscVLabel(arg_glbop->GetName(), false)));
                        cur_block->push_back(
                        rvconstructor->ConstructSImm(RISCV_SD, glb_reg2, GetPhysicalReg(RISCV_sp), arg_off));
                    }
                    arg_off += 8;
                }
                ireg_cnt++;
            } else if (type == BasicInstruction::FLOAT32) {
                if (freg_cnt < 8) {
                } else {
                    if (arg_op->GetOperandType() == BasicOperand::REG) {
                        auto arg_regop = (RegOperand *)arg_op;
                        auto arg_reg = GetllvmReg(arg_regop->GetRegNo(), FLOAT64);
                        cur_block->push_back(
                        rvconstructor->ConstructSImm(RISCV_FSD, arg_reg, GetPhysicalReg(RISCV_sp), arg_off));
                    } else if (arg_op->GetOperandType() == BasicOperand::IMMF32) {
                        auto arg_immop = (ImmF32Operand *)arg_op;
                        auto arg_imm = arg_immop->GetFloatVal();
                        auto imm_reg = GetNewReg(INT64);
                        cur_block->push_back(rvconstructor->ConstructCopyRegImmI(imm_reg, *(int *)&arg_imm, INT64));
                        cur_block->push_back(
                        rvconstructor->ConstructSImm(RISCV_SD, imm_reg, GetPhysicalReg(RISCV_sp), arg_off));
                    }
                    arg_off += 8;
                }
                freg_cnt++;
            } else if (type == BasicInstruction::DOUBLE) {
                if (ireg_cnt < 8) {
                } else {
                    if (arg_op->GetOperandType() == BasicOperand::REG) {
                        auto arg_regop = (RegOperand *)arg_op;
                        auto arg_reg = GetllvmReg(arg_regop->GetRegNo(), FLOAT64);
                        cur_block->push_back(
                        rvconstructor->ConstructSImm(RISCV_FSD, arg_reg, GetPhysicalReg(RISCV_sp), arg_off));
                    }
                    arg_off += 8;
                }
                ireg_cnt++;
            }
        }
    }

    // Call Label
    auto call_funcname = ins->GetFunctionName();
    if (ireg_cnt > 8) {
        ireg_cnt = 8;
    }
    if (freg_cnt > 8) {
        freg_cnt = 8;
    }
    // Return Value
    auto return_type = ins->GetRetType();
    auto result_op = (RegOperand *)ins->GetResult();
    cur_block->push_back(rvconstructor->ConstructCall(RISCV_CALL, call_funcname, ireg_cnt, freg_cnt));
    cur_func->UpdateParaSize(stkpara_cnt * 8);
    // if(stkpara_cnt != 0){
    //     cur_block->push_back(rvconstructor->ConstructIImm(RISCV_ADDI,GetPhysicalReg(RISCV_sp),GetPhysicalReg(RISCV_sp),sub_sz));
    // }
    if (return_type == BasicInstruction::I32) {
        auto copy_ret_ins =
        rvconstructor->ConstructCopyReg(GetllvmReg(result_op->GetRegNo(), INT64), GetPhysicalReg(RISCV_a0), INT64);
        cur_block->push_back(copy_ret_ins);
    } else if (return_type == BasicInstruction::FLOAT32) {
        // Lazy("Not tested");
        auto copy_ret_ins =
        rvconstructor->ConstructCopyReg(GetllvmReg(result_op->GetRegNo(), FLOAT64), GetPhysicalReg(RISCV_fa0), FLOAT64);
        cur_block->push_back(copy_ret_ins);
    } else if (return_type == BasicInstruction::VOID) {
    }
    //TODO("Implement this if you need");
}

template <>
void RiscV64Selector::ConvertAndAppend<RetInstruction *>(RetInstruction *ins) {
    if (ins->GetRetVal() != nullptr) {
        auto retval = ins->GetRetVal();
        switch (retval->GetOperandType()) {
            case BasicOperand::IMMI32: {
                // 处理立即数整型返回值
                auto *retimm_op = static_cast<ImmI32Operand *>(retval);
                int imm = retimm_op->GetIntImmVal();

                auto retcopy_instr = rvconstructor->ConstructUImm(RISCV_LI, GetPhysicalReg(RISCV_a0), imm);
                cur_block->push_back(retcopy_instr);
                break;
            }
            case BasicOperand::IMMF32: {
                // 处理立即数浮点返回值
                auto *retimm_op = static_cast<ImmF32Operand *>(retval);
                float imm = retimm_op->GetFloatVal();

                auto retcopy_instr = rvconstructor->ConstructCopyRegImmF(GetPhysicalReg(RISCV_fa0), imm, FLOAT64);
                cur_block->push_back(retcopy_instr);
                break;
            }
            case BasicOperand::REG: {
                // 处理寄存器返回值
                auto *retreg_val = static_cast<RegOperand *>(retval);
                auto reg_type = ins->GetType();

                switch (reg_type) {
                    case BasicInstruction::FLOAT32: {
                        // FLOAT32 类型返回值
                        auto reg = GetllvmReg(retreg_val->GetRegNo(), FLOAT64);
                        auto retcopy_instr = rvconstructor->ConstructCopyReg(GetPhysicalReg(RISCV_fa0), reg, FLOAT64);
                        cur_block->push_back(retcopy_instr);
                        break;
                    }
                    case BasicInstruction::I32: {
                        // I32 类型返回值
                        auto reg = GetllvmReg(retreg_val->GetRegNo(), INT64);
                        auto retcopy_instr = rvconstructor->ConstructCopyReg(GetPhysicalReg(RISCV_a0), reg, INT64);
                        cur_block->push_back(retcopy_instr);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
    }

    // 构造返回指令
    auto ret_instr = rvconstructor->ConstructIImm(RISCV_JALR, GetPhysicalReg(RISCV_x0), GetPhysicalReg(RISCV_ra), 0);

    switch (ins->GetType()) {
        case BasicInstruction::I32:
            ret_instr->setRetType(1);
            break;
        case BasicInstruction::FLOAT32:
            ret_instr->setRetType(2);
            break;
        default:
            ret_instr->setRetType(0);
            break;
    }

    cur_block->push_back(ret_instr);
}


template <>
void RiscV64Selector::ConvertAndAppend<FptosiInstruction *>(FptosiInstruction *ins) {
    auto src_op = ins->GetSrc();
    auto dst_op = static_cast<RegOperand *>(ins->GetResultReg());

    switch (src_op->GetOperandType()) {
        case BasicOperand::REG: {
            // 处理源操作数为寄存器的情况
            auto reg_src = static_cast<RegOperand *>(src_op);

            // 使用 FCVT.W.S 指令将浮点数转换为整型
            auto fmv_instr = rvconstructor->ConstructR2(
                RISCV_FCVT_W_S, 
                GetllvmReg(dst_op->GetRegNo(), INT64),
                GetllvmReg(reg_src->GetRegNo(), FLOAT64));

            cur_block->push_back(fmv_instr);
            break;
        }
        case BasicOperand::IMMF32: {
            // 处理源操作数为浮点立即数的情况
            auto imm_src = static_cast<ImmF32Operand *>(src_op);

            // 构造立即数复制指令，将浮点立即数直接复制到目标寄存器
            auto copy_instr = rvconstructor->ConstructCopyRegImmI(
                GetllvmReg(dst_op->GetRegNo(), INT64), 
                imm_src->GetFloatVal(), 
                INT64);

            cur_block->push_back(copy_instr);
            break;
        }
        default:
            break;
    }

    // TODO: 如果需要，补充其他逻辑
}


template <>
void RiscV64Selector::ConvertAndAppend<SitofpInstruction *>(SitofpInstruction *ins) {
    auto src_op = ins->GetSrc();
    auto dst_op = static_cast<RegOperand *>(ins->GetResultReg());

    switch (src_op->GetOperandType()) {
        case BasicOperand::REG: {
            // 处理源操作数为寄存器的情况
            auto reg_src = static_cast<RegOperand *>(src_op);

            // 使用 FCVT.S.W 指令将整型转换为浮点型
            auto fcvt_instr = rvconstructor->ConstructR2(
                RISCV_FCVT_S_W, 
                GetllvmReg(dst_op->GetRegNo(), FLOAT64),
                GetllvmReg(reg_src->GetRegNo(), INT64));

            cur_block->push_back(fcvt_instr);
            break;
        }
        case BasicOperand::IMMI32: {
            // 处理源操作数为立即数的情况
            auto imm_src = static_cast<ImmI32Operand *>(src_op);

            // 创建一个中间寄存器来保存立即数
            auto temp_reg = GetNewReg(INT64);

            // 将立即数加载到中间寄存器
            auto copy_instr = rvconstructor->ConstructCopyRegImmI(temp_reg, imm_src->GetIntImmVal(), INT64);
            cur_block->push_back(copy_instr);

            // 使用 FCVT.S.W 将中间寄存器中的整型值转换为浮点型
            auto fcvt_instr = rvconstructor->ConstructR2(
                RISCV_FCVT_S_W, 
                GetllvmReg(dst_op->GetRegNo(), FLOAT64), 
                temp_reg);

            cur_block->push_back(fcvt_instr);
            break;
        }
        default:
            break;
    }

    // TODO: 如果需要，补充其他逻辑
}


template <>
void RiscV64Selector::ConvertAndAppend<ZextInstruction *>(ZextInstruction *ins) {
    auto ext_reg = GetllvmReg(static_cast<RegOperand *>(ins->GetSrc())->GetRegNo(), INT64);
    auto cmp_ins = cmp_context[ext_reg];
    auto result_reg = GetllvmReg(static_cast<RegOperand *>(ins->GetResult())->GetRegNo(), INT64);

    if (cmp_ins->GetOpcode() == BasicInstruction::ICMP) {
        auto icmp_ins = static_cast<IcmpInstruction *>(cmp_ins);
        auto op1 = icmp_ins->GetOp1();
        auto op2 = icmp_ins->GetOp2();
        auto cur_cond = icmp_ins->GetCond();
        Register reg_1, reg_2;

        // Adjust condition if operand1 is IMMI32
        if (op1->GetOperandType() == BasicOperand::IMMI32) {
            std::swap(op1, op2);
            switch (cur_cond) {
                case BasicInstruction::sgt: cur_cond = BasicInstruction::slt; break;
                case BasicInstruction::sge: cur_cond = BasicInstruction::sle; break;
                case BasicInstruction::slt: cur_cond = BasicInstruction::sgt; break;
                case BasicInstruction::sle: cur_cond = BasicInstruction::sge; break;
                default: break;
            }
        }

        // Handle immediate values for both operands
        if (op1->GetOperandType() == BasicOperand::IMMI32) {
            int op1_val = static_cast<ImmI32Operand *>(op1)->GetIntImmVal();
            int op2_val = static_cast<ImmI32Operand *>(op2)->GetIntImmVal();
            int rval = 0;

            switch (cur_cond) {
                case BasicInstruction::eq: rval = (op1_val == op2_val); break;
                case BasicInstruction::ne: rval = (op1_val != op2_val); break;
                case BasicInstruction::sgt: rval = (op1_val > op2_val); break;
                case BasicInstruction::sge: rval = (op1_val >= op2_val); break;
                case BasicInstruction::slt: rval = (op1_val < op2_val); break;
                case BasicInstruction::sle: rval = (op1_val <= op2_val); break;
                default: break;
            }

            cur_block->push_back(rvconstructor->ConstructCopyRegImmI(result_reg, rval, INT64));
            return;
        }

        if (op2->GetOperandType() == BasicOperand::IMMI32) {
            auto op1_reg = GetllvmReg(static_cast<RegOperand *>(op1)->GetRegNo(), INT64);
            int op2_imm = static_cast<ImmI32Operand *>(op2)->GetIntImmVal();

            switch (cur_cond) {
                case BasicInstruction::eq:
                    cur_block->push_back(rvconstructor->ConstructIImm(RISCV_SLTIU, result_reg, op1_reg, 1));
                    return;
                case BasicInstruction::ne:
                    cur_block->push_back(rvconstructor->ConstructR(RISCV_SLTU, result_reg, GetPhysicalReg(RISCV_x0), op1_reg));
                    return;
                case BasicInstruction::sgt:
                    cur_block->push_back(rvconstructor->ConstructR(RISCV_SLT, result_reg, GetPhysicalReg(RISCV_x0), op1_reg));
                    return;
                case BasicInstruction::slt:
                    cur_block->push_back(rvconstructor->ConstructIImm(RISCV_SLTI, result_reg, op1_reg, op2_imm));
                    return;
                case BasicInstruction::ult:
                    cur_block->push_back(rvconstructor->ConstructIImm(RISCV_SLTIU, result_reg, op1_reg, op2_imm));
                    return;
                default: break;
            }
        }

        // Handle registers
        reg_1 = GetllvmReg(static_cast<RegOperand *>(op1)->GetRegNo(), INT64);
        reg_2 = (op2->GetOperandType() == BasicOperand::REG) 
                    ? GetllvmReg(static_cast<RegOperand *>(op2)->GetRegNo(), INT64)
                    : GetNewReg(INT64);

        auto mid_reg = GetNewReg(INT64);
        switch (cur_cond) {
            case BasicInstruction::eq:
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SUBW, mid_reg, reg_1, reg_2));
                cur_block->push_back(rvconstructor->ConstructIImm(RISCV_SLTIU, result_reg, mid_reg, 1));
                break;
            case BasicInstruction::ne:
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SUBW, mid_reg, reg_1, reg_2));
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SLTU, result_reg, GetPhysicalReg(RISCV_x0), mid_reg));
                break;
            case BasicInstruction::sgt:
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SLT, result_reg, reg_2, reg_1));
                break;
            case BasicInstruction::slt:
                cur_block->push_back(rvconstructor->ConstructR(RISCV_SLT, result_reg, reg_1, reg_2));
                break;
            default: break;
        }
    } else if (cmp_ins->GetOpcode() == BasicInstruction::FCMP) {
        auto fcmp_ins = static_cast<FcmpInstruction *>(cmp_ins);
        Register cmp_op1, cmp_op2;

        if (fcmp_ins->GetOp1()->GetOperandType() == BasicOperand::REG) {
            cmp_op1 = GetllvmReg(static_cast<RegOperand *>(fcmp_ins->GetOp1())->GetRegNo(), FLOAT64);
        } else if (fcmp_ins->GetOp1()->GetOperandType() == BasicOperand::IMMF32) {
            cmp_op1 = GetNewReg(FLOAT64);
            float val = static_cast<ImmF32Operand *>(fcmp_ins->GetOp1())->GetFloatVal();
            cur_block->push_back(rvconstructor->ConstructCopyRegImmF(cmp_op1, val, FLOAT64));
        }

        if (fcmp_ins->GetOp2()->GetOperandType() == BasicOperand::REG) {
            cmp_op2 = GetllvmReg(static_cast<RegOperand *>(fcmp_ins->GetOp2())->GetRegNo(), FLOAT64);
        } else if (fcmp_ins->GetOp2()->GetOperandType() == BasicOperand::IMMF32) {
            cmp_op2 = GetNewReg(FLOAT64);
            float val = static_cast<ImmF32Operand *>(fcmp_ins->GetOp2())->GetFloatVal();
            cur_block->push_back(rvconstructor->ConstructCopyRegImmF(cmp_op2, val, FLOAT64));
        }

        switch (fcmp_ins->GetCond()) {
            case BasicInstruction::OEQ:
            case BasicInstruction::UEQ:
                cur_block->push_back(rvconstructor->ConstructR(RISCV_FEQ_S, result_reg, cmp_op1, cmp_op2));
                break;
            case BasicInstruction::OLT:
            case BasicInstruction::ULT:
                cur_block->push_back(rvconstructor->ConstructR(RISCV_FLT_S, result_reg, cmp_op1, cmp_op2));
                break;
            case BasicInstruction::OGT:
            case BasicInstruction::UGT:
                cur_block->push_back(rvconstructor->ConstructR(RISCV_FLT_S, result_reg, cmp_op2, cmp_op1));
                break;
            default: break;
        }
    }
}


template <> void RiscV64Selector::ConvertAndAppend<GetElementptrInstruction *>(GetElementptrInstruction *ins) {
    auto global_op = (GlobalOperand *)ins->GetPtrVal();
    auto result_op = (RegOperand *)ins->GetResult();

    int product = 1;
    for (auto size : ins->GetDims()) {
        product *= size;
    }
    int const_offset = 0;
    auto offset_reg = GetNewReg(INT64);
    auto result_reg = GetllvmReg(result_op->GetRegNo(), INT64);

    int offset_reg_assigned = 0;
    for (int i = 0; i < ins->GetIndexes().size(); i++) {
        if (ins->GetIndexes()[i]->GetOperandType() == BasicOperand::IMMI32) {
            const_offset += (((ImmI32Operand *)ins->GetIndexes()[i])->GetIntImmVal()) * product;
        } else {
            auto index_op = (RegOperand *)ins->GetIndexes()[i];
            auto index_reg = GetllvmReg(index_op->GetRegNo(), INT64);
            if (product != 1) {
                auto this_inc = GetNewReg(INT64);
                auto product_reg = GetNewReg(INT64);
                cur_block->push_back(rvconstructor->ConstructCopyRegImmI(product_reg, product, INT64));
                cur_block->push_back(rvconstructor->ConstructR(RISCV_MUL, this_inc, index_reg, product_reg));
                if (offset_reg_assigned == 0) {
                    offset_reg_assigned = 1;
                    cur_block->push_back(rvconstructor->ConstructCopyReg(offset_reg, this_inc, INT64));
                } else {
                    auto new_offset = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructR(RISCV_ADD, new_offset, offset_reg, this_inc));
                    offset_reg = new_offset;
                }
            } else {
                if (offset_reg_assigned == 0) {
                    offset_reg_assigned = 1;
                    auto offset_reg_set_instr = rvconstructor->ConstructCopyReg(offset_reg, index_reg, INT64);
                    cur_block->push_back(offset_reg_set_instr);
                } else {
                    auto new_offset = GetNewReg(INT64);
                    cur_block->push_back(rvconstructor->ConstructR(RISCV_ADD, new_offset, offset_reg, index_reg));
                    offset_reg = new_offset;
                }
            }
        }
        if (i < ins->GetDims().size()) {
            product /= ins->GetDims()[i];
        }
    }
    bool all_imm = false;
    if (const_offset != 0) {
        if (offset_reg_assigned == 0) {
            offset_reg_assigned = 1;
            all_imm = true;

            auto li_instr = rvconstructor->ConstructCopyRegImmI(offset_reg, const_offset * 4, INT64);

            cur_block->push_back(li_instr);
        } else {
            auto new_offset = GetNewReg(INT64);
            cur_block->push_back(rvconstructor->ConstructIImm(RISCV_ADDI, new_offset, offset_reg, const_offset));
            offset_reg = new_offset;
        }
    }
    if (ins->GetPtrVal()->GetOperandType() == BasicOperand::REG) {
        auto ptr_op = (RegOperand *)ins->GetPtrVal();
        auto offsetfull_reg = GetNewReg(INT64);
        if (offset_reg_assigned) {
            auto sll_instr = rvconstructor->ConstructIImm(RISCV_SLLI, offsetfull_reg, offset_reg, 2);
            if (all_imm) {
                offsetfull_reg = offset_reg;
            }
            if (llvm_rv_allocas.find(ptr_op->GetRegNo()) == llvm_rv_allocas.end()) {
                auto base_reg = GetllvmReg(ptr_op->GetRegNo(), INT64);
                if (!all_imm) {
                    cur_block->push_back(sll_instr);
                }
                auto addoffset_instr = rvconstructor->ConstructR(RISCV_ADD, result_reg, base_reg, offsetfull_reg);
                cur_block->push_back(addoffset_instr);
            } else {
                auto sp_offset = llvm_rv_allocas[ptr_op->GetRegNo()];
                auto base_reg = GetNewReg(INT64);
                auto load_basereg_instr =
                rvconstructor->ConstructIImm(RISCV_ADDI, base_reg, GetPhysicalReg(RISCV_sp), sp_offset);
                ((RiscV64Function *)cur_func)->AddAllocaIns(load_basereg_instr);
                cur_block->push_back(load_basereg_instr);
                if (!all_imm) {
                    cur_block->push_back(sll_instr);
                }
                auto addoffset_instr = rvconstructor->ConstructR(RISCV_ADD, result_reg, base_reg, offsetfull_reg);
                cur_block->push_back(addoffset_instr);
            }
        } else {
            if (llvm_rv_allocas.find(ptr_op->GetRegNo()) == llvm_rv_allocas.end()) {
                cur_block->push_back(
                rvconstructor->ConstructCopyReg(result_reg, GetllvmReg(ptr_op->GetRegNo(), INT64), INT64));
            } else {
                auto sp_offset = llvm_rv_allocas[ptr_op->GetRegNo()];
                auto load_basereg_instr =
                rvconstructor->ConstructIImm(RISCV_ADDI, result_reg, GetPhysicalReg(RISCV_sp), sp_offset);
                ((RiscV64Function *)cur_func)->AddAllocaIns(load_basereg_instr);
                cur_block->push_back(load_basereg_instr);
            }
        }
    } else if (ins->GetPtrVal()->GetOperandType() == BasicOperand::GLOBAL) {
        if (offset_reg_assigned) {
            auto basehi_reg = GetNewReg(INT64);
            auto basefull_reg = GetNewReg(INT64);
            auto offsetfull_reg = GetNewReg(INT64);

            auto lui_instr =
            rvconstructor->ConstructULabel(RISCV_LUI, basehi_reg, RiscVLabel(global_op->GetName(), true));
            auto addi_instr = rvconstructor->ConstructILabel(RISCV_ADDI, basefull_reg, basehi_reg,
                                                             RiscVLabel(global_op->GetName(), false));
            auto sll_instr = rvconstructor->ConstructIImm(RISCV_SLLI, offsetfull_reg, offset_reg, 2);
            if (all_imm) {
                offsetfull_reg = offset_reg;
            }
            auto addoffset_instr = rvconstructor->ConstructR(RISCV_ADD, result_reg, basefull_reg, offsetfull_reg);

            cur_block->push_back(lui_instr);
            cur_block->push_back(addi_instr);
            if (!all_imm) {
                cur_block->push_back(sll_instr);
            }
            cur_block->push_back(addoffset_instr);
        } else {
            auto result_hi_reg = GetNewReg(INT64);

            auto lui_instr =
            rvconstructor->ConstructULabel(RISCV_LUI, result_hi_reg, RiscVLabel(global_op->GetName(), true));
            auto addi_instr = rvconstructor->ConstructILabel(RISCV_ADDI, result_reg, result_hi_reg,
                                                             RiscVLabel(global_op->GetName(), false));

            cur_block->push_back(lui_instr);
            cur_block->push_back(addi_instr);
        }
    }
    //TODO("Implement this if you need");
}

template <> void RiscV64Selector::ConvertAndAppend<PhiInstruction *>(PhiInstruction *ins) {
    auto res_op = (RegOperand *)ins->GetResultReg();
    Register result_reg;
    if (ins->GetDataType() == BasicInstruction::I32 || ins->GetDataType() == BasicInstruction::PTR) {
        result_reg = GetllvmReg(res_op->GetRegNo(), INT64);
    } else if (ins->GetDataType() == BasicInstruction::FLOAT32) {
        result_reg = GetllvmReg(res_op->GetRegNo(), FLOAT64);
    }
    auto m_phi = new MachinePhiInstruction(result_reg);
    for (auto [label, val] : ins->GetPhiList()) {
        auto label_op = (LabelOperand *)label;
        if (val->GetOperandType() == BasicOperand::REG && ins->GetDataType() == BasicInstruction::I32) {
            auto reg_op = (RegOperand *)val;
            auto val_reg = GetllvmReg(reg_op->GetRegNo(), INT64);
            m_phi->pushPhiList(label_op->GetLabelNo(), val_reg);
        } else if (val->GetOperandType() == BasicOperand::REG && ins->GetDataType() == BasicInstruction::FLOAT32) {
            auto reg_op = (RegOperand *)val;
            auto val_reg = GetllvmReg(reg_op->GetRegNo(), FLOAT64);
            m_phi->pushPhiList(label_op->GetLabelNo(), val_reg);
        } else if (val->GetOperandType() == BasicOperand::IMMI32) {
            auto immi_op = (ImmI32Operand *)val;
            m_phi->pushPhiList(label_op->GetLabelNo(), immi_op->GetIntImmVal());
        } else if (val->GetOperandType() == BasicOperand::IMMF32) {
            auto immf_op = (ImmF32Operand *)val;
            m_phi->pushPhiList(label_op->GetLabelNo(), immf_op->GetFloatVal());
        } else if (val->GetOperandType() == BasicOperand::REG && ins->GetDataType() == BasicInstruction::PTR) {
            auto reg_op = (RegOperand *)val;
            auto val_reg = GetllvmReg(reg_op->GetRegNo(), INT64);
            m_phi->pushPhiList(label_op->GetLabelNo(), val_reg);
        }
    }
    cur_block->push_back(m_phi);
    //TODO("Implement this if you need");
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
        break;
    }
}

void RiscV64Selector::SelectInstructionAndBuildCFG() {
    // 与中间代码生成一样, 如果你完全无从下手, 可以先看看输出是怎么写的
    // 即riscv64gc/instruction_print/*  common/machine_passes/machine_printer.h

    // 指令选择除了一些函数调用约定必须遵守的情况需要物理寄存器，其余情况必须均为虚拟寄存器
    dest->global_def = IR->global_def;
    // 遍历每个LLVM IR函数
    for (auto [defI,cfg] : IR->llvm_cfg) {
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
        // TODO("Add function parameter if you need");
        for (int i = 0; i < defI->GetFormalSize(); i++) {
            MachineDataType type;
            if (defI->formals[i] == BasicInstruction::LLVMType::I32 || defI->formals[i] == BasicInstruction::LLVMType::PTR) {
                type = INT64;
            } else if (defI->formals[i] == BasicInstruction::LLVMType::FLOAT32) {
                type = FLOAT64;
            }
            cur_func->AddParameter(GetllvmReg(((RegOperand *)defI->formals_reg[i])->GetRegNo(), type));
        }

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
    return llvm_rv_regtable[ir_reg];
}

Register RiscV64Selector::GetNewReg(MachineDataType type) {
    return cur_func->GetNewRegister(type.data_type, type.data_length);
}
