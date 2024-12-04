#include "../include/Instruction.h"
#include "../include/basic_block.h"
#include <assert.h>
#include <unordered_map>

static std::unordered_map<int, RegOperand *> RegOperandMap;
static std::map<int, LabelOperand *> LabelOperandMap;
static std::map<std::string, GlobalOperand *> GlobalOperandMap;

RegOperand *GetNewRegOperand(int RegNo) {
    auto it = RegOperandMap.find(RegNo);
    if (it == RegOperandMap.end()) {
        auto R = new RegOperand(RegNo);
        RegOperandMap[RegNo] = R;
        return R;
    } else {
        return it->second;
    }
}

LabelOperand *GetNewLabelOperand(int LabelNo) {
    auto it = LabelOperandMap.find(LabelNo);
    if (it == LabelOperandMap.end()) {
        auto L = new LabelOperand(LabelNo);
        LabelOperandMap[LabelNo] = L;
        return L;
    } else {
        return it->second;
    }
}

GlobalOperand *GetNewGlobalOperand(std::string name) {
    auto it = GlobalOperandMap.find(name);
    if (it == GlobalOperandMap.end()) {
        auto G = new GlobalOperand(name);
        GlobalOperandMap[name] = G;
        return G;
    } else {
        return it->second;
    }
}

void IRgenArithmeticI32(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int reg1, int reg2, int result_reg) {
    B->InsertInstruction(1, new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::I32, GetNewRegOperand(reg1),
                                                      GetNewRegOperand(reg2), GetNewRegOperand(result_reg)));
}

void IRgenArithmeticF32(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int reg1, int reg2, int result_reg) {
    B->InsertInstruction(1,
                         new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::FLOAT32, GetNewRegOperand(reg1),
                                                   GetNewRegOperand(reg2), GetNewRegOperand(result_reg)));
}

void IRgenArithmeticI32ImmLeft(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int val1, int reg2, int result_reg) {
    B->InsertInstruction(1, new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::I32, new ImmI32Operand(val1),
                                                      GetNewRegOperand(reg2), GetNewRegOperand(result_reg)));
}

void IRgenArithmeticF32ImmLeft(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, float val1, int reg2,
                               int result_reg) {
    B->InsertInstruction(1,
                         new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::FLOAT32, new ImmF32Operand(val1),
                                                   GetNewRegOperand(reg2), GetNewRegOperand(result_reg)));
}

void IRgenArithmeticI32ImmAll(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int val1, int val2, int result_reg) {
    B->InsertInstruction(1, new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::I32, new ImmI32Operand(val1),
                                                      new ImmI32Operand(val2), GetNewRegOperand(result_reg)));
}

void IRgenArithmeticF32ImmAll(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, float val1, float val2,
                              int result_reg) {
    B->InsertInstruction(1,
                         new ArithmeticInstruction(opcode, BasicInstruction::LLVMType::FLOAT32, new ImmF32Operand(val1),
                                                   new ImmF32Operand(val2), GetNewRegOperand(result_reg)));
}

void IRgenIcmp(LLVMBlock B, BasicInstruction::IcmpCond cmp_op, int reg1, int reg2, int result_reg) {
    B->InsertInstruction(1, new IcmpInstruction(BasicInstruction::LLVMType::I32, GetNewRegOperand(reg1),
                                                GetNewRegOperand(reg2), cmp_op, GetNewRegOperand(result_reg)));
}

void IRgenFcmp(LLVMBlock B, BasicInstruction::FcmpCond cmp_op, int reg1, int reg2, int result_reg) {
    B->InsertInstruction(1, new FcmpInstruction(BasicInstruction::LLVMType::FLOAT32, GetNewRegOperand(reg1),
                                                GetNewRegOperand(reg2), cmp_op, GetNewRegOperand(result_reg)));
}

