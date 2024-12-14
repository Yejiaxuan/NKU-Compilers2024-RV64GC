#include "simple_adce.h"

#include <stack>
#include <unordered_map>
#include <bitset>
#include <deque>
#include <queue>

// 假设最大寄存器数量为1024，可根据实际情况调整
constexpr int MAX_REGS_ADCE = 1024;

// 判断指令是否有副作用（与DCE中类似）
static bool HasSideEffect(BasicInstruction* instr) {
    switch (instr->GetOpcode()) {
        case BasicInstruction::STORE:
        case BasicInstruction::CALL:
        case BasicInstruction::RET:
            return true;
        default:
            return false;
    }
}

// 获取指令的结果寄存器号，如果没有则返回-1（与DCE中类似）
static int GetResultRegNo(BasicInstruction* instr) {
    Operand result = nullptr;
    switch (instr->GetOpcode()) {
        case BasicInstruction::LOAD: {
            auto loadInstr = dynamic_cast<LoadInstruction*>(instr);
            result = loadInstr->GetResult();
            break;
        }
        case BasicInstruction::ADD:
        case BasicInstruction::SUB:
        case BasicInstruction::MUL:
        case BasicInstruction::DIV:
        case BasicInstruction::BITXOR:
        case BasicInstruction::MOD:
        case BasicInstruction::FADD:
        case BasicInstruction::FSUB:
        case BasicInstruction::FMUL:
        case BasicInstruction::FDIV:
        case BasicInstruction::ICMP:
        case BasicInstruction::FCMP:
        case BasicInstruction::ZEXT:
        case BasicInstruction::FPTOSI:
        case BasicInstruction::SITOFP:
        case BasicInstruction::GETELEMENTPTR:
        case BasicInstruction::ALLOCA:
        case BasicInstruction::PHI:
        case BasicInstruction::CALL: {
            if (auto arithInstr = dynamic_cast<ArithmeticInstruction*>(instr)) {
                result = arithInstr->GetResult();
            } else if (auto icmpInstr = dynamic_cast<IcmpInstruction*>(instr)) {
                result = icmpInstr->GetResult();
            } else if (auto fcmpInstr = dynamic_cast<FcmpInstruction*>(instr)) {
                result = fcmpInstr->GetResult();
            } else if (auto phiInstr = dynamic_cast<PhiInstruction*>(instr)) {
                result = phiInstr->GetResultReg();
            } else if (auto callInstr = dynamic_cast<CallInstruction*>(instr)) {
                result = callInstr->GetResult();
            } else if (auto gepInstr = dynamic_cast<GetElementptrInstruction*>(instr)) {
                result = gepInstr->GetResult();
            } else if (auto allocInstr = dynamic_cast<AllocaInstruction*>(instr)) {
                result = allocInstr->GetResult();
            } else if (auto zextInstr = dynamic_cast<ZextInstruction*>(instr)) {
                result = zextInstr->GetResult();
            } else if (auto fptosiInstr = dynamic_cast<FptosiInstruction*>(instr)) {
                result = fptosiInstr->GetResult();
            } else if (auto sitofpInstr = dynamic_cast<SitofpInstruction*>(instr)) {
                result = sitofpInstr->GetResult();
            }
            break;
        }
        default:
            break;
    }

    if (result && result->GetOperandType() == BasicOperand::REG) {
        if (auto reg = dynamic_cast<RegOperand*>(result)) {
            return reg->GetRegNo();
        }
    }
    return -1;
}

