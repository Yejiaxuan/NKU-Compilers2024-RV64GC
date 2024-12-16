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

Operand RegOperand::CopyOperand() { return GetNewRegOperand(reg_no); }

Operand ImmI32Operand::CopyOperand() { return new ImmI32Operand(immVal); }

Operand ImmF32Operand::CopyOperand() { return new ImmF32Operand(immVal); }

Operand ImmI64Operand::CopyOperand() { return new ImmI64Operand(immVal); }

Operand LabelOperand::CopyOperand() { return GetNewLabelOperand(label_no); }

Operand GlobalOperand::CopyOperand() { return GetNewGlobalOperand(name); }

Instruction LoadInstruction::CopyInstruction() {
    Operand npointer = pointer->CopyOperand();
    Operand nresult = result->CopyOperand();
    return new LoadInstruction(type, npointer, nresult);
}

Instruction StoreInstruction::CopyInstruction() {
    Operand npointer = pointer->CopyOperand();
    Operand nvalue = value->CopyOperand();
    return new StoreInstruction(type, npointer, nvalue);
}

Instruction ArithmeticInstruction::CopyInstruction() {

    Operand nop1 = op1->CopyOperand();
    Operand nop2 = op2->CopyOperand();
    Operand nresult = result->CopyOperand();
    if (op3 == nullptr) {
        return new ArithmeticInstruction(opcode, type, nop1, nop2, nresult);
    }
    Operand nop3 = op3->CopyOperand();
    return new ArithmeticInstruction(opcode, type, nop1, nop2, nop3, nresult);
}

Instruction IcmpInstruction::CopyInstruction() {
    Operand nop1 = op1->CopyOperand();
    Operand nop2 = op2->CopyOperand();
    Operand nresult = result->CopyOperand();
    return new IcmpInstruction(type, nop1, nop2, cond, nresult);
}

Instruction FcmpInstruction::CopyInstruction() {
    Operand nop1 = op1->CopyOperand();
    Operand nop2 = op2->CopyOperand();
    Operand nresult = result->CopyOperand();
    return new FcmpInstruction(type, nop1, nop2, cond, nresult);
}

Instruction PhiInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    std::vector<std::pair<Operand, Operand>> nval_labels;
    // std::map<operand,operand> nval_labels;

    for (auto Phiop : phi_list) {
        Operand nlabel = Phiop.first->CopyOperand();
        Operand nvalue = Phiop.second->CopyOperand();
        // nval_labels.push_back({nlabel,nvalue});
        nval_labels.push_back(std::make_pair(nlabel, nvalue));
    }

    return new PhiInstruction(type, nresult, nval_labels);
}

Instruction AllocaInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    std::vector<int> ndims;
    for (auto dimint : dims) {
        ndims.push_back(dimint);
    }
    return new AllocaInstruction(type, ndims, nresult);
}

Instruction BrCondInstruction::CopyInstruction() {
    Operand ncond = cond->CopyOperand();
    Operand ntrueLabel = trueLabel->CopyOperand();
    Operand nfalseLabel = falseLabel->CopyOperand();
    return new BrCondInstruction(ncond, ntrueLabel, nfalseLabel);
}

Instruction BrUncondInstruction::CopyInstruction() {
    Operand ndestLabel = destLabel->CopyOperand();
    return new BrUncondInstruction(ndestLabel);
}

Instruction CallInstruction::CopyInstruction() {
    Operand nresult = NULL;
    if (ret_type != VOID) {
        nresult = result->CopyOperand();
    }
    std::vector<std::pair<enum LLVMType, Operand>> nargs;
    for (auto n : args) {
        nargs.push_back({n.first, n.second->CopyOperand()});
    }
    return new CallInstruction(ret_type, nresult, name, nargs);
}

Instruction RetInstruction::CopyInstruction() { return new RetInstruction(ret_type, ret_val); }

Instruction GetElementptrInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    Operand nptrval = ptrval->CopyOperand();
    std::vector<Operand> nindexes;
    for (auto index : indexes) {
        Operand nindex = index->CopyOperand();
        nindexes.push_back(nindex);
    }

    return new GetElementptrInstruction(type, nresult, nptrval, dims, nindexes, I32);
}

Instruction FptosiInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    Operand nvalue = value->CopyOperand();

    return new FptosiInstruction(nresult, nvalue);
}

Instruction SitofpInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    Operand nvalue = value->CopyOperand();

    return new SitofpInstruction(nresult, nvalue);
}

Instruction ZextInstruction::CopyInstruction() {
    Operand nresult = result->CopyOperand();
    Operand nvalue = value->CopyOperand();

    return new ZextInstruction(to_type, nresult, from_type, nvalue);
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

int LoadInstruction::GetResultRegNo() {
    if (result && result->GetOperandType() == BasicOperand::REG) {
        return ((RegOperand*)result)->GetRegNo();
    }
    return -1;
}
std::vector<int> LoadInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    if (pointer->GetOperandType() == BasicOperand::REG) {
        regs.push_back(((RegOperand*)pointer)->GetRegNo());
    }
    return regs;
}

