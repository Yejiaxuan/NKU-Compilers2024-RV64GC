#include "mem2reg.h"
#include <tuple>

static std::set<Instruction> EraseSet;
static std::map<int, int> mem2reg_map;
std::map<int, std::set<int>> defs, uses;
std::map<int, int> def_num;
std::set<int> no_use_vset, onedom_vset;
std::map<int, std::set<int>> sameblock_vset_map;

static std::set<int> common_allocas;
static std::map<PhiInstruction *, int> phi_map;
// alloca_type_map用于记录alloca变量对应的数据类型，用于插入phi指令时创建正确类型的phi
static std::map<int, BasicInstruction::LLVMType> alloca_type_map;

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
    alloca_type_map[v] = type; // 记录该alloca的类型

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

    // only def once 并且支配所有use
    /*if (def_num[v] == 1) {  // 仅定义一次
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


    // 一般情况，需要插入phi指令
    /*common_allocas.insert(v);
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
    std::map<int, int> v_result_map;
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
                v_result_map[v] = ((RegOperand *)(StoreI->GetValue()))->GetRegNo();
                EraseSet.insert(I);
            }
        }
    }
    for (auto [id, bb] : *C->block_map) {
        for (auto I : bb->Instruction_list) {
            if (I->GetOpcode() == BasicInstruction::LOAD) {
                auto LoadI = (LoadInstruction *)I;
                if (LoadI->GetPointer()->GetOperandType() != BasicOperand::REG) {
                    continue;
                }
                int v = ((RegOperand *)(LoadI->GetPointer()))->GetRegNo();
                if (vset.find(v) == vset.end()) {
                    continue;
                }
                mem2reg_map[((RegOperand *)(LoadI->GetResult()))->GetRegNo()] = v_result_map[v];
                EraseSet.insert(I);
            }
        }
    }
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
    common_allocas.clear();
    phi_map.clear();

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
    }

    // 处理不同情况
    Mem2RegNoUseAlloca(C, no_use_vset);
    Mem2RegOneDefDomAllUses(C, onedom_vset);
    for (auto [id, vset] : sameblock_vset_map) {
        Mem2RegUseDefInSameBlock(C, vset, id);
    }

    // 对common_allocas插入phi指令
    /*for (int v : common_allocas) {
        std::set<int> F;
        std::set<int> W = defs[v];
        BasicInstruction::LLVMType type = alloca_type_map[v];
        while (!W.empty()) {
            int BB_X = *W.begin();
            W.erase(BB_X);
            for (auto BB_Y : C->DomTree.GetDF(BB_X)) {
                if (F.find(BB_Y) == F.end()) {
                    PhiInstruction *PhiI = new PhiInstruction(type, GetNewRegOperand(++C->max_reg));
                    (*C->block_map)[BB_Y]->InsertInstruction(0, PhiI);
                    phi_map[PhiI] = v;
                    F.insert(BB_Y);
                    if (defs[v].find(BB_Y) == defs[v].end()) {
                        W.insert(BB_Y);
                    }
                }
            }
        }
    }*/

    // TODO("InsertPhi"); 
}

int in_allocas(std::set<int> &S, Instruction I) {
    if (I->GetOpcode() == BasicInstruction::LOAD) {
        auto LoadI = (LoadInstruction *)I;
        if (LoadI->GetPointer()->GetOperandType() != BasicOperand::REG) {
            return -1;
        }
        int pointer = ((RegOperand *)LoadI->GetPointer())->GetRegNo();
        if (S.find(pointer) != S.end()) {
            return pointer;
        }
    }
    if (I->GetOpcode() == BasicInstruction::STORE) {
        auto StoreI = (StoreInstruction *)I;
        if (StoreI->GetPointer()->GetOperandType() != BasicOperand::REG) {
            return -1;
        }
        int pointer = ((RegOperand *)StoreI->GetPointer())->GetRegNo();
        if (S.find(pointer) != S.end()) {
            return pointer;
        }
    }
    return -1;
}

