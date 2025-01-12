#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "symtab.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#ifndef ERROR
#define ERROR(...)                                                                                                     \
    do {                                                                                                               \
        char message[256];                                                                                             \
        sprintf(message, __VA_ARGS__);                                                                                 \
        std::cerr << "\033[;31;1m";                                                                                    \
        std::cerr << "ERROR: ";                                                                                        \
        std::cerr << "\033[0;37;1m";                                                                                   \
        std::cerr << message << std::endl;                                                                             \
        std::cerr << "\033[0;33;1m";                                                                                   \
        std::cerr << "File: \033[4;37;1m" << __FILE__ << ":" << __LINE__ << std::endl;                                 \
        std::cerr << "\033[0m";                                                                                        \
        assert(false);                                                                                                 \
    } while (0)
#endif

#define ENABLE_TODO
#ifdef ENABLE_TODO
#ifndef TODO
#define TODO(...)                                                                                                      \
    do {                                                                                                               \
        char message[256];                                                                                             \
        sprintf(message, __VA_ARGS__);                                                                                 \
        std::cerr << "\033[;34;1m";                                                                                    \
        std::cerr << "TODO: ";                                                                                         \
        std::cerr << "\033[0;37;1m";                                                                                   \
        std::cerr << message << std::endl;                                                                             \
        std::cerr << "\033[0;33;1m";                                                                                   \
        std::cerr << "File: \033[4;37;1m" << __FILE__ << ":" << __LINE__ << std::endl;                                 \
        std::cerr << "\033[0m";                                                                                        \
        assert(false);                                                                                                 \
    } while (0)
#endif
#else
#ifndef TODO
#define TODO(...)
#endif
#endif

#define ENABLE_LOG
#ifdef ENABLE_LOG
#ifndef Log
#define Log(...)                                                                                                       \
    do {                                                                                                               \
        char message[256];                                                                                             \
        sprintf(message, __VA_ARGS__);                                                                                 \
        std::cerr << "\033[;35;1m[\033[4;33;1m" << __FILE__ << ":" << __LINE__ << "\033[;35;1m "                       \
                  << __PRETTY_FUNCTION__ << "]";                                                                       \
        std::cerr << "\033[0;37;1m ";                                                                                  \
        std::cerr << message << std::endl;                                                                             \
        std::cerr << "\033[0m";                                                                                        \
    } while (0)
#endif
#else
#ifndef Log
#define Log(...)
#endif
#endif

