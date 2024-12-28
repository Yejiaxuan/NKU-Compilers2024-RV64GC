// Reference: https://github.com/yuhuifishash/SysY/blob/master/optimize/function/simple_function.cc line1-line419
#include "tco.h"

void TCOPass::MakeFunctionOneExit(CFG *C) {
    std::queue<LLVMBlock> OneExitqueue;
    enum BasicInstruction::LLVMType ret_type = BasicInstruction::VOID;
    int ret_cnt = 0;
    for (auto [id, bb] : *C->block_map) {
        auto I = bb->Instruction_list.back();
        if (I->GetOpcode() != BasicInstruction::RET) {
            continue;
        }
        ret_cnt++;
        auto RetI = (RetInstruction *)I;
        ret_type = RetI->GetType();
        OneExitqueue.push(bb);
    }
    if (ret_cnt == 0) {
        C->block_map->clear();
        C->max_label = -1;
        C->max_reg = -1;
        auto bb = C->NewBlock();
        if (C->function_def->GetReturnType() == BasicInstruction::VOID) {
            bb->InsertInstruction(1, new RetInstruction(BasicInstruction::VOID, nullptr));
        } else if (C->function_def->GetReturnType() == BasicInstruction::I32) {
            bb->InsertInstruction(1, new RetInstruction(BasicInstruction::I32, new ImmI32Operand(0)));
        } else if (C->function_def->GetReturnType() == BasicInstruction::FLOAT32) {
            bb->InsertInstruction(1, new RetInstruction(BasicInstruction::FLOAT32, new ImmF32Operand(0)));
        } else {
            ERROR("Unexpected Type");
        }
        C->ret_block = bb;
        C->BuildCFG();
        return;
    }
    if (ret_cnt == 1) {
        C->ret_block = OneExitqueue.front();
        if (!OneExitqueue.empty()) {
            OneExitqueue.pop();
        }
        return;
    }
    auto B = C->NewBlock();
    if (ret_type != BasicInstruction::VOID) {
        auto ret_ptr = GetNewRegOperand(++C->max_reg);
        auto B_Retreg = GetNewRegOperand(++C->max_reg);
        auto bb0 = C->block_map->begin()->second;
        auto AllocaI = new AllocaInstruction(ret_type, ret_ptr);
        bb0->InsertInstruction(0, AllocaI);
        while (!OneExitqueue.empty()) {
            auto bb = OneExitqueue.front();
            OneExitqueue.pop();
            auto RetI = (RetInstruction *)bb->Instruction_list.back();
            bb->Instruction_list.pop_back();
            bb->InsertInstruction(1, new StoreInstruction(ret_type, ret_ptr, RetI->GetRetVal()));
            bb->InsertInstruction(1, new BrUncondInstruction(GetNewLabelOperand(B->block_id)));
        }
        B->InsertInstruction(1, new LoadInstruction(ret_type, ret_ptr, B_Retreg));
        B->InsertInstruction(1, new RetInstruction(ret_type, B_Retreg));
    } else {
        while (!OneExitqueue.empty()) {
            auto bb = OneExitqueue.front();
            OneExitqueue.pop();
            bb->Instruction_list.pop_back();
            bb->InsertInstruction(1, new BrUncondInstruction(GetNewLabelOperand(B->block_id)));
        }
        B->InsertInstruction(1, new RetInstruction(BasicInstruction::VOID, nullptr));
    }

    C->ret_block = B;
    C->BuildCFG();
}

void TCOPass::RetMotion(CFG *C) {
    if (C->function_def->GetReturnType() != BasicInstruction::VOID) {
        return;
    }

    auto blockmap = *C->block_map;
    std::function<int(int)> GetRetBB = [&](int ubbid) {
        if (C->G[ubbid].empty()) {
            return -1;
        }
        while (!C->G[ubbid].empty()) {
            if (C->G[ubbid].size() >= 2) {
                return -1;
            }
            ubbid = C->G[ubbid][0]->block_id;
            auto bb = blockmap[ubbid];
            if (bb->Instruction_list.size() > 1) {
                return -1;
            }
        }
        return ubbid;
    };

    for (auto [id, bb] : blockmap) {
        for (auto I : bb->Instruction_list) {
            if (I->GetOpcode() == BasicInstruction::CALL) {
                auto callI = (CallInstruction *)I;
                auto function_name = callI->GetFunctionName();
                if (function_name != C->function_def->GetFunctionName()) {
                    continue;
                }
                auto retbbid = GetRetBB(id);
                if (retbbid == -1) {
                    continue;
                }
                bb->Instruction_list.pop_back();
                bb->InsertInstruction(1, new RetInstruction(BasicInstruction::VOID, nullptr));
                break;
            }
        }
    }
}

