#include "../../include/ir.h"
#include "function_basicinfo.h"
#include "inline.h"
#include "simplify_cfg.h"
#include "../analysis/dominator_tree.h"
#include <iostream>

extern std::map<std::string, CFG *> CFGMap;
extern FunctionCallGraph fcallgraph;

Operand InlinePass::InlineCFG(CFG *uCFG, CFG *vCFG, LLVMBlock StartBB, LLVMBlock EndBB, std::map<int, int> &reg_replace_map,
                  std::map<int, int> &label_replace_map) {
    label_replace_map[0] = StartBB->block_id;
    for (auto [id, bb] : *vCFG->block_map) {
        if (id == 0 || bb->Instruction_list.empty()) {
            continue;
        }
        auto newbb = uCFG->NewBlock();
        label_replace_map[bb->block_id] = newbb->block_id;
    }

    for (auto [id, bb] : *vCFG->block_map) {
        auto nowbb_id = label_replace_map[id];
        auto nowbb = (*uCFG->block_map)[nowbb_id];
        for (auto I : bb->Instruction_list) {
            if (I->GetOpcode() == BasicInstruction::RET) {
                auto newI = new BrUncondInstruction(GetNewLabelOperand(EndBB->block_id));
                nowbb->InsertInstruction(1, newI);
                continue;
            }
            Instruction nowI;
            if (is_reinline) {
                nowI = I;
            } else {
                nowI = I->CopyInstruction();
            }
            nowI->ReplaceLabelByMap(label_replace_map);
            auto use_ops = nowI->GetNonResultOperands();
            for (auto &op : use_ops) {
                if (op->GetOperandType() != BasicOperand::REG) {
                    continue;
                }
                auto RegOp = (RegOperand *)op;
                auto RegNo = RegOp->GetRegNo();
                if (reg_replace_map.find(RegNo) == reg_replace_map.end()) {
                    auto newNo = ++uCFG->max_reg;
                    op = GetNewRegOperand(newNo);
                    reg_replace_map[RegNo] = newNo;
                }
            }

            auto ResultReg = (RegOperand *)nowI->GetResultReg();

            if (ResultReg != NULL && reg_replace_map.find(ResultReg->GetRegNo()) == reg_replace_map.end()) {
                auto ResultRegNo = ResultReg->GetRegNo();
                int newNo = ++uCFG->max_reg;
                auto newReg = GetNewRegOperand(newNo);
                reg_replace_map[ResultRegNo] = newNo;
            }

            nowI->ReplaceRegByMap(reg_replace_map);
            nowbb->InsertInstruction(1, nowI);
            nowI->SetBlockID(nowbb_id);
        }
    }
    auto retBB = (BasicBlock *)vCFG->ret_block;
    auto RetI = (RetInstruction *)retBB->Instruction_list.back();
    auto RetResult = RetI->GetRetVal();

    if (RetResult != NULL) {
        if (RetResult->GetOperandType() == BasicOperand::REG) {
            auto oldRegNo = ((RegOperand *)RetResult)->GetRegNo();
            return GetNewRegOperand(reg_replace_map[oldRegNo]);
        }
        return RetResult;
    }
    return nullptr;
}

