#include "simple_adce.h"
#include <algorithm>

// 初始化阶段缓存终结指令
void ADCEPass::Initialize(CFG* C) {
    DomTree.C = C;
    PostDomTree.C = C;
    DomTree.BuildDominatorTree();         // 构建支配树
    PostDomTree.BuildDominatorTree(true); // 构建后支配树

    // 缓存每个基本块的终结指令
    terminalInstructions.clear();  // 清空缓存
    CacheTerminalInstructions(C);

    // 初始化布尔数组大小
    ResetliveBlock(C);
}

// 缓存每个基本块的终结指令
void ADCEPass::CacheTerminalInstructions(CFG* C) {
    auto& blockmap = *C->block_map;
    for (auto [id, bb] : blockmap) {
        if (!bb->Instruction_list.empty()) {
            terminalInstructions[id] = bb->Instruction_list.back();
        } else {
            std::cout << "Block " << id << " has no instructions." << std::endl;
        }
    }
}


// 初始化布尔数组大小
void ADCEPass::ResetliveBlock(CFG* C) {
    liveBlock.assign(C->max_label + 1, false);
}

// 判断指令是否有副作用
bool ADCEPass::HasSideEffect(const Instruction& instr) {
    auto opcode = instr->GetOpcode();
    return (opcode == BasicInstruction::STORE ||
            opcode == BasicInstruction::CALL ||
            opcode == BasicInstruction::RET);
}

// 初始化工作列表和定义映射
void ADCEPass::PopulateWorklistAndDefMap(CFG* C) {
    auto blockmap = *C->block_map;
    for (auto& [id, basicBlock] : blockmap) {
        for (auto& instr : basicBlock->Instruction_list) {
            instr->SetBlockID(id);
            if (HasSideEffect(instr)) {
                worklist.push_back(instr);  // 添加到工作列表
            }
            if (instr->GetResultReg() != nullptr) {
                defmap[instr->GetResultRegNo()] = instr;
            }
        }
    }
}

// 移除死代码，保留活跃指令
void ADCEPass::RemoveDeadInstructions(CFG* C) {
    auto PostDomTreeidom = PostDomTree.idom;
    auto blockmap = *C->block_map;

    for (auto [id, bb] : blockmap ) {
        auto terminalI = FindTerminal(C, id); // 使用缓存的终结指令
        auto tmp_Instruction_list = bb->Instruction_list;
        bb->Instruction_list.clear();
        for (auto I : tmp_Instruction_list) {
            if (liveInstructionset.find(I) == liveInstructionset.end()) {
                if (terminalI == I) {
                    std::cout << "Replacing terminal instruction in block " << id 
                              << " with unconditional branch." << std::endl;
                    auto livebbid = PostDomTreeidom[id]->block_id;
                    while (!liveBlock[livebbid]) {
                        livebbid = PostDomTreeidom[livebbid]->block_id;
                    }
                    I = new BrUncondInstruction(GetNewLabelOperand(livebbid));
                } else {
                    continue;
                }
            }
            bb->InsertInstruction(1, I);
        }
    }
}

// 构建控制依赖图
std::vector<std::vector<LLVMBlock>> ADCEPass::BuildCDG(CFG* C) {
    std::vector<std::vector<LLVMBlock>> CDG;
    std::vector<std::vector<LLVMBlock>> CDG_precursor;
    std::vector<int> controlDependenceInDegree;
    CDG.resize(C->max_label + 2);
    CDG_precursor.resize(C->max_label + 1);
    controlDependenceInDegree.resize(C->max_label + 1, 0);
    auto blockmap = (*C->block_map);

    for (int i = 0; i <= C->max_label; ++i) {
        auto domFrontier = PostDomTree.GetDF(i);
        for (auto vbbid : domFrontier) {
            CDG[vbbid].push_back(blockmap[i]);
            if (vbbid != i) {
                controlDependenceInDegree[i]++;
            }
            CDG_precursor[blockmap[i]->block_id].push_back(blockmap[vbbid]);
        }
    }

    for (int i = 0; i <= C->max_label; ++i) {
        if (!controlDependenceInDegree[i]) {
            CDG[C->max_label + 1].push_back(blockmap[i]);
        }
    }

    // CDG
    // 前驱输出
    std::cout << "Control Dependence Graph Predecessors:" << std::endl;
    for (int i = 0; i <= C->max_label; ++i) {
        std::cout << "Block " << i << " has control dependence predecessors: ";
        for (auto& pred : CDG_precursor[i]) {
            std::cout << pred->block_id << " ";
        }
        std::cout << std::endl;
    }

    return CDG_precursor;
}


