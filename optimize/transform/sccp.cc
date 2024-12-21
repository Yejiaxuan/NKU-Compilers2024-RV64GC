#include "sccp.h"

static std::map<Instruction, ConstLattice> ConstLatticeMap;
static std::map<Instruction, std::vector<Instruction>> SSAG{};    // SSA-Graph
static std::map<int, Instruction> ResultMap;                      //<regno,the instruction that define regno>

 /** 功能: 构建SSA图，用于跟踪变量定义和使用之间的关系
 * 主要步骤:
 * 1. 清空全局映射
 * 2. 记录所有形参到结果映射
 * 3. 第一遍遍历建立寄存器定义点映射
 * 4. 第二遍遍历建立SSA边（使用-定义关系）*/
void SCCPPass::BuildSSAGraph(CFG *C) {
    // 清空全局映射
    ResultMap.clear();
    SSAG.clear();

    // 将形参寄存器加入结果映射
    for (auto formal_param : C->function_def->formals_reg) {
        auto reg_param = static_cast<RegOperand*>(formal_param);
        ResultMap[reg_param->GetRegNo()] = C->function_def;
    }

    // 第一遍: 建立寄存器定义点映射
    for (const auto& [block_id, block] : *C->block_map) {
        // 使用引用避免拷贝
        const auto& instr_list = block->Instruction_list;
        for (const auto& instr : instr_list) {
            int result_reg = instr->GetResultRegNo(); 
            if (result_reg != -1) {
                ResultMap[result_reg] = instr;
            }
        }
    }

    // 第二遍: 建立使用-定义关系(SSA边)
    std::unordered_set<Instruction> processed;
    for (const auto& [block_id, block] : *C->block_map) {
        const auto& instr_list = block->Instruction_list;
        
        for (const auto& instr : instr_list) {
            // 获取指令的操作数
            const auto& operands = instr->GetNonResultOperands();
            
            for (const auto& op : operands) {
                // 只关注寄存器操作数
                if (op->GetOperandType() != BasicOperand::REG) {
                    continue;
                }
                
                auto reg_op = static_cast<RegOperand*>(op);
                int reg_num = reg_op->GetRegNo();
                
                // 查找定义该寄存器的指令
                auto def_instr = ResultMap.find(reg_num);
                if (def_instr == ResultMap.end()) {
                    continue;
                }

                // 如果是函数参数,特殊处理
                if (def_instr->second == C->function_def) {
                    if (!processed.count(instr)) {
                        SSAG[def_instr->second].push_back(instr);
                        processed.insert(instr);
                    }
                } else {
                    // 添加 SSA 边
                    SSAG[def_instr->second].push_back(instr);
                }
            }
        }
    }
}

 /** 功能: 为CFG中的每个指令设置其所属基本块的ID
 * 实现: 使用广度优先搜索遍历CFG，为每个基本块中的指令设置块ID*/
void SCCPPass::SetInstructionBlockID(CFG *C) {
    // 为入口函数设置块ID
    C->function_def->SetBlockID(0);
    
    // 使用的队列和集合
    std::queue<int> block_queue;
    std::unordered_set<int> visited;
    
    // 从入口块开始
    block_queue.push(0);
    
    while (!block_queue.empty()) {
        int current_block_id = block_queue.front();
        block_queue.pop();
        
        // 如果已访问过则跳过
        if (!visited.insert(current_block_id).second) {
            continue;
        }
        
        // 获取当前基本块
        auto current_block = (*C->block_map)[current_block_id];
        
        // 为块内所有指令设置块ID
        for (auto& instruction : current_block->Instruction_list) {
            instruction->SetBlockID(current_block_id);
            
            // 根据指令类型处理后继块
            if (auto br_uncond = dynamic_cast<BrUncondInstruction*>(instruction)) {
                int target_id = static_cast<LabelOperand*>(br_uncond->GetDestLabel())->GetLabelNo();
                if (!visited.count(target_id)) {
                    block_queue.push(target_id);
                }
            }
            else if (auto br_cond = dynamic_cast<BrCondInstruction*>(instruction)) {
                // 获取条件跳转的两个目标
                int true_target = static_cast<LabelOperand*>(br_cond->GetTrueLabel())->GetLabelNo();
                int false_target = static_cast<LabelOperand*>(br_cond->GetFalseLabel())->GetLabelNo();
                
                // 将未访问的目标加入队列
                if (!visited.count(true_target)) {
                    block_queue.push(true_target);
                }
                if (!visited.count(false_target)) {
                    block_queue.push(false_target);
                }
            }
        }
    }
}