#ifndef Assert
#define Assert(EXP)                                                                                                    \
    do {                                                                                                               \
        if (!(EXP)) {                                                                                                  \
            ERROR("Assertion failed: %s", #EXP);                                                                       \
        }                                                                                                              \
    } while (0)
#endif


// TODO(): 加入更多你需要的成员变量和函数
// TODO(): 加入更多你需要的指令类

// 你首先需要阅读utils/Instruction_out.cc来了解指令是如何输出的
// 除注释特别说明外，成员函数不允许设置为nullptr，否则指令输出时会出现段错误

// 我们规定，对于GlobalOperand, LabelOperand, RegOperand, 只要操作数相同, 地址也相同
// 所以这些Operand的构造函数是private, 使用GetNew***Operand函数来获取新的操作数变量

// 对于不同位置的指令，即使内容完全相同，也不推荐使用相同的地址, 
// 当你向基本块中插入指令时，推荐先new一个，不要使用之前已经插入过的指令，你需要保证地址不同，
// 否则在代码优化阶段你会需要调试一些奇怪的bug。(例如你修改了一处指令的操作数，却发现好几条指令的操作数都被修改了)

// 对于Operand，大部分Operand只要内容相同，地址就相同
// 所以最好不要对Operand相关类增加修改私有变量的函数。
// 否则你可能修改了一个Operand的私有变量，然后发现所有Operand的私有变量都被修改了
// 如果你想修改一条指令的操作数，不要直接修改操作数的私有变量，
// 你可以先使用GetNew***Operand函数来获取一个新的操作数op2，然后设置指令的操作数为op2

// 请注意代码中的typedef，为了方便书写，将一些类的指针进行了重命名, 如果不习惯该种风格，可以自行修改


class BasicOperand;
typedef BasicOperand *Operand;
// @operands in instruction
class BasicOperand {
public:
    enum operand_type { REG = 1, IMMI32 = 2, IMMF32 = 3, GLOBAL = 4, LABEL = 5, IMMI64 = 6 };

protected:
    operand_type operandType;

public:
    BasicOperand() {}
    operand_type GetOperandType() { return operandType; }
    virtual std::string GetFullName() = 0;
    virtual Operand CopyOperand() = 0;
};

// @register operand;%r+register No
class RegOperand : public BasicOperand {
    int reg_no;
    RegOperand(int RegNo) {
        this->operandType = REG;
        this->reg_no = RegNo;
    }

public:
    int GetRegNo() { return reg_no; }

    friend RegOperand *GetNewRegOperand(int RegNo);
    virtual std::string GetFullName();
    virtual Operand CopyOperand();
};
RegOperand *GetNewRegOperand(int RegNo);

// @integer32 immediate
class ImmI32Operand : public BasicOperand {
    int immVal;

public:
    int GetIntImmVal() { return immVal; }

    ImmI32Operand(int immVal) {
        this->operandType = IMMI32;
        this->immVal = immVal;
    }
    virtual std::string GetFullName();
    virtual Operand CopyOperand();
};

// @integer64 immediate
class ImmI64Operand : public BasicOperand {
    long long immVal;

public:
    long long GetLlImmVal() { return immVal; }

    ImmI64Operand(long long immVal) {
        this->operandType = IMMI64;
        this->immVal = immVal;
    }
    virtual std::string GetFullName();
    virtual Operand CopyOperand();
};

// @float32 immediate
class ImmF32Operand : public BasicOperand {
    float immVal;

public:
    float GetFloatVal() { return immVal; }

    long long GetFloatByteVal();

    ImmF32Operand(float immVal) {
        this->operandType = IMMF32;
        this->immVal = immVal;
    }
    virtual std::string GetFullName();
    virtual Operand CopyOperand();
};

// @label %L+label No
class LabelOperand : public BasicOperand {
    int label_no;
    LabelOperand(int LabelNo) {
        this->operandType = LABEL;
        this->label_no = LabelNo;
    }

public:
    int GetLabelNo() { return label_no; }

    friend LabelOperand *GetNewLabelOperand(int LabelNo);
    virtual std::string GetFullName();
    virtual Operand CopyOperand();
};

LabelOperand *GetNewLabelOperand(int RegNo);

// @global identifier @+name
class GlobalOperand : public BasicOperand {
    std::string name;
    GlobalOperand(std::string gloName) {
        this->operandType = GLOBAL;
        this->name = gloName;
    }

public:
    std::string GetName() { return name; }

    friend GlobalOperand *GetNewGlobalOperand(std::string name);
    virtual std::string GetFullName();
    virtual Operand CopyOperand();
};

GlobalOperand *GetNewGlobalOperand(std::string name);

class BasicInstruction;

typedef BasicInstruction *Instruction;

// @instruction
class BasicInstruction {
public:
    // @Instriction types
    enum LLVMIROpcode {
        OTHER = 0,
        LOAD = 1,
        STORE = 2,
        ADD = 3,
        SUB = 4,
        ICMP = 5,
        PHI = 6,
        ALLOCA = 7,
        MUL = 8,
        DIV = 9,
        BR_COND = 10,
        BR_UNCOND = 11,
        FADD = 12,
        FSUB = 13,
        FMUL = 14,
        FDIV = 15,
        FCMP = 16,
        MOD = 17,
        BITXOR = 18,
        RET = 19,
        ZEXT = 20,
        SHL = 21,
        FPTOSI = 24,
        GETELEMENTPTR = 25,
        CALL = 26,
        SITOFP = 27,
        GLOBAL_VAR = 28,
        GLOBAL_STR = 29,
        LL_ADDMOD = 30,
        UMIN_I32 = 31,
        UMAX_I32 = 32,
        SMIN_I32 = 33,
        SMAX_I32 = 34,
        BITCAST = 35,
        FMIN_F32 = 36,
        FMAX_F32 = 37,
        BITAND = 38,
    };

    // @Operand datatypes
    enum LLVMType { I32 = 1, FLOAT32 = 2, PTR = 3, VOID = 4, I8 = 5, I1 = 6, I64 = 7, DOUBLE = 8 };

    // @ <cond> in icmp Instruction
    enum IcmpCond {
        eq = 1,     //: equal
        ne = 2,     //: not equal
        ugt = 3,    //: unsigned greater than
        uge = 4,    //: unsigned greater or equal
        ult = 5,    //: unsigned less than
        ule = 6,    //: unsigned less or equal
        sgt = 7,    //: signed greater than
        sge = 8,    //: signed greater or equal
        slt = 9,    //: signed less than
        sle = 10    //: signed less or equal
    };

    enum FcmpCond {
        FALSE = 1,    //: no comparison, always returns false
        OEQ = 2,      // ordered and equal
        OGT = 3,      //: ordered and greater than
        OGE = 4,      //: ordered and greater than or equal
        OLT = 5,      //: ordered and less than
        OLE = 6,      //: ordered and less than or equal
        ONE = 7,      //: ordered and not equal
        ORD = 8,      //: ordered (no nans)
        UEQ = 9,      //: unordered or equal
        UGT = 10,     //: unordered or greater than
        UGE = 11,     //: unordered or greater than or equal
        ULT = 12,     //: unordered or less than
        ULE = 13,     //: unordered or less than or equal
        UNE = 14,     //: unordered or not equal
        UNO = 15,     //: unordered (either nans)
        TRUE = 16     //: no comparison, always returns true
    };

private:

    int BlockID = 0;

protected:
    LLVMIROpcode opcode;

public:
    int GetOpcode() { return opcode; }    // one solution: convert to pointer of subclasses

    void SetBlockID(int blockno) { BlockID = blockno; }
    int GetBlockID() { return BlockID; }

    virtual void PrintIR(std::ostream &s) = 0;
    virtual void ReplaceRegByMap(const std::map<int, int> &Rule) = 0;
    virtual void ReplaceLabelByMap(const std::map<int, int> &Rule) = 0;
    virtual std::vector<Operand> GetNonResultOperands() = 0;
    virtual Instruction CopyInstruction() = 0;
    virtual int GetResultRegNo() = 0;
    virtual std::vector<int> GetUsedRegisters() = 0;
    virtual Operand GetResultReg() = 0;
    virtual void resetOperand(Operand oldO,Operand newO){return;}
    virtual void SetNonResultOperands(std::vector<Operand> ops) = 0;
    virtual int IsFuncDef() { return 0; }
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map) = 0;
};

