#include "tco.h"

void TCOPass::MakeFunctionOneExit(CFG *C) {
    std::queue<LLVMBlock> exitQueue;
    BasicInstruction::LLVMType retInstrType = BasicInstruction::VOID;
    int retBlockCount = 0;

    for (auto &pairBlock : *(C->block_map)) {
        auto &bbId = pairBlock.first;
        auto &basicBlk = pairBlock.second;

        auto lastInst = basicBlk->Instruction_list.back();
        if (lastInst->GetOpcode() != BasicInstruction::RET) {
            continue;
        }

        retBlockCount++;
        auto *returnInstr = dynamic_cast<RetInstruction *>(lastInst);
        retInstrType = returnInstr->GetType();
        exitQueue.push(basicBlk);
    }

    if (retBlockCount == 0) {
        std::cout << "[MakeFunctionOneExit] 未检测到任何返回块，创建新块..." << std::endl;
        C->block_map->clear();
        C->max_label = -1;
        C->max_reg = -1;

        auto newBlock = C->NewBlock();
        switch (C->function_def->GetReturnType()) {
            case BasicInstruction::VOID:
                newBlock->InsertInstruction(1, new RetInstruction(BasicInstruction::VOID, nullptr));
                break;
            case BasicInstruction::I32:
                newBlock->InsertInstruction(1, new RetInstruction(BasicInstruction::I32, new ImmI32Operand(0)));
                break;
            case BasicInstruction::FLOAT32:
                newBlock->InsertInstruction(1, new RetInstruction(BasicInstruction::FLOAT32, new ImmF32Operand(0)));
                break;
            default:
                ERROR("Unexpected Type in MakeFunctionOneExit");
                break;
        }
        C->ret_block = newBlock;
        C->BuildCFG();
        return;
    }

    if (retBlockCount == 1) {
        std::cout << "[MakeFunctionOneExit] 只找到 1 个返回块，使用它作为唯一的 ret_block" << std::endl;
        C->ret_block = exitQueue.front();
        exitQueue.pop();
        return;
    }

    std::cout << "[MakeFunctionOneExit] 检测到多个返回块，需要合并..." << std::endl;
    auto mergedRetBlock = C->NewBlock();

    if (retInstrType != BasicInstruction::VOID) {
        auto regRetPtr = GetNewRegOperand(++C->max_reg);
        auto regFinalRet = GetNewRegOperand(++C->max_reg);

        auto &firstBlockInMap = C->block_map->begin()->second;
        auto *allocaInst = new AllocaInstruction(retInstrType, regRetPtr);
        firstBlockInMap->InsertInstruction(0, allocaInst);

        while (!exitQueue.empty()) {
            auto curBB = exitQueue.front();
            exitQueue.pop();
            auto *originRet = dynamic_cast<RetInstruction *>(curBB->Instruction_list.back());
            curBB->Instruction_list.pop_back();
            curBB->InsertInstruction(1, new StoreInstruction(retInstrType, regRetPtr, originRet->GetRetVal()));
            curBB->InsertInstruction(1, new BrUncondInstruction(GetNewLabelOperand(mergedRetBlock->block_id)));
        }

        mergedRetBlock->InsertInstruction(1, new LoadInstruction(retInstrType, regRetPtr, regFinalRet));
        mergedRetBlock->InsertInstruction(1, new RetInstruction(retInstrType, regFinalRet));
    } else {
        while (!exitQueue.empty()) {
            auto curBB = exitQueue.front();
            exitQueue.pop();
            curBB->Instruction_list.pop_back();
            curBB->InsertInstruction(1, new BrUncondInstruction(GetNewLabelOperand(mergedRetBlock->block_id)));
        }
        mergedRetBlock->InsertInstruction(1, new RetInstruction(BasicInstruction::VOID, nullptr));
    }

    C->ret_block = mergedRetBlock;
    C->BuildCFG();
}

void TCOPass::RetMotion(CFG *C) {
    if (C->function_def->GetReturnType() != BasicInstruction::VOID) {
        std::cout << "[RetMotion] 跳过：函数返回类型不是 VOID" << std::endl;
        return;
    }

    auto blockMapCopy = *(C->block_map);

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

    for (auto &pairBlock : blockMapCopy) {
        auto &bbId = pairBlock.first;
        auto &basicBlk = pairBlock.second;

        for (auto &inst : basicBlk->Instruction_list) {
            if (inst->GetOpcode() == BasicInstruction::CALL) {
                auto *callI = dynamic_cast<CallInstruction *>(inst);
                if (!callI) {
                    continue;
                }

                if (callI->GetFunctionName() != C->function_def->GetFunctionName()) {
                    continue;
                }

                auto retBlockCandidate = fetchRetBB(bbId);
                if (retBlockCandidate == -1) {
                    continue;
                }

                std::cout << "[RetMotion] 在块 " << bbId
                          << " 检测到对自身函数的调用，可直接替换为 ret void" << std::endl;

                basicBlk->Instruction_list.pop_back();
                basicBlk->InsertInstruction(1, new RetInstruction(BasicInstruction::VOID, nullptr));
                break;
            }
        }
    }
}