/*
 * 功能: 获取操作数的常量格状态
 * 参数: 
 *   - Operand val: 待分析的操作数
 *   - std::map<int, Instruction>& regresult_map: 寄存器到定义指令的映射
 * 返回: 操作数对应的常量格状态
 */
// Reference: https://github.com/yuhuifishash/SysY/blob/master/optimize/propagating/sccp.cc line129-line487
ConstLattice GetOperandLattice(Operand val, std::map<int, Instruction> &regresult_map) {
    if (val->GetOperandType() == val->IMMI32) {
        return ConstLattice(ConstLattice::CONST, ConstLattice::I32, ((ImmI32Operand *)val)->GetIntImmVal());
    }
    if (val->GetOperandType() == val->IMMF32) {
        return ConstLattice(ConstLattice::CONST, ConstLattice::FLOAT, ((ImmF32Operand *)val)->GetFloatVal());
        ;
    }
    if (val->GetOperandType() == val->REG) {
        int regno = ((RegOperand *)val)->GetRegNo();
        auto regno_defIns = regresult_map[regno];
        if (regno_defIns->IsFuncDef()) {
            return ConstLattice(ConstLattice::VAR, ConstLattice::NONE, 0);
        }
        return ConstLatticeMap[regno_defIns];
    }
    return ConstLattice(ConstLattice::UNINIT, ConstLattice::NONE, 0);
}

int LoadInstruction::ConstPropagate(std::map<int, Instruction> &regresult_map) {
    auto &lattice = ConstLatticeMap[this];
    if (lattice.status == lattice.VAR) {
        return 0;
    }
    lattice.status = lattice.VAR;
    return 1;
}

int StoreInstruction::ConstPropagate(std::map<int, Instruction> &regresult_map) { return 0; }

int ArithmeticInstruction::ConstPropagate(std::map<int, Instruction> &regresult_map) {
    auto &lattice = ConstLatticeMap[this];

    auto op1_lattice = GetOperandLattice(op1, regresult_map);
    auto op2_lattice = GetOperandLattice(op2, regresult_map);

    auto op1_lattice_status = op1_lattice.status;
    auto op2_lattice_status = op2_lattice.status;

    if (lattice.status == ConstLattice::VAR) {
        return 0;
    }

    if (op1_lattice_status == ConstLattice::VAR || op2_lattice_status == ConstLattice::VAR || op3 != nullptr) {
        lattice.status = ConstLattice::VAR;
        return 1;
    }

    if (lattice.status == ConstLattice::CONST) {
        return 0;
    }
    if (this->GetDataType() == I32) {
        int op1_lattice_i32val = op1_lattice.vals.I32Val;
        int op2_lattice_i32val = op2_lattice.vals.I32Val;
        lattice.status = ConstLattice::CONST;
        lattice.val_type = ConstLattice::I32;

        if (opcode == ADD) {
            lattice.vals.I32Val = op1_lattice_i32val + op2_lattice_i32val;
        } else if (opcode == SUB) {
            lattice.vals.I32Val = op1_lattice_i32val - op2_lattice_i32val;
        } else if (opcode == MUL) {
            lattice.vals.I32Val = op1_lattice_i32val * op2_lattice_i32val;
        } else if (opcode == DIV) {
            lattice.vals.I32Val = op1_lattice_i32val / op2_lattice_i32val;
        } else if (opcode == MOD) {
            lattice.vals.I32Val = op1_lattice_i32val % op2_lattice_i32val;
        } else if (opcode == UMAX_I32) {
            lattice.vals.I32Val = std::max((uint32_t)op1_lattice_i32val, (uint32_t)op2_lattice_i32val);
        } else if (opcode == UMIN_I32) {
            lattice.vals.I32Val = std::min((uint32_t)op1_lattice_i32val, (uint32_t)op2_lattice_i32val);
        } else if (opcode == SMAX_I32) {
            lattice.vals.I32Val = std::max(op1_lattice_i32val, op2_lattice_i32val);
        } else if (opcode == SMIN_I32) {
            lattice.vals.I32Val = std::min(op1_lattice_i32val, op2_lattice_i32val);
        } else if (opcode == BITXOR) {
            lattice.vals.I32Val = op1_lattice_i32val ^ op2_lattice_i32val;
        } else if (opcode == BITAND) {
            lattice.vals.I32Val = op1_lattice_i32val & op2_lattice_i32val;
        } else {    // should not reach here
            assert(false);
        }
    } else if (this->GetDataType() == FLOAT32) {
        float op1_lattice_floatval = op1_lattice.vals.FloatVal;
        float op2_lattice_floatval = op2_lattice.vals.FloatVal;
        lattice.status = ConstLattice::CONST;
        lattice.val_type = ConstLattice::FLOAT;

        if (opcode == FADD) {
            lattice.vals.FloatVal = op1_lattice_floatval + op2_lattice_floatval;
        } else if (opcode == FSUB) {
            lattice.vals.FloatVal = op1_lattice_floatval - op2_lattice_floatval;
        } else if (opcode == FMUL) {
            lattice.vals.FloatVal = op1_lattice_floatval * op2_lattice_floatval;
        } else if (opcode == FDIV) {
            lattice.vals.FloatVal = op1_lattice_floatval / op2_lattice_floatval;
        } else if (opcode == FMIN_F32) {
            lattice.vals.FloatVal = std::min(op1_lattice_floatval, op2_lattice_floatval);
        } else if (opcode == FMAX_F32) {
            lattice.vals.FloatVal = std::max(op1_lattice_floatval, op2_lattice_floatval);
        } else {    // should not reach here
            assert(false);
        }
    } else {    // should not reach here
        assert(false);
    }

    return 1;
}

