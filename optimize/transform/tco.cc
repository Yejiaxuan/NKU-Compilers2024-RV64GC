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
    for (auto [_, cfg] : llvmIR->llvm_cfg) {
        RetMotion(cfg);
        TailRecursiveEliminate(cfg);
        MakeFunctionOneExit(cfg);
    }
}
