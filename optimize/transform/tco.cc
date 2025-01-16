#include "tco.h"

void TCOPass::MakeFunctionOneExit(CFG *C) {
    std::cout << "[MakeFunctionOneExit] 开始执行，准备合并多个返回块..." << std::endl;

    std::queue<LLVMBlock> exitQueue;
    BasicInstruction::LLVMType retInstrType = BasicInstruction::VOID;
    int retBlockCount = 0;

    // 遍历所有基本块，收集含有返回指令的块
    for (auto &pairBlock : *(C->block_map)) {
        auto &bbId = pairBlock.first;
        auto &basicBlk = pairBlock.second;

        if (basicBlk->Instruction_list.empty()) {
            continue;
        }

        auto lastInst = basicBlk->Instruction_list.back();
        if (lastInst->GetOpcode() != BasicInstruction::RET) {
            continue;
        }

        retBlockCount++;
        auto *returnInstr = dynamic_cast<RetInstruction *>(lastInst);
        if (!returnInstr) {
            std::cout << "[MakeFunctionOneExit] 警告：无法动态转换为 RetInstruction，跳过" << std::endl;
            continue;
        }

        std::cout << "[MakeFunctionOneExit] 检测到返回指令所在块 ID: " << bbId << std::endl;
        retInstrType = returnInstr->GetType();
        exitQueue.push(basicBlk);
    }

    if (retBlockCount == 0) {
        std::cout << "[MakeFunctionOneExit] 未检测到任何返回块，创建新块并插入默认返回..." << std::endl;
        C->block_map->clear();
        C->max_label = -1;
        C->max_reg = -1;

        auto newBlock = C->NewBlock();
        switch (C->function_def->GetReturnType()) {
            case BasicInstruction::VOID:
                std::cout << "[MakeFunctionOneExit] 函数返回类型为 VOID，插入 ret void" << std::endl;
                newBlock->InsertInstruction(1, new RetInstruction(BasicInstruction::VOID, nullptr));
                break;
            case BasicInstruction::I32:
                std::cout << "[MakeFunctionOneExit] 函数返回类型为 I32，插入 ret i32 0" << std::endl;
                newBlock->InsertInstruction(1, new RetInstruction(BasicInstruction::I32, new ImmI32Operand(0)));
                break;
            case BasicInstruction::FLOAT32:
                std::cout << "[MakeFunctionOneExit] 函数返回类型为 FLOAT32，插入 ret float 0" << std::endl;
                newBlock->InsertInstruction(1, new RetInstruction(BasicInstruction::FLOAT32, new ImmF32Operand(0)));
                break;
            default:
                std::cout << "[MakeFunctionOneExit] 错误：未预期的返回类型！" << std::endl;
                ERROR("Unexpected Type in MakeFunctionOneExit");
                break;
        }
        C->ret_block = newBlock;
        std::cout << "[MakeFunctionOneExit] 新返回块已创建并构建 CFG" << std::endl;
        C->BuildCFG();
        return;
    }

    if (retBlockCount == 1) {
        std::cout << "[MakeFunctionOneExit] 只找到 1 个返回块，无需合并，直接使用它作为唯一的 ret_block" << std::endl;
        C->ret_block = exitQueue.front();
        exitQueue.pop();
        return;
    }

    std::cout << "[MakeFunctionOneExit] 检测到多个返回块，共计 " << retBlockCount << " 个，需要合并..." << std::endl;
    auto mergedRetBlock = C->NewBlock();

    // 如果返回类型不是 VOID，需要生成临时寄存器来合并返回值
    if (retInstrType != BasicInstruction::VOID) {
        std::cout << "[MakeFunctionOneExit] 返回类型非 VOID，为所有返回块创建统一的合并返回值处理..." << std::endl;
        auto regRetPtr = GetNewRegOperand(++C->max_reg);
        auto regFinalRet = GetNewRegOperand(++C->max_reg);

        // 在第一个基本块的前端插入 Alloca，用于存储返回值
        auto &firstBlockInMap = C->block_map->begin()->second;
        auto *allocaInst = new AllocaInstruction(retInstrType, regRetPtr);
        std::cout << "[MakeFunctionOneExit] 在块 " << C->block_map->begin()->first 
                  << " 中插入 AllocaInstruction，以存储返回值" << std::endl;
        firstBlockInMap->InsertInstruction(0, allocaInst);

        // 将各个返回块的最终返回值写入同一个地址，然后跳转到 mergedRetBlock
        while (!exitQueue.empty()) {
            auto curBB = exitQueue.front();
            exitQueue.pop();
            auto *originRet = dynamic_cast<RetInstruction *>(curBB->Instruction_list.back());
            if (!originRet) {
                std::cout << "[MakeFunctionOneExit] 警告：无法转换为 RetInstruction，跳过" << std::endl;
                continue;
            }
            curBB->Instruction_list.pop_back();
            curBB->InsertInstruction(1, new StoreInstruction(retInstrType, regRetPtr, originRet->GetRetVal()));
            curBB->InsertInstruction(1, new BrUncondInstruction(GetNewLabelOperand(mergedRetBlock->block_id)));
            std::cout << "[MakeFunctionOneExit] 块 " << curBB->block_id 
                      << " 已重定向到 mergedRetBlock" << std::endl;
        }

        // 在合并块中从同一地址读出，然后统一 ret
        mergedRetBlock->InsertInstruction(1, new LoadInstruction(retInstrType, regRetPtr, regFinalRet));
        mergedRetBlock->InsertInstruction(1, new RetInstruction(retInstrType, regFinalRet));
        std::cout << "[MakeFunctionOneExit] 已在合并块插入统一返回逻辑" << std::endl;
    } else {
        // 返回类型为 VOID，只需要将所有返回指令改为无条件跳转到合并块
        std::cout << "[MakeFunctionOneExit] 返回类型为 VOID，直接跳转合并" << std::endl;
        while (!exitQueue.empty()) {
            auto curBB = exitQueue.front();
            exitQueue.pop();
            curBB->Instruction_list.pop_back();
            curBB->InsertInstruction(1, new BrUncondInstruction(GetNewLabelOperand(mergedRetBlock->block_id)));
            std::cout << "[MakeFunctionOneExit] 块 " << curBB->block_id 
                      << " 重定向到 mergedRetBlock" << std::endl;
        }
        mergedRetBlock->InsertInstruction(1, new RetInstruction(BasicInstruction::VOID, nullptr));
    }

    C->ret_block = mergedRetBlock;
    std::cout << "[MakeFunctionOneExit] 合并返回块设置完成，正在重建 CFG..." << std::endl;
    C->BuildCFG();
    std::cout << "[MakeFunctionOneExit] CFG 重建完成" << std::endl;
}

