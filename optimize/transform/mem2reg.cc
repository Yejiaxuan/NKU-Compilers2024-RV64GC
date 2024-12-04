#include "mem2reg.h"
#include <tuple>

static std::set<Instruction> EraseSet;
static std::map<int, int> mem2reg_map;
std::map<int, std::set<int>> defs, uses;
std::map<int, int> def_num;
std::set<int> no_use_vset, onedom_vset;
std::map<int, std::set<int>> sameblock_vset_map;

// 检查该条alloca指令是否可以被mem2reg
// eg. 数组不可以mem2reg
// eg. 如果该指针直接被使用不可以mem2reg(在SysY一般不可能发生,SysY不支持指针语法)
void Mem2RegPass::IsPromotable(CFG *C, Instruction AllocaInst) {
    auto AllocaI = (AllocaInstruction *)AllocaInst;
    if (!(AllocaI->GetDims().empty())) {
        return;  // 数组无法提升
    }

    int v = ((RegOperand *)(AllocaI->GetResult()))->GetRegNo();
    BasicInstruction::LLVMType type = AllocaI->GetDataType();

    auto alloca_defs = defs[v];
    auto alloca_uses = uses[v];

    if (alloca_uses.empty()) {  // 未被使用
        EraseSet.insert(AllocaInst);
        no_use_vset.insert(v);
        return;
    }

    if (alloca_defs.size() == 1) {
        int block_id = *(alloca_defs.begin());
        if (alloca_uses.size() == 1 && *(alloca_uses.begin()) == block_id) {  // 定义和使用在同一基本块
            EraseSet.insert(AllocaInst);
            sameblock_vset_map[block_id].insert(v);
            return;
        }
    }

    /*// nextTODO：only def once（之后进阶处理）
    if (def_num[v] == 1) {  // 仅定义一次
        int block_id = *(alloca_defs.begin());
        bool dom_flag = true;
        for (auto load_BBid : alloca_uses) {
            if (!C->IsDominate(block_id, load_BBid)) {
                dom_flag = false;
                break;
            }
        }
        if (dom_flag) {  // 定义点支配所有使用点
            EraseSet.insert(AllocaInst);
            onedom_vset.insert(v);
            return;
        }
    }*/



    /*// nextTODO: insert phi（之后进阶处理）
    common_allocas.insert(v);
    EraseSet.insert(AllocaInst);*/

    // TODO("IsPromotable"); 
}
/*
    int a1 = 5,a2 = 3,a3 = 11,b = 4
    return b // a1,a2,a3 is useless
-----------------------------------------------
pseudo IR is:
    %r0 = alloca i32 ;a1
    %r1 = alloca i32 ;a2
    %r2 = alloca i32 ;a3
    %r3 = alloca i32 ;b
    store 5 -> %r0 ;a1 = 5
    store 3 -> %r1 ;a2 = 3
    store 11 -> %r2 ;a3 = 11
    store 4 -> %r3 ;b = 4
    %r4 = load i32 %r3
    ret i32 %r4
--------------------------------------------------
%r0,%r1,%r2只有store, 但没有load,所以可以删去
优化后的IR(pseudo)为:
    %r3 = alloca i32
    store 4 -> %r3
    %r4 - load i32 %r3
    ret i32 %r4
*/

// vset is the set of alloca regno that only store but not load
// 该函数对你的时间复杂度有一定要求, 你需要保证你的时间复杂度小于等于O(nlognlogn), n为该函数的指令数
// 提示:deque直接在中间删除是O(n)的, 可以先标记要删除的指令, 最后想一个快速的方法统一删除
void Mem2RegPass::Mem2RegNoUseAlloca(CFG *C, std::set<int> &vset) {
    // this function is used in InsertPhi
    for (auto [id, bb] : *C->block_map) {
        for (auto I : bb->Instruction_list) {
            if (I->GetOpcode() == BasicInstruction::STORE) {
                auto StoreI = (StoreInstruction *)I;
                if (StoreI->GetPointer()->GetOperandType() != BasicOperand::REG) {
                    continue;
                }
                int v = ((RegOperand *)(StoreI->GetPointer()))->GetRegNo();
                if (vset.find(v) == vset.end()) {
                    continue;
                }
                EraseSet.insert(I);
            }
        }
    }
    // TODO("Mem2RegNoUseAlloca");
}

/*
    int b = getint();
    b = b + 10
    return b // def and use of b are in same block
-----------------------------------------------
pseudo IR is:
    %r0 = alloca i32 ;b
    %r1 = call getint()
    store %r1 -> %r0
    %r2 = load i32 %r0
    %r3 = %r2 + 10
    store %r3 -> %r0
    %r4 = load i32 %r0
    ret i32 %r4
--------------------------------------------------
%r0的所有load和store都在同一个基本块内
优化后的IR(pseudo)为:
    %r1 = call getint()
    %r3 = %r1 + 10
    ret %r3

对于每一个load，我们只需要找到最近的store,然后用store的值替换之后load的结果即可
*/