int IcmpInstruction::ConstPropagate(std::map<int, Instruction> &regresult_map) {
    auto &lattice = ConstLatticeMap[this];

    auto op1_lattice = GetOperandLattice(op1, regresult_map);
    auto op2_lattice = GetOperandLattice(op2, regresult_map);

    auto op1_lattice_status = op1_lattice.status;
    auto op2_lattice_status = op2_lattice.status;

    if (lattice.status == ConstLattice::VAR) {
        return 0;
    }

    if (op1_lattice_status == 2 || op2_lattice_status == 2) {
        lattice.status = ConstLattice::VAR;
        return 1;
    }

    if (lattice.status == ConstLattice::CONST) {
        return 0;
    }

    int op1_lattice_i32val = op1_lattice.vals.I32Val;
    int op2_lattice_i32val = op2_lattice.vals.I32Val;
    lattice.status = ConstLattice::CONST;
    lattice.val_type = ConstLattice::I32;

    if (cond == eq) {
        lattice.vals.I32Val = op1_lattice_i32val == op2_lattice_i32val;
    } else if (cond == ne) {
        lattice.vals.I32Val = op1_lattice_i32val != op2_lattice_i32val;
    } else if (cond == sgt) {
        lattice.vals.I32Val = op1_lattice_i32val > op2_lattice_i32val;
    } else if (cond == sge) {
        lattice.vals.I32Val = op1_lattice_i32val >= op2_lattice_i32val;
    } else if (cond == slt) {
        lattice.vals.I32Val = op1_lattice_i32val < op2_lattice_i32val;
    } else if (cond == sle) {
        lattice.vals.I32Val = op1_lattice_i32val <= op2_lattice_i32val;
    } else {    // should not reach here
        assert(false);
    }
    return 1;
}

int FcmpInstruction::ConstPropagate(std::map<int, Instruction> &regresult_map) {
    auto &lattice = ConstLatticeMap[this];

    auto op1_lattice = GetOperandLattice(op1, regresult_map);
    auto op2_lattice = GetOperandLattice(op2, regresult_map);

    auto op1_lattice_status = op1_lattice.status;
    auto op2_lattice_status = op2_lattice.status;

    if (lattice.status == ConstLattice::VAR) {
        return 0;
    }

    if (op1_lattice_status == 2 || op2_lattice_status == 2) {
        lattice.status = ConstLattice::VAR;
        return 1;
    }

    if (lattice.status == ConstLattice::CONST) {
        return 0;
    }

    float op1_lattice_floatval = op1_lattice.vals.FloatVal;
    float op2_lattice_floatval = op2_lattice.vals.FloatVal;
    lattice.status = ConstLattice::CONST;
    lattice.val_type = ConstLattice::FLOAT;

    if (this->GetDataType() == FLOAT32) {
        if (cond == OEQ) {
            lattice.vals.I32Val = op1_lattice_floatval == op2_lattice_floatval;
        } else if (cond == ONE) {
            lattice.vals.I32Val = op1_lattice_floatval != op2_lattice_floatval;
        } else if (cond == OGT) {
            lattice.vals.I32Val = op1_lattice_floatval > op2_lattice_floatval;
        } else if (cond == OGE) {
            lattice.vals.I32Val = op1_lattice_floatval >= op2_lattice_floatval;
        } else if (cond == OLT) {
            lattice.vals.I32Val = op1_lattice_floatval < op2_lattice_floatval;
        } else if (cond == OLE) {
            lattice.vals.I32Val = op1_lattice_floatval <= op2_lattice_floatval;
        } else {    // should not reach here
            assert(false);
        }
    } else {    // should not reach here
        assert(false);
    }

    return 1;
}