// 判断指令是否使用给定的reg_no寄存器（与DCE中类似）
static bool UsesRegister(BasicInstruction* instr, int reg_no) {
    auto CheckOperandUsesReg = [&](Operand op) {
        return (op->GetOperandType() == BasicOperand::REG &&
                dynamic_cast<RegOperand*>(op) &&
                dynamic_cast<RegOperand*>(op)->GetRegNo() == reg_no);
    };

    switch (instr->GetOpcode()) {
        case BasicInstruction::LOAD: {
            auto loadInstr = dynamic_cast<LoadInstruction*>(instr);
            return CheckOperandUsesReg(loadInstr->GetPointer());
        }
        case BasicInstruction::STORE: {
            auto storeInstr = dynamic_cast<StoreInstruction*>(instr);
            return CheckOperandUsesReg(storeInstr->GetValue()) || CheckOperandUsesReg(storeInstr->GetPointer());
        }
        case BasicInstruction::ADD:
        case BasicInstruction::SUB:
        case BasicInstruction::MUL:
        case BasicInstruction::DIV:
        case BasicInstruction::BITXOR:
        case BasicInstruction::MOD:
        case BasicInstruction::FADD:
        case BasicInstruction::FSUB:
        case BasicInstruction::FMUL:
        case BasicInstruction::FDIV: {
            auto arithInstr = dynamic_cast<ArithmeticInstruction*>(instr);
            return CheckOperandUsesReg(arithInstr->GetOperand1()) || CheckOperandUsesReg(arithInstr->GetOperand2());
        }
        case BasicInstruction::ICMP: {
            auto icmpInstr = dynamic_cast<IcmpInstruction*>(instr);
            return CheckOperandUsesReg(icmpInstr->GetOp1()) || CheckOperandUsesReg(icmpInstr->GetOp2());
        }
        case BasicInstruction::FCMP: {
            auto fcmpInstr = dynamic_cast<FcmpInstruction*>(instr);
            return CheckOperandUsesReg(fcmpInstr->GetOp1()) || CheckOperandUsesReg(fcmpInstr->GetOp2());
        }
        case BasicInstruction::PHI: {
            auto phiInstr = dynamic_cast<PhiInstruction*>(instr);
            for (auto &val_label : phiInstr->GetPhiList()) {
                if (CheckOperandUsesReg(val_label.first))
                    return true;
            }
            break;
        }
        case BasicInstruction::CALL: {
            auto callInstr = dynamic_cast<CallInstruction*>(instr);
            for (auto &arg : callInstr->GetParameterList()) {
                if (CheckOperandUsesReg(arg.second))
                    return true;
            }
            break;
        }
        case BasicInstruction::GETELEMENTPTR: {
            auto gepInstr = dynamic_cast<GetElementptrInstruction*>(instr);
            if (CheckOperandUsesReg(gepInstr->GetPtrVal()))
                return true;
            for (auto &idx : gepInstr->GetIndexes()) {
                if (CheckOperandUsesReg(idx))
                    return true;
            }
            break;
        }
        case BasicInstruction::FPTOSI: {
            auto castInstr = dynamic_cast<FptosiInstruction*>(instr);
            return CheckOperandUsesReg(castInstr->GetSrc());
        }
        case BasicInstruction::SITOFP: {
            auto castInstr = dynamic_cast<SitofpInstruction*>(instr);
            return CheckOperandUsesReg(castInstr->GetSrc());
        }
        case BasicInstruction::ZEXT: {
            auto zextInstr = dynamic_cast<ZextInstruction*>(instr);
            return CheckOperandUsesReg(zextInstr->GetSrc());
        }
        case BasicInstruction::BR_COND: {
            auto brInstr = dynamic_cast<BrCondInstruction*>(instr);
            return CheckOperandUsesReg(brInstr->GetCond());
        }
        case BasicInstruction::RET: {
            auto retInstr = dynamic_cast<RetInstruction*>(instr);
            if(retInstr->GetRetVal())
                return CheckOperandUsesReg(retInstr->GetRetVal());
            break;
        }
        default:
            break;
    }
    return false;
}

// 从入口块出发，进行BFS或DFS，标记可达基本块
static std::unordered_set<int> ComputeReachableBlocks(CFG *C) {
    std::unordered_set<int> visited;
    std::queue<int> q;
    // 假设入口基本块为0号块
    q.push(0);
    visited.insert(0);

    while(!q.empty()) {
        int curr = q.front();
        q.pop();
        for (auto &succ : C->GetSuccessor(curr)) {
            int succ_id = succ->block_id;
            if (visited.find(succ_id) == visited.end()) {
                visited.insert(succ_id);
                q.push(succ_id);
            }
        }
    }

    return visited;
}