void IRgenIcmpImmRight(LLVMBlock B, BasicInstruction::IcmpCond cmp_op, int reg1, int val2, int result_reg) {
    B->InsertInstruction(1, new IcmpInstruction(BasicInstruction::LLVMType::I32, GetNewRegOperand(reg1),
                                                new ImmI32Operand(val2), cmp_op, GetNewRegOperand(result_reg)));
}

void IRgenFcmpImmRight(LLVMBlock B, BasicInstruction::FcmpCond cmp_op, int reg1, float val2, int result_reg) {
    B->InsertInstruction(1, new FcmpInstruction(BasicInstruction::LLVMType::FLOAT32, GetNewRegOperand(reg1),
                                                new ImmF32Operand(val2), cmp_op, GetNewRegOperand(result_reg)));
}

void IRgenFptosi(LLVMBlock B, int src, int dst) {
    B->InsertInstruction(1, new FptosiInstruction(GetNewRegOperand(dst), GetNewRegOperand(src)));
}

void IRgenSitofp(LLVMBlock B, int src, int dst) {
    B->InsertInstruction(1, new SitofpInstruction(GetNewRegOperand(dst), GetNewRegOperand(src)));
}

void IRgenZextI1toI32(LLVMBlock B, int src, int dst) {
    B->InsertInstruction(1, new ZextInstruction(BasicInstruction::LLVMType::I32, GetNewRegOperand(dst),
                                                BasicInstruction::LLVMType::I1, GetNewRegOperand(src)));
}

void IRgenGetElementptrIndexI32(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr,
                        std::vector<int> dims, std::vector<Operand> indexs) {
    B->InsertInstruction(1, new GetElementptrInstruction(type, GetNewRegOperand(result_reg), ptr, dims, indexs, BasicInstruction::I32));
}

void IRgenGetElementptrIndexI64(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr,
                        std::vector<int> dims, std::vector<Operand> indexs) {
    B->InsertInstruction(1, new GetElementptrInstruction(type, GetNewRegOperand(result_reg), ptr, dims, indexs, BasicInstruction::I64));
}

void IRgenLoad(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr) {
    B->InsertInstruction(1, new LoadInstruction(type, ptr, GetNewRegOperand(result_reg)));
}

void IRgenStore(LLVMBlock B, BasicInstruction::LLVMType type, int value_reg, Operand ptr) {
    B->InsertInstruction(1, new StoreInstruction(type, ptr, GetNewRegOperand(value_reg)));
}

void IRgenStore(LLVMBlock B, BasicInstruction::LLVMType type, Operand value, Operand ptr) {
    B->InsertInstruction(1, new StoreInstruction(type, ptr, value));
}

void IRgenCall(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg,
               std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args, std::string name) {
    B->InsertInstruction(1, new CallInstruction(type, GetNewRegOperand(result_reg), name, args));
}

void IRgenCallVoid(LLVMBlock B, BasicInstruction::LLVMType type,
                   std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args, std::string name) {
    B->InsertInstruction(1, new CallInstruction(type, GetNewRegOperand(-1), name, args));
}

void IRgenCallNoArgs(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, std::string name) {
    B->InsertInstruction(1, new CallInstruction(type, GetNewRegOperand(result_reg), name));
}

void IRgenCallVoidNoArgs(LLVMBlock B, BasicInstruction::LLVMType type, std::string name) {
    B->InsertInstruction(1, new CallInstruction(type, GetNewRegOperand(-1), name));
}

void IRgenRetReg(LLVMBlock B, BasicInstruction::LLVMType type, int reg) {
    B->InsertInstruction(1, new RetInstruction(type, GetNewRegOperand(reg)));
}

void IRgenRetImmInt(LLVMBlock B, BasicInstruction::LLVMType type, int val) {
    B->InsertInstruction(1, new RetInstruction(type, new ImmI32Operand(val)));
}

void IRgenRetImmFloat(LLVMBlock B, BasicInstruction::LLVMType type, float val) {
    B->InsertInstruction(1, new RetInstruction(type, new ImmF32Operand(val)));
}