void TCOPass::TailRecursionToLoop(CFG *cfg, FuncDefInstruction defI) {
    std::string currentFuncName = defI->GetFunctionName();

    for (auto &pairBlock : *(cfg->block_map)) {
        int bb_id = pairBlock.first;
        auto blockPtr = pairBlock.second;
        auto &instList = blockPtr->Instruction_list;

        if (instList.size() < 2) {
            continue;
        }

        auto lastInst = instList.back();
        auto beforeLastInst = *(instList.rbegin() + 1);

        if (lastInst->GetOpcode() != BasicInstruction::RET) {
            continue;
        }

        if (beforeLastInst->GetOpcode() != BasicInstruction::CALL) {
            continue;
        }

        auto *callInst = dynamic_cast<CallInstruction *>(beforeLastInst);
        if (!callInst) {
            continue;
        }

        if (callInst->GetFunctionName() != currentFuncName) {
            continue;
        }

        auto *retInst = dynamic_cast<RetInstruction *>(lastInst);
        if (!retInst) {
            continue;
        }
        if (retInst->GetRetVal() != callInst->DefinesResult()) {
            continue;
        }

        std::cout << "【尾递归检测】函数 " << currentFuncName 
                  << " 中的基本块 " << bb_id 
                  << " 存在尾递归，将进行循环化处理。" << std::endl;

        ConvertTailRecursionToLoop(cfg, blockPtr, callInst, defI);
    }

    if (defI->isTCO) {
        auto &mapBlocks = *(cfg->block_map);
        auto &entryBlock = mapBlocks[1];
        auto &entryInstrs = entryBlock->Instruction_list;

        for (size_t idx = 0; idx < defI->formals.size(); ++idx) {
            auto *phiInst = new PhiInstruction(
                defI->formals[idx], 
                defI->phi_result_list[idx],
                defI->phi_list_list[idx]
            );
            entryInstrs.insert(entryInstrs.begin(), phiInst);
        }
    }
}

void TCOPass::ConvertTailRecursionToLoop(CFG *cfg, BasicBlock *block, 
                                         CallInstruction *callInst,
                                         FuncDefInstruction defI)
{
    auto &allBlocks = *(cfg->block_map);

    auto &theFirstBlock = allBlocks[0];
    auto &firstBlkInstrList = theFirstBlock->Instruction_list;
    auto &theSecondBlock = allBlocks[1];
    auto &secondBlkInstrList = theSecondBlock->Instruction_list;

    if (!defI->isTCO) {
        for (auto reverseIter = firstBlkInstrList.rbegin() + 1; 
             reverseIter != firstBlkInstrList.rend(); 
             ++reverseIter)
        {
            secondBlkInstrList.push_front(*reverseIter);
        }
        while (firstBlkInstrList.size() > 1) {
            firstBlkInstrList.pop_front();
        }
    }

    auto paramOperands = callInst->GetParameterList();

    for (size_t idx = 0; idx < defI->formals.size(); ++idx) {
        Operand oldParam = defI->formals_reg[idx];

        std::vector<std::pair<Operand, Operand>> phiPairs;
        phiPairs.emplace_back(GetNewLabelOperand(0), oldParam);
        phiPairs.emplace_back(GetNewLabelOperand(block->block_id), paramOperands[idx].second);

        if (!defI->isTCO) {
            defI->phi_list_list.resize(defI->formals.size());
            defI->phi_result_list.resize(defI->formals.size());
            defI->phi_list_list[idx].push_back({GetNewLabelOperand(0), oldParam});

            Operand newPhiReg = GetNewRegOperand(++cfg->max_reg);
            defI->phi_result_list[idx] = newPhiReg;
        }

        defI->phi_list_list[idx].push_back(
            { GetNewLabelOperand(block->block_id), paramOperands[idx].second }
        );

        for (auto &blkPair : allBlocks) {
            auto &instrList = blkPair.second->Instruction_list;
            for (auto &instr : instrList) {
                instr->resetOperand(oldParam, defI->phi_result_list[idx]);
            }
        }
    }

    auto &instrsInBlock = block->Instruction_list;
    instrsInBlock.pop_back();
    instrsInBlock.pop_back();

    instrsInBlock.push_back(
        new BrUncondInstruction(GetNewLabelOperand(1))
    );

    defI->isTCO = true;
}

void TCOPass::Execute() {
    for (auto [_, cfg] : llvmIR->llvm_cfg) {
        RetMotion(cfg);
        TailRecursionToLoop(cfg, _);
        MakeFunctionOneExit(cfg);
    }
}
