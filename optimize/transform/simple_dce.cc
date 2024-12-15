#include "simple_dce.h"
#include <stack>
#include <unordered_map>
#include <bitset>
#include <deque>
#include <unordered_set>

void SimpleDCEPass::Execute() {
    for (auto &[defI, cfg] : llvmIR->llvm_cfg) {
        EliminateDeadCode(cfg);
    }
}

// 判断指令是否有副作用
bool HasSideEffect(BasicInstruction* instr) {
    switch (instr->GetOpcode()) {
        case BasicInstruction::STORE:
        case BasicInstruction::CALL:
        case BasicInstruction::RET:
        case BasicInstruction::BR_COND:
        case BasicInstruction::BR_UNCOND:
        case BasicInstruction::ALLOCA:
        case BasicInstruction::GETELEMENTPTR:
        case BasicInstruction::GLOBAL_VAR:    // 添加全局变量定义
        case BasicInstruction::GLOBAL_STR:    // 添加全局字符串定义
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
            LoadInstruction* loadInstr = static_cast<LoadInstruction*>(instr);
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
            else if (auto loadInstr = dynamic_cast<LoadInstruction*>(instr)) {
                result = loadInstr->GetResult();
            }
            else if (auto fptosiInstr = dynamic_cast<FptosiInstruction*>(instr)) {
                result = fptosiInstr->GetResult();
            }
            else if (auto sitofpInstr = dynamic_cast<SitofpInstruction*>(instr)) {
                result = sitofpInstr->GetResult();
            }
            else if (auto zextInstr = dynamic_cast<ZextInstruction*>(instr)) {
                result = zextInstr->GetResult();
            }
            // 其他指令类型同理
            break;
        default:
            break;
    }

    if (result && result->GetOperandType() == BasicOperand::REG) {
        RegOperand* reg = static_cast<RegOperand*>(result);
        if (reg) {
            int reg_no = reg->GetRegNo();
            Log("Defining reg_no %d in instruction opcode %d", reg_no, instr->GetOpcode());
            return reg_no;
        }
    } else {
        Log("No REG result for instruction opcode %d", instr->GetOpcode());
    }
    return -1;
}