void IRgenRetVoid(LLVMBlock B) {
    B->InsertInstruction(1, new RetInstruction(BasicInstruction::LLVMType::VOID, nullptr));
}

void IRgenBRUnCond(LLVMBlock B, int dst_label) {
    B->InsertInstruction(1, new BrUncondInstruction(GetNewLabelOperand(dst_label)));
}

void IRgenBrCond(LLVMBlock B, int cond_reg, int true_label, int false_label) {
    B->InsertInstruction(1, new BrCondInstruction(GetNewRegOperand(cond_reg), GetNewLabelOperand(true_label),
                                                  GetNewLabelOperand(false_label)));
}

void IRgenAlloca(LLVMBlock B, BasicInstruction::LLVMType type, int reg) {
    B->InsertInstruction(0, new AllocaInstruction(type, GetNewRegOperand(reg)));
}

void IRgenAllocaArray(LLVMBlock B, BasicInstruction::LLVMType type, int reg, std::vector<int> dims) {
    B->InsertInstruction(0, new AllocaInstruction(type, dims, GetNewRegOperand(reg)));
}

void LoadInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (pointer->GetOperandType() == BasicOperand::REG) {
        auto pointer_reg = (RegOperand *)pointer;
        if (Rule.find(pointer_reg->GetRegNo()) != Rule.end())
            this->pointer = GetNewRegOperand(Rule.find(pointer_reg->GetRegNo())->second);
    }
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            this->result = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
}

void StoreInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (pointer->GetOperandType() == BasicOperand::REG) {
        auto pointer_reg = (RegOperand *)pointer;
        if (Rule.find(pointer_reg->GetRegNo()) != Rule.end())
            this->pointer = GetNewRegOperand(Rule.find(pointer_reg->GetRegNo())->second);
    }
    if (value->GetOperandType() == BasicOperand::REG) {
        auto value_reg = (RegOperand *)value;
        if (Rule.find(value_reg->GetRegNo()) != Rule.end())
            this->value = GetNewRegOperand(Rule.find(value_reg->GetRegNo())->second);
    }
}

void ArithmeticInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (op3 != nullptr) {
        if (op3->GetOperandType() == BasicOperand::REG) {
            auto op3_reg = (RegOperand *)op3;
            if (Rule.find(op3_reg->GetRegNo()) != Rule.end())
                this->op3 = GetNewRegOperand(Rule.find(op3_reg->GetRegNo())->second);
        }
    }
    if (op2->GetOperandType() == BasicOperand::REG) {
        auto op2_reg = (RegOperand *)op2;
        if (Rule.find(op2_reg->GetRegNo()) != Rule.end())
            this->op2 = GetNewRegOperand(Rule.find(op2_reg->GetRegNo())->second);
    }
    if (op1->GetOperandType() == BasicOperand::REG) {
        auto op1_reg = (RegOperand *)op1;
        if (Rule.find(op1_reg->GetRegNo()) != Rule.end())
            this->op1 = GetNewRegOperand(Rule.find(op1_reg->GetRegNo())->second);
    }
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            this->result = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
}

void IcmpInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (op2->GetOperandType() == BasicOperand::REG) {
        auto op2_reg = (RegOperand *)op2;
        if (Rule.find(op2_reg->GetRegNo()) != Rule.end())
            this->op2 = GetNewRegOperand(Rule.find(op2_reg->GetRegNo())->second);
    }
    if (op1->GetOperandType() == BasicOperand::REG) {
        auto op1_reg = (RegOperand *)op1;
        if (Rule.find(op1_reg->GetRegNo()) != Rule.end())
            this->op1 = GetNewRegOperand(Rule.find(op1_reg->GetRegNo())->second);
    }
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            this->result = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
}

void FcmpInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (op2->GetOperandType() == BasicOperand::REG) {
        auto op2_reg = (RegOperand *)op2;
        if (Rule.find(op2_reg->GetRegNo()) != Rule.end())
            this->op2 = GetNewRegOperand(Rule.find(op2_reg->GetRegNo())->second);
    }
    if (op1->GetOperandType() == BasicOperand::REG) {
        auto op1_reg = (RegOperand *)op1;
        if (Rule.find(op1_reg->GetRegNo()) != Rule.end())
            this->op1 = GetNewRegOperand(Rule.find(op1_reg->GetRegNo())->second);
    }
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            this->result = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
}

void PhiInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    for (auto &label_pair : phi_list) {
        auto &op1 = label_pair.first;
        if (op1->GetOperandType() == BasicOperand::REG) {
            auto op1_reg = (RegOperand *)op1;
            if (Rule.find(op1_reg->GetRegNo()) != Rule.end())
                op1 = GetNewRegOperand(Rule.find(op1_reg->GetRegNo())->second);
        }
        auto &op2 = label_pair.second;
        if (op2->GetOperandType() == BasicOperand::REG) {
            auto op2_reg = (RegOperand *)op2;
            if (Rule.find(op2_reg->GetRegNo()) != Rule.end())
                op2 = GetNewRegOperand(Rule.find(op2_reg->GetRegNo())->second);
        }
    }
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            this->result = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
}

void AllocaInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            this->result = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
}

void BrCondInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (cond->GetOperandType() == BasicOperand::REG) {
        auto cond_reg = (RegOperand *)cond;
        if (Rule.find(cond_reg->GetRegNo()) != Rule.end())
            this->cond = GetNewRegOperand(Rule.find(cond_reg->GetRegNo())->second);
    }
}

void BrUncondInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {}

void GlobalVarDefineInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {}

void GlobalStringConstInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {}

void CallInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    for (auto &arg_pair : args) {
        if (arg_pair.second->GetOperandType() == BasicOperand::REG) {
            auto op = (RegOperand *)arg_pair.second;
            if (Rule.find(op->GetRegNo()) != Rule.end())
                arg_pair.second = GetNewRegOperand(Rule.find(op->GetRegNo())->second);
        }
    }
    if (result != NULL) {
        if (result->GetOperandType() == BasicOperand::REG) {
            auto result_reg = (RegOperand *)result;
            if (Rule.find(result_reg->GetRegNo()) != Rule.end())
                this->result = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
        }
    }
}

void RetInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (ret_val != NULL) {
        if (ret_val->GetOperandType() == BasicOperand::REG) {
            auto result_reg = (RegOperand *)ret_val;
            if (Rule.find(result_reg->GetRegNo()) != Rule.end())
                ret_val = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
        }
    }
}

void GetElementptrInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            this->result = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
    if (ptrval->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)ptrval;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            this->ptrval = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
    for (auto &idx_pair : indexes) {
        if (idx_pair->GetOperandType() == BasicOperand::REG) {
            auto idx_reg = (RegOperand *)idx_pair;
            if (Rule.find(idx_reg->GetRegNo()) != Rule.end())
                idx_pair = GetNewRegOperand(Rule.find(idx_reg->GetRegNo())->second);
        }
    }
}

void FunctionDefineInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {}

void FunctionDeclareInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {}

void FptosiInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            this->result = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
    if (value->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)value;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            value = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
}

void SitofpInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            this->result = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
    if (value->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)value;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            value = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
}

void ZextInstruction::ReplaceRegByMap(const std::map<int, int> &Rule) {
    if (result->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)result;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            this->result = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
    if (value->GetOperandType() == BasicOperand::REG) {
        auto result_reg = (RegOperand *)value;
        if (Rule.find(result_reg->GetRegNo()) != Rule.end())
            value = GetNewRegOperand(Rule.find(result_reg->GetRegNo())->second);
    }
}