int StoreInstruction::GetResultRegNo() {
    // store没有结果寄存器
    return -1;
}
std::vector<int> StoreInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    if (value->GetOperandType() == BasicOperand::REG) {
        regs.push_back(((RegOperand*)value)->GetRegNo());
    }
    if (pointer->GetOperandType() == BasicOperand::REG) {
        regs.push_back(((RegOperand*)pointer)->GetRegNo());
    }
    return regs;
}

int ArithmeticInstruction::GetResultRegNo() {
    if (result && result->GetOperandType() == BasicOperand::REG) {
        return ((RegOperand*)result)->GetRegNo();
    }
    return -1;
}
std::vector<int> ArithmeticInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    if (op1->GetOperandType() == BasicOperand::REG) regs.push_back(((RegOperand*)op1)->GetRegNo());
    if (op2->GetOperandType() == BasicOperand::REG) regs.push_back(((RegOperand*)op2)->GetRegNo());
    if (op3 && op3->GetOperandType() == BasicOperand::REG) regs.push_back(((RegOperand*)op3)->GetRegNo());
    return regs;
}

int IcmpInstruction::GetResultRegNo() {
    if (result && result->GetOperandType() == BasicOperand::REG) {
        return ((RegOperand*)result)->GetRegNo();
    }
    return -1;
}
std::vector<int> IcmpInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    if (op1->GetOperandType() == BasicOperand::REG) regs.push_back(((RegOperand*)op1)->GetRegNo());
    if (op2->GetOperandType() == BasicOperand::REG) regs.push_back(((RegOperand*)op2)->GetRegNo());
    return regs;
}

int FcmpInstruction::GetResultRegNo() {
    if (result && result->GetOperandType() == BasicOperand::REG) {
        return ((RegOperand*)result)->GetRegNo();
    }
    return -1;
}
std::vector<int> FcmpInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    if (op1->GetOperandType() == BasicOperand::REG) regs.push_back(((RegOperand*)op1)->GetRegNo());
    if (op2->GetOperandType() == BasicOperand::REG) regs.push_back(((RegOperand*)op2)->GetRegNo());
    return regs;
}

int PhiInstruction::GetResultRegNo() {
    if (result && result->GetOperandType() == BasicOperand::REG) {
        return ((RegOperand*)result)->GetRegNo();
    }
    return -1;
}
std::vector<int> PhiInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    for (auto &pair : GetPhiList()) {
        Operand val = pair.second;
        if (val->GetOperandType() == BasicOperand::REG) {
            regs.push_back(((RegOperand*)val)->GetRegNo());
        }
    }
    return regs;
}

int AllocaInstruction::GetResultRegNo() {
    if (result && result->GetOperandType() == BasicOperand::REG) {
        return ((RegOperand*)result)->GetRegNo();
    }
    return -1;
}
std::vector<int> AllocaInstruction::GetUsedRegisters() {
    return {}; // alloca无使用寄存器
}

int BrCondInstruction::GetResultRegNo() {
    // 无结果寄存器
    return -1;
}
std::vector<int> BrCondInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    if (cond->GetOperandType() == BasicOperand::REG) {
        regs.push_back(((RegOperand*)cond)->GetRegNo());
    }
    return regs;
}

int BrUncondInstruction::GetResultRegNo() {
    return -1;
}
std::vector<int> BrUncondInstruction::GetUsedRegisters() {
    return {}; // 无条件跳转无使用寄存器
}

int GlobalVarDefineInstruction::GetResultRegNo() {
    return -1; // 定义指令无返回值寄存器
}
std::vector<int> GlobalVarDefineInstruction::GetUsedRegisters() {
    return {}; 
}

int GlobalStringConstInstruction::GetResultRegNo() {
    return -1;
}
std::vector<int> GlobalStringConstInstruction::GetUsedRegisters() {
    return {};
}

int CallInstruction::GetResultRegNo() {
    if (result && result->GetOperandType() == BasicOperand::REG) {
        return ((RegOperand*)result)->GetRegNo();
    }
    return -1;
}
std::vector<int> CallInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    for (auto &arg : GetParameterList()) {
        if (arg.second->GetOperandType() == BasicOperand::REG) {
            regs.push_back(((RegOperand*)arg.second)->GetRegNo());
        }
    }
    return regs;
}

int RetInstruction::GetResultRegNo() {
    return -1; // ret本身无结果寄存器
}
std::vector<int> RetInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    if (ret_val && ret_val->GetOperandType() == BasicOperand::REG) {
        regs.push_back(((RegOperand*)ret_val)->GetRegNo());
    }
    return regs;
}

int GetElementptrInstruction::GetResultRegNo() {
    if (result && result->GetOperandType() == BasicOperand::REG) {
        return ((RegOperand*)result)->GetRegNo();
    }
    return -1;
}
std::vector<int> GetElementptrInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    if (ptrval->GetOperandType() == BasicOperand::REG) {
        regs.push_back(((RegOperand*)ptrval)->GetRegNo());
    }
    for (auto &idx : indexes) {
        if (idx->GetOperandType() == BasicOperand::REG) {
            regs.push_back(((RegOperand*)idx)->GetRegNo());
        }
    }
    return regs;
}

