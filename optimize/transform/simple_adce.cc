#include "simple_adce.h"
#include <functional>

/*this pass will do Aggressive Dead Code Elimination, it will eliminate useless Control Flow Structure
(such as useless loop)
*/

std::vector<std::vector<LLVMBlock>> ADCEPass::BuildCDG(CFG *C) {
    std::vector<std::vector<LLVMBlock>> CDG;
    std::vector<std::vector<LLVMBlock>> CDG_precursor;
    std::vector<int> rd;
    auto G = C->G;
    auto invG = C->invG;
    DominatorTree DomTree;
    DominatorTree PostDomTree;
    DomTree.C= C;
    PostDomTree.C = C;
    DomTree.BuildDominatorTree();
    PostDomTree.BuildPostDominatorTree();
    auto blockmap = (*C->block_map);
    CDG.resize(C->max_label + 2);
    rd.resize(C->max_label + 1, 0);
    CDG_precursor.resize(C->max_label + 1);
    for (int i = 0; i <= C->max_label; ++i) {
        auto domFrontier = PostDomTree.GetDF(i);
        for (auto vbbid : domFrontier) {
            CDG[vbbid].push_back(blockmap[i]);
            if (vbbid != i) {
                rd[i]++;
            }
            CDG_precursor[blockmap[i]->block_id].push_back(blockmap[vbbid]);
        }
    }
    for (int i = 0; i <= C->max_label; ++i) {
        if (!rd[i]) {
            CDG[C->max_label + 1].push_back(blockmap[i]);
        }
    }
    return CDG_precursor;
}
Instruction ADCEPass::FindTerminal(CFG *C, int bbid) {
    auto blockmap = (*C->block_map);
    auto bb = blockmap[bbid];
    return bb->Instruction_list.back();
}
void ADCEPass::ADCE(CFG *C) {
    std::deque<Instruction> worklist;
    std::map<int, Instruction> defmap;
    std::set<Instruction> liveInstructionset;
    std::set<int> liveBBset;
    auto CDG_precursor = BuildCDG(C);
    auto invG = C->invG;
    DominatorTree DomTree;
    DominatorTree PostDomTree;
    DomTree.C= C;
    PostDomTree.C = C;
    DomTree.BuildDominatorTree();
    PostDomTree.BuildPostDominatorTree();
    auto DomTreeidom = DomTree.idom;
    auto PostDomTreeidom = PostDomTree.idom;
    auto blockmap = *C->block_map;
    for (auto [id, bb] : blockmap) {
        for (auto I : bb->Instruction_list) {
            I->SetBlockID(id);
            if (I->GetOpcode() == BasicInstruction::STORE || I->GetOpcode() == BasicInstruction::CALL || I->GetOpcode() == BasicInstruction::RET) {
                worklist.push_back(I);
            }
            if (I->GetResultReg() != nullptr) {
                defmap[I->GetResultRegNo()] = I;
            }
        }
    }

    while (!worklist.empty()) {
        auto I = worklist.front();
        worklist.pop_front();
        if (liveInstructionset.find(I) != liveInstructionset.end()) {
            continue;
        }
        liveInstructionset.insert(I);
        auto parBBno = I->GetBlockID();
        auto parBB = blockmap[I->GetBlockID()];
        liveBBset.insert(parBBno);
        if (I->GetOpcode() == BasicInstruction::PHI) {
            auto PhiI = (PhiInstruction *)I;
            for (auto [Labelop, Regop] : PhiI->GetPhiList()) {
                auto Label = (LabelOperand *)Labelop;
                auto Labelno = Label->GetLabelNo();
                auto terminalI = FindTerminal(C, Labelno);
                if (liveInstructionset.find(terminalI) == liveInstructionset.end()) {
                    worklist.push_front(terminalI);
                    liveBBset.insert(Labelno);
                }
            }
        }

        if (parBBno != -1) {
            for (auto CDG_pre : CDG_precursor[parBBno]) {
                auto CDG_preno = CDG_pre->block_id;
                auto terminalI = FindTerminal(C, CDG_preno);
                if (liveInstructionset.find(terminalI) == liveInstructionset.end()) {
                    worklist.push_front(terminalI);
                }
            }
        }

        for (auto op : I->GetNonResultOperands()) {
            if (op->GetOperandType() == BasicOperand::REG) {
                auto Regop = (RegOperand *)op;
                auto Regopno = Regop->GetRegNo();
                if (defmap.find(Regopno) == defmap.end()) {
                    continue;
                }
                auto DefI = defmap[Regopno];
                if (liveInstructionset.find(DefI) == liveInstructionset.end()) {
                    worklist.push_front(DefI);
                }
            }
        }
    }

    for (auto [id, bb] : *C->block_map) {
        auto terminalI = FindTerminal(C, id);
        auto tmp_Instruction_list = bb->Instruction_list;
        bb->Instruction_list.clear();
        for (auto I : tmp_Instruction_list) {
            if (liveInstructionset.find(I) == liveInstructionset.end()) {
                if (terminalI == I) {
                    auto livebbid = PostDomTreeidom[id]->block_id;
                    while (liveBBset.find(livebbid) == liveBBset.end()) {
                        livebbid = PostDomTreeidom[id]->block_id;
                    }
                    I = new BrUncondInstruction(GetNewLabelOperand(livebbid));
                } else {
                    continue;
                }
            }
            bb->InsertInstruction(1, I);
        }
    }
    // DEBUG : search for basicblock to delete
    // not-a-liveBlock only have one bruncond instruction,
    // which is added in the last loop(I = new BrUncondInstruction(GetNewLabelOperand(livebbid));)
    // std::cerr<<C->function_def->GetFunctionName()<<" not-a-liveBlock :\n";
    // for(int i = 0;i < C->max_label;++i){
    //     if(liveBBset.find(i) == liveBBset.end()){
    //         std::cerr<<i<<'\n';
    //     }
    // }
    // std::cerr<<" before instruction cnt : "<<cnt<<" now instruction cnt : "<<liveInstructionset.size()+cnt2<<'\n';
    defmap.clear();
    liveInstructionset.clear();
    liveBBset.clear();
    // std::cerr<<cnt<<" "<<liveInstructionset.size()+cnt2<<'\n';
}

