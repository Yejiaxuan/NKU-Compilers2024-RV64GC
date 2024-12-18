#include "../../include/Instruction.h"
#include "../../include/ir.h"
#include <assert.h>

extern std::map<FuncDefInstruction, int> max_label_map;
extern std::map<FuncDefInstruction, int> max_reg_map;
std::map<std::string, CFG *> CFGMap;
void LLVMIR::CFGInit() {
    for (auto &[defI, bb_map] : function_block_map) {
        CFG *cfg = new CFG();
        cfg->block_map = &bb_map;
        cfg->function_def = defI;
        cfg->max_reg = max_reg_map[defI];
        cfg->max_label = max_label_map[defI];
        llvm_cfg[defI] = cfg;
        CFGMap[defI->GetFunctionName()] = cfg;
        cfg->BuildCFG();
        //TODO("init your members in class CFG if you need");
        
    }
}

void LLVMIR::BuildCFG() {
    for (auto [defI, cfg] : llvm_cfg) {
        cfg->BuildCFG();
    }
}

void CFG::BuildCFG() {
    // 清空现有的邻接表
    G.clear();
    invG.clear();

    // 步骤 1：调整 G 和 invG 的大小以适应所有的基本块 ID
    G.resize(max_label + 1);
    invG.resize(max_label + 1);

    // 步骤 2：遍历所有基本块并确定后继基本块
    for (auto &[block_id, block] : *block_map) {
        std::vector<int> successors;
        Instruction lastIns = block->Instruction_list[block->Instruction_list.size() - 1];
        // 判断最后一条指令的类型
        BasicInstruction::LLVMIROpcode opcode = static_cast<BasicInstruction::LLVMIROpcode>(lastIns->GetOpcode());

        switch (opcode) {
            case BasicInstruction::BR_COND: {
                // 条件跳转指令
                BrCondInstruction *brInst = (BrCondInstruction *)lastIns;
                int trueBlockID= ((LabelOperand *)brInst->GetTrueLabel())->GetLabelNo();
                int falseBlockID = ((LabelOperand *)brInst->GetFalseLabel())->GetLabelNo();;
                successors.push_back(trueBlockID);
                successors.push_back(falseBlockID);
                break;
            }
            case BasicInstruction::BR_UNCOND: {
                // 无条件跳转指令
                BrUncondInstruction *brInst = (BrUncondInstruction *)lastIns;
                int destBlockID = ((LabelOperand *)brInst->GetDestLabel())->GetLabelNo();
                successors.push_back(destBlockID);
                break;
            }
            case BasicInstruction::RET: {
                // 返回指令，存储到返回块集合中
                ret_block = block;
                break;
            }
        }

        // 更新邻接表 G 和 invG
        for (int succ_id : successors) {
            if (succ_id >= 0 && succ_id < (int)G.size()) {
                LLVMBlock succ_block = (*block_map)[succ_id];
                G[block_id].push_back(succ_block);
                invG[succ_id].push_back(block);
            }
        }
    }
    //TODO("BuildCFG");
}
std::vector<LLVMBlock> CFG::GetPredecessor(LLVMBlock B) { return invG[B->block_id]; }

std::vector<LLVMBlock> CFG::GetPredecessor(int bbid) { return invG[bbid]; }

std::vector<LLVMBlock> CFG::GetSuccessor(LLVMBlock B) { return G[B->block_id]; }

std::vector<LLVMBlock> CFG::GetSuccessor(int bbid) { return G[bbid]; }
