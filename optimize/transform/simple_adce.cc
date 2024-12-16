#include "simple_adce.h"
#include <iostream>
#include <cassert>

/**
 * @brief 构建控制依赖图（CDG）。
 *
 * 利用支配树的支配前沿（Dominance Frontier）来构建控制依赖图。
 *
 * @param C 指向当前处理的控制流图（CFG）。
 */
void ADCEPass::BuildControlDependence(CFG *C) {
    DominatorTree *dom_tree = dom_analysis->GetDomTree(C);
    control_dependence.clear();
    for(auto &[y_id, y_bb] : *(C->block_map)) {
        // 获取基本块 y 的支配前沿
        std::set<int> df = dom_tree->GetDF(y_id);
        for(auto x_id : df) {
            // 基本块 x 在基本块 y 的支配前沿上，因此 x 控制依赖于 y
            control_dependence[y_id].insert(x_id);
        }
    }
}

/**
 * @brief 计算基本块中活跃的指令集合。
 *
 * 根据 ADCE 算法，通过迭代标记和传播活跃指令来识别哪些指令是有用的。
 *
 * @param C 指向当前处理的控制流图（CFG）。
 * @return 返回活跃指令的集合。
 */
std::set<BasicInstruction*> ADCEPass::ComputeLiveInstructions(CFG *C) {
    std::set<BasicInstruction*> live;          // 活跃的指令集合
    std::queue<BasicInstruction*> worklist;    // 工作列表，用于迭代处理
    std::map<Operand, BasicInstruction*> defMap; // 定义映射：操作数（寄存器） -> 定义它的指令

    // 步骤 1：构建 defMap 并初始化工作列表
    for(auto &[block_id, bb] : *(C->block_map)) {
        for(auto &instr_ptr : bb->Instruction_list) {
            BasicInstruction* instr = instr_ptr; // 假设 Instruction 是 BasicInstruction*
            
            // 如果指令有结果寄存器，记录其定义关系
            Operand res = instr->GetResultReg();
            if(res && res->GetOperandType() == BasicOperand::REG) {
                defMap[res] = instr;
            }

            // 将所有有副作用的指令加入工作列表
            if(instr->GetOpcode() == BasicInstruction::STORE ||
               instr->GetOpcode() == BasicInstruction::CALL ||
               instr->GetOpcode() == BasicInstruction::RET ||
               instr->GetOpcode() == BasicInstruction::BR_COND ||
               instr->GetOpcode() == BasicInstruction::BR_UNCOND ||
               instr->GetOpcode() == BasicInstruction::ALLOCA ||
               instr->GetOpcode() == BasicInstruction::GETELEMENTPTR ||
               instr->GetOpcode() == BasicInstruction::GLOBAL_VAR ||
               instr->GetOpcode() == BasicInstruction::GLOBAL_STR) {
                worklist.push(instr);
                live.insert(instr);
            }
        }
    }

    // 步骤 2：迭代标记活跃指令
    while(!worklist.empty()) {
        BasicInstruction* inst = worklist.front();
        worklist.pop();

        // 获取指令的所有使用（use）
        std::vector<Operand> uses = inst->GetNonResultOperands();
        for(auto &use : uses) {
            if(use->GetOperandType() == BasicOperand::REG) {
                // 查找定义该操作数的指令
                auto it = defMap.find(use);
                if(it != defMap.end()) {
                    BasicInstruction* def_inst = it->second;
                    if(live.find(def_inst) == live.end()) {
                        worklist.push(def_inst);
                        live.insert(def_inst);
                    }
                }
            }
        }

        // 如果是 phi 指令，标记所有前驱块的终结指令为活跃
        if(inst->GetOpcode() == BasicInstruction::PHI) {
            PhiInstruction* phiInst = dynamic_cast<PhiInstruction*>(inst);
            if(phiInst) {
                for(auto &[val, label] : phiInst->GetPhiList()) {
                    if(label->GetOperandType() == BasicOperand::LABEL) {
                        int label_no = dynamic_cast<LabelOperand*>(label)->GetLabelNo();
                        auto pred_block_it = C->block_map->find(label_no);
                        if(pred_block_it != C->block_map->end()) {
                            LLVMBlock pred_bb = pred_block_it->second;
                            if(!pred_bb->Instruction_list.empty()) {
                                BasicInstruction* pred_terminator = pred_bb->Instruction_list.back();
                                if(live.find(pred_terminator) == live.end()) {
                                    worklist.push(pred_terminator);
                                    live.insert(pred_terminator);
                                }
                            }
                        }
                    }
                }
            }
        }

        // 步骤 3：添加控制依赖的终结指令到工作列表
        int block_id = inst->GetBlockID();
        auto cdg_it = control_dependence.find(block_id);
        if(cdg_it != control_dependence.end()) {
            for(auto dep_block_id : cdg_it->second) {
                auto dep_block_it = C->block_map->find(dep_block_id);
                if(dep_block_it != C->block_map->end()) {
                    LLVMBlock dep_block = dep_block_it->second;
                    if(!dep_block->Instruction_list.empty()) {
                        BasicInstruction* dep_terminator = dep_block->Instruction_list.back();
                        if(live.find(dep_terminator) == live.end()) {
                            worklist.push(dep_terminator);
                            live.insert(dep_terminator);
                        }
                    }
                }
            }
        }
    }

    return live;
}

/**
 * @brief 执行 ADCE 优化。
 *
 * 对每个函数的控制流图执行 ADCE，以删除死代码。
 */
void ADCEPass::Execute() {
    for(auto &[defI, cfg] : llvmIR->llvm_cfg) {
        // 步骤 1：构建控制依赖图（CDG）
        BuildControlDependence(cfg);

        // 步骤 2：计算活跃的指令
        std::set<BasicInstruction*> live = ComputeLiveInstructions(cfg);

        // 步骤 3：删除未标记为活跃的指令
        for(auto &[block_id, bb] : *(cfg->block_map)) {
            std::deque<BasicInstruction*> new_inst_list;
            for(auto &instr_ptr : bb->Instruction_list) {
                BasicInstruction* instr = instr_ptr;
                if(live.find(instr) != live.end()) {
                    new_inst_list.push_back(instr);
                }
                else {
                    // 删除指令并释放内存
                    delete instr;
                }
            }
            // 替换 Instruction_list 为 new_inst_list
            bb->Instruction_list = new_inst_list;
        }

        // 可选步骤：移除空的基本块（如果有需要）
        // 这里不实现，依据具体需求添加
    }
}

