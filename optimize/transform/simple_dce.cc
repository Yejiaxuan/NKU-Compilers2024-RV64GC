#include "simple_dce.h"
#include <stack>
#include <unordered_map>
#include <bitset>
#include <deque>
#include <unordered_set>

void SimpleDCEPass::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        EliminateDeadCode(cfg);
    }
}

// 判断指令是否有副作用
bool HasSideEffect(BasicInstruction* instr) {
    switch (instr->GetOpcode()) {
        case BasicInstruction::STORE:
        case BasicInstruction::CALL:
        case BasicInstruction::RET:
        case BasicInstruction::BR_COND:
        case BasicInstruction::BR_UNCOND:
        case BasicInstruction::ALLOCA:
        case BasicInstruction::GETELEMENTPTR:
        case BasicInstruction::GLOBAL_VAR:    // 添加全局变量定义
        case BasicInstruction::GLOBAL_STR:    // 添加全局字符串定义
            return true;
        default:
            return false;
    }
}

// 死代码消除函数
void SimpleDCEPass::EliminateDeadCode(CFG *C) {
    // 构建定义链：reg_no -> 定义该寄存器的指令
    std::unordered_map<int, BasicInstruction*> def_chain;
    for(auto &[block_id, block] : *(C->block_map)) {
        for(auto &instr : block->Instruction_list) {
            int res_reg = instr->GetResultRegNo();
            if(res_reg != -1) {
                // 在 SSA 形式下，每个寄存器只有一个定义
                if(def_chain.find(res_reg) != def_chain.end()) {
                    ERROR("Register %d has multiple definitions.", res_reg);
                }
                def_chain[res_reg] = instr;
            }
        }
    }

    // 集合来存储有用的指令
    std::unordered_set<BasicInstruction*> useful_instructions;
    // 队列来处理有用的指令
    std::queue<BasicInstruction*> worklist;

    // 步骤1：初始化工作列表，将所有具有副作用的指令标记为有用
    for(auto &[block_id, block] : *(C->block_map)) {
        for(auto &instr : block->Instruction_list) {
            if (HasSideEffect(instr)) {
                if (useful_instructions.find(instr) == useful_instructions.end()) {
                    useful_instructions.insert(instr);
                    worklist.push(instr);
                }
            }
        }
    }

    // 步骤2：迭代标记有用的指令
    while(!worklist.empty()) {
        BasicInstruction* instr = worklist.front();
        worklist.pop();

        // 获取指令使用的所有寄存器号
        std::vector<int> used_regs = instr->GetUsedRegisters();
        for(auto reg_no : used_regs) {
            if(def_chain.find(reg_no) != def_chain.end()) {
                BasicInstruction* def_instr = def_chain[reg_no];
                if(useful_instructions.find(def_instr) == useful_instructions.end()) {
                    useful_instructions.insert(def_instr);
                    worklist.push(def_instr);
                }
            }
        }
    }

    // 步骤3：删除未标记为有用的指令
    for(auto &[block_id, block] : *(C->block_map)) {
        for(auto it = block->Instruction_list.begin(); it != block->Instruction_list.end(); ) {
            BasicInstruction* instr = *it;
            if(useful_instructions.find(instr) == useful_instructions.end()) {
                // 无用的指令，删除
                delete instr;
                it = block->Instruction_list.erase(it);
            }
            else {
                // 有用的指令，保留
                ++it;
            }
        }
    }
}