void TCOPass::RetMotion(CFG *C) {
    std::cout << "[RetMotion] 开始执行 RetMotion 优化..." << std::endl;

    if (C->function_def->GetReturnType() != BasicInstruction::VOID) {
        std::cout << "[RetMotion] 跳过：函数返回类型不是 VOID" << std::endl;
        return;
    }

    auto blockMapCopy = *(C->block_map);

    // 检查一个块到达 retBlock 的唯一性
    std::function<int(int)> fetchRetBB = [&](int startId) {
        if (C->G[startId].empty()) {
            return -1;
        }

        int currentId = startId;
        while (!C->G[currentId].empty()) {
            if (C->G[currentId].size() >= 2) {
                return -1;
            }
            currentId = C->G[currentId][0]->block_id;

            auto nextBB = blockMapCopy[currentId];
            if (nextBB->Instruction_list.size() > 1) {
                return -1;
            }
        }
        return currentId;
    };

    // 遍历所有基本块，如果检测到对自身函数（且返回类型为 void）的调用，则可直接将其替换为 ret void
    for (auto &pairBlock : blockMapCopy) {
        auto &bbId = pairBlock.first;
        auto &basicBlk = pairBlock.second;

        if (basicBlk->Instruction_list.empty()) {
            continue;
        }

        for (auto &inst : basicBlk->Instruction_list) {
            if (inst->GetOpcode() == BasicInstruction::CALL) {
                auto *callI = dynamic_cast<CallInstruction *>(inst);
                if (!callI) {
                    std::cout << "[RetMotion] 警告：CallInstruction 动态转换失败，跳过" << std::endl;
                    continue;
                }

                if (callI->GetFunctionName() != C->function_def->GetFunctionName()) {
                    // 只针对对自身函数的调用
                    continue;
                }

                auto retBlockCandidate = fetchRetBB(bbId);
                if (retBlockCandidate == -1) {
                    continue;
                }

                std::cout << "[RetMotion] 在块 " << bbId
                          << " 中检测到对自身函数的调用，可直接替换为 ret void。" << std::endl;
                // 将此块中最后一条指令（通常应是 CALL）替换为 ret void
                basicBlk->Instruction_list.pop_back();
                basicBlk->InsertInstruction(1, new RetInstruction(BasicInstruction::VOID, nullptr));
                break;
            }
        }
    }

    std::cout << "[RetMotion] RetMotion 优化执行结束" << std::endl;
}

