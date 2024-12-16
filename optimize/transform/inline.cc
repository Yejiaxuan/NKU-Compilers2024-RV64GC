#include "inline.h"
#include <stack>
#include <set>
#include <map>
#include <algorithm>
#include <iostream>

extern std::map<std::string, CFG *> CFGMap;

bool InlinePass::CanInlineFunction(FuncDefInstruction funcDef, int caller_instr_count) {
    std::string funcName = funcDef->GetFunctionName();
    CFG* cfg = CFGMap[funcName];

    // 统计被调函数的指令数
    int callee_instr_count = 0;
    for (auto &[id, bb] : *cfg->block_map) {
        callee_instr_count += (int)bb->Instruction_list.size();
    }

    // 检查被调函数是否是自递归(条件3相关)
    bool is_recursive = false;
    for (auto &[id, bb] : *cfg->block_map) {
        for (auto inst : bb->Instruction_list) {
            if (inst->GetOpcode() == BasicInstruction::CALL) {
                auto callInst = (CallInstruction *)inst;
                if (callInst->GetFunctionName() == funcName) {
                    is_recursive = true;
                    break;
                }
            }
        }
        if (is_recursive) break;
    }

    // 检查函数参数是否包含指针类型
    bool has_pointer_arg = false;
    for (auto t : funcDef->formals) {
        if (t == BasicInstruction::PTR) {
            has_pointer_arg = true;
            break;
        }
    }

    // 内联条件(满足任意即可)：
    // 1. 被调函数指令数 <= 30
    // 2. 内联后总指令数（caller指令数 + callee指令数） <= 200
    // 3. 函数参数包含指针且非自递归
    bool cond1 = (callee_instr_count <= 30);
    bool cond2 = ((caller_instr_count + callee_instr_count) <= 200);
    bool cond3 = (has_pointer_arg && !is_recursive);

    if (cond1 || cond2 || cond3) {
        return true;
    } else {
        return false;
    }
}

int RecalculateCallerInstrCount(CFG* cfg) {
    int count = 0;
    for (auto &bb_pair : *cfg->block_map) {
        BasicBlock *block = bb_pair.second;
        count += (int)block->Instruction_list.size();
    }
    return count;
}