void InlinePass::InlineCFG(CFG *uCFG, CFG *vCFG, uint32_t CallINo) {
    std::map<int, int> reg_replace_map;
    std::map<int, int> label_replace_map;
    std::set<Instruction> EraseSet;
    auto StartBB = uCFG->NewBlock();
    auto EndBB = uCFG->NewBlock();
    auto StartBB_id = StartBB->block_id;
    auto EndBB_id = EndBB->block_id;
    int formal_regno = -1;
    auto CallI = (CallInstruction *)fcallgraph.CGCallI[uCFG][vCFG][CallINo];
    auto BlockMap = *uCFG->block_map;
    auto uCFG_BBid = CallI->GetBlockID();
    auto oldbb = BlockMap[uCFG_BBid];

    for (auto formal : CallI->GetParameterList()) {
        ++formal_regno;
        if (formal.second->GetOperandType() == BasicOperand::IMMI32) {
            std::stack<Instruction> PhiISta;
            while (oldbb->Instruction_list.front()->GetOpcode() == BasicInstruction::PHI) {
                PhiISta.push(oldbb->Instruction_list.front());
                oldbb->Instruction_list.pop_front();
            }
            auto newAddReg = GetNewRegOperand(++uCFG->max_reg);
            oldbb->InsertInstruction(
            0, new ArithmeticInstruction(BasicInstruction::ADD, BasicInstruction::I32, formal.second, new ImmI32Operand(0), newAddReg));
            while (!PhiISta.empty()) {
                oldbb->InsertInstruction(0, PhiISta.top());
                PhiISta.pop();
            }
            reg_replace_map[formal_regno] = newAddReg->GetRegNo();
            fcallgraph.CGINum[uCFG]++;
        } else if (formal.second->GetOperandType() == BasicOperand::IMMF32) {
            std::stack<Instruction> PhiISta;
            while (oldbb->Instruction_list.front()->GetOpcode() == BasicInstruction::PHI) {
                PhiISta.push(oldbb->Instruction_list.front());
                oldbb->Instruction_list.pop_front();
            }
            auto newAddReg = GetNewRegOperand(++uCFG->max_reg);
            oldbb->InsertInstruction(
            0, new ArithmeticInstruction(BasicInstruction::FADD, BasicInstruction::FLOAT32, formal.second, new ImmF32Operand(0), newAddReg));
            while (!PhiISta.empty()) {
                oldbb->InsertInstruction(0, PhiISta.top());
                PhiISta.pop();
            }
            reg_replace_map[formal_regno] = newAddReg->GetRegNo();
            fcallgraph.CGINum[uCFG]++;
        } else {
            reg_replace_map[formal_regno] = ((RegOperand *)formal.second)->GetRegNo();
        }
    }

    Operand NewResultOperand = InlineCFG(uCFG, vCFG, StartBB, EndBB, reg_replace_map, label_replace_map);

    auto vfirstlabelno = label_replace_map[0];
    auto uAllocaBB = (BasicBlock *)BlockMap[vfirstlabelno];
    for (auto I : uAllocaBB->Instruction_list) {
        if (I->GetOpcode() != BasicInstruction::ALLOCA) {
            continue;
        }
        BlockMap[0]->InsertInstruction(0, I);
        EraseSet.insert(I);
    }

    auto tmp_Instruction_list = uAllocaBB->Instruction_list;
    uAllocaBB->Instruction_list.clear();
    for (auto I : tmp_Instruction_list) {
        if (EraseSet.find(I) != EraseSet.end()) {
            continue;
        }
        uAllocaBB->InsertInstruction(1, I);
    }
    EraseSet.clear();
    if (CallI->GetRetType() != BasicInstruction::VOID) {
        if (CallI->GetResultType() == BasicInstruction::I32) {
            EndBB->InsertInstruction(1, new ArithmeticInstruction(BasicInstruction::ADD, BasicInstruction::I32, (ImmI32Operand *)NewResultOperand,
                                                                  new ImmI32Operand(0), CallI->GetResultReg()));
        } else {
            EndBB->InsertInstruction(1, new ArithmeticInstruction(BasicInstruction::FADD, BasicInstruction::FLOAT32, (ImmF32Operand *)NewResultOperand,
                                                                  new ImmF32Operand(0), CallI->GetResultReg()));
        }
        fcallgraph.CGINum[uCFG]++;
    }

    auto StartBB_label = GetNewLabelOperand(StartBB_id);
    BrUncondInstruction *newBrI;
    auto oldbb_it = oldbb->Instruction_list.begin();
    bool EndbbBegin = false;
    for (auto it = oldbb->Instruction_list.begin(); it != oldbb->Instruction_list.end(); ++it) {
        auto I = *it;
        if (!EndbbBegin && I == CallI) {
            EndbbBegin = true;
            newBrI = new BrUncondInstruction(StartBB_label);
            newBrI->SetBlockID(oldbb->block_id);
            oldbb_it = it;
        } else if (EndbbBegin) {
            I->SetBlockID(EndBB_id);
            EndBB->InsertInstruction(1, I);
        }
    }
    while (oldbb_it != oldbb->Instruction_list.end()) {
        oldbb->Instruction_list.pop_back();
    }
    oldbb->InsertInstruction(1, newBrI);
    label_replace_map.clear();
    label_replace_map[oldbb->block_id] = EndBB->block_id;
    for (auto nextBB : uCFG->G[uCFG_BBid]) {
        for (auto I : nextBB->Instruction_list) {
            if (I->GetOpcode() != BasicInstruction::PHI) {
                continue;
            }
            auto PhiI = (PhiInstruction *)I;
            PhiI->ReplaceLabelByMap(label_replace_map);
        }
    }
    label_replace_map.clear();
}

bool IsInlineBetter(CFG *uCFG, CFG *vCFG) {
    bool flag3 = false;
    auto vFuncdef = vCFG->function_def->formals;
    for (auto type : vFuncdef) {
        if (type == BasicInstruction::PTR) {
            flag3 = true;
            break;
        }
    }
    flag3 &= uCFG != vCFG;
    bool flag1 = fcallgraph.CGINum[vCFG] <= 30;
    bool flag2 = fcallgraph.CGINum[uCFG] + fcallgraph.CGINum[vCFG] <= 200;
    return flag1 || flag2 || flag3;
}