bool TCOPass::TailRecursiveEliminateCheck(CFG* C) {
    auto FuncdefI = C->function_def;
    if (FuncdefI->GetFormalSize() > 5) {
        return false;
    }
    auto bb0 = (*C->block_map->begin()).second;
    int AllocaRegCnt = 0;
    std::unordered_map<int, int> AllocaReg;
    std::unordered_map<int, int> GEPMap;
    for (auto I : bb0->Instruction_list) {
        if (I->GetOpcode() != BasicInstruction::ALLOCA) {
            continue;
        }
        auto AllocaI = (AllocaInstruction *)I;
        if (AllocaI->GetDims().empty()) {
            continue;
        }
        AllocaReg[AllocaI->GetResultRegNo()] = ++AllocaRegCnt;
    }

    if (!AllocaRegCnt) {
        return true;
    }
    for (auto [id, bb] : *C->block_map) {
        for (auto I : bb->Instruction_list) {
            if (I->GetOpcode() != BasicInstruction::GETELEMENTPTR && I->GetOpcode() != BasicInstruction::CALL) {
                continue;
            }

            if (I->GetOpcode() == BasicInstruction::GETELEMENTPTR) {
                auto GetelementptrI = (GetElementptrInstruction *)I;
                auto PtrReg = ((RegOperand *)GetelementptrI->GetPtrVal())->GetRegNo();
                auto ResultReg = GetelementptrI->GetResultRegNo();
                if (PtrReg == 0 || AllocaReg.find(PtrReg) == AllocaReg.end()) {
                    continue;
                }
                GEPMap[ResultReg] = PtrReg;
            } else {
                auto CallI = (CallInstruction *)I;
                for (auto args : CallI->GetParameterList()) {
                    auto args_regno = ((RegOperand *)args.second)->GetRegNo();
                    if (GEPMap.find(args_regno) != GEPMap.end()) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

void TCOPass::TailRecursiveEliminate(CFG* C) {
    bool TRECheck = TailRecursiveEliminateCheck(C);
    if (!TRECheck) {
        return;
    }
    auto FuncdefI = C->function_def;
    auto bb0 = (*C->block_map->begin()).second;
    bool NeedtoInsertPTR = false;
    std::deque<Instruction> StoreDeque;
    std::deque<Instruction> AllocaDeque; 
    std::set<Instruction> EraseSet;
    std::unordered_map<int, RegOperand*> PtrUsed;
    for (auto [id, bb] : *C->block_map) {
        if (bb->Instruction_list.back()->GetOpcode() != BasicInstruction::RET) {
            continue;
        }
        auto retI = (RetInstruction *)bb->Instruction_list.back();
        for (auto I : bb->Instruction_list) {
            if (I->GetOpcode() != BasicInstruction::CALL) {
                continue;
            }
            auto callI = (CallInstruction *)I;
            if (callI->GetFunctionName() != FuncdefI->GetFunctionName()) {
                continue;
            }
            bool opt1 = callI->GetResult() != NULL && callI->GetResult()->GetFullName() == retI->GetRetVal()->GetFullName();
            bool opt2 = callI->GetResult() == NULL && retI->GetType() == BasicInstruction::VOID;
            if (opt1 || opt2) {
                auto list_size = callI->GetParameterList().size();
                for (auto i = 0; i < list_size; i++) {
                    auto callI_reg = (RegOperand *)(callI->GetParameterList()[i].second);
                    if (callI_reg->GetRegNo() == i) {
                        continue;
                    }
                    if (FuncdefI->formals[i] == BasicInstruction::PTR) {
                        NeedtoInsertPTR = true;
                        if (PtrUsed.find(i) == PtrUsed.end()) {
                            auto PtrReg = GetNewRegOperand(++C->max_reg);
                            PtrUsed[i] = PtrReg;
                        }
                    }
                }
            }
        }
    }
    if (NeedtoInsertPTR) {
        for (u_int32_t i = 0; i < FuncdefI->GetFormalSize(); ++i) {
            if (FuncdefI->formals[i] == BasicInstruction::PTR && PtrUsed.find(i) != PtrUsed.end()) {
                auto PtrReg = PtrUsed[i];
                AllocaDeque.push_back(new AllocaInstruction(BasicInstruction::PTR, PtrReg));
                StoreDeque.push_back(new StoreInstruction(BasicInstruction::PTR, PtrReg, FuncdefI->formals_reg[i]));
            }
        }
        for (auto [id, bb] : *C->block_map) {
            auto tmp_Instruction_list = bb->Instruction_list;
            bb->Instruction_list.clear();
            for (auto I : tmp_Instruction_list) {
                auto ResultOperands = I->GetNonResultOperands();
                bool NeedtoUpdate = false;
                if (id != 0 && NeedtoInsertPTR && !ResultOperands.empty()) {
                    for (u_int32_t i = 0; i < ResultOperands.size(); i++) {
                        auto ResultReg = ResultOperands[i];
                        for (u_int32_t j = 0; j < FuncdefI->formals_reg.size(); j++) {
                            if (PtrUsed.find(j) == PtrUsed.end()) {
                                continue;
                            }
                            auto DefReg = FuncdefI->formals_reg[j];
                            if (ResultReg->GetFullName() == DefReg->GetFullName()) {
                                NeedtoUpdate = true;
                                auto PtrReg = GetNewRegOperand(++C->max_reg);
                                bb->InsertInstruction(1, new LoadInstruction(BasicInstruction::PTR, PtrUsed[j], PtrReg));
                                ResultOperands[i] = PtrReg;
                                break;
                            }
                        }
                    }
                    if (NeedtoUpdate) {
                        I->SetNonResultOperands(ResultOperands);
                    }
                }
                bb->InsertInstruction(1, I);
            }
        }
    }

    while (!StoreDeque.empty()) {
        auto I = StoreDeque.back();
        bb0->InsertInstruction(0, I);
        StoreDeque.pop_back();
    }
    for (auto *it : AllocaDeque) {
        bb0->InsertInstruction(0, it);
    }
    while (!AllocaDeque.empty()) {
        AllocaDeque.pop_back();
    }
    for (auto [id, bb] : *C->block_map) {
        if (bb->Instruction_list.back()->GetOpcode() != BasicInstruction::RET || bb->Instruction_list.size() <= 1) {
            continue;
        }
        auto retI = (RetInstruction *)bb->Instruction_list.back();
        auto I = *(--(--bb->Instruction_list.end()));
        if (I->GetOpcode() != BasicInstruction::CALL) {
            continue;
        }
        auto callI = (CallInstruction *)I;
        if (callI->GetFunctionName() != FuncdefI->GetFunctionName()) {
            continue;
        }
        bool opt1 = callI->GetResult() != NULL && callI->GetResult()->GetFullName() == retI->GetRetVal()->GetFullName();
        bool opt2 = callI->GetResult() == NULL && retI->GetType() == BasicInstruction::VOID;
        if (opt1 || opt2) {
            EraseSet.insert(callI);
            EraseSet.insert(retI);
            auto list_size = callI->GetParameterList().size();
            auto bb0_it = --bb0->Instruction_list.end();
            auto bb0_ptr_it = bb0->Instruction_list.begin();
            while ((*bb0_ptr_it)->GetOpcode() == BasicInstruction::ALLOCA) {
                bb0_ptr_it++;
            }
            for (auto i = 0; i < list_size; i++) {
                auto callI_type = callI->GetParameterList()[i].first;
                auto callI_reg = (RegOperand *)(callI->GetParameterList()[i].second);
                if (callI_reg->GetRegNo() == i || (callI_type == BasicInstruction::PTR && PtrUsed.find(i) == PtrUsed.end())) {
                    continue;
                }
                Instruction allocaI;
                if (callI->GetParameterList()[i].first == BasicInstruction::PTR) {
                    bb0_ptr_it--;
                    allocaI = *bb0_ptr_it;
                } else {
                    bb0_it--;
                    allocaI = *bb0_it;
                    while (allocaI->GetOpcode() != BasicInstruction::ALLOCA) {
                        bb0_it--;
                        allocaI = *bb0_it;
                    }
                }
                auto storeI = new StoreInstruction(callI->GetParameterList()[i].first, allocaI->GetResultReg(),
                                                   callI->GetParameterList()[i].second);
                bb->InsertInstruction(1, storeI);
            }
            bb->InsertInstruction(1, new BrUncondInstruction(GetNewLabelOperand(1)));
        }
    }
    for (auto [id, bb] : *C->block_map) {
        auto tmp_Instruction_list = bb->Instruction_list;
        bb->Instruction_list.clear();
        for (auto I : tmp_Instruction_list) {
            if (EraseSet.find(I) != EraseSet.end()) {
                continue;
            }
            bb->InsertInstruction(1, I);
        }
    }
    EraseSet.clear();
    PtrUsed.clear();
    C->BuildCFG();
}

void TCOPass::Execute() {
    std::cout << "[TCOPass::Execute] 开始执行 TCO 优化流程..." << std::endl;
    for (auto &[funcDefInstr, cfg] : llvmIR->llvm_cfg) {
        std::cout << "[TCOPass::Execute] 函数 " << funcDefInstr->GetFunctionName() << " 的 CFG 开始处理..." << std::endl;
        RetMotion(cfg);
        TailRecursiveEliminate(cfg);
        MakeFunctionOneExit(cfg);
        std::cout << "[TCOPass::Execute] 函数 " << funcDefInstr->GetFunctionName() << " 的 CFG 处理完毕" << std::endl;
    }
    std::cout << "[TCOPass::Execute] 所有函数的 TCO 优化流程执行完毕" << std::endl;
}