void ADCEPass::Execute() {
    // 遍历每个函数的CFG进行ADCE优化
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        // 对当前CFG执行激进死代码消除
        ADCE(cfg);
        
        // 重建CFG以确保控制流图结构正确
        cfg->BuildCFG();
    }
}

/*#include "simple_adce.h"
#include <iostream>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>

// 使用无序容器提升查找速度
// 假设 block_id 是一个 int，可直接作为无序容器的键
using BlockMapTy = std::unordered_map<int, BasicBlock*>;
using BlockSetTy = std::unordered_set<int>;

bool IsCriticalBlock(CFG *C, int block_id) {
    // 获取该块的前驱和后继
    auto preds = C->GetPredecessor(block_id);
    auto succs = C->GetSuccessor(block_id);

    // 如果该块是其他块唯一的前驱或后继，则为关键块
    if (preds.size() == 1 && succs.size() == 1) {
        // 例如，前驱唯一性或后继唯一性判断
        int pred_id = (*preds.begin())->block_id;
        int succ_id = (*succs.begin())->block_id;
        auto pred_succs = C->GetSuccessor(pred_id);
        auto succ_preds = C->GetPredecessor(succ_id);
        if (pred_succs.size() == 1 || succ_preds.size() == 1) {
            return true; // 是关键块
        }
    }
    return false;
}

int ADCEPass::FindFinalTarget(CFG *C, int start_block_id, std::unordered_set<int>& visited) {
    // 使用显式栈迭代代替递归
    std::stack<int> stk;
    stk.push(start_block_id);

    while (!stk.empty()) {
        int block_id = stk.top();
        stk.pop();

        if (visited.find(block_id) != visited.end()) {
            std::cerr << "Cycle detected when processing block " << block_id << std::endl;
            return -1;
        }
        visited.insert(block_id);

        // 优先检查后继
        auto succs = C->GetSuccessor(block_id);
        bool found_nonempty = false;
        for (auto succ : succs) {
            int succ_id = succ->block_id;
            auto it = C->block_map->find(succ_id);
            if (it != C->block_map->end()) {
                BasicBlock* succ_bb = it->second;
                if (!succ_bb->Instruction_list.empty()) {
                    // 如果只有一条无条件跳转指令，则继续向下追踪
                    if (succ_bb->Instruction_list.size() == 1 &&
                        succ_bb->Instruction_list.front()->GetOpcode() == BasicInstruction::BR_UNCOND) {
                        stk.push(succ_id);
                    } else {
                        return succ_id;
                    }
                    found_nonempty = true;
                    break; // 如果已经找到可用的后继，直接跳出
                }
            }
        }

        if (!found_nonempty) {
            // 后继无效，检查前驱
            auto preds = C->GetPredecessor(block_id);
            for (auto pred : preds) {
                int pred_id = pred->block_id;
                if (C->block_map->find(pred_id) != C->block_map->end()) {
                    return pred_id; 
                }
            }
        }
    }

    return -1;
}

int ADCEPass::GetAlternatePredecessor(CFG *C, int block_id) {
    auto predecessors = C->GetPredecessor(block_id);
    if (predecessors.empty()) {
        std::cerr << "Warning: Block " << block_id << " has no predecessors." << std::endl;
        return -1;
    }

    for (auto &pred : predecessors) {
        if (pred->block_id != block_id) { 
            return pred->block_id;
        }
    }

    std::cerr << "Error: No valid predecessor found for block " << block_id << std::endl;
    return -1;
}

void ADCEPass::RemoveEmptyBlocks(CFG *C) {
    BlockSetTy empty_blocks;

    // 阶段 1：识别所有空块或仅有 BR_UNCOND 的块
    for (auto &[block_id, bb] : *(C->block_map)) {
        if (bb->Instruction_list.empty()) {
            empty_blocks.insert(block_id);
        } else if (bb->Instruction_list.size() == 1 &&
                   bb->Instruction_list.front()->GetOpcode() == BasicInstruction::BR_UNCOND) {
            empty_blocks.insert(block_id);
        }
    }

    // 阶段 1.5：标记所有在 PHI 指令中引用的块
    BlockSetTy blocks_in_phi;
    for (auto &[block_id, bb] : *(C->block_map)) {
        for (auto &instr_ptr : bb->Instruction_list) {
            if (instr_ptr->GetOpcode() == BasicInstruction::PHI) {
                PhiInstruction* phiInst = dynamic_cast<PhiInstruction*>(instr_ptr);
                if (!phiInst) continue;

                for (auto &[label, val] : phiInst->GetPhiList()) {
                    LabelOperand* lbl = dynamic_cast<LabelOperand*>(label);
                    if (lbl) {
                        int label_no = lbl->GetLabelNo();
                        blocks_in_phi.insert(label_no);  // 标记在 PHI 中的块
                    }
                }
            }
        }
    }

    // 从 empty_blocks 中移除所有在 PHI 中引用的块
    for (auto block_id : blocks_in_phi) {
        empty_blocks.erase(block_id);
    }

    // 阶段 2：为每个空块记录最终的后继块
    std::unordered_map<int, int> empty_to_final_successor;
    empty_to_final_successor.reserve(empty_blocks.size());
    for (auto block_id : empty_blocks) {
        std::unordered_set<int> visited;
        int final_target = FindFinalTarget(C, block_id, visited);
        empty_to_final_successor[block_id] = final_target;
    }

    // 阶段 3：更新所有 PHI 指令的前驱引用
    /*for (auto &[block_id, bb] : *(C->block_map)) {
        for (auto &instr_ptr : bb->Instruction_list) {
            if (instr_ptr->GetOpcode() == BasicInstruction::PHI) {
                PhiInstruction* phiInst = dynamic_cast<PhiInstruction*>(instr_ptr);
                if (!phiInst) continue;

                std::vector<std::pair<Operand, Operand>> new_phi_list;
                for (auto &[label, val] : phiInst->GetPhiList()) {
                    LabelOperand* lbl = dynamic_cast<LabelOperand*>(label);
                    if (lbl) {
                        int label_no = lbl->GetLabelNo();
                        auto it = empty_to_final_successor.find(label_no);
                        if (it != empty_to_final_successor.end()) {
                            int final_target = it->second;
                            if (final_target == -1 || final_target == block_id) {
                                // 尝试获取替代前驱
                                final_target = GetAlternatePredecessor(C, label_no);
                                if (final_target == -1) {
                                    std::unordered_set<int> visited;
                                    final_target = FindFinalTarget(C, label_no, visited);
                                }
                                if (final_target == -1) continue;
                            }
                            Operand new_label = GetNewLabelOperand(final_target);
                            new_phi_list.emplace_back(new_label, val);
                        } else {
                            new_phi_list.emplace_back(label, val);  // 保持引用不变
                        }
                    } else {
                        new_phi_list.emplace_back(label, val);  // 保持引用不变
                    }
                }
                phiInst->ReplacePhiList(new_phi_list);
            }
        }
    }

    // 阶段 4：更新前驱块的跳转指令
    std::unordered_set<int> affected_blocks; 
    for (auto block_id : empty_blocks) {
        int final_target = empty_to_final_successor[block_id];
        if (final_target == -1) continue;

        for (auto pred : C->invG[block_id]) {
            int pred_id = pred->block_id;
            if (pred_id == block_id) continue;
            affected_blocks.insert(pred_id);
        }
    }

for (auto pred_id : affected_blocks) {
    auto pred_it = C->block_map->find(pred_id);
    if (pred_it == C->block_map->end()) continue;
    BasicBlock* pred_bb = pred_it->second;
    if (pred_bb->Instruction_list.empty()) continue;

    BasicInstruction* lastInst = pred_bb->Instruction_list.back();
    if (lastInst->GetOpcode() == BasicInstruction::BR_UNCOND) {
        BrUncondInstruction* brUncond = dynamic_cast<BrUncondInstruction*>(lastInst);
        if (brUncond) {
            int old_label = dynamic_cast<LabelOperand*>(brUncond->GetDestLabel())->GetLabelNo();

            // 检查 old_label 是否在 empty_blocks 中
            if (empty_blocks.find(old_label) == empty_blocks.end()) {
                continue; // 如果不是空块，跳过更新
            }

            auto it = empty_to_final_successor.find(old_label);
            if (it != empty_to_final_successor.end() && it->second != -1) {
                brUncond->ReplaceLabel(GetNewLabelOperand(it->second));
            }
        }
    } else if (lastInst->GetOpcode() == BasicInstruction::BR_COND) {
        // 对条件跳转类似处理，确保只修改空块
        BrCondInstruction* brCond = dynamic_cast<BrCondInstruction*>(lastInst);
        if (brCond) {
            LabelOperand* trueLabel = dynamic_cast<LabelOperand*>(brCond->GetTrueLabel());
            LabelOperand* falseLabel = dynamic_cast<LabelOperand*>(brCond->GetFalseLabel());

            if (trueLabel) {
                int tl = trueLabel->GetLabelNo();
                if (empty_blocks.find(tl) != empty_blocks.end()) {
                    auto it = empty_to_final_successor.find(tl);
                    if (it != empty_to_final_successor.end() && it->second != -1) {
                        brCond->ReplaceTrueLabel(GetNewLabelOperand(it->second));
                    }
                }
            }
            if (falseLabel) {
                int fl = falseLabel->GetLabelNo();
                if (empty_blocks.find(fl) != empty_blocks.end()) {
                    auto it = empty_to_final_successor.find(fl);
                    if (it != empty_to_final_successor.end() && it->second != -1) {
                        brCond->ReplaceFalseLabel(GetNewLabelOperand(it->second));
                    }
                }
            }
        }
    }
}


    // 阶段 5：删除所有空的基本块
    for (auto block_id : empty_blocks) {
        if (IsCriticalBlock(C, block_id)) {
        continue; // 跳过关键块
    }
        auto it = C->block_map->find(block_id);
        if (it != C->block_map->end()) {
            for (auto pred : C->invG[block_id]) {
                auto &succ_list = C->G[pred->block_id];
                succ_list.erase(std::remove_if(succ_list.begin(), succ_list.end(),
                    [&](LLVMBlock b) { return b->block_id == block_id; }),
                    succ_list.end());
            }
            for (auto succ : C->G[block_id]) {
                auto &pred_list = C->invG[succ->block_id];
                pred_list.erase(std::remove_if(pred_list.begin(), pred_list.end(),
                    [&](LLVMBlock b) { return b->block_id == block_id; }),
                    pred_list.end());
            }

            C->G[block_id].clear();
            C->invG[block_id].clear();
            delete it->second;
            C->block_map->erase(block_id);
        }
    }
}



void ADCEPass::RemoveUselessControlFlow(CFG *C) {
    // 逻辑不变，保持原样即可
    for(auto &[block_id, bb] : *(C->block_map)) {
        if(bb->Instruction_list.empty()) continue;

        BasicInstruction* lastInst = bb->Instruction_list.back();
        if(lastInst->GetOpcode() == BasicInstruction::BR_COND) {
            BrCondInstruction* brCond = dynamic_cast<BrCondInstruction*>(lastInst);
            if(brCond) {
                Operand cond = brCond->GetCond();
                if(auto imm = dynamic_cast<ImmI32Operand*>(cond)) {
                    // 条件分支简化逻辑保持原样
                    if(imm->GetIntImmVal() != 0) { 
                        BrUncondInstruction* brUncond = new BrUncondInstruction(brCond->GetTrueLabel());
                        delete lastInst;
                        bb->Instruction_list.back() = brUncond;
                    } else {
                        BrUncondInstruction* brUncond = new BrUncondInstruction(brCond->GetFalseLabel());
                        delete lastInst;
                        bb->Instruction_list.back() = brUncond;
                    }
                }
            }
        }
    }
}

/**
 * @brief 构建控制依赖图（CDG）。
 *
 * 利用支配树的支配前沿（Dominance Frontier）来构建控制依赖图。
 *
 * @param C 指向当前处理的控制流图（CFG）。
 
void ADCEPass::BuildControlDependence(CFG *C) {
    DominatorTree *dom_tree = dom_analysis->GetDomTree(C);
    control_dependence.clear();
    for(auto &[y_id, y_bb] : *(C->block_map)) {
        // 获取基本块 y 的支配前沿
        std::set<int> df = dom_tree->GetDF(y_id);
        for(auto x_id : df) {
            // 基本块 x 在基本块 y 的支配前沿上，因此 x 控制依赖于 y
            control_dependence[x_id].insert(y_id);
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
 
std::set<BasicInstruction*> ADCEPass::ComputeLiveInstructions(CFG *C) {
    std::set<BasicInstruction*> live;
    std::queue<BasicInstruction*> worklist;
    std::map<int, BasicInstruction*> defMap; // 使用寄存器编号作为键

    // 构建 defMap 并初始化工作列表
    for(auto &[block_id, bb] : *(C->block_map)) {
        for(auto &instr_ptr : bb->Instruction_list) {
            BasicInstruction* instr = instr_ptr;
            Operand res = instr->GetResultReg();
            if(res && res->GetOperandType() == BasicOperand::REG) {
                int reg_no = static_cast<RegOperand*>(res)->GetRegNo();
                defMap[reg_no] = instr;
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
                //Log("Marking instruction %p as live in block %d\n", instr, block_id);
            }
        }
    }

    // 迭代标记活跃指令
    while(!worklist.empty()) {
        BasicInstruction* inst = worklist.front();
        worklist.pop();
        //Log("Processing live instruction %p\n", inst);

        // 获取指令的所有使用
        std::vector<Operand> uses = inst->GetNonResultOperands();
        for(auto &use : uses) {
            if(use->GetOperandType() == BasicOperand::REG) {
                int reg_no = static_cast<RegOperand*>(use)->GetRegNo();
                auto it = defMap.find(reg_no);
                if(it != defMap.end()) {
                    BasicInstruction* def_inst = it->second;
                    if(live.find(def_inst) == live.end()) {
                        worklist.push(def_inst);
                        live.insert(def_inst);
                        //Log("Marking definition instruction %p as live\n", def_inst);
                    }
                }
            }
        }

        // 处理 PHI 指令
        if(inst->GetOpcode() == BasicInstruction::PHI) {
            PhiInstruction* phiInst = dynamic_cast<PhiInstruction*>(inst);
            if(phiInst) {
                for(auto &[val, label] : phiInst->GetPhiList()) {
                    if(val->GetOperandType() == BasicOperand::REG) {
                        int reg_no = static_cast<RegOperand*>(val)->GetRegNo();
                        auto it = defMap.find(reg_no);
                        if(it != defMap.end()) {
                            BasicInstruction* def_inst = it->second;
                            if(live.find(def_inst) == live.end()) {
                                worklist.push(def_inst);
                                live.insert(def_inst);
                                //Log("Marking PHI operand definition instruction %p as live\n", def_inst);
                            }
                        }
                    }
                    if(label->GetOperandType() == BasicOperand::LABEL) {
                        int label_no = static_cast<LabelOperand*>(label)->GetLabelNo();
                        auto pred_block_it = C->block_map->find(label_no);
                        if(pred_block_it != C->block_map->end()) {
                            LLVMBlock pred_bb = pred_block_it->second;
                            if(!pred_bb->Instruction_list.empty()) {
                                BasicInstruction* pred_terminator = pred_bb->Instruction_list.back();
                                if(live.find(pred_terminator) == live.end()) {
                                    worklist.push(pred_terminator);
                                    live.insert(pred_terminator);
                                    //Log("Marking terminator instruction %p of predecessor block %d as live\n", pred_terminator, label_no);
                                }
                            }
                        }
                    }
                }
            }
        }

        // 新增：处理控制依赖
        int block_id = inst->GetBlockID();
        auto cdg_it = control_dependence.find(block_id);
        if (cdg_it != control_dependence.end()) {
            for (auto dep_block_id : cdg_it->second) {
                auto dep_block_it = C->block_map->find(dep_block_id);
                if (dep_block_it != C->block_map->end()) {
                    LLVMBlock dep_block = dep_block_it->second;
                    if (!dep_block->Instruction_list.empty()) {
                        BasicInstruction* dep_terminator = dep_block->Instruction_list.back();
                        if (live.find(dep_terminator) == live.end()) {
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
// BuildControlDependence和ComputeLiveInstructions保持原逻辑不变，
// 可考虑使用unordered_map/unordered_set对控制依赖和defMap进行加速。
// 以及在ComputeLiveInstructions中使用use-def链优化等。

void ADCEPass::Execute() {
    for(auto &[defI, cfg] : llvmIR->llvm_cfg) {
        // 构建控制依赖图（CDG）
        BuildControlDependence(cfg);

        // 计算活跃指令
        std::set<BasicInstruction*> live = ComputeLiveInstructions(cfg);

        // 删除不活跃指令
        for(auto &[block_id, bb] : *(cfg->block_map)) {
            std::deque<BasicInstruction*> new_inst_list;
            for (auto &instr_ptr : bb->Instruction_list) {
                BasicInstruction* instr = instr_ptr;
                if(live.find(instr) != live.end()) {
                    new_inst_list.push_back(instr);
                } else {
                    delete instr;
                }
            }
            bb->Instruction_list = new_inst_list;
        }

        RemoveUselessControlFlow(cfg);
        RemoveEmptyBlocks(cfg);
        
    }
}
*/
