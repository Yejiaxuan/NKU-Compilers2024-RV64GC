#include "simplify_cfg.h"
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <bitset>
#include <deque>

// 假设最大寄存器数量为1024，根据实际情况调整
constexpr int MAX_REGS = 1024;

void SimplifyCFGPass::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        EliminateUnreachedBlocksInsts(cfg);
        EliminateDeadCode(cfg);
    }
}

// 判断指令是否有副作用
bool HasSideEffect(BasicInstruction* instr) {
    switch (instr->GetOpcode()) {
        case BasicInstruction::STORE:
        case BasicInstruction::CALL:
        case BasicInstruction::RET:
            return true;
        default:
            return false;
    }
}

// 获取指令的结果寄存器号，如果没有返回-1
int GetResultRegNo(BasicInstruction* instr) {
    Operand result = nullptr;
    switch (instr->GetOpcode()) {
        case BasicInstruction::LOAD: {
            LoadInstruction* loadInstr = dynamic_cast<LoadInstruction*>(instr);
            result = loadInstr->GetResult();
            break;
        }
        case BasicInstruction::ADD:
        case BasicInstruction::SUB:
        case BasicInstruction::MUL:
        case BasicInstruction::DIV:
        case BasicInstruction::BITXOR:
        case BasicInstruction::MOD:
        case BasicInstruction::FADD:
        case BasicInstruction::FSUB:
        case BasicInstruction::FMUL:
        case BasicInstruction::FDIV:
        case BasicInstruction::ICMP:
        case BasicInstruction::FCMP:
        case BasicInstruction::ZEXT:
        case BasicInstruction::FPTOSI:
        case BasicInstruction::SITOFP:
        case BasicInstruction::GETELEMENTPTR:
        case BasicInstruction::ALLOCA:
        case BasicInstruction::PHI:
        case BasicInstruction::CALL:
            // 根据具体指令类型获取result Operand
            if (auto arithInstr = dynamic_cast<ArithmeticInstruction*>(instr)) {
                result = arithInstr->GetResult();
            }
            else if (auto icmpInstr = dynamic_cast<IcmpInstruction*>(instr)) {
                result = icmpInstr->GetResult();
            }
            else if (auto fcmpInstr = dynamic_cast<FcmpInstruction*>(instr)) {
                result = fcmpInstr->GetResult();
            }
            else if (auto phiInstr = dynamic_cast<PhiInstruction*>(instr)) {
                result = phiInstr->GetResultReg();
            }
            else if (auto callInstr = dynamic_cast<CallInstruction*>(instr)) {
                result = callInstr->GetResult();
            }
            // 其他指令类型同理
            break;
        default:
            break;
    }

    if (result && result->GetOperandType() == BasicOperand::REG) {
        RegOperand* reg = dynamic_cast<RegOperand*>(result);
        if (reg) {
            return reg->GetRegNo();
        }
    }
    return -1;
}