// load
// Syntax: <result>=load <ty>, ptr <pointer>
class LoadInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand pointer;
    Operand result;

public:
    enum LLVMType GetDataType() { return type; }
    Operand GetPointer() { return pointer; }
    void SetPointer(Operand op) { pointer = op; }
    Operand GetResult() { return result; }

    LoadInstruction(enum LLVMType type, Operand pointer, Operand result) {
        opcode = LLVMIROpcode::LOAD;
        this->type = type;
        this->result = result;
        this->pointer = pointer;
    }
    void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return result; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(pointer==oldO){
            pointer=newO;
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map);
};

// store
// Syntax: store <ty> <value>, ptr<pointer>
class StoreInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand pointer;
    Operand value;

public:
    enum LLVMType GetDataType() { return type; }
    Operand GetPointer() { return pointer; }
    Operand GetValue() { return value; }
    void SetValue(Operand op) { value = op; }
    void SetPointer(Operand op) { pointer = op; }

    StoreInstruction(enum LLVMType type, Operand pointer, Operand value) {
        opcode = LLVMIROpcode::STORE;
        this->type = type;
        this->pointer = pointer;
        this->value = value;
    }

    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return nullptr; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(pointer==oldO){
            pointer=newO;
        }
        if(value==oldO){
            value=newO;
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map);
};

