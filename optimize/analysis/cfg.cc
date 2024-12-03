#include "../../include/Instruction.h"
#include "../../include/ir.h"
#include <assert.h>

void LLVMIR::CFGInit() {
    for (auto &[defI, bb_map] : function_block_map) {
        CFG *cfg = new CFG();
        cfg->block_map = &bb_map;
        cfg->function_def = defI;
        cfg->BuildCFG();
        TODO("init your members in class CFG if you need");
        llvm_cfg[defI] = cfg;
    }
}

void LLVMIR::BuildCFG() {
    for (auto [defI, cfg] : llvm_cfg) {
        cfg->BuildCFG();
    }
}

void CFG::BuildCFG() { 
    // 获取基本块的数量
    int numBlocks = block_map->size();

    // 调整邻接表的大小
    G.resize(numBlocks);
    invG.resize(numBlocks);

    // 遍历所有的基本块
    for (auto &[block_id, block] : *block_map) {
        // 获取当前基本块的指令列表
        auto &instructions = block->Instruction_list;

        if (instructions.empty()) {
            continue; // 如果基本块没有指令，跳过
        }

        // 获取基本块的最后一条指令
        Instruction lastInst = instructions.back();

        // 存储该基本块的后继基本块ID
        std::vector<int> successors;

        // 获取指令的操作码
        BasicInstruction::LLVMIROpcode opcode = static_cast<BasicInstruction::LLVMIROpcode>(lastInst->GetOpcode());

        switch (opcode) {
            case BasicInstruction::BR_COND: {
                // 条件跳转指令
                BrCondInstruction *brInst = dynamic_cast<BrCondInstruction *>(lastInst);
                if (brInst) {
                    Operand trueLabel = brInst->GetTrueLabel();
                    Operand falseLabel = brInst->GetFalseLabel();

                    int trueBlockID = dynamic_cast<LabelOperand *>(trueLabel)->GetLabelNo();
                    int falseBlockID = dynamic_cast<LabelOperand *>(falseLabel)->GetLabelNo();

                    successors.push_back(trueBlockID);
                    successors.push_back(falseBlockID);
                }
                break;
            }
            case BasicInstruction::BR_UNCOND: {
                // 无条件跳转指令
                BrUncondInstruction *brInst = dynamic_cast<BrUncondInstruction *>(lastInst);
                if (brInst) {
                    Operand destLabel = brInst->GetDestLabel();
                    int destBlockID = dynamic_cast<LabelOperand *>(destLabel)->GetLabelNo();

                    successors.push_back(destBlockID);
                }
                break;
            }
            case BasicInstruction::RET: {
                // 返回指令，没有后继
                // 不需要添加任何后继
                break;
            }
            default: {
                // 对于其他指令，如果没有显式的跳转，我们假设控制流顺序流向下一个基本块
                int nextBlockID = block_id + 1;
                if (block_map->find(nextBlockID) != block_map->end()) {
                    successors.push_back(nextBlockID);
                }
                break;
            }
        }

        // 构建邻接表和逆邻接表
        for (int succ_id : successors) {
            LLVMBlock succ_block = (*block_map)[succ_id];

            // 在控制流图中添加边
            G[block_id].push_back(succ_block);

            // 在逆控制流图中添加边
            invG[succ_id].push_back(block);
        }
    }
    //TODO("BuildCFG"); 
}

std::vector<LLVMBlock> CFG::GetPredecessor(LLVMBlock B) { return invG[B->block_id]; }

std::vector<LLVMBlock> CFG::GetPredecessor(int bbid) { return invG[bbid]; }

std::vector<LLVMBlock> CFG::GetSuccessor(LLVMBlock B) { return G[B->block_id]; }

std::vector<LLVMBlock> CFG::GetSuccessor(int bbid) { return G[bbid]; }