bool UsesRegister(BasicInstruction* instr, int reg_no) {
    switch (instr->GetOpcode()) {
        case BasicInstruction::LOAD: {
            LoadInstruction* loadInstr = dynamic_cast<LoadInstruction*>(instr);
            if (loadInstr->GetPointer()->GetOperandType() == BasicOperand::REG) {
                RegOperand* ptrReg = dynamic_cast<RegOperand*>(loadInstr->GetPointer());
                if (ptrReg && ptrReg->GetRegNo() == reg_no) return true;
            }
            break;
        }
        case BasicInstruction::STORE: {
            StoreInstruction* storeInstr = dynamic_cast<StoreInstruction*>(instr);
            // 检查value和pointer
            if (storeInstr->GetValue()->GetOperandType() == BasicOperand::REG) {
                RegOperand* valReg = dynamic_cast<RegOperand*>(storeInstr->GetValue());
                if (valReg && valReg->GetRegNo() == reg_no) return true;
            }
            if (storeInstr->GetPointer()->GetOperandType() == BasicOperand::REG) {
                RegOperand* ptrReg = dynamic_cast<RegOperand*>(storeInstr->GetPointer());
                if (ptrReg && ptrReg->GetRegNo() == reg_no) return true;
            }
            break;
        }
        case BasicInstruction::ADD:
        case BasicInstruction::SUB:
        case BasicInstruction::MUL:
        case BasicInstruction::DIV:
        case BasicInstruction::BITXOR:
        case BasicInstruction::MOD:
        case BasicInstruction::FADD:
        case BasicInstruction::FSUB:
        case BasicInstruction::FMUL:
        case BasicInstruction::FDIV: {
            ArithmeticInstruction* arithInstr = dynamic_cast<ArithmeticInstruction*>(instr);
            if(arithInstr) {
                Operand op1 = arithInstr->GetOperand1();
                Operand op2 = arithInstr->GetOperand2();
                if((op1->GetOperandType() == BasicOperand::REG && dynamic_cast<RegOperand*>(op1)->GetRegNo() == reg_no) ||
                   (op2->GetOperandType() == BasicOperand::REG && dynamic_cast<RegOperand*>(op2)->GetRegNo() == reg_no)) {
                    return true;
                }
            }
            break;
        }
        case BasicInstruction::ICMP: {
            IcmpInstruction* icmpInstr = dynamic_cast<IcmpInstruction*>(instr);
            if(icmpInstr) {
                Operand op1 = icmpInstr->GetOp1();
                Operand op2 = icmpInstr->GetOp2();
                if((op1->GetOperandType() == BasicOperand::REG && dynamic_cast<RegOperand*>(op1)->GetRegNo() == reg_no) ||
                   (op2->GetOperandType() == BasicOperand::REG && dynamic_cast<RegOperand*>(op2)->GetRegNo() == reg_no)) {
                    return true;
                }
            }
            break;
        }
        case BasicInstruction::FCMP: {
            FcmpInstruction* fcmpInstr = dynamic_cast<FcmpInstruction*>(instr);
            if(fcmpInstr) {
                Operand op1 = fcmpInstr->GetOp1();
                Operand op2 = fcmpInstr->GetOp2();
                if((op1->GetOperandType() == BasicOperand::REG && dynamic_cast<RegOperand*>(op1)->GetRegNo() == reg_no) ||
                   (op2->GetOperandType() == BasicOperand::REG && dynamic_cast<RegOperand*>(op2)->GetRegNo() == reg_no)) {
                    return true;
                }
            }
            break;
        }
        case BasicInstruction::PHI: {
            PhiInstruction* phiInstr = dynamic_cast<PhiInstruction*>(instr);
            if(phiInstr) {
                for(auto &val_label : phiInstr->GetPhiList()) {
                    Operand val = val_label.first;
                    if(val->GetOperandType() == BasicOperand::REG && dynamic_cast<RegOperand*>(val)->GetRegNo() == reg_no) {
                        return true;
                    }
                }
            }
            break;
        }
        case BasicInstruction::CALL: {
            CallInstruction* callInstr = dynamic_cast<CallInstruction*>(instr);
            if(callInstr) {
                for(auto &arg : callInstr->GetParameterList()) {
                    if(arg.second->GetOperandType() == BasicOperand::REG && dynamic_cast<RegOperand*>(arg.second)->GetRegNo() == reg_no) {
                        return true;
                    }
                }
            }
            break;
        }
        case BasicInstruction::GETELEMENTPTR: {
            GetElementptrInstruction* gepInstr = dynamic_cast<GetElementptrInstruction*>(instr);
            if(gepInstr) {
                Operand ptr = gepInstr->GetPtrVal();
                if(ptr->GetOperandType() == BasicOperand::REG && dynamic_cast<RegOperand*>(ptr)->GetRegNo() == reg_no) {
                    return true;
                }
                for(auto &idx : gepInstr->GetIndexes()) {
                    if(idx->GetOperandType() == BasicOperand::REG && dynamic_cast<RegOperand*>(idx)->GetRegNo() == reg_no) {
                        return true;
                    }
                }
            }
            break;
        }
        // 处理更多指令类型
        default:
            break;
    }
    return false;
}
// 删除从函数入口开始到达不了的基本块和指令
// 不需要考虑控制流为常量的情况，你只需要从函数入口基本块(0号基本块)开始DFS，将没有被访问到的基本块和指令删去即可
void SimplifyCFGPass::EliminateUnreachedBlocksInsts(CFG *C) { 
    // 步骤 1：执行深度优先搜索（DFS）以找出可达的基本块
    std::vector<bool> visited(C->G.size(), false);
    std::stack<int> s; // DFS 使用的栈
    s.push(0); // 从入口块（ID 为0）开始

    while(!s.empty()){
        int cur = s.top();
        s.pop();

        if(visited[cur]){
            continue; // 如果当前基本块已访问，跳过
        }

        visited[cur] = true;

        for(auto &nxt : C->G[cur]){
            if(!visited[nxt->block_id]){
                s.push(nxt->block_id);
            }
        }
    }

    // 步骤 2：收集不可达的基本块
    std::vector<int> unreachable_block_ids;
    for(int i = 0; i < static_cast<int>(visited.size()); i++){
        if(!visited[i]){
            unreachable_block_ids.push_back(i);
        }
    }

    // 步骤 3：删除不可达的基本块和指令
    for(int block_id : unreachable_block_ids){
        auto it = C->block_map->find(block_id);
        if(it != C->block_map->end()){
            LLVMBlock unreachable_block = it->second;

            // 删除基本块中的指令
            for(auto instr : unreachable_block->Instruction_list){
                delete instr; // 假设指令是动态分配的内存
            }
            unreachable_block->Instruction_list.clear();

            // 从 block_map 中删除该基本块
            delete unreachable_block; // 假设基本块是动态分配的
            C->block_map->erase(it);
        }

        // 清空 G 和 invG 中的连接
        if(block_id < static_cast<int>(C->G.size())){
            C->G[block_id].clear();
        }
        if(block_id < static_cast<int>(C->invG.size())){
            C->invG[block_id].clear();
        }
    }

    // 步骤 4：更新 G 和 invG，移除指向不可达块的边
    for(int i = 0; i < static_cast<int>(C->G.size()); i++){
        if(visited[i]){
            std::vector<LLVMBlock> newSuccessors;
            for(auto &succ_block : C->G[i]){
                if(visited[succ_block->block_id]){
                    newSuccessors.push_back(succ_block);
                }
            }
            C->G[i] = newSuccessors;
        }
    }

    for(int i = 0; i < static_cast<int>(C->invG.size()); i++){
        if(visited[i]){
            std::vector<LLVMBlock> newPredecessors;
            for(auto &pred_block : C->invG[i]){
                if(visited[pred_block->block_id]){
                    newPredecessors.push_back(pred_block);
                }
            }
            C->invG[i] = newPredecessors;
        }
    }

    // 步骤 5：处理可达块中的指令，删除跳转或返回指令后的所有指令
    for(auto &[block_id, block] : *C->block_map){
        // 使用索引遍历，以便安全删除后续指令
        for(int i = 0; i < static_cast<int>(block->Instruction_list.size()); i++){
            BasicInstruction* instr = block->Instruction_list[i];
            NodeAttribute::opcode opcode = static_cast<NodeAttribute::opcode>(instr->GetOpcode());

            if(opcode == BasicInstruction::BR_COND ||
               opcode == BasicInstruction::BR_UNCOND ||
               opcode == BasicInstruction::RET){

                // 删除当前指令后的所有指令
                for(int j = i + 1; j < static_cast<int>(block->Instruction_list.size()); j++){
                    delete block->Instruction_list[j]; // 假设指令是动态分配的内存
                }

                // 截断指令列表到当前指令位置
                block->Instruction_list.erase(block->Instruction_list.begin() + i + 1, block->Instruction_list.end());

                // 跳转或返回指令后面的指令已删除，退出当前指令列表的遍历
                break;
            }
        }
    }
    //TODO("EliminateUnreachedBlocksInsts"); 
}
// 死代码消除函数
void SimplifyCFGPass::EliminateDeadCode(CFG *C) {
    // 使用一个集合来存储有用的寄存器
    std::unordered_set<int> useful_regs;
    // 使用一个队列来处理有用的寄存器
    std::queue<int> worklist;

    // 步骤1：初始化工作列表
    for(auto &[block_id, block] : *(C->block_map)) {
        // 从后向前遍历指令列表
        for(auto it = block->Instruction_list.rbegin(); it != block->Instruction_list.rend(); ++it) {
            BasicInstruction* instr = *it;
            if (HasSideEffect(instr)) {
                // 对于有副作用的指令，标记其所有操作数为有用
                switch (instr->GetOpcode()) {
                    case BasicInstruction::STORE: {
                        StoreInstruction* storeInstr = dynamic_cast<StoreInstruction*>(instr);
                        if (storeInstr->GetValue()->GetOperandType() == BasicOperand::REG) {
                            RegOperand* valReg = dynamic_cast<RegOperand*>(storeInstr->GetValue());
                            if (valReg && useful_regs.find(valReg->GetRegNo()) == useful_regs.end()) {
                                useful_regs.insert(valReg->GetRegNo());
                                worklist.push(valReg->GetRegNo());
                            }
                        }
                        if (storeInstr->GetPointer()->GetOperandType() == BasicOperand::REG) {
                            RegOperand* ptrReg = dynamic_cast<RegOperand*>(storeInstr->GetPointer());
                            if (ptrReg && useful_regs.find(ptrReg->GetRegNo()) == useful_regs.end()) {
                                useful_regs.insert(ptrReg->GetRegNo());
                                worklist.push(ptrReg->GetRegNo());
                            }
                        }
                        break;
                    }
                    case BasicInstruction::CALL: {
                        CallInstruction* callInstr = dynamic_cast<CallInstruction*>(instr);
                        for(auto &arg : callInstr->GetParameterList()) {
                            if(arg.second->GetOperandType() == BasicOperand::REG) {
                                RegOperand* argReg = dynamic_cast<RegOperand*>(arg.second);
                                if (argReg && useful_regs.find(argReg->GetRegNo()) == useful_regs.end()) {
                                    useful_regs.insert(argReg->GetRegNo());
                                    worklist.push(argReg->GetRegNo());
                                }
                            }
                        }
                        break;
                    }
                    case BasicInstruction::PHI: {
                        PhiInstruction* phiInstr = dynamic_cast<PhiInstruction*>(instr);
                        for(auto &val_label : phiInstr->GetPhiList()) {
                            Operand val = val_label.first;
                            if(val->GetOperandType() == BasicOperand::REG) {
                                RegOperand* valReg = dynamic_cast<RegOperand*>(val);
                                if(valReg && useful_regs.find(valReg->GetRegNo()) == useful_regs.end()) {
                                    useful_regs.insert(valReg->GetRegNo());
                                    worklist.push(valReg->GetRegNo());
                                }
                            }
                        }
                        break;
                    }
                    case BasicInstruction::RET: {
                        RetInstruction* retInstr = dynamic_cast<RetInstruction*>(instr);
                        if(retInstr->GetRetVal() && retInstr->GetRetVal()->GetOperandType() == BasicOperand::REG) {
                            RegOperand* retReg = dynamic_cast<RegOperand*>(retInstr->GetRetVal());
                            if(retReg && useful_regs.find(retReg->GetRegNo()) == useful_regs.end()) {
                                useful_regs.insert(retReg->GetRegNo());
                                worklist.push(retReg->GetRegNo());
                            }
                        }
                        break;
                    }
                    // 处理更多具有副作用的指令类型
                    default:
                        break;
                }
            }

            // 如果指令有结果寄存器，标记为有用
            int res_reg = GetResultRegNo(instr);
            if(res_reg != -1 && useful_regs.find(res_reg) == useful_regs.end()) {
                useful_regs.insert(res_reg);
                worklist.push(res_reg);
            }
        }
    }

    // 步骤2：迭代标记有用的寄存器
    while(!worklist.empty()) {
        int reg = worklist.front();
        worklist.pop();

        for(auto &[block_id, block] : *(C->block_map)) {
            for(auto &instr : block->Instruction_list) {
                // 如果该指令使用了当前寄存器
                if(UsesRegister(instr, reg)) {
                    // 获取该指令的结果寄存器
                    int instr_res_reg = GetResultRegNo(instr);
                    if(instr_res_reg != -1 && useful_regs.find(instr_res_reg) == useful_regs.end()) {
                        useful_regs.insert(instr_res_reg);
                        worklist.push(instr_res_reg);
                    }

                    // 根据指令类型，标记其操作数为有用
                    switch(instr->GetOpcode()) {
                        case BasicInstruction::ADD:
                        case BasicInstruction::SUB:
                        case BasicInstruction::MUL:
                        case BasicInstruction::DIV:
                        case BasicInstruction::BITXOR:
                        case BasicInstruction::MOD:
                        case BasicInstruction::FADD:
                        case BasicInstruction::FSUB:
                        case BasicInstruction::FMUL:
                        case BasicInstruction::FDIV: {
                            ArithmeticInstruction* arithInstr = dynamic_cast<ArithmeticInstruction*>(instr);
                            if(arithInstr) {
                                Operand op1 = arithInstr->GetOperand1();
                                Operand op2 = arithInstr->GetOperand2();
                                if(op1->GetOperandType() == BasicOperand::REG) {
                                    RegOperand* op1Reg = dynamic_cast<RegOperand*>(op1);
                                    if(op1Reg && useful_regs.find(op1Reg->GetRegNo()) == useful_regs.end()) {
                                        useful_regs.insert(op1Reg->GetRegNo());
                                        worklist.push(op1Reg->GetRegNo());
                                    }
                                }
                                if(op2->GetOperandType() == BasicOperand::REG) {
                                    RegOperand* op2Reg = dynamic_cast<RegOperand*>(op2);
                                    if(op2Reg && useful_regs.find(op2Reg->GetRegNo()) == useful_regs.end()) {
                                        useful_regs.insert(op2Reg->GetRegNo());
                                        worklist.push(op2Reg->GetRegNo());
                                    }
                                }
                            }
                            break;
                        }
                        case BasicInstruction::ICMP: {
                            IcmpInstruction* icmpInstr = dynamic_cast<IcmpInstruction*>(instr);
                            if(icmpInstr) {
                                Operand op1 = icmpInstr->GetOp1();
                                Operand op2 = icmpInstr->GetOp2();
                                if(op1->GetOperandType() == BasicOperand::REG) {
                                    RegOperand* op1Reg = dynamic_cast<RegOperand*>(op1);
                                    if(op1Reg && useful_regs.find(op1Reg->GetRegNo()) == useful_regs.end()) {
                                        useful_regs.insert(op1Reg->GetRegNo());
                                        worklist.push(op1Reg->GetRegNo());
                                    }
                                }
                                if(op2->GetOperandType() == BasicOperand::REG) {
                                    RegOperand* op2Reg = dynamic_cast<RegOperand*>(op2);
                                    if(op2Reg && useful_regs.find(op2Reg->GetRegNo()) == useful_regs.end()) {
                                        useful_regs.insert(op2Reg->GetRegNo());
                                        worklist.push(op2Reg->GetRegNo());
                                    }
                                }
                            }
                            break;
                        }
                        case BasicInstruction::FCMP: {
                            FcmpInstruction* fcmpInstr = dynamic_cast<FcmpInstruction*>(instr);
                            if(fcmpInstr) {
                                Operand op1 = fcmpInstr->GetOp1();
                                Operand op2 = fcmpInstr->GetOp2();
                                if(op1->GetOperandType() == BasicOperand::REG) {
                                    RegOperand* op1Reg = dynamic_cast<RegOperand*>(op1);
                                    if(op1Reg && useful_regs.find(op1Reg->GetRegNo()) == useful_regs.end()) {
                                        useful_regs.insert(op1Reg->GetRegNo());
                                        worklist.push(op1Reg->GetRegNo());
                                    }
                                }
                                if(op2->GetOperandType() == BasicOperand::REG) {
                                    RegOperand* op2Reg = dynamic_cast<RegOperand*>(op2);
                                    if(op2Reg && useful_regs.find(op2Reg->GetRegNo()) == useful_regs.end()) {
                                        useful_regs.insert(op2Reg->GetRegNo());
                                        worklist.push(op2Reg->GetRegNo());
                                    }
                                }
                            }
                            break;
                        }
                        case BasicInstruction::PHI: {
                            PhiInstruction* phiInstr = dynamic_cast<PhiInstruction*>(instr);
                            if(phiInstr) {
                                for(auto &val_label : phiInstr->GetPhiList()) {
                                    Operand val = val_label.first;
                                    if(val->GetOperandType() == BasicOperand::REG) {
                                        RegOperand* valReg = dynamic_cast<RegOperand*>(val);
                                        if(valReg && useful_regs.find(valReg->GetRegNo()) == useful_regs.end()) {
                                            useful_regs.insert(valReg->GetRegNo());
                                            worklist.push(valReg->GetRegNo());
                                        }
                                    }
                                }
                            }
                            break;
                        }
                        // 处理更多指令类型
                        default:
                            break;
                    }
                }
            }
        }
    }

    // 步骤3：删除未标记为有用的指令
    for(auto &[block_id, block] : *(C->block_map)) {
        // 使用迭代器安全删除指令
        for(auto it = block->Instruction_list.begin(); it != block->Instruction_list.end(); ) {
            BasicInstruction* instr = *it;
            int res_reg = GetResultRegNo(instr);
            if(res_reg != -1 && useful_regs.find(res_reg) == useful_regs.end()) {
                // 无用的指令，删除
                delete instr;
                it = block->Instruction_list.erase(it);
            } else {
                // 有用的指令，保留
                ++it;
            }
        }
    }
}