//<result>=add <ty> <op1>,<op2>
//<result>=sub <ty> <op1>,<op2>
//<result>=mul <ty> <op1>,<op2>
//<result>=div <ty> <op1>,<op2>
//<result>=xor <ty> <op1>,<op2>
class ArithmeticInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand op1;
    Operand op2;
    Operand op3;
    Operand result;

public:
    enum LLVMType GetDataType() { return type; }
    Operand GetOperand1() { return op1; }
    Operand GetOperand2() { return op2; }
    Operand GetOperand3() { return op3; }
    Operand GetResult() { return result; }
    void SetOperand1(Operand op) { op1 = op; }
    void SetOperand2(Operand op) { op2 = op; }
    void SetResultReg(Operand op) { result = op; }
    void Setopcode(LLVMIROpcode id) { opcode = id; }
    ArithmeticInstruction(LLVMIROpcode opcode, enum LLVMType type, Operand op1, Operand op2, Operand result) {
        this->opcode = opcode;
        this->op1 = op1;
        this->op2 = op2;
        this->op3 = nullptr;
        this->result = result;
        this->type = type;
    }

    ArithmeticInstruction(LLVMIROpcode opcode, enum LLVMType type, Operand op1, Operand op2, Operand op3,
                          Operand result) {
        this->opcode = opcode;
        this->op1 = op1;
        this->op2 = op2;
        this->op3 = op3;
        this->result = result;
        this->type = type;
    }

    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return result; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(op1==oldO){
            op1=newO;
        }
        if(op2==oldO){
            op2=newO;
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map);
};

//<result>=icmp <cond> <ty> <op1>,<op2>
class IcmpInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand op1;
    Operand op2;
    IcmpCond cond;
    Operand result;

public:
    enum LLVMType GetDataType() { return type; }
    Operand GetOp1() { return op1; }
    Operand GetOp2() { return op2; }
    IcmpCond GetCond() { return cond; }
    Operand GetResult() { return result; }
    void SetOp1(Operand op) { op1 = op; }
    void SetOp2(Operand op) { op2 = op; }
    void SetCond(IcmpCond newcond) { cond = newcond; }

    IcmpInstruction(enum LLVMType type, Operand op1, Operand op2, IcmpCond cond, Operand result) {
        this->opcode = LLVMIROpcode::ICMP;
        this->type = type;
        this->op1 = op1;
        this->op2 = op2;
        this->cond = cond;
        this->result = result;
    }
    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return result; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(op1==oldO){
            op1=newO;
        }
        if(op2==oldO){
            op2=newO;
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map);
};

//<result>=fcmp <cond> <ty> <op1>,<op2>
class FcmpInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand op1;
    Operand op2;
    FcmpCond cond;
    Operand result;

public:
    enum LLVMType GetDataType() { return type; }
    Operand GetOp1() { return op1; }
    Operand GetOp2() { return op2; }
    FcmpCond GetCond() { return cond; }
    Operand GetResult() { return result; }

    FcmpInstruction(enum LLVMType type, Operand op1, Operand op2, FcmpCond cond, Operand result) {
        this->opcode = LLVMIROpcode::FCMP;
        this->type = type;
        this->op1 = op1;
        this->op2 = op2;
        this->cond = cond;
        this->result = result;
    }
    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return result; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(op1==oldO){
            op1=newO;
        }
        if(op2==oldO){
            op2=newO;
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map);
};