// 使用缓存的终结指令
Instruction ADCEPass::FindTerminal(CFG* C, int bbid) {
    auto it = terminalInstructions.find(bbid);
    if (it != terminalInstructions.end()) {
        return it->second; 
    }
    return Instruction(); // 返回空指令
}

// 执行 ADCE 优化
void ADCEPass::ADCE(CFG* C) {
    // 清空之前的内容
    worklist.clear();
    defmap.clear();
    liveInstructionset.clear();
    liveBlock.assign(C->max_label + 1, false); // 重置布尔数组

    // 构建控制依赖图并获取前驱列表
    auto CDG_precursor = BuildCDG(C);
    auto invG = C->invG;

    auto DomTreeidom = DomTree.idom;
    auto PostDomTreeidom = PostDomTree.idom;
    auto blockmap = *C->block_map;

    // 初始化工作列表和定义映射
    PopulateWorklistAndDefMap(C);

    // 开始处理工作列表
    while (!worklist.empty()) {
        auto I = worklist.back();  // LIFO 模式，取出最后一个指令
        worklist.pop_back();
        if (liveInstructionset.find(I) != liveInstructionset.end()) {
            continue;
        }
        liveInstructionset.insert(I);
        auto parBBno = I->GetBlockID();
        auto parBB = blockmap[I->GetBlockID()];
        liveBlock[parBBno] = true; // 标记基本块为活跃

        if (I->GetOpcode() == BasicInstruction::PHI) {
            auto PhiI = (PhiInstruction *)I;
            for (auto [Labelop, Regop] : PhiI->GetPhiList()) {
                auto Label = (LabelOperand *)Labelop;
                auto Labelno = Label->GetLabelNo();
                auto terminalI = FindTerminal(C, Labelno); // 使用缓存的终结指令
                if (liveInstructionset.find(terminalI) == liveInstructionset.end()) {
                    worklist.push_back(terminalI);  // 添加到工作列表
                    liveBlock[Labelno] = true;     // 标记基本块为活跃
                }
            }
        }

        if (parBBno != -1) {
            for (auto CDG_pre : CDG_precursor[parBBno]) {
                auto CDG_preno = CDG_pre->block_id;
                auto terminalI = FindTerminal(C, CDG_preno); // 使用缓存的终结指令
                if (liveInstructionset.find(terminalI) == liveInstructionset.end()) {
                    worklist.push_back(terminalI);  // 添加到工作列表
                }
            }
        }

        for (auto op : I->GetNonResultOperands()) {
            if (op->GetOperandType() == BasicOperand::REG) {
                auto Regop = (RegOperand *)op;
                auto Regopno = Regop->GetRegNo();
                if (defmap.find(Regopno) == defmap.end()) {
                    continue;
                }
                auto DefI = defmap[Regopno];
                if (liveInstructionset.find(DefI) == liveInstructionset.end()) {
                    worklist.push_back(DefI);  // 添加到工作列表
                }
            }
        }
    }

    // 删除死代码
    RemoveDeadInstructions(C);
}

// 执行 ADCE 优化
void ADCEPass::Execute() {
    for (auto [defI, cfg] : llvmIR->llvm_cfg) {
        Initialize(cfg); // 初始化支配树和后支配树，并缓存终结指令
        ADCE(cfg);       // 执行 ADCE 优化
        //cfg->BuildCFG(); // 重建控制流图
    }
}
