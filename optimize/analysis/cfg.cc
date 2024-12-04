#include "../../include/Instruction.h"
#include "../../include/ir.h"
#include <assert.h>

void LLVMIR::CFGInit() {
    for (auto &[defI, bb_map] : function_block_map) {
        CFG *cfg = new CFG();
        cfg->block_map = &bb_map;
        cfg->function_def = defI;
        cfg->BuildCFG();
        //TODO("init your members in class CFG if you need");
        llvm_cfg[defI] = cfg;
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

    // 步骤 1：确定最大的基本块 ID 以调整 G 和 invG 的大小
    int max_block_id = 0;
    for(auto &[block_id, block] : *block_map){
        if(block_id > max_block_id) max_block_id = block_id;
    }

    // 调整 G 和 invG 的大小以适应所有的基本块 ID
    G.resize(max_block_id + 1);
    invG.resize(max_block_id + 1);

    // 步骤 2：遍历所有基本块并确定后继基本块
    for(auto &[block_id, block] : *block_map){
        // 获取当前基本块的指令列表
        auto &instructions = block->Instruction_list;

        /*if(instructions.empty()){
            continue; // 跳过空的基本块
        }*/

        // 获取当前基本块的最后一条指令
        Instruction lastIns = instructions.back();

        // 获取最后一条指令的操作码
        BasicInstruction::LLVMIROpcode opcode = static_cast<BasicInstruction::LLVMIROpcode>(lastIns->GetOpcode());

        std::vector<int> successors;

        switch(opcode){
            case BasicInstruction::BR_COND: {
                // 条件跳转指令
                BrCondInstruction *brInst = dynamic_cast<BrCondInstruction *>(lastIns);
                if(brInst){
                    Operand trueLabel = brInst->GetTrueLabel();
                    Operand falseLabel = brInst->GetFalseLabel();

                    LabelOperand *trueLabelOp = dynamic_cast<LabelOperand *>(trueLabel);
                    LabelOperand *falseLabelOp = dynamic_cast<LabelOperand *>(falseLabel);

                    if(trueLabelOp && falseLabelOp){
                        int trueBlockID = trueLabelOp->GetLabelNo();
                        int falseBlockID = falseLabelOp->GetLabelNo();

                        // 确保目标基本块存在
                        if(block_map->find(trueBlockID) != block_map->end()){
                            successors.push_back(trueBlockID);
                        }
                        if(block_map->find(falseBlockID) != block_map->end()){
                            successors.push_back(falseBlockID);
                        }
                    }
                }
                //break;
            }
            case BasicInstruction::BR_UNCOND: {
                // 无条件跳转指令
                BrUncondInstruction *brInst = dynamic_cast<BrUncondInstruction *>(lastIns);
                if(brInst){
                    Operand destLabel = brInst->GetDestLabel();
                    LabelOperand *destLabelOp = dynamic_cast<LabelOperand *>(destLabel);
                    if(destLabelOp){
                        int destBlockID = destLabelOp->GetLabelNo();
                        // 确保目标基本块存在
                        if(block_map->find(destBlockID) != block_map->end()){
                            successors.push_back(destBlockID);
                        }
                    }
                }
                //break;
            }
            case BasicInstruction::RET: {
                // 返回指令，没有后继基本块
                //break;
            }
            default: {
                // 假设控制流顺序流向下一个基本块
                int nextBlockID = block_id + 1;
                if(block_map->find(nextBlockID) != block_map->end()){
                    successors.push_back(nextBlockID);
                }
                //break;
            }
        }

        // 步骤 3：使用后继基本块更新 G 和 invG
        for(int succ_id : successors){
            if(succ_id >= 0 && succ_id < (int)G.size()){
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