// phi syntax:
//<result>=phi <ty> [val1,label1],[val2,label2],……
class PhiInstruction : public BasicInstruction {
private:
    enum LLVMType type;
    Operand result;
    std::vector<std::pair<Operand, Operand>> phi_list;

public:
    const std::vector<std::pair<Operand, Operand>>& GetPhiList() const { return phi_list; }
    Operand GetResultReg() { return result; }
    PhiInstruction(enum LLVMType type, Operand result, decltype(phi_list) val_labels) {
        this->opcode = LLVMIROpcode::PHI;
        this->type = type;
        this->result = result;
        this->phi_list = val_labels;
    }
    PhiInstruction(enum LLVMType type, Operand result) {
        this->opcode = LLVMIROpcode::PHI;
        this->type = type;
        this->result = result;
    }
    void InsertPhi(Operand val, Operand label) { phi_list.emplace_back(label, val); }
    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule);
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    void resetOperand(Operand oldO,Operand newO)  {
        for (auto &pair : phi_list) {
            if (pair.first==oldO) pair.first=newO;  // value
            if (pair.second==oldO) pair.second=newO; // label
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map) { return 0; }
    void ErasePhi(int label_id)
    {
        for (auto it = phi_list.begin(); it != phi_list.end(); ++it) {
            auto [label, val] = *it;
            if (((LabelOperand *)label)->GetLabelNo() == label_id) {
                phi_list.erase(it);
                break;
            }
        }
    }
};

// alloca
// usage 1: <result>=alloca <type>
// usage 2: %3 = alloca [20 x [20 x i32]]
class AllocaInstruction : public BasicInstruction {
    enum LLVMType type;
    Operand result;
    std::vector<int> dims;

public:
    enum LLVMType GetDataType() { return type; }
    Operand GetResult() { return result; }
    std::vector<int> GetDims() { return dims; }
    AllocaInstruction(enum LLVMType dttype, Operand result) {
        this->opcode = LLVMIROpcode::ALLOCA;
        this->type = dttype;
        this->result = result;
    }
    AllocaInstruction(enum LLVMType dttype, std::vector<int> ArrDims, Operand result) {
        this->opcode = LLVMIROpcode::ALLOCA;
        this->type = dttype;
        this->result = result;
        dims = ArrDims;
    }

    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands() { return std::vector<Operand>{}; }
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return result; }
    void SetNonResultOperands(std::vector<Operand> ops) {}
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map) { return 0; }
    int GetAllocaSize() {
    int sz = 1;
    for (auto d : dims) {
        sz *= d;
    }
    return sz;
}
};

// Conditional branch
// Syntax:
// br i1 <cond>, label <iftrue>, label <iffalse>
class BrCondInstruction : public BasicInstruction {
    Operand cond;
    Operand trueLabel;
    Operand falseLabel;

public:
    Operand GetCond() { return cond; }
    Operand GetTrueLabel() { return trueLabel; }
    Operand GetFalseLabel() { return falseLabel; }
    BrCondInstruction(Operand cond, Operand trueLabel, Operand falseLabel) {
        this->opcode = BR_COND;
        this->cond = cond;
        this->trueLabel = trueLabel;
        this->falseLabel = falseLabel;
    }

    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule);
    void ReplaceTrueLabel(Operand newLabel) { trueLabel = newLabel; }  // 添加的方法
    void ReplaceFalseLabel(Operand newLabel) { falseLabel = newLabel; } // 添加的方法
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return NULL; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(cond==oldO){
            cond=newO;
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map) { return 0; }
};

// Unconditional branch
// Syntax:
// br label <dest>
class BrUncondInstruction : public BasicInstruction {
    Operand destLabel;

public:
    Operand GetDestLabel() { return destLabel; }
    BrUncondInstruction(Operand destLabel) {
        this->opcode = BR_UNCOND;
        this->destLabel = destLabel;
    }
    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule);
    void ReplaceLabel(Operand newLabel) { destLabel = newLabel; }
    std::vector<Operand> GetNonResultOperands() { return std::vector<Operand>{}; }
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return nullptr; }
    void SetNonResultOperands(std::vector<Operand> ops) {}
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map) { return 0; }
    int GetTarget() { return ((LabelOperand *)destLabel)->GetLabelNo(); }
};

