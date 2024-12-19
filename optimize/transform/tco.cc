#include "tco.h"

bool TCOPass::TailRecursiveEliminateCheck(CFG *C) {
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
    bool NeedtoInsertPTR = 0;
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
                        NeedtoInsertPTR = 1;
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
                bool NeedtoUpdate = 0;
                if (id != 0 && NeedtoInsertPTR && !ResultOperands.empty()) {
                    for (u_int32_t i = 0; i < ResultOperands.size(); i++) {
                        auto ResultReg = ResultOperands[i];
                        for (u_int32_t j = 0; j < FuncdefI->formals_reg.size(); j++) {
                            if (PtrUsed.find(j) == PtrUsed.end()) {
                                continue;
                            }
                            auto DefReg = FuncdefI->formals_reg[j];
                            if (ResultReg->GetFullName() == DefReg->GetFullName()) {
                                NeedtoUpdate = 1;
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
    while (!StoreDeque.empty()) {
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
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        TailRecursiveEliminate(cfg);
    }
}