// vset is the set of alloca regno that load and store are all in the BB block_id
// 该函数对你的时间复杂度有一定要求，你需要保证你的时间复杂度小于等于O(nlognlogn), n为该函数的指令数
void Mem2RegPass::Mem2RegUseDefInSameBlock(CFG *C, std::set<int> &vset, int block_id) {
    // this function is used in InsertPhi
    std::map<int, int> curr_reg_map;    //<alloca reg, current store value regno>
    for (auto I : (*C->block_map)[block_id]->Instruction_list) {
        if (I->GetOpcode() == BasicInstruction::STORE) {
            auto StoreI = (StoreInstruction *)I;
            if (StoreI->GetPointer()->GetOperandType() != BasicOperand::REG) {
                continue;
            }
            int v = ((RegOperand *)(StoreI->GetPointer()))->GetRegNo();
            if (vset.find(v) == vset.end()) {
                continue;
            }
            curr_reg_map[v] = ((RegOperand *)(StoreI->GetValue()))->GetRegNo();
            EraseSet.insert(I);
        }
        if (I->GetOpcode() == BasicInstruction::LOAD) {
            auto LoadI = (LoadInstruction *)I;
            if (LoadI->GetPointer()->GetOperandType() != BasicOperand::REG) {
                continue;
            }
            int v = ((RegOperand *)(LoadI->GetPointer()))->GetRegNo();
            if (vset.find(v) == vset.end()) {
                continue;
            }
            mem2reg_map[((RegOperand *)(LoadI->GetResult()))->GetRegNo()] = curr_reg_map[v];
            EraseSet.insert(I);
        }
    }
    // TODO("Mem2RegUseDefInSameBlock");
}

// vset is the set of alloca regno that one store dominators all load instructions
// 该函数对你的时间复杂度有一定要求，你需要保证你的时间复杂度小于等于O(nlognlogn)
void Mem2RegPass::Mem2RegOneDefDomAllUses(CFG *C, std::set<int> &vset) {
    // this function is used in InsertPhi

    // TODO("Mem2RegOneDefDomAllUses");
}

void Mem2RegPass::InsertPhi(CFG *C) {
    // 清空之前的记录
    defs.clear();
    uses.clear();
    def_num.clear();
    no_use_vset.clear();
    onedom_vset.clear();
    sameblock_vset_map.clear();

    // 获取每个变量的定义和使用信息
    for (auto [id, BB] : *C->block_map) {
        for (auto I : BB->Instruction_list) {
            if (I->GetOpcode() == BasicInstruction::STORE) {
                auto StoreI = (StoreInstruction *)I;
                if (StoreI->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {
                    continue;
                }
                int ptr_reg = ((RegOperand *)(StoreI->GetPointer()))->GetRegNo();
                defs[ptr_reg].insert(id);
                def_num[ptr_reg]++;
            } else if (I->GetOpcode() == BasicInstruction::LOAD) {
                auto LoadI = (LoadInstruction *)I;
                if (LoadI->GetPointer()->GetOperandType() == BasicOperand::GLOBAL) {
                    continue;
                }
                int ptr_reg = ((RegOperand *)(LoadI->GetPointer()))->GetRegNo();
                uses[ptr_reg].insert(id);
            }
        }
    }

    // 遍历 entry 基本块中的 alloca 指令，判断是否可以提升
    LLVMBlock entry_BB = (*C->block_map)[0];
    for (auto I : entry_BB->Instruction_list) {
        if (I->GetOpcode() != BasicInstruction::ALLOCA) {
            continue;
        }

        IsPromotable(C, I);

        // nextTODO：insert phi（进阶处理）
    }

    // 处理不同情况
    Mem2RegNoUseAlloca(C, no_use_vset);
    Mem2RegOneDefDomAllUses(C, onedom_vset);
    for (auto [id, vset] : sameblock_vset_map) {
        Mem2RegUseDefInSameBlock(C, vset, id);
    }

    // 对于需要插入 phi 节点的变量（common_allocas），后续处理
    // TODO: 实现插入 phi 节点的逻辑


    // TODO("InsertPhi"); 
}

void Mem2RegPass::VarRename(CFG *C) {
    // 处理mem2reg_map链
    for (auto [id, bb] : *C->block_map) {
        for (auto I : bb->Instruction_list) {
            if (I->GetOpcode() == BasicInstruction::LOAD && 
                ((LoadInstruction *)I)->GetPointer()->GetOperandType() == BasicOperand::REG) {
                auto LoadI = (LoadInstruction *)I;
                int result = ((RegOperand *)(LoadI->GetResult()))->GetRegNo();
                if (mem2reg_map.find(result) != mem2reg_map.end()) {
                    int result2 = mem2reg_map[result];
                    while (mem2reg_map.find(result2) != mem2reg_map.end()) {
                        mem2reg_map[result] = mem2reg_map[result2];
                        result2 = mem2reg_map[result];
                    }
                }
            }
        }
    }

    // 删除标记要删除的指令
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
    // 替换寄存器映射
    for (auto [id, bb] : *C->block_map) {
        for (auto I : bb->Instruction_list) {
            I->ReplaceRegByMap(mem2reg_map);
        }
    }

    EraseSet.clear();
    mem2reg_map.clear();

    // TODO("VarRename"); 
}

void Mem2RegPass::Mem2Reg(CFG *C) {
    InsertPhi(C);
    VarRename(C);
}

void Mem2RegPass::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        Mem2Reg(cfg);
    }
}