int CallInstruction::ConstPropagate(std::map<int, Instruction> &regresult_map) {
    auto &lattice = ConstLatticeMap[this];
    if (lattice.status == ConstLattice::VAR) {
        return 0;
    }
    lattice.status = ConstLattice::VAR;
    return 1;
}

int GetElementptrInstruction::ConstPropagate(std::map<int, Instruction> &regresult_map) {
    auto &lattice = ConstLatticeMap[this];
    if (lattice.status == ConstLattice::VAR) {
        return 0;
    }
    lattice.status = ConstLattice::VAR;
    return 1;
}

int FptosiInstruction::ConstPropagate(std::map<int, Instruction> &regresult_map) {
    auto &lattice = ConstLatticeMap[this];

    auto op_lattice = GetOperandLattice(value, regresult_map);
    auto op_lattice_status = op_lattice.status;
    float op_lattice_floatval = op_lattice.vals.FloatVal;

    if (lattice.status == ConstLattice::VAR) {
        return 0;
    }

    if (op_lattice_status == ConstLattice::VAR) {
        lattice.status = ConstLattice::VAR;
        return 1;
    }

    if (lattice.status == ConstLattice::CONST) {
        return 0;
    }

    lattice.status = ConstLattice::CONST;
    lattice.val_type = ConstLattice::I32;
    lattice.vals.I32Val = (int)op_lattice_floatval;

    return 1;
}

int SitofpInstruction::ConstPropagate(std::map<int, Instruction> &regresult_map) {
    auto &lattice = ConstLatticeMap[this];

    auto op_lattice = GetOperandLattice(value, regresult_map);
    auto op_lattice_status = op_lattice.status;
    int op_lattice_i32val = op_lattice.vals.I32Val;

    if (lattice.status == ConstLattice::VAR) {
        return 0;
    }

    if (op_lattice_status == ConstLattice::VAR) {
        lattice.status = ConstLattice::VAR;
        return 1;
    }

    if (lattice.status == ConstLattice::CONST) {
        return 0;
    }

    lattice.status = ConstLattice::CONST;
    lattice.val_type = ConstLattice::FLOAT;
    lattice.vals.FloatVal = (float)op_lattice_i32val;

    return 1;
}

int ZextInstruction::ConstPropagate(std::map<int, Instruction> &regresult_map) {
    auto &lattice = ConstLatticeMap[this];

    auto op_lattice = GetOperandLattice(value, regresult_map);
    auto op_lattice_status = op_lattice.status;
    int op_lattice_i32val = op_lattice.vals.I32Val;

    if (lattice.status == ConstLattice::VAR) {
        return 0;
    }

    if (op_lattice_status == ConstLattice::VAR) {
        lattice.status = ConstLattice::VAR;
        return 1;
    }

    if (lattice.status == ConstLattice::CONST) {
        return 0;
    }

    lattice.status = ConstLattice::CONST;
    lattice.val_type = ConstLattice::I32;
    lattice.vals.I32Val = op_lattice_i32val;

    return 1;
}

/*
 * 功能: 更新phi指令的格状态
 * 参数:
 *   - Instruction I: 待更新的指令
 *   - ConstLattice prev_lattice: 前一个格状态
 * 返回: 布尔值，表示格状态是否发生变化
 */