/*
Global Id Define Instruction Syntax
Example 1:
    @p = global [105 x i32] zeroinitializer
Example 2:
    @p = global [105 x [104 x i32]] [[104 x i32] [], [104 x i32] zeroinitializer, ...]
*/
class GlobalVarDefineInstruction : public BasicInstruction {
public:
    // 如果arrayval的dims为空, 则该定义为非数组, 初始值为init_val
    // 如果arrayval的dims为空, 且init_val为nullptr, 则初始值为0

    // 如果arrayval的dims不为空, 则该定义为数组, 初始值在arrayval中
    enum LLVMType type;
    std::string name;
    Operand init_val;
    VarAttribute arrayval;
    GlobalVarDefineInstruction(std::string nam, enum LLVMType typ, Operand i_val)
        : name(nam), type(typ), init_val(i_val) {
        this->opcode = LLVMIROpcode::GLOBAL_VAR;
    }
    GlobalVarDefineInstruction(std::string nam, enum LLVMType typ, VarAttribute v)
        : name(nam), type(typ), arrayval(v), init_val{nullptr} {
        this->opcode = LLVMIROpcode::GLOBAL_VAR;
    }
    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands() { return std::vector<Operand>{}; }
    virtual Instruction CopyInstruction() { return nullptr; }
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return NULL; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(init_val==oldO){
            init_val=newO;
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops) {}
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map) { return 0; }
};

class GlobalStringConstInstruction : public BasicInstruction {
public:
    std::string str_val;
    std::string str_name;
    GlobalStringConstInstruction(std::string strval, std::string strname) : str_val(strval), str_name(strname) {
        this->opcode = LLVMIROpcode::GLOBAL_STR;
    }

    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands() { return std::vector<Operand>(); }
    virtual Instruction CopyInstruction() { return nullptr; }
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return NULL; }
    void SetNonResultOperands(std::vector<Operand> ops) {}
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map) { return 0; }
};

/*
Call Instruction Syntax
Example 1:
    %12 = call i32 (ptr, ...)@printf(ptr @.str,i32 %11)
Example 2:
    call void @DFS(i32 0,i32 %4)

如果你调用了void类型的函数，将result设置为nullptr即可
*/
class CallInstruction : public BasicInstruction {
    // Datas About the Instruction
private:
    enum LLVMType ret_type;
    Operand result;    // result can be null
    std::string name;
    std::vector<std::pair<enum LLVMType, Operand>> args;

public:
    Operand GetResult() { return result; }
    // Construction Function:Set All datas
    CallInstruction(enum LLVMType retType, Operand res, std::string FuncName,
                    std::vector<std::pair<enum LLVMType, Operand>> arguments)
        : ret_type(retType), result(res), name(FuncName), args(arguments) {
        this->opcode = CALL;
        if (res != NULL && res->GetOperandType() == BasicOperand::REG) {
            if (((RegOperand *)res)->GetRegNo() == -1) {
                result = NULL;
            }
        }
    }
    CallInstruction(enum LLVMType retType, Operand res, std::string FuncName)
        : ret_type(retType), result(res), name(FuncName) {
        this->opcode = CALL;
        if (res != NULL && res->GetOperandType() == BasicOperand::REG) {
            if (((RegOperand *)res)->GetRegNo() == -1) {
                result = NULL;
            }
        }
    }

    enum LLVMType GetRetType() { return ret_type; }
    std::string GetFunctionName() { return name; }
    void SetFunctionName(std::string new_name) { name = new_name; }
    std::vector<std::pair<enum LLVMType, Operand>> GetParameterList() { return args; }
    void push_back_Parameter(std::pair<enum LLVMType, Operand> newPara) { args.push_back(newPara); }
    void push_back_Parameter(enum LLVMType type, Operand val) { args.push_back(std::make_pair(type, val)); }
    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return result; }
    virtual LLVMType GetResultType() { return ret_type; }
    Operand DefinesResult(){
        return result;
    }
    void resetOperand(Operand oldO,Operand newO)  {
        for (auto &[type, operand] : args) {
            if(operand==oldO){
                operand=newO;
            }
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map);
};