CFG *InlinePass::CopyCFG(CFG *uCFG) {
    CFG *vCFG = new CFG;
    auto max_label = uCFG->max_label;
    auto func_def = uCFG->function_def;
    auto newFuncDef = new FunctionDefineInstruction(func_def->GetReturnType(), ".....CopyInlineFunc");
    vCFG->function_def = newFuncDef;
    vCFG->block_map = new std::map<int, LLVMBlock>;
    vCFG->max_label = -1;
    int lastid = -1;
    for (auto [id, bb] : *uCFG->block_map) {
        LLVMBlock new_block;
        while (id != lastid) {
            lastid++;
            new_block = vCFG->NewBlock();
        }
        lastid = id;
        for (auto I : bb->Instruction_list) {
            auto newI = I->CopyInstruction();
            new_block->InsertInstruction(1, newI);
            newI->SetBlockID(id);
            if (newI->GetOpcode() == BasicInstruction::RET) {
                vCFG->ret_block = new_block;
            }
            if (newI->GetOpcode() == BasicInstruction::CALL &&
                CFGMap.find(((CallInstruction *)newI)->GetFunctionName()) != CFGMap.end()) {
                auto CallI = (CallInstruction *)newI;
                if (CallI->GetFunctionName() == uCFG->function_def->GetFunctionName()) {
                    fcallgraph.CGNum[uCFG][uCFG]++;
                    reinlinecallI[CallI] = true;
                    fcallgraph.CGCallI[uCFG][uCFG].push_back(CallI);
                }
            }
        }
    }
    vCFG->max_reg = uCFG->max_reg;
    fcallgraph.CG[uCFG].push_back(vCFG);
    fcallgraph.CGNum[uCFG][vCFG] = 1;
    fcallgraph.CGINum[vCFG] = fcallgraph.CGINum[uCFG];
    return vCFG;
}

void InlinePass::InlineDFS(CFG *uCFG) {
    if (CFGvis.find(uCFG) != CFGvis.end()) {
        return;
    }
    CFGvis[uCFG] = true;

    for (auto vCFG : fcallgraph.CG[uCFG]) {
        if (uCFG == vCFG) {
            continue;
        }
        InlineDFS(vCFG);
        if (fcallgraph.CG.find(vCFG) == fcallgraph.CG.end() ||
            fcallgraph.CGNum[vCFG].find(vCFG) != fcallgraph.CGNum[vCFG].end()) {
            continue;
        }
        auto map_size = fcallgraph.CGNum[uCFG][vCFG];
        for (uint32_t i = 0; i < map_size; ++i) {
            if (!IsInlineBetter(uCFG, vCFG)) {
                break;
            }
            InlineCFG(uCFG, vCFG, i);
            fcallgraph.CGINum[uCFG] += fcallgraph.CGINum[vCFG];
            uCFG->BuildCFG();
        }
    }

#ifdef O3_ENABLE
    if (fcallgraph.CG.find(uCFG) != fcallgraph.CG.end() &&
        fcallgraph.CGNum[uCFG].find(uCFG) != fcallgraph.CGNum[uCFG].end()) {
        int i = 0;
        while (IsInlineBetter(uCFG, uCFG)) {
            is_reinline = true;
            auto vCFG = CopyCFG(uCFG);
            auto oldI = (CallInstruction *)fcallgraph.CGCallI[uCFG][uCFG][i];
            auto CallI = (CallInstruction *)oldI->CopyInstruction();
            CallI->SetFunctionName(vCFG->function_def->GetFunctionName());
            CallI->SetBlockID(oldI->GetBlockID());
            fcallgraph.CGCallI[uCFG][uCFG][i] = CallI;
            oldI->SetFunctionName(vCFG->function_def->GetFunctionName());
            auto block_map = uCFG->block_map;
            auto oldbb = (*block_map)[oldI->GetBlockID()];
            fcallgraph.CGCallI[uCFG][vCFG].push_back(oldI);

            InlineCFG(uCFG, vCFG, 0);

            fcallgraph.CGINum[uCFG] += fcallgraph.CGINum[vCFG];
            fcallgraph.CG[uCFG].pop_back();
            fcallgraph.CGNum[uCFG][vCFG] = 0;
            fcallgraph.CGCallI[uCFG][vCFG].pop_back();
            uCFG->BuildCFG();
            i++;
        }
        is_reinline = false;
    }
#endif
}

void InlinePass::EliminateUselessFunction() {
    std::set<std::string> CallStrSet;

    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        for (auto [id, bb] : *(cfg->block_map)) {
            for (auto I : bb->Instruction_list) {
                if (I->GetOpcode() == BasicInstruction::CALL) {
                    auto CallI = (CallInstruction *)I;
                    CallStrSet.insert(CallI->GetFunctionName());
                }
            }
        }
    }

    std::set<FuncDefInstruction> EraseFuncDefSet;
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        auto func_name = defI->GetFunctionName();
        if (func_name == "main") {
            continue;
        }
        if (CallStrSet.find(func_name) == CallStrSet.end()) {
            EraseFuncDefSet.insert(defI);
        }
    }

    for (auto I : EraseFuncDefSet) {
        llvmIR->llvm_cfg.erase(I);
        llvmIR->function_block_map.erase(I);
    }
}

void InlinePass::Execute() {
    CFGvis.clear();
    reinlinecallI.clear();
    is_reinline = false;

    InlineDFS(fcallgraph.MainCFG);

    DomAnalysis domAnalysis(llvmIR);
    domAnalysis.Execute();

    SimplifyCFGPass simplifyCFG(llvmIR);
    simplifyCFG.Execute();

    for (auto& kv : CFGMap) {
        auto cfg = kv.second;
        for (auto [id, bb] : *(cfg->block_map)) {
            for (auto I : bb->Instruction_list) {
                I->SetBlockID(id);
            }
        }
    }

    EliminateUselessFunction();
}