bool SCCPPass::UpdateLatticeStatus(Instruction I, ConstLattice prev_lattice) {
    auto& curr_lattice = ConstLatticeMap[I];

    // Case 1: 当前格为未初始化状态
    if (curr_lattice.status == ConstLattice::UNINIT) {
        curr_lattice = prev_lattice;
        return true;
    }

    // Case 2: 当前是常量,但新状态是变量
    if (curr_lattice.status == ConstLattice::CONST && 
        prev_lattice.status == ConstLattice::VAR) {
        curr_lattice.status = ConstLattice::VAR;
        return true;
    }

    // Case 3: 两个都是常量但值不同 
    if (curr_lattice.status == ConstLattice::CONST && 
        prev_lattice.status == ConstLattice::CONST) {
        
        const bool values_differ = [&]() {
            if (curr_lattice.val_type == ConstLattice::I32) {
                return curr_lattice.vals.I32Val != prev_lattice.vals.I32Val;
            }
            if (curr_lattice.val_type == ConstLattice::FLOAT) {
                return curr_lattice.vals.FloatVal != prev_lattice.vals.FloatVal;
            }
            return false;
        }();

        if (values_differ) {
            curr_lattice.status = ConstLattice::VAR;
            return true;
        }
    }

    return false;
}

/*
 * 功能: 访问并处理单个指令，更新其常量状态和相关工作列表
 * 参数:
 *   - Instruction I: 待访问的指令
 *   - SSAWorklist: SSA工作列表
 *   - CFGedgeExec: CFG边执行状态映射
 *   - CFGWorklist: CFG工作列表
 *   - regresult_map: 寄存器结果映射
 *   - SSA_G: SSA图
 */
void SCCPPass::VisitOperation(
    Instruction I, 
    std::set<Instruction>& SSAWorklist,
    std::map<std::pair<int, int>, int>& CFGedgeExec,
    std::set<std::pair<int, int>>& CFGWorklist,
    std::map<int, Instruction>& regresult_map,
    std::map<Instruction, std::vector<Instruction>>& SSA_G) {

    // 处理 PHI 指令
    if (auto phi = dynamic_cast<PhiInstruction*>(I)) {
        bool changed = false;
        for (const auto& [label, value] : phi->GetPhiList()) {
            int pred_block = static_cast<LabelOperand*>(label)->GetLabelNo();
            int curr_block = I->GetBlockID();
            
            // 检查控制流边是否可执行
            if (CFGedgeExec.count({pred_block, curr_block})) {
                auto operand_lattice = GetOperandLattice(value, regresult_map);
                changed |= UpdateLatticeStatus(I, operand_lattice);
            }
        }

        // 如果格状态发生变化,更新SSA工作表
        if (changed) {
            SSAWorklist.insert(SSA_G[I].begin(), SSA_G[I].end());
        }
        return;
    }

    // 处理条件分支指令
    if (auto br_cond = dynamic_cast<BrCondInstruction*>(I)) {
        auto cond_lattice = GetOperandLattice(br_cond->GetCond(), regresult_map);
        
        if (cond_lattice.status == ConstLattice::CONST) {
            // 确定性分支
            int target_block = cond_lattice.vals.I32Val ? 
                static_cast<LabelOperand*>(br_cond->GetTrueLabel())->GetLabelNo() :
                static_cast<LabelOperand*>(br_cond->GetFalseLabel())->GetLabelNo();

            if (!CFGedgeExec.count({I->GetBlockID(), target_block})) {
                CFGWorklist.emplace(I->GetBlockID(), target_block);
            }
        }
        else if (cond_lattice.status == ConstLattice::VAR) {
            // 不确定分支,两个路径都可能执行
            auto true_target = static_cast<LabelOperand*>(br_cond->GetTrueLabel())->GetLabelNo();
            auto false_target = static_cast<LabelOperand*>(br_cond->GetFalseLabel())->GetLabelNo();

            if (!CFGedgeExec.count({I->GetBlockID(), true_target})) {
                CFGWorklist.emplace(I->GetBlockID(), true_target);
            }
            if (!CFGedgeExec.count({I->GetBlockID(), false_target})) {
                CFGWorklist.emplace(I->GetBlockID(), false_target); 
            }
        }
        return;
    }

    // 处理其他指令
    if (I->ConstPropagate(regresult_map)) {
        SSAWorklist.insert(SSA_G[I].begin(), SSA_G[I].end());
    }
}

/*
 * 功能: 将可以确定为常量的寄存器替换为对应的常量值
 * 参数: CFG* C - 控制流图指针
 * 实现: 遍历所有指令，将常量寄存器替换为实际常量值
 */