/*
Ret Instruction Syntax
Example 1:
    ret i32 0
Example 2:
    ret void
Example 3:
    ret i32 %r7

如果你需要不需要返回值，将ret_val设置为nullptr即可
*/
class RetInstruction : public BasicInstruction {
    // Datas About the Instruction
private:
    enum LLVMType ret_type;
    Operand ret_val;

public:
    // Construction Function:Set All datas
    RetInstruction(enum LLVMType retType, Operand res) : ret_type(retType), ret_val(res) { this->opcode = RET; }
    // Getters
    enum LLVMType GetType() { return ret_type; }
    Operand GetRetVal() { return ret_val; }

    virtual void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return nullptr; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(ret_val==oldO){
            ret_val=newO;
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map) { return 0; }
};

/*
    index_type 表示getelementptr的index类型, 这里我们为了简化实现，统一认为index要么是i32，要么是i64
    如果你有特殊需要，可以自行修改指令类
*/
class GetElementptrInstruction : public BasicInstruction {
private:
    enum LLVMType index_type;
    Operand result;
    Operand ptrval;

    enum LLVMType type;
    std::vector<int> dims; // example: i32, [4 x i32], [3 x [4 x float]]

    std::vector<Operand> indexes;

public:
    GetElementptrInstruction(enum LLVMType typ, Operand res, Operand ptr, LLVMType idx_typ)
        : type(typ), index_type(idx_typ), result(res), ptrval(ptr) {
        opcode = GETELEMENTPTR;
    }
    GetElementptrInstruction(enum LLVMType typ, Operand res, Operand ptr, std::vector<int> dim, LLVMType idx_typ)
        : type(typ), index_type(idx_typ), result(res), ptrval(ptr), dims(dim) {
        opcode = GETELEMENTPTR;
    }
    GetElementptrInstruction(enum LLVMType typ, Operand res, Operand ptr, std::vector<int> dim,
                             std::vector<Operand> index, LLVMType idx_typ)
        : type(typ), index_type(idx_typ), result(res), ptrval(ptr), dims(dim), indexes(index) {
        opcode = GETELEMENTPTR;
    }
    void push_dim(int d) { dims.push_back(d); }
    void push_idx_reg(int idx_reg_no) { indexes.push_back(GetNewRegOperand(idx_reg_no)); }
    void push_idx_imm32(int imm_idx) { indexes.push_back(new ImmI32Operand(imm_idx)); }
    void push_index(Operand idx) { indexes.push_back(idx); }
    void change_index(int i, Operand op) { indexes[i] = op; }

    enum LLVMType GetType() { return type; }
    Operand GetResult() { return result; }
    void SetResult(Operand op) { result = op; }
    Operand GetPtrVal() { return ptrval; }
    std::vector<int> GetDims() { return dims; }
    std::vector<Operand> GetIndexes() { return indexes; }

    void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return result; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(ptrval==oldO){
            ptrval=newO;
        }
        for (auto &idx : indexes) {
            if(idx==oldO){
                idx=newO;
            }
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map);
};

class FunctionDefineInstruction : public BasicInstruction {
private:
    enum LLVMType return_type;
    std::string Func_name;

public:
    std::vector<enum LLVMType> formals;
    std::vector<Operand> formals_reg;
    bool isTCO = false;
    std::vector<std::vector<std::pair<Operand, Operand>>> phi_list_list;
    std::vector<Operand> phi_result_list;
    FunctionDefineInstruction(enum LLVMType t, std::string n) {
        return_type = t;
        Func_name = n;
    }
    void InsertFormal(enum LLVMType t) {
        formals.push_back(t);
        formals_reg.push_back(GetNewRegOperand(formals_reg.size()));
    }
    enum LLVMType GetReturnType() { return return_type; }
    std::string GetFunctionName() { return Func_name; }

