#include "riscv64_lowerframe.h"

constexpr int save_regids[] = {
    RISCV_s0,  RISCV_s1,  RISCV_s2,  RISCV_s3,  RISCV_s4,   RISCV_s5,   RISCV_s6,  RISCV_s7,  RISCV_s8,
    RISCV_s9,  RISCV_s10, RISCV_s11, RISCV_fs0, RISCV_fs1,  RISCV_fs2,  RISCV_fs3, RISCV_fs4, RISCV_fs5,
    RISCV_fs6, RISCV_fs7, RISCV_fs8, RISCV_fs9, RISCV_fs10, RISCV_fs11, RISCV_ra,
};
constexpr int saveregnum = 25;

extern bool optimize_flag;

/*
    假设IR中的函数定义为f(i32 %r0, i32 %r1)
    则parameters应该存放两个虚拟寄存器%0,%1

    在LowerFrame后应当为
    add %0,a0,x0  (%0 <- a0)
    add %1,a1,x0  (%1 <- a1)

    对于浮点寄存器按照类似的方法处理即可
*/
void RiscV64LowerFrame::Execute() {
    // 在每个函数的开头处插入获取参数的指令
    for (auto func : unit->functions) {
        current_func = func;
        for (auto &b : func->blocks) {
            cur_block = b;
            if (b->getLabelId() == 0) {    // 函数入口，需要插入获取参数的指令
                Register para_basereg = current_func->GetNewReg(INT64);
                int i32_cnt = 0;
                int f32_cnt = 0;
                int para_offset = 0;
                for (auto para : func->GetParameters()) {    // 你需要在指令选择阶段正确设置parameters的值
                    if (para.type.data_type == INT64.data_type) {
                        if (i32_cnt < 8) {    // 插入使用寄存器传参的指令
                            b->push_front(rvconstructor->ConstructR(RISCV_ADD, para, GetPhysicalReg(RISCV_a0 + i32_cnt),
                                                                    GetPhysicalReg(RISCV_x0)));
                        }
                        if (i32_cnt >= 8) {    // 插入使用内存传参的指令
                            b->push_front(rvconstructor->ConstructIImm(RISCV_LD, para, GetPhysicalReg(RISCV_fp), 
                                                                    para_offset));
                            para_offset += 8;
                            //TODO("Implement this if you need");
                        }
                        i32_cnt++;
                    } else if (para.type.data_type == FLOAT64.data_type) {    // 处理浮点数
                        if (f32_cnt < 8) {
                            b->push_front(
                            rvconstructor->ConstructCopyReg(para, GetPhysicalReg(RISCV_fa0 + f32_cnt), FLOAT64));
                        }
                        if (f32_cnt >= 8) {
                            b->push_front(
                            rvconstructor->ConstructIImm(RISCV_FLD, para, GetPhysicalReg(RISCV_fp), para_offset));
                            para_offset += 8;
                        }   
                        f32_cnt++;
                        //TODO("Implement this if you need");
                    } else {
                        ERROR("Unknown type");
                    }
                }
                if (para_offset != 0) {
                    current_func->SetHasInParaInStack(true);
                    cur_block->push_front(
                    rvconstructor->ConstructCopyReg(para_basereg, GetPhysicalReg(RISCV_fp), INT64));
                }
            }
        }
    }
}