void SCCPPass::ReplaceRegToConst(CFG *C) {
    std::queue<int> block_queue;
    std::unordered_set<int> visited;
    
    block_queue.push(0);
    visited.insert(0);
    
    while (!block_queue.empty()) {
        int current_block_id = block_queue.front();
        block_queue.pop();
        
        auto current_block = (*C->block_map)[current_block_id];
        
        for (auto& instruction : current_block->Instruction_list) {
            // 替换操作数部分
            auto operands = instruction->GetNonResultOperands();
            
            for (auto& op : operands) {
                if (op->GetOperandType() != BasicOperand::REG) {
                    continue;
                }
                
                auto reg_op = static_cast<RegOperand*>(op);
                auto def_instr = ResultMap[reg_op->GetRegNo()];
                auto& lattice = ConstLatticeMap[def_instr];
                
                if (lattice.status == ConstLattice::CONST) {
                    if (lattice.val_type == ConstLattice::I32) {
                        op = new ImmI32Operand(lattice.vals.I32Val);
                    } 
                    else if (lattice.val_type == ConstLattice::FLOAT) {
                        op = new ImmF32Operand(lattice.vals.FloatVal);
                    }
                }
            }
            
            instruction->SetNonResultOperands(operands);
            
            // 处理后继块部分
            if (auto br_uncond = dynamic_cast<BrUncondInstruction*>(instruction)) {
                int target = static_cast<LabelOperand*>(br_uncond->GetDestLabel())->GetLabelNo();
                if (!visited.count(target)) {
                    block_queue.push(target);
                    visited.insert(target);
                }
            }
            else if (auto br_cond = dynamic_cast<BrCondInstruction*>(instruction)) {
                int true_target = static_cast<LabelOperand*>(br_cond->GetTrueLabel())->GetLabelNo();
                int false_target = static_cast<LabelOperand*>(br_cond->GetFalseLabel())->GetLabelNo();
                
                if (!visited.count(true_target)) {
                    block_queue.push(true_target);
                    visited.insert(true_target);
                }
                if (!visited.count(false_target)) {
                    block_queue.push(false_target);
                    visited.insert(false_target);
                }
            }
        }
    }
}

/*
 * 功能: SCCP优化后的死块消除
 * 参数:
 *   - CFG* C: 控制流图指针
 *   - CFGedgeExec: CFG边执行状态映射
 * 实现: 删除不可达的基本块，更新相关跳转指令
 */
void SCCPPass::DeadBlockEliminateAfterSCCP(CFG *C, std::map<std::pair<int, int>, int> &CFGedgeExec) {
    std::map<int, int> block_ref_cnt;
    block_ref_cnt[0] = 1;
    for (auto [from_block_id, from_block] : *C->block_map) {
        for (auto dest_block : C->G[from_block_id]) {
            auto dest_block_id = dest_block->block_id;
            auto execute = CFGedgeExec[std::make_pair(from_block_id, dest_block_id)];
            if (execute == 0) {
                auto &from_block = (*C->block_map)[from_block_id];
                auto &dest_block = (*C->block_map)[dest_block_id];
                auto &from_block_instrlist = from_block->Instruction_list;
                auto &last_br = from_block_instrlist[from_block_instrlist.size() - 1];
                if (last_br->GetOpcode() == BasicInstruction::BR_COND) {
                    auto br_cond = (BrCondInstruction *)last_br;
                    auto br_true = (LabelOperand *)br_cond->GetTrueLabel();
                    auto br_false = (LabelOperand *)br_cond->GetFalseLabel();
                    auto br_uncond_label = br_true;
                    if (br_true->GetLabelNo() == dest_block_id) {
                        br_uncond_label = br_false;
                    } else if (br_false->GetLabelNo() == dest_block_id) {
                        br_uncond_label = br_true;
                    }
                    last_br = new BrUncondInstruction(br_uncond_label);
                    last_br->SetBlockID(from_block_id);
                }
                for (auto &I : dest_block->Instruction_list) {
                    if (I->GetOpcode() != BasicInstruction::PHI) {
                        break;
                    }
                    auto PhiI = (PhiInstruction *)I;
                    PhiI->ErasePhi(from_block->block_id);
                }
            } else {
                block_ref_cnt[dest_block_id] = block_ref_cnt[dest_block_id] + 1;
            }
        }
    }
    std::queue<int> del_queue;
    for (auto [id, bb] : *C->block_map) {
        if (block_ref_cnt[id] == 0) {
            del_queue.push(id);
        }
    }
    while (!del_queue.empty()) {
        auto del_id = del_queue.front();
        del_queue.pop();
        delete (*C->block_map)[del_id];
        C->block_map->erase(del_id);
    }
}

