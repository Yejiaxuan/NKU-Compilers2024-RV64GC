#include "../../include/ir.h"
#include "function_basicinfo.h"
#include "inline.h"
#include "simplify_cfg.h"
#include "../analysis/dominator_tree.h"
#include <iostream>

extern std::map<std::string, CFG *> CFGMap;
extern FunctionCallGraph fcallgraph;

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

    RemoveUnusedFunctions();
}

bool whether(CFG *uCFG, CFG *vCFG) {
    auto vFuncdef = vCFG->function_def->formals;
    bool flag1 = fcallgraph.CGINum[vCFG] <= 30;
    bool flag2 = fcallgraph.CGINum[uCFG] + fcallgraph.CGINum[vCFG] <= 200;
    return flag1 && flag2 ;

}

// Reference: https://github.com/yuhuifishash/SysY/blob/master/optimize/function/function_inline.cc line60-line130
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

// Reference: https://github.com/yuhuifishash/SysY/blob/master/optimize/function/function_inline.cc line131-line246
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

CFG* InlinePass::Clone(CFG* originalCFG) {
    // 创建一个新的CFG对象
    auto newCFG = new CFG();
    
    // 复制函数定义相关信息
    auto originalDef = originalCFG->function_def;
    newCFG->function_def = new FunctionDefineInstruction(
        originalDef->GetReturnType(),
        "inline_copy_" + std::to_string(rand())  // 生成随机函数名
    );

    // 初始化新CFG的属性
    newCFG->block_map = new std::map<int, LLVMBlock>();
    newCFG->max_reg = originalCFG->max_reg;
    newCFG->max_label = -1;

    // 创建所有基本块
    std::map<int, LLVMBlock> tempBlocks;
    for(int i = 0; i <= originalCFG->max_label; i++) {
        tempBlocks[i] = newCFG->NewBlock();
    }

    // 复制每个基本块的指令
    for(const auto& [blockId, block] : *originalCFG->block_map) {
        auto targetBlock = tempBlocks[blockId];
        
        // 复制所有指令
        for(auto inst : block->Instruction_list) {
            auto copiedInst = inst->CopyInstruction();
            if(!copiedInst) continue;
            
            copiedInst->SetBlockID(blockId);
            targetBlock->InsertInstruction(1, copiedInst);
            
            // 处理特殊指令
            switch(copiedInst->GetOpcode()) {
                case BasicInstruction::RET:
                    newCFG->ret_block = targetBlock;
                    break;
                    
                case BasicInstruction::CALL: {
                    auto callInst = static_cast<CallInstruction*>(copiedInst);
                    auto funcName = callInst->GetFunctionName();
                    
                    // 处理递归调用
                    if(funcName == originalCFG->function_def->GetFunctionName()) {
                        fcallgraph.CGNum[originalCFG][originalCFG]++;
                        reinlinecallI[callInst] = true;
                        fcallgraph.CGCallI[originalCFG][originalCFG].push_back(callInst);
                    }
                    break;
                }
            }
        }
    }

    // 更新调用图信息
    fcallgraph.CG[originalCFG].push_back(newCFG);
    fcallgraph.CGNum[originalCFG][newCFG] = 1;
    fcallgraph.CGINum[newCFG] = fcallgraph.CGINum[originalCFG];

    return newCFG;
}