int FunctionDefineInstruction::GetResultRegNo() {
    return -1;
}
std::vector<int> FunctionDefineInstruction::GetUsedRegisters() {
    return {};
}

int FunctionDeclareInstruction::GetResultRegNo() {
    return -1;
}
std::vector<int> FunctionDeclareInstruction::GetUsedRegisters() {
    return {};
}

int FptosiInstruction::GetResultRegNo() {
    if (result && result->GetOperandType() == BasicOperand::REG) {
        return ((RegOperand*)result)->GetRegNo();
    }
    return -1;
}
std::vector<int> FptosiInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    if (value->GetOperandType() == BasicOperand::REG) {
        regs.push_back(((RegOperand*)value)->GetRegNo());
    }
    return regs;
}

int SitofpInstruction::GetResultRegNo() {
    if (result && result->GetOperandType() == BasicOperand::REG) {
        return ((RegOperand*)result)->GetRegNo();
    }
    return -1;
}
std::vector<int> SitofpInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    if (value->GetOperandType() == BasicOperand::REG) {
        regs.push_back(((RegOperand*)value)->GetRegNo());
    }
    return regs;
}

int ZextInstruction::GetResultRegNo() {
    if (result && result->GetOperandType() == BasicOperand::REG) {
        return ((RegOperand*)result)->GetRegNo();
    }
    return -1;
}
std::vector<int> ZextInstruction::GetUsedRegisters() {
    std::vector<int> regs;
    if (value->GetOperandType() == BasicOperand::REG) {
        regs.push_back(((RegOperand*)value)->GetRegNo());
    }
    return regs;
}

std::vector<Operand> LoadInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    ret.push_back(pointer);
    return ret;
}

std::vector<Operand> StoreInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    ret.push_back(pointer);
    ret.push_back(value);
    return ret;
}

std::vector<Operand> ArithmeticInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    ret.push_back(op1);
    ret.push_back(op2);
    if (op3 != nullptr) {
        ret.push_back(op3);
    }
    return ret;
}

std::vector<Operand> IcmpInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    ret.push_back(op1);
    ret.push_back(op2);
    // ret.push_back(cond);
    return ret;
}

std::vector<Operand> FcmpInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    ret.push_back(op1);
    ret.push_back(op2);
    // ret.push_back(cond);
    return ret;
}

std::vector<Operand> PhiInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    for (auto label_val_pair : phi_list) {
        ret.push_back(label_val_pair.second);
    }
    return ret;
}

std::vector<Operand> BrCondInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    ret.push_back(cond);
    return ret;
}

std::vector<Operand> CallInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    for (auto arg_pair : args) {
        ret.push_back(arg_pair.second);
    }
    return ret;
}

std::vector<Operand> RetInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    if (ret_val != NULL)
        ret.push_back(ret_val);
    return ret;
}

std::vector<Operand> GetElementptrInstruction::GetNonResultOperands() {
    std::vector<Operand> ret(indexes);
    ret.push_back(ptrval);
    return ret;
}


std::vector<Operand> FunctionDefineInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    std::cerr << "func_define_Instruction get_nonresult_operands()\n";
    return ret;
}

std::vector<Operand> FunctionDeclareInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    std::cerr << "func_declare_Instruction get_nonresult_operands()\n";
    return ret;
}

std::vector<Operand> FptosiInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    ret.push_back(value);
    return ret;
}

std::vector<Operand> SitofpInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    ret.push_back(value);
    return ret;
}

std::vector<Operand> ZextInstruction::GetNonResultOperands() {
    std::vector<Operand> ret;
    ret.push_back(value);
    return ret;
}

void PhiInstruction::ReplaceLabelByMap(const std::map<int, int> &Rule) {
    for (auto &[label, val] : phi_list) {
        auto l = (LabelOperand *)label;
        if (Rule.find(l->GetLabelNo()) != Rule.end()) {
            label = GetNewLabelOperand(Rule.find(l->GetLabelNo())->second);
        }
    }
}

void BrCondInstruction::ReplaceLabelByMap(const std::map<int, int> &Rule) {
    auto true_label = (LabelOperand *)this->trueLabel;
    auto false_label = (LabelOperand *)this->falseLabel;

    if (Rule.find(true_label->GetLabelNo()) != Rule.end()) {
        trueLabel = GetNewLabelOperand(Rule.find(true_label->GetLabelNo())->second);
    }

    if (Rule.find(false_label->GetLabelNo()) != Rule.end()) {
        falseLabel = GetNewLabelOperand(Rule.find(false_label->GetLabelNo())->second);
    }
}

void BrUncondInstruction::ReplaceLabelByMap(const std::map<int, int> &Rule) {
    auto dest_label = (LabelOperand *)this->destLabel;

    if (Rule.find(dest_label->GetLabelNo()) != Rule.end()) {
        destLabel = GetNewLabelOperand(Rule.find(dest_label->GetLabelNo())->second);
    }
}