void RiscV64LowerStack::Execute() {
    // 在函数在寄存器分配后执行
    // TODO: 在函数开头保存 函数被调者需要保存的寄存器，并开辟栈空间
    // TODO: 在函数结尾恢复 函数被调者需要保存的寄存器，并收回栈空间
    // TODO: 开辟和回收栈空间
    // 具体需要保存/恢复哪些可以查阅RISC-V函数调用约定
    for (auto func : unit->functions) {
        current_func = func;

#ifdef SAVEREGBYCOPY
        // 分配保存寄存器的虚拟寄存器
        Register save_regs[saveregnum];
        for (int i = 0; i < saveregnum; i++) {
            if (save_regids[i] >= RISCV_x0 && save_regids[i] <= RISCV_x31) {
                save_regs[i] = func->GetNewRegister(INT64.data_type, INT64.data_length);
            } else {
                save_regs[i] = func->GetNewRegister(FLOAT64.data_type, FLOAT64.data_length);
            }
        }
#endif

        for (auto &b : func->blocks) {
            cur_block = b;
            if (b->getLabelId() == 0) { // 函数入口
                Register para_basereg = current_func->GetNewReg(INT64);
                int i32_cnt = 0;
                int f32_cnt = 0;
                int para_offset = 0;
                for (auto para : func->GetParameters()) {
                    if (para.type.data_type == INT64.data_type) {
                        if (i32_cnt < 8) { // 前8个参数从寄存器传递
                            b->push_front(
                                rvconstructor->ConstructCopyReg(para, GetPhysicalReg(RISCV_a0 + i32_cnt), INT64));
                        }
                        if (i32_cnt >= 8) { // 超过8个参数从栈中加载
                            b->push_front(
                                rvconstructor->ConstructIImm(RISCV_LD, para, GetPhysicalReg(RISCV_fp), para_offset));
                            para_offset += 8;
                        }
                        i32_cnt++;
                    } else if (para.type.data_type == FLOAT64.data_type) { // 处理浮点数参数
                        if (f32_cnt < 8) { // 前8个浮点数参数从寄存器传递
                            b->push_front(
                                rvconstructor->ConstructCopyReg(para, GetPhysicalReg(RISCV_fa0 + f32_cnt), FLOAT64));
                        }
                        if (f32_cnt >= 8) { // 超过8个浮点数参数从栈中加载
                            b->push_front(
                                rvconstructor->ConstructIImm(RISCV_FLD, para, GetPhysicalReg(RISCV_fp), para_offset));
                            para_offset += 8;
                        }
                        f32_cnt++;
                    } else {
                        ERROR("Unknown type");
                    }
                }

                if (para_offset != 0) {
                    current_func->SetHasInParaInStack(true);
                    cur_block->push_front(
                        rvconstructor->ConstructCopyReg(para_basereg, GetPhysicalReg(RISCV_fp), INT64));
                }

#ifdef SAVEREGBYCOPY
                // 保存被调用者需要保存的寄存器
                for (int i = 0; i < saveregnum; i++) {
                    if (save_regs[i].type == INT64) {
                        b->push_front(
                            rvconstructor->ConstructCopyReg(save_regs[i], GetPhysicalReg(save_regids[i]), INT64));
                    } else {
                        b->push_front(
                            rvconstructor->ConstructCopyReg(save_regs[i], GetPhysicalReg(save_regids[i]), FLOAT64));
                    }
                }
#endif
            }

#ifdef SAVEREGBYCOPY
            // 在返回指令之前恢复被调用者需要保存的寄存器
            auto last_ins = *(b->ReverseBegin());
            Assert(last_ins->arch == MachineBaseInstruction::RiscV);
            auto riscv_last_ins = (RiscV64Instruction *)last_ins;
            if (riscv_last_ins->getOpcode() == RISCV_JALR) {
                if (riscv_last_ins->getRd() == GetPhysicalReg(RISCV_x0) &&
                    riscv_last_ins->getRs1() == GetPhysicalReg(RISCV_ra) &&
                    riscv_last_ins->getImm() == 0) {

                    b->pop_back(); // 移除 JALR 指令

                    // 收回栈空间
                    if (func->GetStackSize() <= 2032) {
                        b->push_back(rvconstructor->ConstructIImm(RISCV_ADDI, GetPhysicalReg(RISCV_sp),
                                                                  GetPhysicalReg(RISCV_sp), func->GetStackSize()));
                    } else {
                        auto stacksz_reg = GetPhysicalReg(RISCV_t0);
                        b->push_back(rvconstructor->ConstructUImm(RISCV_LI, stacksz_reg, func->GetStackSize()));
                        b->push_back(rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_sp),
                                                               GetPhysicalReg(RISCV_sp), stacksz_reg));
                    }

                    // 恢复被调用者需要保存的寄存器
                    if (func->HasInParaInStack()) {
                        // 恢复帧指针
                        b->push_back(
                            rvconstructor->ConstructCopyReg(GetPhysicalReg(RISCV_fp),
                                                           para_basereg, INT64));
                    }

                    for (int i = 0; i < saveregnum; i++) {
                        if (save_regs[i].type == INT64) {
                            b->push_back(
                                rvconstructor->ConstructCopyReg(GetPhysicalReg(save_regids[i]), save_regs[i], INT64));
                        } else {
                            b->push_back(
                                rvconstructor->ConstructCopyReg(GetPhysicalReg(save_regids[i]), save_regs[i], FLOAT64));
                        }
                    }

                    b->push_back(riscv_last_ins); // 重新插入 JALR 指令
                }
            }
#endif
        }

        // 确保所有返回路径都正确恢复寄存器和收回栈空间
        // 通过遍历控制流图中的所有返回指令并插入恢复和收回指令
        for (auto &b : func->blocks) {
            for (auto ins_it = b->begin(); ins_it != b->end(); ++ins_it) {
                auto ins = *ins_it;
                if (ins->arch == MachineBaseInstruction::RiscV) {
                    auto riscv_ins = (RiscV64Instruction *)ins;
                    if (riscv_ins->getOpcode() == RISCV_JALR &&
                        riscv_ins->getRd() == GetPhysicalReg(RISCV_x0) &&
                        riscv_ins->getRs1() == GetPhysicalReg(RISCV_ra) &&
                        riscv_ins->getImm() == 0) { // 返回指令

#ifdef SAVEREGBYCOPY
                        // 收回栈空间
                        if (func->GetStackSize() <= 2032) {
                            auto addi_ins = rvconstructor->ConstructIImm(RISCV_ADDI, GetPhysicalReg(RISCV_sp),
                                                                       GetPhysicalReg(RISCV_sp), func->GetStackSize());
                            b->insert(ins_it, addi_ins);
                        } else {
                            auto stacksz_reg = GetPhysicalReg(RISCV_t0);
                            auto li_ins = rvconstructor->ConstructUImm(RISCV_LI, stacksz_reg, func->GetStackSize());
                            auto add_ins = rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_sp),
                                                                    GetPhysicalReg(RISCV_sp), stacksz_reg);
                            b->insert(ins_it, li_ins);
                            b->insert(ins_it, add_ins);
                        }

                        // 恢复寄存器
                        for (int i = 0; i < saveregnum; i++) {
                            if (save_regs[i].type == INT64) {
                                auto ld_ins = rvconstructor->ConstructCopyReg(GetPhysicalReg(save_regids[i]),
                                                                              save_regs[i], INT64);
                                b->insert(ins_it, ld_ins);
                            } else {
                                auto fld_ins = rvconstructor->ConstructCopyReg(GetPhysicalReg(save_regids[i]),
                                                                               save_regs[i], FLOAT64);
                                b->insert(ins_it, fld_ins);
                            }
                        }
#endif
                    }
                }
            }
        }

        // 栈指针对齐（16字节对齐）
        int stack_size = func->GetStackSize();
        if (stack_size % 16 != 0) {
            int aligned_size = ((stack_size + 15) / 16) * 16;
            int padding = aligned_size - stack_size;
            for (auto &b : func->blocks) {
                if (b->getLabelId() == 0) { // 函数入口
                    if (stack_size <= 2032) {
                        b->push_front(rvconstructor->ConstructIImm(RISCV_ADDI, GetPhysicalReg(RISCV_sp),
                                                                   GetPhysicalReg(RISCV_sp),
                                                                   -(stack_size + padding)));
                    } else {
                        auto stacksz_reg = GetPhysicalReg(RISCV_t0);
                        b->push_front(rvconstructor->ConstructUImm(RISCV_LI, stacksz_reg, stack_size + padding));
                        b->push_front(rvconstructor->ConstructR(RISCV_SUB, GetPhysicalReg(RISCV_sp),
                                                              GetPhysicalReg(RISCV_sp), stacksz_reg));
                    }
                }

                // 在返回指令处收回额外的栈空间
                for (auto ins_it = b->begin(); ins_it != b->end(); ++ins_it) {
                    auto ins = *ins_it;
                    if (ins->arch == MachineBaseInstruction::RiscV) {
                        auto riscv_ins = (RiscV64Instruction *)ins;
                        if (riscv_ins->getOpcode() == RISCV_JALR &&
                            riscv_ins->getRd() == GetPhysicalReg(RISCV_x0) &&
                            riscv_ins->getRs1() == GetPhysicalReg(RISCV_ra) &&
                            riscv_ins->getImm() == 0) { // 返回指令

#ifdef SAVEREGBYCOPY
                            if (aligned_size <= 2032) {
                                auto addi_ins = rvconstructor->ConstructIImm(RISCV_ADDI, GetPhysicalReg(RISCV_sp),
                                                                           GetPhysicalReg(RISCV_sp), aligned_size);
                                b->insert(ins_it, addi_ins);
                            } else {
                                auto stacksz_reg = GetPhysicalReg(RISCV_t0);
                                auto li_ins = rvconstructor->ConstructUImm(RISCV_LI, stacksz_reg, aligned_size);
                                auto add_ins = rvconstructor->ConstructR(RISCV_ADD, GetPhysicalReg(RISCV_sp),
                                                                        GetPhysicalReg(RISCV_sp), stacksz_reg);
                                b->insert(ins_it, li_ins);
                                b->insert(ins_it, add_ins);
                            }
#endif
                        }
                    }
                }
            }
        }

#ifdef PRINT_DBG
        // 可选的调试输出
        RiscV64Printer printer(std::cerr, unit);
        printer.SyncFunction(func);
        for (auto &b : func->blocks) {
            printer.SyncBlock(b);
            for (auto ins : b->GetInsList()) {
                printer.printAsm(*ins);
            }
        }
#endif
    }
    //Log("RiscV64LowerStack");

    // 到此我们就完成目标代码生成的所有工作了
}