/*
 * 功能: 删除已经被确定为常量的指令
 * 参数: CFG* C - 控制流图指针
 * 实现: 遍历所有基本块，删除状态为CONST的指令
 */
void SCCPPass::ConstInstructionEliminate(CFG *C) {
    for (auto [id, bb] : *C->block_map) {
        auto tmp_ins_list = bb->Instruction_list;
        bb->Instruction_list.clear();
        for (auto I : tmp_ins_list) {
            auto lattice = ConstLatticeMap[I];
            if (lattice.status != ConstLattice::CONST) {
                bb->InsertInstruction(1, I);
            }
        }
    }
}

/*
 * 功能: 执行稀疏条件常量传播优化的主函数
 * 参数: CFG* C - 控制流图指针
 * 主要步骤:
 * 1. 初始化所有指令的常量状态
 * 2. 迭代处理SSA和CFG工作列表
 * 3. 替换常量和删除死代码
 * 4. 清理数据结构
 */
void SCCPPass::PerformSCCP(CFG *C) {
    // 1. 初始化数据结构
    std::map<std::pair<int, int>, int> CFGedgeExec;  // 记录CFG边的执行状态
    std::set<std::pair<int, int>> CFGWorklist;       // CFG工作列表
    std::set<Instruction> SSAWorklist;               // SSA工作列表 
    std::vector<int> blockVisited(C->max_label + 1); // 记录块是否访问过

    // 初始化所有指令的格状态
    for (const auto& [_, block] : *C->block_map) {
        for (auto instr : block->Instruction_list) {
            ConstLatticeMap[instr] = ConstLattice();
        }
    }

    // 2. 从入口块开始传播
    CFGWorklist.insert({-1, 0});  // 添加入口边

    // 主循环:不断处理SSA和CFG工作列表直到收敛
    while (!SSAWorklist.empty() || !CFGWorklist.empty()) {
        if (!SSAWorklist.empty()) {  // 优先处理SSA工作列表
            // 取出一条指令进行处理
            auto instr = *SSAWorklist.begin();
            SSAWorklist.erase(SSAWorklist.begin());

            // 对PHI指令特殊处理
            if (instr->GetOpcode() == BasicInstruction::PHI) {
                VisitOperation(instr, SSAWorklist, CFGedgeExec, CFGWorklist, ResultMap, SSAG);
                continue;
            }

            // 对普通指令,只有当其所在块可达时才处理
            int block_id = instr->GetBlockID();
            if (block_id == 0) {  // 入口块总是可达
                VisitOperation(instr, SSAWorklist, CFGedgeExec, CFGWorklist, ResultMap, SSAG);
            } else {
                // 检查是否有可达的前驱块
                for (auto pred : C->invG[block_id]) {
                    if (CFGedgeExec.count({pred->block_id, block_id})) {
                        VisitOperation(instr, SSAWorklist, CFGedgeExec, CFGWorklist, ResultMap, SSAG);
                        break;
                    }
                }
            }
        } else {  // 处理CFG工作列表
            // 取出一条CFG边进行处理
            auto edge = *CFGWorklist.begin();
            CFGWorklist.erase(CFGWorklist.begin());
            CFGedgeExec[edge] = 1;

            // 获取目标块
            int target_id = edge.second;
            auto target_block = (*C->block_map)[target_id];
            bool is_first_visit = !blockVisited[target_id];
            
            if (is_first_visit) {
                blockVisited[target_id] = 1;
            }

            // 处理块内指令
            if (!target_block->Instruction_list.empty()) {
                for (auto instr : target_block->Instruction_list) {
                    // 对PHI指令或首次访问的块中的指令进行处理
                    if (instr->GetOpcode() == BasicInstruction::PHI || is_first_visit) {
                        VisitOperation(instr, SSAWorklist, CFGedgeExec, CFGWorklist, ResultMap, SSAG);
                    }
                    // 非PHI指令且非首次访问时停止处理
                    else if (!is_first_visit) {
                        break;
                    }
                }

                // 处理无条件跳转
                auto last_instr = target_block->Instruction_list.back();
                if (last_instr->GetOpcode() == BasicInstruction::BR_UNCOND) {
                    auto br_uncond = static_cast<BrUncondInstruction*>(last_instr);
                    auto next_edge = std::make_pair(target_id, br_uncond->GetTarget());
                    
                    if (!CFGedgeExec.count(next_edge)) {
                        CFGWorklist.insert(next_edge);
                    }
                }
            }
        }
    }

    // 3. 应用优化结果
    ReplaceRegToConst(C);         // 替换寄存器为常量
    ConstInstructionEliminate(C); // 删除常量指令
    DeadBlockEliminateAfterSCCP(C, CFGedgeExec); // 消除死块

    // 4. 清理工作数据
    ConstLatticeMap.clear();
    ResultMap.clear();
    SSAG.clear();
}