std::vector<int> GetUsedRegisters(BasicInstruction* instr) {
    std::vector<int> regs;
    switch(instr->GetOpcode()) {
        case BasicInstruction::LOAD: {
            LoadInstruction* loadInstr = static_cast<LoadInstruction*>(instr);
            Operand ptr = loadInstr->GetPointer();
            if(ptr->GetOperandType() == BasicOperand::REG) {
                RegOperand* ptrReg = static_cast<RegOperand*>(ptr);
                regs.push_back(ptrReg->GetRegNo());
            }
            break;
        }
        case BasicInstruction::STORE: {
            StoreInstruction* storeInstr = static_cast<StoreInstruction*>(instr);
            Operand val = storeInstr->GetValue();
            Operand ptr = storeInstr->GetPointer();
            if(val->GetOperandType() == BasicOperand::REG) {
                RegOperand* valReg = static_cast<RegOperand*>(val);
                regs.push_back(valReg->GetRegNo());
            }
            if(ptr->GetOperandType() == BasicOperand::REG) {
                RegOperand* ptrReg = static_cast<RegOperand*>(ptr);
                regs.push_back(ptrReg->GetRegNo());
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
            ArithmeticInstruction* arithInstr = static_cast<ArithmeticInstruction*>(instr);
            if(arithInstr->GetOperand1()->GetOperandType() == BasicOperand::REG) {
                RegOperand* op1Reg = static_cast<RegOperand*>(arithInstr->GetOperand1());
                regs.push_back(op1Reg->GetRegNo());
            }
            if(arithInstr->GetOperand2()->GetOperandType() == BasicOperand::REG) {
                RegOperand* op2Reg = static_cast<RegOperand*>(arithInstr->GetOperand2());
                regs.push_back(op2Reg->GetRegNo());
            }
            if(arithInstr->GetOperand3() && arithInstr->GetOperand3()->GetOperandType() == BasicOperand::REG) {
                RegOperand* op3Reg = static_cast<RegOperand*>(arithInstr->GetOperand3());
                regs.push_back(op3Reg->GetRegNo());
            }
            break;
        }
        case BasicInstruction::ICMP: {
            IcmpInstruction* icmpInstr = static_cast<IcmpInstruction*>(instr);
            if(icmpInstr->GetOp1()->GetOperandType() == BasicOperand::REG) {
                RegOperand* op1Reg = static_cast<RegOperand*>(icmpInstr->GetOp1());
                regs.push_back(op1Reg->GetRegNo());
            }
            if(icmpInstr->GetOp2()->GetOperandType() == BasicOperand::REG) {
                RegOperand* op2Reg = static_cast<RegOperand*>(icmpInstr->GetOp2());
                regs.push_back(op2Reg->GetRegNo());
            }
            break;
        }
        case BasicInstruction::FCMP: {
            FcmpInstruction* fcmpInstr = static_cast<FcmpInstruction*>(instr);
            if(fcmpInstr->GetOp1()->GetOperandType() == BasicOperand::REG) {
                RegOperand* op1Reg = static_cast<RegOperand*>(fcmpInstr->GetOp1());
                regs.push_back(op1Reg->GetRegNo());
            }
            if(fcmpInstr->GetOp2()->GetOperandType() == BasicOperand::REG) {
                RegOperand* op2Reg = static_cast<RegOperand*>(fcmpInstr->GetOp2());
                regs.push_back(op2Reg->GetRegNo());
            }
            break;
        }
        case BasicInstruction::PHI: {
            PhiInstruction* phiInstr = static_cast<PhiInstruction*>(instr);
            for(auto &val_label : phiInstr->GetPhiList()) {
                Operand val = val_label.second;  // 正确地获取值
                if(val->GetOperandType() == BasicOperand::REG) {
                    RegOperand* valReg = static_cast<RegOperand*>(val);
                    regs.push_back(valReg->GetRegNo());
                }
            }
            break;
        }
        case BasicInstruction::CALL: {
            CallInstruction* callInstr = static_cast<CallInstruction*>(instr);
            for(auto &arg : callInstr->GetParameterList()) {
                if(arg.second->GetOperandType() == BasicOperand::REG) {
                    RegOperand* argReg = static_cast<RegOperand*>(arg.second);
                    regs.push_back(argReg->GetRegNo());
                }
            }
            // 注意：不再将结果寄存器作为使用寄存器添加
            break;
        }
        case BasicInstruction::GETELEMENTPTR: {
            GetElementptrInstruction* gepInstr = static_cast<GetElementptrInstruction*>(instr);
            Operand ptr = gepInstr->GetPtrVal();
            if(ptr->GetOperandType() == BasicOperand::REG) {
                RegOperand* ptrReg = static_cast<RegOperand*>(ptr);
                regs.push_back(ptrReg->GetRegNo());
            }
            for(auto &idx : gepInstr->GetIndexes()) {
                if(idx->GetOperandType() == BasicOperand::REG) {
                    RegOperand* idxReg = static_cast<RegOperand*>(idx);
                    regs.push_back(idxReg->GetRegNo());
                }
            }
            break;
        }
        case BasicInstruction::RET: {
            RetInstruction* retInstr = static_cast<RetInstruction*>(instr);
            if(retInstr->GetRetVal() && retInstr->GetRetVal()->GetOperandType() == BasicOperand::REG) {
                RegOperand* retReg = static_cast<RegOperand*>(retInstr->GetRetVal());
                regs.push_back(retReg->GetRegNo());
            }
            break;
        }
        case BasicInstruction::BR_COND: {
            BrCondInstruction* brCondInstr = static_cast<BrCondInstruction*>(instr);
            Operand cond = brCondInstr->GetCond();
            if(cond->GetOperandType() == BasicOperand::REG) {
                RegOperand* condReg = static_cast<RegOperand*>(cond);
                regs.push_back(condReg->GetRegNo());
            }
            break;
        }
        case BasicInstruction::BR_UNCOND: {
            // 通常，无条件跳转不使用寄存器，但如果有使用寄存器的情况
            BrUncondInstruction* brUncondInstr = static_cast<BrUncondInstruction*>(instr);
            Operand dest = brUncondInstr->GetDestLabel();
            if(dest->GetOperandType() == BasicOperand::REG) {
                RegOperand* destReg = static_cast<RegOperand*>(dest);
                regs.push_back(destReg->GetRegNo());
            }
            break;
        }
        case BasicInstruction::ALLOCA: {
            AllocaInstruction* allocaInstr = static_cast<AllocaInstruction*>(instr);
            // ALLOCA 通常不使用寄存器，但如果有指针操作数，需要处理
            // 假设 ALLOCA 只有结果寄存器，无使用寄存器
            // 如果有使用寄存器，需根据实际情况添加
            break;
        }
        case BasicInstruction::ZEXT: {
            ZextInstruction* zextInstr = static_cast<ZextInstruction*>(instr);
            Operand src = zextInstr->GetSrc();
            if(src->GetOperandType() == BasicOperand::REG) {
                RegOperand* srcReg = static_cast<RegOperand*>(src);
                regs.push_back(srcReg->GetRegNo());
            }
            break;
        }
        case BasicInstruction::FPTOSI: {
            FptosiInstruction* fptosiInstr = static_cast<FptosiInstruction*>(instr);
            Operand src = fptosiInstr->GetSrc();
            if(src->GetOperandType() == BasicOperand::REG) {
                RegOperand* srcReg = static_cast<RegOperand*>(src);
                regs.push_back(srcReg->GetRegNo());
            }
            break;
        }
        case BasicInstruction::SITOFP: {
            SitofpInstruction* sitofpInstr = static_cast<SitofpInstruction*>(instr);
            Operand src = sitofpInstr->GetSrc();
            if(src->GetOperandType() == BasicOperand::REG) {
                RegOperand* srcReg = static_cast<RegOperand*>(src);
                regs.push_back(srcReg->GetRegNo());
            }
            break;
        }
        // 处理更多指令类型
        default:
            break;
    }
    return regs;
}


// 死代码消除函数
void SimpleDCEPass::EliminateDeadCode(CFG *C) {
    // 构建定义链：reg_no -> 定义该寄存器的指令
    std::unordered_map<int, BasicInstruction*> def_chain;
    for(auto &[block_id, block] : *(C->block_map)) {
        for(auto &instr : block->Instruction_list) {
            int res_reg = GetResultRegNo(instr);
            if(res_reg != -1) {
                // 在 SSA 形式下，每个寄存器只有一个定义
                if(def_chain.find(res_reg) != def_chain.end()) {
                    ERROR("Register %d has multiple definitions.", res_reg);
                }
                def_chain[res_reg] = instr;
            }
        }
    }

    // 集合来存储有用的指令
    std::unordered_set<BasicInstruction*> useful_instructions;
    // 队列来处理有用的指令
    std::queue<BasicInstruction*> worklist;

    // 步骤1：初始化工作列表，将所有具有副作用的指令标记为有用
    for(auto &[block_id, block] : *(C->block_map)) {
        for(auto &instr : block->Instruction_list) {
            if (HasSideEffect(instr)) {
                if (useful_instructions.find(instr) == useful_instructions.end()) {
                    useful_instructions.insert(instr);
                    worklist.push(instr);
                }
            }
        }
    }

    // 步骤2：迭代标记有用的指令
    while(!worklist.empty()) {
        BasicInstruction* instr = worklist.front();
        worklist.pop();

        // 获取指令使用的所有寄存器号
        std::vector<int> used_regs = GetUsedRegisters(instr);
        for(auto reg_no : used_regs) {
            if(def_chain.find(reg_no) != def_chain.end()) {
                BasicInstruction* def_instr = def_chain[reg_no];
                if(useful_instructions.find(def_instr) == useful_instructions.end()) {
                    useful_instructions.insert(def_instr);
                    worklist.push(def_instr);
                }
            }
        }
    }

    // 步骤3：删除未标记为有用的指令
    for(auto &[block_id, block] : *(C->block_map)) {
        for(auto it = block->Instruction_list.begin(); it != block->Instruction_list.end(); ) {
            BasicInstruction* instr = *it;
            if(useful_instructions.find(instr) == useful_instructions.end()) {
                // 无用的指令，删除
                delete instr;
                it = block->Instruction_list.erase(it);
            }
            else {
                // 有用的指令，保留
                ++it;
            }
        }
    }
}
