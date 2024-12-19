#include "tco.h"

void TCOPass::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        TailRecursionToLoop(cfg, defI);
    }
}

void TCOPass::TailRecursionToLoop(CFG *cfg, FuncDefInstruction defI) {
    for (auto &[block_id, block] : *(cfg->block_map)) {
        CallInstruction *call_instr = nullptr;
        if (IsTailRecursiveCall(block, defI, call_instr)) {
            ConvertTailRecursionToLoop(cfg, block, call_instr, defI);
        }
    }

    if (defI->isTCO) {
        auto &entry_block = cfg->block_map->at(1);
        auto &entry_instr_list = entry_block->Instruction_list;
        for (size_t i = 0; i < defI->formals.size(); ++i) {
            entry_instr_list.insert(entry_instr_list.begin(),
                new PhiInstruction(defI->formals[i], defI->phi_result_list[i], defI->phi_list_list[i]));
        }
    }
}

bool TCOPass::IsTailRecursiveCall(BasicBlock* block, FuncDefInstruction defI, CallInstruction*& call_instr) {
    auto &instr_list = block->Instruction_list;

    if (instr_list.size() < 2)
        return false;

    auto last_instr = instr_list.back();
    auto second_last_instr = *(instr_list.rbegin() + 1);

    if (last_instr->GetOpcode() != BasicInstruction::RET)
        return false;

    if (second_last_instr->GetOpcode() != BasicInstruction::CALL)
        return false;

    call_instr = static_cast<CallInstruction*>(second_last_instr);

    if (call_instr->GetFunctionName() != defI->GetFunctionName())
        return false;

    RetInstruction *ret_instr = static_cast<RetInstruction*>(last_instr);
    if (ret_instr->GetRetVal() != call_instr->DefinesResult())
        return false;

    return true;
}

void TCOPass::ConvertTailRecursionToLoop(CFG *cfg, BasicBlock *block, CallInstruction *call_instr,
                                         FuncDefInstruction defI) {
    auto &entry_block = cfg->block_map->at(1);
    auto &entry_instr_list = entry_block->Instruction_list;
    auto &first_block = cfg->block_map->at(0);
    auto &first_instr_list = first_block->Instruction_list;

    if (!defI->isTCO) {
        for (auto it = first_instr_list.rbegin() + 1; it != first_instr_list.rend(); ++it) {
            entry_instr_list.push_front(*it);
        }
        while (first_instr_list.size() > 1) {
            first_instr_list.pop_front();
        }
    }

    UpdatePhiNodes(cfg, defI, block, call_instr);

    auto &instr_list = block->Instruction_list;
    instr_list.pop_back();
    instr_list.pop_back();

    instr_list.push_back(new BrUncondInstruction(GetNewLabelOperand(1)));
    defI->isTCO = true;
}

void TCOPass::UpdatePhiNodes(CFG *cfg, FuncDefInstruction defI, BasicBlock *block, CallInstruction *call_instr) {
    auto args = call_instr->GetParameterList();

    for (size_t i = 0; i < defI->formals.size(); ++i) {
        Operand param = defI->formals_reg[i];

        if (!defI->isTCO) {
            defI->phi_list_list.resize(defI->formals.size());
            defI->phi_result_list.resize(defI->formals.size());

            defI->phi_list_list[i].emplace_back(GetNewLabelOperand(0), param);

            Operand phi = GetNewRegOperand(++cfg->max_reg);
            defI->phi_result_list[i] = phi;
        }

        int block_id = block->block_id;
        defI->phi_list_list[i].emplace_back(GetNewLabelOperand(block_id), args[i].second);

        for (auto &[blk_id, blk] : *(cfg->block_map)) {
            auto &instr_list = blk->Instruction_list;
            for (auto &instr : instr_list) {
                instr->resetOperand(param, defI->phi_result_list[i]);
            }
        }
    }
}