    void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction() { return nullptr; }
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return NULL; }
    int GetFormalSize() { return formals.size(); }
    void SetNonResultOperands(std::vector<Operand> ops) {}
    virtual int IsFuncDef() { return 1; }
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map) { return 0; }
};
typedef FunctionDefineInstruction *FuncDefInstruction;

class FunctionDeclareInstruction : public BasicInstruction {
private:
    enum LLVMType return_type;
    std::string Func_name;

public:
    std::vector<enum LLVMType> formals;
    bool isTCO = false;
    std::vector<std::vector<std::pair<Operand, Operand>>> phi_list_list;
    std::vector<Operand> phi_result_list;
    FunctionDeclareInstruction(enum LLVMType t, std::string n) {
        return_type = t;
        Func_name = n;
    }
    void InsertFormal(enum LLVMType t) { formals.push_back(t); }
    enum LLVMType GetReturnType() { return return_type; }
    std::string GetFunctionName() { return Func_name; }

    void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction() { return nullptr; }
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return NULL; }
    void SetNonResultOperands(std::vector<Operand> ops) {}
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map) { return 0; }
};

// 这条指令目前只支持float和i32的转换，如果你需要double, i64等类型，需要自己添加更多变量
class FptosiInstruction : public BasicInstruction {
private:
    Operand result;
    Operand value;

public:
    FptosiInstruction(Operand result_receiver, Operand value_for_cast)
        : result(result_receiver), value(value_for_cast) {
        this->opcode = FPTOSI;
    }
    Operand GetResult() { return result; }
    Operand GetSrc() { return value; }
    void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return result; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(value==oldO){
            value=newO;
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map);
};

// 这条指令目前只支持float和i32的转换，如果你需要double, i64等类型，需要自己添加更多变量
class SitofpInstruction : public BasicInstruction {
private:
    Operand result;
    Operand value;

public:
    SitofpInstruction(Operand result_receiver, Operand value_for_cast)
        : result(result_receiver), value(value_for_cast) {
        this->opcode = SITOFP;
    }

    Operand GetResult() { return result; }
    Operand GetSrc() { return value; }
    void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return result; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(value==oldO){
            value=newO;
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map);
};

// 无符号扩展，你大概率需要它来将i1无符号扩展至i32(即对应c语言bool类型转int)
class ZextInstruction : public BasicInstruction {
private:
    LLVMType from_type;
    LLVMType to_type;
    Operand result;
    Operand value;

public:
    Operand GetResult() { return result; }
    Operand GetSrc() { return value; }
    ZextInstruction(LLVMType to_type, Operand result_receiver, LLVMType from_type, Operand value_for_cast)
        : to_type(to_type), result(result_receiver), from_type(from_type), value(value_for_cast) {
        this->opcode = ZEXT;
    }
    void PrintIR(std::ostream &s);
    void ReplaceRegByMap(const std::map<int, int> &Rule);
    void ReplaceLabelByMap(const std::map<int, int> &Rule) {}
    std::vector<Operand> GetNonResultOperands();
    virtual Instruction CopyInstruction();
    virtual int GetResultRegNo();
    virtual std::vector<int> GetUsedRegisters();
    Operand GetResultReg() { return result; }
    void resetOperand(Operand oldO,Operand newO)  {
        if(value==oldO){
            value=newO;
        }
        return;
    }
    void SetNonResultOperands(std::vector<Operand> ops);
    virtual int ConstPropagate(std::map<int, Instruction> &regresult_map);
};

std::ostream &operator<<(std::ostream &s, BasicInstruction::LLVMType type);
std::ostream &operator<<(std::ostream &s, BasicInstruction::LLVMIROpcode type);
std::ostream &operator<<(std::ostream &s, BasicInstruction::IcmpCond type);
std::ostream &operator<<(std::ostream &s, BasicInstruction::FcmpCond type);
std::ostream &operator<<(std::ostream &s, Operand op);
#endif