void InlinePass::InlineFunctionCall(CFG *callerCFG, BasicBlock *callerBlock,
                                    CallInstruction *callInst,
                                    FuncDefInstruction calleeFunc) {
    std::string calleeName = calleeFunc->GetFunctionName();
    CFG* calleeCFG = CFGMap[calleeName];
    auto &calleeBlockMap = *calleeCFG->block_map;

    // 原调用指令的返回值寄存器(可能为NULL，如果返回类型为void)
    Operand callResult = callInst->GetResult();

    // 函数参数
    std::vector<std::pair<BasicInstruction::LLVMType, Operand>> callArgs = callInst->GetParameterList();

    // 被调用函数的形式参数与类型
    std::vector<enum BasicInstruction::LLVMType> formalTypes = calleeFunc->formals;
    std::vector<Operand> formalRegs = calleeFunc->formals_reg;

    // 为被调函数的基本块创建新块id
    std::map<int, int> oldToNewBlockID;
    std::map<int, LLVMBlock> newBlocks;

    // 为每个callee的基本块创建对应的新基本块
    for (auto &bb_pair : calleeBlockMap) {
        int old_id = bb_pair.first;
        BasicBlock *old_bb = bb_pair.second;
        int new_id = (int)callerCFG->G.size();
        callerCFG->G.resize(new_id+1);
        callerCFG->invG.resize(new_id+1);
        oldToNewBlockID[old_id] = new_id;

        // 利用NewBlock函数创建新块
        LLVMBlock new_bb = llvmIR->NewBlock(callerCFG->function_def, new_id);
        newBlocks[new_id] = new_bb;

        // 拷贝指令
        for (auto inst : old_bb->Instruction_list) {
            Instruction newInst = inst->CopyInstruction();
            new_bb->Instruction_list.push_back(newInst);
        }
    }

    // 构建reg映射用于参数替换（假设参数一定是reg或立即数）
    std::map<int,int> regMap;
    // 对参数进行简单映射，如果实参为寄存器，直接映射寄存器号
    for (size_t i = 0; i < formalRegs.size(); i++) {
        Operand formalReg = formalRegs[i];
        Operand actualArg = callArgs[i].second;
        if (formalReg && formalReg->GetOperandType() == BasicOperand::REG) {
            int oldReg = ((RegOperand*)formalReg)->GetRegNo();
            if (actualArg && actualArg->GetOperandType() == BasicOperand::REG) {
                int actualReg = ((RegOperand*)actualArg)->GetRegNo();
                regMap[oldReg] = actualReg;
            } else {
                // 如果是立即数或其他非reg operand，没有直接映射
                // 后续可根据需要对指令中出现的oldReg手动替换为actualArg
                // 简化处理，不实现复杂逻辑，只进行reg->reg映射
            }
        }
    }

    // 使用ReplaceRegByMap对内联后的指令进行参数替换(仅限reg->reg)
    for (auto &bb_pair : newBlocks) {
        auto block = bb_pair.second;
        for (auto inst : block->Instruction_list) {
            inst->ReplaceRegByMap(regMap);
            // 若有立即数实参需要替换，可在此对inst进行类型判断和手动SetOperand。
            // 这里省略复杂场景的处理。
        }
    }

    // 分裂callerBlock，处理call指令后继
    BasicBlock *newSuccessorBlock = nullptr;
    {
        int new_caller_block_id = (int)callerCFG->G.size();
        callerCFG->G.resize(new_caller_block_id+1);
        callerCFG->invG.resize(new_caller_block_id+1);
        newSuccessorBlock = llvmIR->NewBlock(callerCFG->function_def, new_caller_block_id);

        // 将callInst后的指令移动到newSuccessorBlock
        auto &inst_list = callerBlock->Instruction_list;
        auto pos = std::find(inst_list.begin(), inst_list.end(), (Instruction)callInst);
        if (pos != inst_list.end()) {
            ++pos; 
            for (; pos != inst_list.end();) {
                newSuccessorBlock->Instruction_list.push_back(*pos);
                pos = inst_list.erase(pos);
            }
        }

        // 删除callInst本身前，需要先插入跳转到callee入口块
        int callee_entry_id = 0;
        if (!oldToNewBlockID.empty()) {
            callee_entry_id = oldToNewBlockID[0];
        }
        Operand entryLabel = GetNewLabelOperand(callee_entry_id);
        Instruction brToInline = new BrUncondInstruction(entryLabel);
        callerBlock->Instruction_list.push_back(brToInline);

        // CFG更新：callerBlock -> callee_entry_block
        callerCFG->G[callerBlock->block_id].push_back(newBlocks[callee_entry_id]);
        callerCFG->invG[callee_entry_id].push_back(callerBlock);
    }

    // 修改内联函数返回点的RET指令
    for (auto &bb_pair : newBlocks) {
        auto block = bb_pair.second;
        for (size_t i = 0; i < block->Instruction_list.size(); i++) {
            Instruction inst = block->Instruction_list[i];
            if (inst->GetOpcode() == BasicInstruction::RET) {
                RetInstruction *retInst = (RetInstruction *)inst;
                Operand retVal = retInst->GetRetVal();

                // 用一条跳转替换ret
                // 如果有返回值，将它赋给callResult对应的reg（如有MOVE指令可用这里）
                if (callResult != nullptr && retVal != nullptr) {
                    // 为演示用Zext代替move：zext callResult = retVal
                    Instruction fakeInst = new ZextInstruction(BasicInstruction::I32, callResult, BasicInstruction::I32, retVal);
                    block->Instruction_list[i] = fakeInst;
                    // 在fakeInst后插入跳转newSuccessorBlock
                    Operand succLabel = GetNewLabelOperand(newSuccessorBlock->block_id);
                    Instruction brToSucc = new BrUncondInstruction(succLabel);
                    block->Instruction_list.insert(block->Instruction_list.begin() + i + 1, brToSucc);
                } else {
                    // 无返回值或void返回
                    Operand succLabel = GetNewLabelOperand(newSuccessorBlock->block_id);
                    Instruction brToSucc = new BrUncondInstruction(succLabel);
                    block->Instruction_list[i] = brToSucc;
                }

                // 处理完ret后跳出循环
                break;
            }
        }
    }

    // 删除callInst
    {
        auto &inst_list = callerBlock->Instruction_list;
        auto pos = std::find(inst_list.begin(), inst_list.end(), (Instruction)callInst);
        if (pos != inst_list.end()) {
            inst_list.erase(pos);
        }
    }

    // 更新内联后的CFG边信息
    for (auto &bb_pair : newBlocks) {
        int new_id = bb_pair.first;
        BasicBlock *block = bb_pair.second;
        if (!block->Instruction_list.empty()) {
            Instruction last = block->Instruction_list.back();
            if (last->GetOpcode() == BasicInstruction::BR_COND) {
                BrCondInstruction *brc = (BrCondInstruction *)last;
                int t_id = ((LabelOperand*)brc->GetTrueLabel())->GetLabelNo();
                int f_id = ((LabelOperand*)brc->GetFalseLabel())->GetLabelNo();
                callerCFG->G[new_id].push_back(newBlocks[t_id]);
                callerCFG->invG[t_id].push_back(newBlocks[new_id]);
                callerCFG->G[new_id].push_back(newBlocks[f_id]);
                callerCFG->invG[f_id].push_back(newBlocks[new_id]);
            } else if (last->GetOpcode() == BasicInstruction::BR_UNCOND) {
                BrUncondInstruction *bru = (BrUncondInstruction *)last;
                int d_id = ((LabelOperand*)bru->GetDestLabel())->GetLabelNo();
                if (newBlocks.find(d_id) != newBlocks.end()) {
                    callerCFG->G[new_id].push_back(newBlocks[d_id]);
                    callerCFG->invG[d_id].push_back(newBlocks[new_id]);
                } else if (d_id == newSuccessorBlock->block_id) {
                    callerCFG->G[new_id].push_back(newSuccessorBlock);
                    callerCFG->invG[newSuccessorBlock->block_id].push_back(newBlocks[new_id]);
                }
            }
            // RET已转为BR_UNCOND形式处理，不需额外操作
        }
    }

    // newSuccessorBlock的前驱已经添加（在RET和BR中已处理）
}