// 尾递归转循环优化的主函数
void TCOPass::TailRecursionToLoop(CFG *cfg, FuncDefInstruction defI) {
    std::string func_name = defI->GetFunctionName();

    for (auto &[block_id, block] : *(cfg->block_map)) {
        auto &instr_list = block->Instruction_list;

        if (instr_list.size() < 2)
            continue;

        // 获取倒数第二和倒数第一条指令
        auto last_instr = instr_list.back();
        auto second_last_instr = *(instr_list.rbegin() + 1);

        // 1. 检查最后一条指令是否是 ret
        if (last_instr->GetOpcode() != BasicInstruction::RET) {
            continue;
        }
        std::cout << 1 << " " << func_name << " " << block_id << std::endl;

        // 2. 检查倒数第二条指令是否是 call
        if (second_last_instr->GetOpcode() != BasicInstruction::CALL) {
            continue;
        }
        std::cout << 2 << std::endl;

        CallInstruction *call_instr = static_cast<CallInstruction *>(second_last_instr);

        // 3. 确认调用的函数是当前函数自身
        if (call_instr->GetFunctionName() != func_name) {
            // std::cout << call_instr->GetFunctionName() << std::endl;
            // std::cout << func_name << std::endl;
            continue;
        }
        std::cout << 3 << std::endl;

        // 4. 确认 ret 返回的值是 call 指令的结果
        RetInstruction *ret_instr = static_cast<RetInstruction *>(last_instr);
        if (ret_instr->GetRetVal() != call_instr->DefinesResult()) {
            continue;
        }

        std::cout << "Tail recursion detected in block " << block_id << " " << func_name << std::endl;

        // 转换尾递归调用为循环
        ConvertTailRecursionToLoop(cfg, block, call_instr, defI);
    }
    if(defI->isTCO==true){
        auto &blocks = *(cfg->block_map);
        auto &entry_block = blocks[1];    // 函数入口基本块
        auto &entry_instr_list = entry_block->Instruction_list;
        for (size_t i = 0; i < defI->formals.size(); ++i) {
            entry_instr_list.insert(entry_instr_list.begin(), new PhiInstruction(defI->formals[i], defI->phi_result_list[i], defI->phi_list_list[i]));
        }
    }
}

// 执行尾递归到循环的转换
void TCOPass::ConvertTailRecursionToLoop(CFG *cfg, BasicBlock *block, CallInstruction *call_instr,
                                         FuncDefInstruction defI) {

    // 1. 获取入口基本块及其参数
    auto &blocks = *(cfg->block_map);
    auto &entry_block = blocks[1];    // 函数入口基本块
    auto &entry_instr_list = entry_block->Instruction_list;
    auto &first_block = blocks[0];
    auto &first_instr_list = first_block->Instruction_list;

    if (defI->isTCO == false) {
        //defI->isTCO = true;

        // std::cout << 666 << std::endl;

        for (auto it = first_instr_list.rbegin() + 1; it != first_instr_list.rend(); ++it) {
            entry_instr_list.push_front(*it);    // 注意这里是 *it，因为 it 是迭代器
        }
        while (first_instr_list.size() > 1) {
            first_instr_list.pop_front();    // 弹出第一个元素
        }
    }

    // std::cout << 777 << std::endl;

    // // 2. 创建 phi 指令，合并初始参数值与新的值

    auto args = call_instr->GetParameterList();

    // std::vector<Operand> phi_operands;
    for (size_t i = 0; i < defI->formals.size(); ++i) {
        Operand param = defI->formals_reg[i];              // 原始参数寄存器
        //Operand phi = GetNewRegOperand(++cfg->max_reg);    // 创建新的寄存器

        // Operand newparam = GetNewRegOperand(++cfg->max_reg);
        // phi_operands.push_back(newparam);

        std::vector<std::pair<Operand, Operand>> phi_list;
        phi_list.push_back({GetNewLabelOperand(0), param});
        int block_id = block->block_id;
        phi_list.push_back({GetNewLabelOperand(block_id), args[i].second});

        //std::cout << 777 << std::endl;

        if (defI->isTCO == false) {
            //defI->isTCO = true;

            defI->phi_list_list.resize(defI->formals.size());
            defI->phi_result_list.resize(defI->formals.size());

            defI->phi_list_list[i].push_back({GetNewLabelOperand(0), param});

            Operand phi = GetNewRegOperand(++cfg->max_reg);
            defI->phi_result_list[i]=phi;
            
            //std::cout << 777 << std::endl;
        }
        defI->phi_list_list[i].push_back({GetNewLabelOperand(block_id), args[i].second});
        //defI->phi_result_list[i]=phi;

        //std::cout << 777 << std::endl;

        for (auto &[block_id, block] : blocks) {
            // 获取当前基本块的指令序列
            auto &instr_list = block->Instruction_list;

            // 遍历当前块的指令序列
            for (auto &instr : instr_list) {
                instr->resetOperand(param, defI->phi_result_list[i]);
            }
        }

        //std::cout << 777 << std::endl;

        // 插入 phi 指令：合并初始参数值
        //entry_instr_list.insert(entry_instr_list.begin(), new PhiInstruction(defI->formals[i], phi, phi_list));
    }

    // // 3. 替换尾递归调用为参数更新
    auto &instr_list = block->Instruction_list;
    instr_list.pop_back();    // 删除尾递归调用指令
    instr_list.pop_back();

    // // auto args = call_instr->GetParameterList(); // 获取调用参数
    // // for (size_t i = 0; i < args.size(); ++i) {
    // //     Operand updated_value = args[i].second; // 尾递归参数的值
    // //     Operand phi_reg = phi_operands[i];      // 对应的 phi 指令的寄存器
    // //     BasicInstruction::LLVMType type = args[i].first;

    // //     // 更新参数值：替换为赋值或算术指令
    // //     auto move_instr = new ArithmeticInstruction(BasicInstruction::ADD, type, updated_value, new
    // ImmI32Operand(0), phi_reg);
    // //     instr_list.push_back(move_instr);
    // // }

    // // 4. 插入跳转指令回入口基本块
    instr_list.push_back(new BrUncondInstruction(GetNewLabelOperand(1)));
    defI->isTCO = true;
}

void TCOPass::Execute() {
    for (auto [_, cfg] : llvmIR->llvm_cfg) {
        RetMotion(cfg);
        TailRecursionToLoop(cfg, _);
        MakeFunctionOneExit(cfg);
    }
}