void Mem2RegPass::VarRename(CFG *C) {
    // VarRename过程: 
    // 思路：使用类似BFS或DFS的方式从入口块开始遍历CFG，为common_allocas变量维护IncomingVals（映射alloca reg到当前可用的value reg）。
    // 对于每个块中的指令：
    //   - 如果是load allocas变量，替换为IncomingVals中的值
    //   - 如果是store到allocas变量，更新IncomingVals[v]
    //   - 如果是phi指令对应allocas，更新IncomingVals[v]为phi的结果寄存器
    // 最后，将IncomingVals传递给后继基本块，并为phi指令添加正确的前驱边。

    std::map<int, std::map<int,int>> WorkList; // <BB_id, <alloca_reg, val_reg>>
    WorkList.insert({0, std::map<int,int>{}});
    std::vector<int> BBvis;
    BBvis.resize(C->max_label+1,0);

    while (!WorkList.empty()) {
        int BB = (*WorkList.begin()).first;
        auto IncomingVals = (*WorkList.begin()).second;
        WorkList.erase(BB);

        if (BBvis[BB]) continue;
        BBvis[BB] = 1;

        // 先对当前BB中的指令进行处理
        for (auto I : (*C->block_map)[BB]->Instruction_list) {
            if (I->GetOpcode() == BasicInstruction::LOAD) {
                auto LoadI = (LoadInstruction *)I;
                int v = in_allocas(common_allocas, I);
                if (v >= 0) {
                    // Load来自common_allocas变量，用IncomingVals[v]替换
                    EraseSet.insert(LoadI);
                    mem2reg_map[((RegOperand *)LoadI->GetResult())->GetRegNo()] = IncomingVals[v];
                }
            } else if (I->GetOpcode() == BasicInstruction::STORE) {
                auto StoreI = (StoreInstruction *)I;
                int v = in_allocas(common_allocas, I);
                if (v >= 0) {
                    // store到common_allocas变量，更新IncomingVals[v]
                    EraseSet.insert(StoreI);
                    IncomingVals[v] = ((RegOperand *)StoreI->GetValue())->GetRegNo();
                }
            } else if (I->GetOpcode() == BasicInstruction::PHI) {
                auto PhiI = (PhiInstruction *)I;
                if (EraseSet.find(PhiI) != EraseSet.end()) {
                    continue;
                }
                auto it = phi_map.find(PhiI);
                if (it != phi_map.end()) {
                    // 当前phi属于alloca变量
                    IncomingVals[it->second] = ((RegOperand *)PhiI->GetResultReg())->GetRegNo();
                }
            }
        }

        // 将IncomingVals传递给后继基本块，并为phi指令添加前驱边
        for (auto succ : C->G[BB]) {
            int BBv = succ->block_id;
            // 为后继块中的phi指令添加前驱信息
            for (auto I : (*C->block_map)[BBv]->Instruction_list) {
                if (I->GetOpcode() != BasicInstruction::PHI) {
                    // phi指令均在块开头，遇到非phi即可停止
                    break;
                }
                auto PhiI = (PhiInstruction *)I;
                auto it = phi_map.find(PhiI);
                if (it != phi_map.end()) {
                    int v = it->second;
                    // 如果当前phi对alloca v有定义，则为phi添加来自BB的前驱值
                    if (IncomingVals.find(v) == IncomingVals.end()) {
                        // 若没有IncomingVals[v]，则说明该alloca v在该路径尚未定义过，phi可能可删除
                        EraseSet.insert(PhiI);
                    } else {
                        PhiI->InsertPhi(GetNewRegOperand(IncomingVals[v]), GetNewLabelOperand(BB));
                    }
                }
            }
            // 将更新后的IncomingVals加入WorkList
            WorkList.insert({BBv, IncomingVals});
        }
    }
    
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
    common_allocas.clear();
    phi_map.clear();

    // TODO("VarRename"); 
}

void Mem2RegInit(CFG *C) {
    for (auto &[id, bb] : *C->block_map) {
        auto tmp_list = bb->Instruction_list;
        bb->Instruction_list.clear();
        for (auto I : tmp_list) {
            if (I->GetOpcode() == BasicInstruction::STORE) {
                auto StoreI = (StoreInstruction *)I;
                auto val = StoreI->GetValue();
                if (val->GetOperandType() == BasicOperand::IMMI32) {
                    auto ArithI =
                    new ArithmeticInstruction(BasicInstruction::ADD, BasicInstruction::I32, val, new ImmI32Operand(0), GetNewRegOperand(++C->max_reg));
                    bb->Instruction_list.push_back(ArithI);
                    StoreI->SetValue(GetNewRegOperand(C->max_reg));
                } else if (val->GetOperandType() == BasicOperand::IMMF32) {
                    auto ArithI =
                    new ArithmeticInstruction(BasicInstruction::FADD, BasicInstruction::FLOAT32, val, new ImmF32Operand(0), GetNewRegOperand(++C->max_reg));
                    bb->Instruction_list.push_back(ArithI);
                    StoreI->SetValue(GetNewRegOperand(C->max_reg));
                }
            }
            bb->Instruction_list.push_back(I);
        }
    }
}

void Mem2RegPass::Mem2Reg(CFG *C) {
    // C->BuildDominatorTree();
    Mem2RegInit(C);
    InsertPhi(C);
    VarRename(C);
}

void Mem2RegPass::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        Mem2Reg(cfg);
    }
}