/*
 * 功能: SCCP优化Pass的入口函数
 * 实现步骤:
 * 1. 对每个函数的CFG进行优化
 * 2. 检查并修复返回块
 * 3. 清理不可达代码
 * 4. 构建支配树
 */
void SCCPPass::Execute() {
    for (auto& [_, C] : llvmIR->llvm_cfg) {
        // 第一步: 初始化CFG
        SetInstructionBlockID(C);
        BuildSSAGraph(C);
        PerformSCCP(C);

        // 第二步: 检查并修复返回块
        bool has_return = false;
        for (const auto& [_, block] : *C->block_map) {
            if (block->Instruction_list.back()->GetOpcode() == BasicInstruction::RET) {
                has_return = true;
                break;
            }
        }

        if (!has_return) {
            C->block_map->clear();
            C->max_label = -1;
            C->max_reg = -1;
            
            auto new_block = C->NewBlock();
            auto ret_type = C->function_def->GetReturnType();
            
            Instruction ret_instr = nullptr;
            switch (ret_type) {
                case BasicInstruction::VOID: 
                    ret_instr = new RetInstruction(ret_type, nullptr); 
                    break;
                case BasicInstruction::I32:
                    ret_instr = new RetInstruction(ret_type, new ImmI32Operand(0));
                    break;    
                case BasicInstruction::FLOAT32:
                    ret_instr = new RetInstruction(ret_type, new ImmF32Operand(0));
                    break;
                default:
                    ERROR("Invalid return type");
            }
            
            new_block->InsertInstruction(1, ret_instr);
            C->ret_block = new_block;
        }

        C->BuildCFG();

        // 第三步: 清理不可达代码
        std::vector<bool> reachable(C->max_label + 1);
        {
            std::queue<LLVMBlock> work_list;
            work_list.push(C->ret_block);

            while (!work_list.empty()) {
                auto current = work_list.front();
                work_list.pop();
                
                if (reachable[current->block_id]) continue;
                
                reachable[current->block_id] = true;
                for (auto pred : C->GetPredecessor(current->block_id)) {
                    if (!reachable[pred->block_id]) {
                        work_list.push(pred);
                    }
                }
            }
        }

        // 删除不可达块并更新分支
        for (auto it = C->block_map->begin(); it != C->block_map->end();) {
            if (!reachable[it->second->block_id]) {
                it = C->block_map->erase(it);
                continue;
            }

            // 更新条件分支
            auto last_instr = it->second->Instruction_list.back();
            if (auto br_cond = dynamic_cast<BrCondInstruction*>(last_instr)) {
                int false_target = static_cast<LabelOperand*>(br_cond->GetFalseLabel())->GetLabelNo();
                int true_target = static_cast<LabelOperand*>(br_cond->GetTrueLabel())->GetLabelNo();

                if (!reachable[false_target]) {
                    it->second->Instruction_list.back() = new BrUncondInstruction(br_cond->GetTrueLabel());
                } else if (!reachable[true_target]) {
                    it->second->Instruction_list.back() = new BrUncondInstruction(br_cond->GetFalseLabel());
                }
            }
            ++it;
        }

        C->BuildCFG();

        // 第四步: 构建支配树
        {
            DominatorTree dom_tree;
            dom_tree.C = C;
            dom_tree.BuildDominatorTree(false);

            DominatorTree post_dom_tree; 
            post_dom_tree.C=C;
            post_dom_tree.BuildDominatorTree(true);
        }
    }
}