void InlinePass::Execute() {
    // 遍历所有函数
    for (auto &[funcDef, cfg] : llvmIR->llvm_cfg) {
        int caller_instr_count = RecalculateCallerInstrCount(cfg);

        // 寻找可内联的调用指令
        std::vector<std::tuple<BasicBlock*, CallInstruction*>> calls_to_inline;
        for (auto &bb_pair : *cfg->block_map) {
            BasicBlock *block = bb_pair.second;
            for (auto inst : block->Instruction_list) {
                if (inst->GetOpcode() == BasicInstruction::CALL) {
                    CallInstruction *callInst = dynamic_cast<CallInstruction*>(inst);
                    if (!callInst) continue;
                    std::string calleeName = callInst->GetFunctionName();
                    auto it = CFGMap.find(calleeName);
                    if (it != CFGMap.end()) {
                        CFG* calleeCFG = it->second;
                        FuncDefInstruction calleeFunc = calleeCFG->function_def;
                        if (CanInlineFunction(calleeFunc, caller_instr_count)) {
                            calls_to_inline.emplace_back(block, callInst);
                        }
                    }
                }
            }
        }

        // 不断内联直到没有可内联的调用
        while (!calls_to_inline.empty()) {
            for (auto &[block, callInst] : calls_to_inline) {
                std::string calleeName = callInst->GetFunctionName();
                auto it = CFGMap.find(calleeName);
                if (it != CFGMap.end()) {
                    CFG* calleeCFG = it->second;
                    FuncDefInstruction calleeFunc = calleeCFG->function_def;
                    InlineFunctionCall(cfg, block, callInst, calleeFunc);
                }
            }

            calls_to_inline.clear();
            caller_instr_count = RecalculateCallerInstrCount(cfg);

            for (auto &bb_pair : *cfg->block_map) {
                BasicBlock *block = bb_pair.second;
                for (auto inst : block->Instruction_list) {
                    if (inst->GetOpcode() == BasicInstruction::CALL) {
                        CallInstruction *callInst = dynamic_cast<CallInstruction*>(inst);
                        if (!callInst) continue;
                        std::string calleeName = callInst->GetFunctionName();
                        auto it = CFGMap.find(calleeName);
                        if (it != CFGMap.end()) {
                            CFG* calleeCFG = it->second;
                            FuncDefInstruction calleeFunc = calleeCFG->function_def;
                            if (CanInlineFunction(calleeFunc, caller_instr_count)) {
                                calls_to_inline.emplace_back(block, callInst);
                            }
                        }
                    }
                }
            }
        }
    }
}