// 激进死代码删除
void SimpleADCEPass::EliminateDeadCode(CFG *C) {
    // 第一步：计算从入口可达的基本块集合
    auto reachable_blocks = ComputeReachableBlocks(C);

    // 删除不可达块中的所有指令
    // 对不可达块可以直接清空，因为不可达块的指令一定不对程序结果产生影响
    for (auto &[block_id, block] : *(C->block_map)) {
        if (reachable_blocks.find(block_id) == reachable_blocks.end()) {
            // 不可达块，直接删除其中所有指令
            for (auto instr : block->Instruction_list) {
                delete instr; // 释放内存
            }
            block->Instruction_list.clear();
        }
    }

    // 对可达块执行类似DCE的过程
    // 与DCE相同：我们需要通过构建def-use链及逆向标记有用指令和寄存器

    // 构建def-use链
    std::unordered_map<int, std::vector<BasicInstruction*>> def_use_chain;
    for(auto &[block_id, block] : *(C->block_map)) {
        if (reachable_blocks.find(block_id) == reachable_blocks.end()) continue; // 只针对可达块
        for(auto &instr : block->Instruction_list) {
            for (int reg_no = 0; reg_no < MAX_REGS_ADCE; ++reg_no) {
                if(UsesRegister(instr, reg_no)) {
                    def_use_chain[reg_no].push_back(instr);
                }
            }
        }
    }

    // 标记有用的寄存器集合
    std::unordered_set<int> useful_regs;
    std::queue<int> worklist;

    // 初始化：所有具有副作用的指令及其引用的操作数寄存器都是有用的
    // 同时，如果指令有结果寄存器并且具有副作用，我们也会保留
    for(auto &[block_id, block] : *(C->block_map)) {
        if (reachable_blocks.find(block_id) == reachable_blocks.end()) continue;
        // 从后向前并不是必要的，但可以保持一致性
        for (auto it = block->Instruction_list.rbegin(); it != block->Instruction_list.rend(); ++it) {
            BasicInstruction* instr = *it;
            if (HasSideEffect(instr)) {
                // 标记其操作数为有用
                // 与DCE中类似的逻辑
                // 分析不同指令类型，将操作数对应的寄存器加入有用集合
                switch (instr->GetOpcode()) {
                    case BasicInstruction::STORE: {
                        auto storeInstr = dynamic_cast<StoreInstruction*>(instr);
                        if (storeInstr->GetValue()->GetOperandType() == BasicOperand::REG) {
                            int val_r = dynamic_cast<RegOperand*>(storeInstr->GetValue())->GetRegNo();
                            if (useful_regs.find(val_r) == useful_regs.end()) {
                                useful_regs.insert(val_r);
                                worklist.push(val_r);
                            }
                        }
                        if (storeInstr->GetPointer()->GetOperandType() == BasicOperand::REG) {
                            int ptr_r = dynamic_cast<RegOperand*>(storeInstr->GetPointer())->GetRegNo();
                            if (useful_regs.find(ptr_r) == useful_regs.end()) {
                                useful_regs.insert(ptr_r);
                                worklist.push(ptr_r);
                            }
                        }
                        break;
                    }
                    case BasicInstruction::CALL: {
                        auto callInstr = dynamic_cast<CallInstruction*>(instr);
                        for (auto &arg : callInstr->GetParameterList()) {
                            if (arg.second->GetOperandType() == BasicOperand::REG) {
                                int arg_r = dynamic_cast<RegOperand*>(arg.second)->GetRegNo();
                                if (useful_regs.find(arg_r) == useful_regs.end()) {
                                    useful_regs.insert(arg_r);
                                    worklist.push(arg_r);
                                }
                            }
                        }
                        break;
                    }
                    case BasicInstruction::PHI: {
                        auto phiInstr = dynamic_cast<PhiInstruction*>(instr);
                        for (auto &val_label : phiInstr->GetPhiList()) {
                            if (val_label.first->GetOperandType() == BasicOperand::REG) {
                                int r = dynamic_cast<RegOperand*>(val_label.first)->GetRegNo();
                                if (useful_regs.find(r) == useful_regs.end()) {
                                    useful_regs.insert(r);
                                    worklist.push(r);
                                }
                            }
                        }
                        break;
                    }
                    case BasicInstruction::RET: {
                        auto retInstr = dynamic_cast<RetInstruction*>(instr);
                        if(retInstr->GetRetVal() && retInstr->GetRetVal()->GetOperandType() == BasicOperand::REG) {
                            int ret_r = dynamic_cast<RegOperand*>(retInstr->GetRetVal())->GetRegNo();
                            if(useful_regs.find(ret_r) == useful_regs.end()) {
                                useful_regs.insert(ret_r);
                                worklist.push(ret_r);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }

                // 对有副作用的指令本身，如果它有结果寄存器，也应该被认为是有用的
                int res_reg = GetResultRegNo(instr);
                if(res_reg != -1 && useful_regs.find(res_reg) == useful_regs.end()) {
                    useful_regs.insert(res_reg);
                    worklist.push(res_reg);
                }
            } else {
                // 无副作用指令不自动标记，除非其结果在后续中被发现有用
                // 如果该指令的result被其他有用指令使用，将在之后的迭代中标记
            }
        }
    }

    // 广度或深度优先标记所有有用指令中涉及的寄存器
    while(!worklist.empty()) {
        int reg = worklist.front();
        worklist.pop();

        // 查找使用该reg的所有指令
        if(def_use_chain.find(reg) != def_use_chain.end()) {
            for(auto *instr : def_use_chain[reg]) {
                // 标记该指令的结果寄存器为有用
                int instr_res_reg = GetResultRegNo(instr);
                if(instr_res_reg != -1 && useful_regs.find(instr_res_reg) == useful_regs.end()) {
                    useful_regs.insert(instr_res_reg);
                    worklist.push(instr_res_reg);
                }

                // 标记该指令的操作数为有用（类似DCE逻辑）
                // 根据指令类型具体处理
                // 这里与DCE相同，重复写出可能略冗余，可封装
                auto MarkOperand = [&](Operand op) {
                    if(op->GetOperandType() == BasicOperand::REG) {
                        int r = dynamic_cast<RegOperand*>(op)->GetRegNo();
                        if(useful_regs.find(r) == useful_regs.end()) {
                            useful_regs.insert(r);
                            worklist.push(r);
                        }
                    }
                };

                switch(instr->GetOpcode()) {
                    case BasicInstruction::ADD:
                    case BasicInstruction::SUB:
                    case BasicInstruction::MUL:
                    case BasicInstruction::DIV:
                    case BasicInstruction::BITXOR:
                    case BasicInstruction::MOD:
                    case BasicInstruction::FADD:
                    case BasicInstruction::FSUB:
                    case BasicInstruction::FMUL:
                    case BasicInstruction::FDIV: {
                        auto arithInstr = dynamic_cast<ArithmeticInstruction*>(instr);
                        MarkOperand(arithInstr->GetOperand1());
                        MarkOperand(arithInstr->GetOperand2());
                        break;
                    }
                    case BasicInstruction::ICMP: {
                        auto icmpInstr = dynamic_cast<IcmpInstruction*>(instr);
                        MarkOperand(icmpInstr->GetOp1());
                        MarkOperand(icmpInstr->GetOp2());
                        break;
                    }
                    case BasicInstruction::FCMP: {
                        auto fcmpInstr = dynamic_cast<FcmpInstruction*>(instr);
                        MarkOperand(fcmpInstr->GetOp1());
                        MarkOperand(fcmpInstr->GetOp2());
                        break;
                    }
                    case BasicInstruction::PHI: {
                        auto phiInstr = dynamic_cast<PhiInstruction*>(instr);
                        for (auto &val_label : phiInstr->GetPhiList()) {
                            MarkOperand(val_label.first);
                        }
                        break;
                    }
                    case BasicInstruction::LOAD: {
                        auto loadInstr = dynamic_cast<LoadInstruction*>(instr);
                        MarkOperand(loadInstr->GetPointer());
                        break;
                    }
                    case BasicInstruction::STORE: {
                        auto storeInstr = dynamic_cast<StoreInstruction*>(instr);
                        MarkOperand(storeInstr->GetValue());
                        MarkOperand(storeInstr->GetPointer());
                        break;
                    }
                    case BasicInstruction::CALL: {
                        auto callInstr = dynamic_cast<CallInstruction*>(instr);
                        for (auto &arg : callInstr->GetParameterList()) {
                            MarkOperand(arg.second);
                        }
                        break;
                    }
                    case BasicInstruction::GETELEMENTPTR: {
                        auto gepInstr = dynamic_cast<GetElementptrInstruction*>(instr);
                        MarkOperand(gepInstr->GetPtrVal());
                        for (auto &idx : gepInstr->GetIndexes()) {
                            MarkOperand(idx);
                        }
                        break;
                    }
                    case BasicInstruction::FPTOSI: {
                        auto castInstr = dynamic_cast<FptosiInstruction*>(instr);
                        MarkOperand(castInstr->GetSrc());
                        break;
                    }
                    case BasicInstruction::SITOFP: {
                        auto castInstr = dynamic_cast<SitofpInstruction*>(instr);
                        MarkOperand(castInstr->GetSrc());
                        break;
                    }
                    case BasicInstruction::ZEXT: {
                        auto zextInstr = dynamic_cast<ZextInstruction*>(instr);
                        MarkOperand(zextInstr->GetSrc());
                        break;
                    }
                    case BasicInstruction::BR_COND: {
                        auto brInstr = dynamic_cast<BrCondInstruction*>(instr);
                        MarkOperand(brInstr->GetCond());
                        break;
                    }
                    case BasicInstruction::RET: {
                        auto retInstr = dynamic_cast<RetInstruction*>(instr);
                        if(retInstr->GetRetVal()) MarkOperand(retInstr->GetRetVal());
                        break;
                    }
                    case BasicInstruction::ALLOCA:
                        // alloca无操作数需要标记
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // 第三步：删除在可达块中无用的指令
    // 无用指令指：1）无副作用，2）结果寄存器不在useful_regs中
    for(auto &[block_id, block] : *(C->block_map)) {
        if (reachable_blocks.find(block_id) == reachable_blocks.end()) continue;
        for(auto it = block->Instruction_list.begin(); it != block->Instruction_list.end(); ) {
            BasicInstruction* instr = *it;
            int res_reg = GetResultRegNo(instr);
            if (!HasSideEffect(instr)) {
                if (res_reg == -1 || useful_regs.find(res_reg) == useful_regs.end()) {
                    // 删除该无用指令
                    delete instr;
                    it = block->Instruction_list.erase(it);
                    continue;
                }
            }
            ++it;
        }
    }

    // 至此，我们已经删除不可达基本块和无用指令，实现了ADCE
}

void SimpleADCEPass::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        EliminateDeadCode(cfg);
    }
}