void InlinePass::InlineDFS(CFG* currentCFG) {
    if(CFGvis[currentCFG]) return;
    CFGvis[currentCFG] = true;

    // 获取所有被调用的函数
    std::vector<std::pair<CFG*, int>> calledFuncs;
    for(auto calleeCFG : fcallgraph.CG[currentCFG]) {
        if(calleeCFG == currentCFG) continue;
        
        InlineDFS(calleeCFG);
        if(fcallgraph.CGNum[currentCFG].count(calleeCFG)) {
            calledFuncs.push_back({calleeCFG, fcallgraph.CGNum[currentCFG][calleeCFG]});
        }
    }

    // 内联其他函数
    for(auto [calleeCFG, callCount] : calledFuncs) {
        if(fcallgraph.CG.count(calleeCFG) && !fcallgraph.CGNum[calleeCFG].count(calleeCFG)) {
            for(int i = 0; i < callCount && whether(currentCFG, calleeCFG); i++) {
                InlineCFG(currentCFG, calleeCFG, i);
                fcallgraph.CGINum[currentCFG] += fcallgraph.CGINum[calleeCFG];
                currentCFG->BuildCFG();
            }
        }
    }

    // 处理递归函数
    if(fcallgraph.CGNum[currentCFG].count(currentCFG)) {
        std::queue<CallInstruction*> recursiveCalls;
        for(auto call : fcallgraph.CGCallI[currentCFG][currentCFG]) {
            recursiveCalls.push(static_cast<CallInstruction*>(call));
        }

        while(!recursiveCalls.empty() && whether(currentCFG, currentCFG)) {
            is_reinline = true;
            
            // 创建函数副本并更新调用
            auto funcCopy = Clone(currentCFG);
            auto originalCall = recursiveCalls.front();
            recursiveCalls.pop();
            
            // 复制调用指令并更新名称
            auto newCall = static_cast<CallInstruction*>(originalCall->CopyInstruction());
            newCall->SetFunctionName(funcCopy->function_def->GetFunctionName());
            newCall->SetBlockID(originalCall->GetBlockID());
            
            // 执行内联
            fcallgraph.CGCallI[currentCFG][funcCopy].push_back(originalCall);
            InlineCFG(currentCFG, funcCopy, 0);
            
            // 清理并更新
            fcallgraph.CGINum[currentCFG] += fcallgraph.CGINum[funcCopy];
            fcallgraph.CG[currentCFG].pop_back();
            fcallgraph.CGNum[currentCFG][funcCopy] = 0;
            fcallgraph.CGCallI[currentCFG][funcCopy].clear();
            
            currentCFG->BuildCFG();
        }
        
        is_reinline = false;
    }
}

void InlinePass::RemoveUnusedFunctions() {
    // 使用邻接表表示函数调用图
    std::unordered_map<std::string, std::vector<std::string>> callGraph;
    std::unordered_map<std::string, FuncDefInstruction> nameToDefMap;
    std::set<std::string> allFuncs;
    
    // 第一遍遍历:构建调用图
    for(const auto& [defI, cfg] : llvmIR->llvm_cfg) {
        std::string caller = defI->GetFunctionName();
        nameToDefMap[caller] = defI;
        allFuncs.insert(caller);
        
        // 遍历所有基本块查找调用指令
        for(const auto& [id, bb] : *(cfg->block_map)) {
            for(auto inst : bb->Instruction_list) {
                if(auto callInst = dynamic_cast<CallInstruction*>(inst)) {
                    std::string callee = callInst->GetFunctionName();
                    callGraph[caller].push_back(callee);
                }
            }
        }
    }
    
    // 使用BFS从main函数开始遍历,标记所有可达函数
    std::unordered_set<std::string> reachableFuncs;
    std::queue<std::string> workList;
    
    // 从main函数开始BFS
    workList.push("main");
    reachableFuncs.insert("main");
    
    while(!workList.empty()) {
        std::string current = workList.front();
        workList.pop();
        
        // 遍历当前函数调用的所有函数
        for(const auto& callee : callGraph[current]) {
            if(reachableFuncs.find(callee) == reachableFuncs.end()) {
                reachableFuncs.insert(callee);
                workList.push(callee);
            }
        }
    }
    
    // 清理不可达的函数定义
    for(const auto& funcName : allFuncs) {
        if(reachableFuncs.find(funcName) == reachableFuncs.end()) {
            auto defI = nameToDefMap[funcName];
            llvmIR->llvm_cfg.erase(defI);
            llvmIR->function_block_map.erase(defI);
        }
    }
}
