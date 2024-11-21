#include "IRgen.h"
#include "../include/ir.h"
#include "semant.h"

extern SemantTable semant_table;    // 也许你会需要一些语义分析的信息

IRgenTable irgen_table;    // 中间代码生成的辅助变量
LLVMIR llvmIR;             // 我们需要在这个变量中生成中间代码
int global_reg_counter = 0;
static FuncDefInstruction funcd;
static int funcd_label = 0;
int max_label = -1;
static int loop_start_label = -1;
static int loop_end_label = -1;
Type::ty curr_ret = Type::VOID;
std::map<int, VarAttribute> reg_Var_Table;
std::map<int, int> FormalArrayTable;
Operand strptr = nullptr;
void AddLibFunctionDeclare();
std::map<FuncDefInstruction, int> max_label_map{};
std::map<FuncDefInstruction, int> max_reg_map{};

// 在基本块B末尾生成一条新指令
void IRgenArithmeticI32(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int reg1, int reg2, int result_reg);
void IRgenArithmeticF32(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int reg1, int reg2, int result_reg);
void IRgenArithmeticI32ImmLeft(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int val1, int reg2, int result_reg);
void IRgenArithmeticF32ImmLeft(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, float val1, int reg2,
                               int result_reg);
void IRgenArithmeticI32ImmAll(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int val1, int val2, int result_reg);
void IRgenArithmeticF32ImmAll(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, float val1, float val2,
                              int result_reg);

void IRgenIcmp(LLVMBlock B, BasicInstruction::IcmpCond cmp_op, int reg1, int reg2, int result_reg);
void IRgenFcmp(LLVMBlock B, BasicInstruction::FcmpCond cmp_op, int reg1, int reg2, int result_reg);
void IRgenIcmpImmRight(LLVMBlock B, BasicInstruction::IcmpCond cmp_op, int reg1, int val2, int result_reg);
void IRgenFcmpImmRight(LLVMBlock B, BasicInstruction::FcmpCond cmp_op, int reg1, float val2, int result_reg);

void IRgenFptosi(LLVMBlock B, int src, int dst);
void IRgenSitofp(LLVMBlock B, int src, int dst);
void IRgenZextI1toI32(LLVMBlock B, int src, int dst);

void IRgenGetElementptrIndexI32(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr,
                                std::vector<int> dims, std::vector<Operand> indexs);

void IRgenGetElementptrIndexI64(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr,
                                std::vector<int> dims, std::vector<Operand> indexs);

void IRgenLoad(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr);
void IRgenStore(LLVMBlock B, BasicInstruction::LLVMType type, int value_reg, Operand ptr);
void IRgenStore(LLVMBlock B, BasicInstruction::LLVMType type, Operand value, Operand ptr);

void IRgenCall(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg,
               std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args, std::string name);
void IRgenCallVoid(LLVMBlock B, BasicInstruction::LLVMType type,
                   std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args, std::string name);

void IRgenCallNoArgs(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, std::string name);
void IRgenCallVoidNoArgs(LLVMBlock B, BasicInstruction::LLVMType type, std::string name);

void IRgenRetReg(LLVMBlock B, BasicInstruction::LLVMType type, int reg);
void IRgenRetImmInt(LLVMBlock B, BasicInstruction::LLVMType type, int val);
void IRgenRetImmFloat(LLVMBlock B, BasicInstruction::LLVMType type, float val);
void IRgenRetVoid(LLVMBlock B);

void IRgenBRUnCond(LLVMBlock B, int dst_label);
void IRgenBrCond(LLVMBlock B, int cond_reg, int true_label, int false_label);

void IRgenAlloca(LLVMBlock B, BasicInstruction::LLVMType type, int reg);
void IRgenAllocaArray(LLVMBlock B, BasicInstruction::LLVMType type, int reg, std::vector<int> dims);
bool IsBr(Instruction ins) {
    int opcode = ins->GetOpcode();
    return opcode == BasicInstruction::LLVMIROpcode::BR_COND || opcode == BasicInstruction::LLVMIROpcode::BR_UNCOND;
}

bool IsRet(Instruction ins) {
    int opcode = ins->GetOpcode();
    return opcode == BasicInstruction::LLVMIROpcode::RET;
}

void AddNoReturnBlock() {
    for (auto block : llvmIR.function_block_map[funcd]) {
        LLVMBlock B = block.second;
        if (B->Instruction_list.empty() || (!IsRet(B->Instruction_list.back()) && !IsBr(B->Instruction_list.back()))) {
            if (curr_ret == Type::VOID) {
                IRgenRetVoid(B);
            } else if (curr_ret == Type::INT) {
                IRgenRetImmInt(B, BasicInstruction::LLVMType::I32, 0);
            } else if (curr_ret == Type::FLOAT) {
                IRgenRetImmFloat(B, BasicInstruction::LLVMType::FLOAT32, 0);
            }
        }
    }
}
RegOperand *GetNewRegOperand(int RegNo);

// generate TypeConverse Instructions from type_src to type_dst
// eg. you can use fptosi instruction to converse float to int
// eg. you can use zext instruction to converse bool to int
void IRgenTypeConverse(LLVMBlock B, Type::ty type_src, Type::ty type_dst, int src, int result) {
    if (type_src == type_dst) {
        global_reg_counter--;
        return;
    }

    if (type_src == Type::FLOAT && type_dst == Type::INT) {
        IRgenFptosi(B, src, result);
    } else if (type_src == Type::INT && type_dst == Type::FLOAT) {
        IRgenSitofp(B, src, result);
    } else if (type_src == Type::BOOL && type_dst == Type::INT) {
        IRgenZextI1toI32(B, src, result);
    } else if (type_src == Type::INT && type_dst == Type::BOOL) {
        IcmpInstruction *convInst =
        new IcmpInstruction(BasicInstruction::I32, GetNewRegOperand(src), new ImmI32Operand(0), BasicInstruction::ne,
                            GetNewRegOperand(result));
        B->InsertInstruction(1, convInst);
    } else if (type_src == Type::BOOL && type_dst == Type::FLOAT) {
        IRgenZextI1toI32(B, src, result);
        src = result;
        IRgenSitofp(B, src, global_reg_counter++);
    } else if (type_src == Type::FLOAT && type_dst == Type::BOOL) {
        FcmpInstruction *convInst =
        new FcmpInstruction(BasicInstruction::FLOAT32, GetNewRegOperand(src), new ImmF32Operand(0),
                            BasicInstruction::ONE, GetNewRegOperand(result));
        B->InsertInstruction(1, convInst);
    } else {
        ERROR("Unsupported type conversion from %d to %d", type_src, type_dst);
    }
}

void BasicBlock::InsertInstruction(int pos, Instruction Ins) {
    assert(pos == 0 || pos == 1);
    if (pos == 0) {
        Instruction_list.push_front(Ins);
    } else if (pos == 1) {
        Instruction_list.push_back(Ins);
    }
}

/*
二元运算指令生成的伪代码：
    假设现在的语法树节点是：AddExp_plus
    该语法树表示 addexp + mulexp

    addexp->codeIR()
    mulexp->codeIR()
    假设mulexp生成完后，我们应该在基本块B0继续插入指令。
    addexp的结果存储在r0寄存器中，mulexp的结果存储在r1寄存器中
    生成一条指令r2 = r0 + r1，并将该指令插入基本块B0末尾。
    标注后续应该在基本块B0插入指令，当前节点的结果寄存器为r2。
    (如果考虑支持浮点数，需要查看语法树节点的类型来判断此时是否需要隐式类型转换)
*/

/*
while语句指令生成的伪代码：
    while的语法树节点为while(cond)stmt

    假设当前我们应该在B0基本块开始插入指令
    新建三个基本块Bcond，Bbody，Bend
    在B0基本块末尾插入一条无条件跳转指令，跳转到Bcond

    设置当前我们应该在Bcond开始插入指令
    cond->codeIR()    //在调用该函数前你可能需要设置真假值出口
    假设cond生成完后，我们应该在B1基本块继续插入指令，Bcond的结果为r0
    如果r0的类型不为bool，在B1末尾生成一条比较语句，比较r0是否为真。
    在B1末尾生成一条条件跳转语句，如果为真，跳转到Bbody，如果为假，跳转到Bend

    设置当前我们应该在Bbody开始插入指令
    stmt->codeIR()
    假设当stmt生成完后，我们应该在B2基本块继续插入指令
    在B2末尾生成一条无条件跳转语句，跳转到Bcond

    设置当前我们应该在Bend开始插入指令
*/
BasicInstruction::LLVMType typeTrans(Type::ty type) {
    switch (type) {
    case Type::VOID:
        return BasicInstruction::VOID;
    case Type::INT:
        return BasicInstruction::I32;
    case Type::FLOAT:
        return BasicInstruction::FLOAT32;
    case Type::BOOL:
        return BasicInstruction::I1;
    case Type::PTR:
        return BasicInstruction::PTR;
    case Type::DOUBLE:
        return BasicInstruction::DOUBLE;
    }
}

void GenerateBinaryOperation(tree_node *left, tree_node *right, tree_node::opcode op, LLVMBlock B) {
    left->codeIR();
    int reg1 = global_reg_counter - 1;

    right->codeIR();
    int reg2 = global_reg_counter - 1;

    Type::ty type1 = left->attribute.T.type;
    Type::ty type2 = right->attribute.T.type;

    if (type1 == Type::INT && type2 == Type::INT) {
        // Both operands are integers
        switch (op) {
        case tree_node::ADD:
            IRgenArithmeticI32(B, BasicInstruction::ADD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::SUB:
            IRgenArithmeticI32(B, BasicInstruction::SUB, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MUL:
            IRgenArithmeticI32(B, BasicInstruction::MUL, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::DIV:
            IRgenArithmeticI32(B, BasicInstruction::DIV, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MOD:
            IRgenArithmeticI32(B, BasicInstruction::MOD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LEQ:
            IRgenIcmp(B, BasicInstruction::sle, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LT:
            IRgenIcmp(B, BasicInstruction::slt, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GEQ:
            IRgenIcmp(B, BasicInstruction::sge, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GT:
            IRgenIcmp(B, BasicInstruction::sgt, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::EQ:
            IRgenIcmp(B, BasicInstruction::eq, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::NE:
            IRgenIcmp(B, BasicInstruction::ne, reg1, reg2, global_reg_counter++);
            break;
        default:
            assert(false && "Unsupported operation for INT type");
        }
    } else if(type1 == Type::INT && type2 == Type::FLOAT){
        IRgenSitofp(B, reg1, global_reg_counter++);
        reg1 = global_reg_counter-1;

        switch (op) {
        case tree_node::ADD:
            IRgenArithmeticF32(B, BasicInstruction::FADD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::SUB:
            IRgenArithmeticF32(B, BasicInstruction::FSUB, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MUL:
            IRgenArithmeticF32(B, BasicInstruction::FMUL, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::DIV:
            IRgenArithmeticF32(B, BasicInstruction::FDIV, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LEQ:
            IRgenFcmp(B, BasicInstruction::OLE, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LT:
            IRgenFcmp(B, BasicInstruction::OLT, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GEQ:
            IRgenFcmp(B, BasicInstruction::OGE, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GT:
            IRgenFcmp(B, BasicInstruction::OGT, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::EQ:
            IRgenFcmp(B, BasicInstruction::OEQ, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::NE:
            IRgenFcmp(B, BasicInstruction::ONE, reg1, reg2, global_reg_counter++);
            break;
        default:
            assert(false && "Unsupported operation for INT/FLOAT combination");
        }
    } else if(type1 == Type::FLOAT && type2 == Type::INT){
        IRgenSitofp(B, reg2, global_reg_counter++);
        reg2 = global_reg_counter-1;

        switch (op) {
        case tree_node::ADD:
            IRgenArithmeticF32(B, BasicInstruction::FADD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::SUB:
            IRgenArithmeticF32(B, BasicInstruction::FSUB, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MUL:
            IRgenArithmeticF32(B, BasicInstruction::FMUL, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::DIV:
            IRgenArithmeticF32(B, BasicInstruction::FDIV, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LEQ:
            IRgenFcmp(B, BasicInstruction::OLE, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LT:
            IRgenFcmp(B, BasicInstruction::OLT, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GEQ:
            IRgenFcmp(B, BasicInstruction::OGE, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GT:
            IRgenFcmp(B, BasicInstruction::OGT, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::EQ:
            IRgenFcmp(B, BasicInstruction::OEQ, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::NE:
            IRgenFcmp(B, BasicInstruction::ONE, reg1, reg2, global_reg_counter++);
            break;
        default:
            assert(false && "Unsupported operation for FLOAT/INT combination");
        }
    } else if (type1 == Type::FLOAT && type2 == Type::FLOAT) {
        // Both operands are floats
        switch (op) {
        case tree_node::ADD:
            IRgenArithmeticF32(B, BasicInstruction::FADD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::SUB:
            IRgenArithmeticF32(B, BasicInstruction::FSUB, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MUL:
            IRgenArithmeticF32(B, BasicInstruction::FMUL, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::DIV:
            IRgenArithmeticF32(B, BasicInstruction::FDIV, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LEQ:
            IRgenFcmp(B, BasicInstruction::OLE, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LT:
            IRgenFcmp(B, BasicInstruction::OLT, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GEQ:
            IRgenFcmp(B, BasicInstruction::OGE, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GT:
            IRgenFcmp(B, BasicInstruction::OGT, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::EQ:
            IRgenFcmp(B, BasicInstruction::OEQ, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::NE:
            IRgenFcmp(B, BasicInstruction::ONE, reg1, reg2, global_reg_counter++);
            break;
        default:
            assert(false && "Unsupported operation for FLOAT type");
        }
    } else if (type1 == Type::BOOL && type2 == Type::BOOL) {
        // Convert both bools to int for arithmetic operations
        IRgenZextI1toI32(B, reg1, global_reg_counter++);
        IRgenZextI1toI32(B, reg2, global_reg_counter++);
        reg1 = global_reg_counter - 2;
        reg2 = global_reg_counter - 1;

        switch (op) {
        case tree_node::ADD:
            IRgenArithmeticI32(B, BasicInstruction::ADD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::SUB:
            IRgenArithmeticI32(B, BasicInstruction::SUB, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MUL:
            IRgenArithmeticI32(B, BasicInstruction::MUL, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::DIV:
            IRgenArithmeticI32(B, BasicInstruction::DIV, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MOD:
            IRgenArithmeticI32(B, BasicInstruction::MOD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LEQ:
            IRgenIcmp(B, BasicInstruction::sle, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LT:
            IRgenIcmp(B, BasicInstruction::slt, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GEQ:
            IRgenIcmp(B, BasicInstruction::sge, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GT:
            IRgenIcmp(B, BasicInstruction::sgt, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::EQ:
            IRgenIcmp(B, BasicInstruction::eq, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::NE:
            IRgenIcmp(B, BasicInstruction::ne, reg1, reg2, global_reg_counter++);
            break;
        default:
            assert(false && "Unsupported operation for BOOL type");
        }
    } else if (type1 == Type::INT && type2 == Type::BOOL) {
        // Convert bool to int, then operate
        IRgenZextI1toI32(B, reg2, global_reg_counter++);
        reg2 = global_reg_counter - 1;

        switch (op) {
        case tree_node::ADD:
            IRgenArithmeticI32(B, BasicInstruction::ADD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::SUB:
            IRgenArithmeticI32(B, BasicInstruction::SUB, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MUL:
            IRgenArithmeticI32(B, BasicInstruction::MUL, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::DIV:
            IRgenArithmeticI32(B, BasicInstruction::DIV, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MOD:
            IRgenArithmeticI32(B, BasicInstruction::MOD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LEQ:
            IRgenIcmp(B, BasicInstruction::sle, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LT:
            IRgenIcmp(B, BasicInstruction::slt, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GEQ:
            IRgenIcmp(B, BasicInstruction::sge, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GT:
            IRgenIcmp(B, BasicInstruction::sgt, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::EQ:
            IRgenIcmp(B, BasicInstruction::eq, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::NE:
            IRgenIcmp(B, BasicInstruction::ne, reg1, reg2, global_reg_counter++);
            break;
        default:
            assert(false && "Unsupported operation for INT/BOOL combination");
        }
    } else if (type1 == Type::BOOL && type2 == Type::INT) {
        // Convert bool to int, then operate
        IRgenZextI1toI32(B, reg1, global_reg_counter++);
        reg1 = global_reg_counter - 1;

        switch (op) {
        case tree_node::ADD:
            IRgenArithmeticI32(B, BasicInstruction::ADD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::SUB:
            IRgenArithmeticI32(B, BasicInstruction::SUB, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MUL:
            IRgenArithmeticI32(B, BasicInstruction::MUL, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::DIV:
            IRgenArithmeticI32(B, BasicInstruction::DIV, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MOD:
            IRgenArithmeticI32(B, BasicInstruction::MOD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LEQ:
            IRgenIcmp(B, BasicInstruction::sle, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LT:
            IRgenIcmp(B, BasicInstruction::slt, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GEQ:
            IRgenIcmp(B, BasicInstruction::sge, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GT:
            IRgenIcmp(B, BasicInstruction::sgt, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::EQ:
            IRgenIcmp(B, BasicInstruction::eq, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::NE:
            IRgenIcmp(B, BasicInstruction::ne, reg1, reg2, global_reg_counter++);
            break;
        default:
            assert(false && "Unsupported operation for BOOL/INT combination");
        }
    } else if (type1 == Type::FLOAT && type2 == Type::BOOL) {
        // Convert bool to int, then int to float
        IRgenZextI1toI32(B, reg2, global_reg_counter++);
        int temreg=global_reg_counter - 1;
        IRgenSitofp(B, temreg, global_reg_counter++);
        reg2 = global_reg_counter - 1;

        switch (op) {
        case tree_node::ADD:
            IRgenArithmeticF32(B, BasicInstruction::FADD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::SUB:
            IRgenArithmeticF32(B, BasicInstruction::FSUB, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MUL:
            IRgenArithmeticF32(B, BasicInstruction::FMUL, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::DIV:
            IRgenArithmeticF32(B, BasicInstruction::FDIV, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LEQ:
            IRgenFcmp(B, BasicInstruction::OLE, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LT:
            IRgenFcmp(B, BasicInstruction::OLT, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GEQ:
            IRgenFcmp(B, BasicInstruction::OGE, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GT:
            IRgenFcmp(B, BasicInstruction::OGT, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::EQ:
            IRgenFcmp(B, BasicInstruction::OEQ, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::NE:
            IRgenFcmp(B, BasicInstruction::ONE, reg1, reg2, global_reg_counter++);
            break;
        default:
            assert(false && "Unsupported operation for FLOAT/BOOL combination");
        }
    } else if (type1 == Type::BOOL && type2 == Type::FLOAT) {
        // Convert bool to int, then int to float
        IRgenZextI1toI32(B, reg1, global_reg_counter++);
        int temreg=global_reg_counter - 1;
        IRgenSitofp(B, temreg, global_reg_counter++);
        reg1 = global_reg_counter - 1;

        switch (op) {
        case tree_node::ADD:
            IRgenArithmeticF32(B, BasicInstruction::FADD, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::SUB:
            IRgenArithmeticF32(B, BasicInstruction::FSUB, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::MUL:
            IRgenArithmeticF32(B, BasicInstruction::FMUL, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::DIV:
            IRgenArithmeticF32(B, BasicInstruction::FDIV, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LEQ:
            IRgenFcmp(B, BasicInstruction::OLE, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::LT:
            IRgenFcmp(B, BasicInstruction::OLT, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GEQ:
            IRgenFcmp(B, BasicInstruction::OGE, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::GT:
            IRgenFcmp(B, BasicInstruction::OGT, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::EQ:
            IRgenFcmp(B, BasicInstruction::OEQ, reg1, reg2, global_reg_counter++);
            break;
        case tree_node::NE:
            IRgenFcmp(B, BasicInstruction::ONE, reg1, reg2, global_reg_counter++);
            break;
        default:
            assert(false && "Unsupported operation for BOOL/FLOAT combination");
        }
    } else {
        assert(false && "Unsupported type combination for binary operation");
    }
}

void __Program::codeIR() {
    AddLibFunctionDeclare();
    auto comp_vector = *comp_list;
    for (auto comp : comp_vector) {
        comp->codeIR();
    }
}

void Exp::codeIR() { addexp->codeIR(); }

void AddExp_plus::codeIR() {

    /*
        Operand reg1 = new RegOperand(addexp->attribute.T.type == Type::INT ? addexp->attribute.V.val.IntVal :
       addexp->result_reg); Operand reg2 = new RegOperand(mulexp->attribute.T.type == Type::INT ?
       mulexp->attribute.V.val.IntVal : mulexp->result_reg);

        Operand result_reg = new RegOperand();
        BasicInstruction::LLVMIROpcode opcode = (addexp->attribute.T.type == Type::FLOAT || mulexp->attribute.T.type ==
       Type::FLOAT) ? BasicInstruction::FADD : BasicInstruction::ADD;

        ArithmeticInstruction* addInst = new ArithmeticInstruction(opcode, BasicInstruction::I32, reg1, reg2,
       result_reg); currentBlock->InsertInstruction(addInst);

        this->result_reg = ((RegOperand*)result_reg)->GetRegNo();  */// 将结果寄存器编号保存到节点
    GenerateBinaryOperation(addexp, mulexp, ADD, llvmIR.GetBlock(funcd, funcd_label));
    this->result_reg = global_reg_counter - 1;
}

void AddExp_sub::codeIR() {
    GenerateBinaryOperation(addexp, mulexp, SUB, llvmIR.GetBlock(funcd, funcd_label));
    this->result_reg = global_reg_counter - 1;
}

void MulExp_mul::codeIR() {
    GenerateBinaryOperation(mulexp, unary_exp, MUL, llvmIR.GetBlock(funcd, funcd_label));
    this->result_reg = global_reg_counter - 1;
}

void MulExp_div::codeIR() {
    GenerateBinaryOperation(mulexp, unary_exp, DIV, llvmIR.GetBlock(funcd, funcd_label));
    this->result_reg = global_reg_counter - 1;
}

void MulExp_mod::codeIR() {
    GenerateBinaryOperation(mulexp, unary_exp, MOD, llvmIR.GetBlock(funcd, funcd_label));
    this->result_reg = global_reg_counter - 1;
}

void RelExp_leq::codeIR() {
    GenerateBinaryOperation(relexp, addexp, LEQ, llvmIR.GetBlock(funcd, funcd_label));
    this->result_reg = global_reg_counter - 1;
}

void RelExp_lt::codeIR() {
    GenerateBinaryOperation(relexp, addexp, LT, llvmIR.GetBlock(funcd, funcd_label));
    this->result_reg = global_reg_counter - 1;
}

void RelExp_geq::codeIR() {
    GenerateBinaryOperation(relexp, addexp, GEQ, llvmIR.GetBlock(funcd, funcd_label));
    this->result_reg = global_reg_counter - 1;
}

void RelExp_gt::codeIR() {
    GenerateBinaryOperation(relexp, addexp, GT, llvmIR.GetBlock(funcd, funcd_label));
    this->result_reg = global_reg_counter - 1;
}

void EqExp_eq::codeIR() {
    GenerateBinaryOperation(eqexp, relexp, EQ, llvmIR.GetBlock(funcd, funcd_label));
    this->result_reg = global_reg_counter - 1;
}

void EqExp_neq::codeIR() {
    GenerateBinaryOperation(eqexp, relexp, NE, llvmIR.GetBlock(funcd, funcd_label));
    this->result_reg = global_reg_counter - 1;
}

// short circuit &&
void LAndExp_and::codeIR() {
    llvmIR.NewBlock(funcd, ++max_label);
    int right_block_label = max_label;
    int end_block_label = false_label;

    landexp->true_label = right_block_label;
    landexp->false_label = end_block_label;
    landexp->codeIR();

    LLVMBlock B1 = llvmIR.GetBlock(funcd, funcd_label);

    int temreg1=global_reg_counter-1;
    IRgenTypeConverse(B1, landexp->attribute.T.type, Type::BOOL, temreg1, global_reg_counter++);
    IRgenBrCond(B1, global_reg_counter - 1, right_block_label, end_block_label);

    funcd_label = right_block_label;
    eqexp->true_label = true_label;
    eqexp->false_label = false_label;
    eqexp->codeIR();

    LLVMBlock B2 = llvmIR.GetBlock(funcd, funcd_label);

    int temreg2=global_reg_counter-1;
    IRgenTypeConverse(B2, eqexp->attribute.T.type, Type::BOOL, temreg2, global_reg_counter++);
}

// short circuit ||
void LOrExp_or::codeIR() {
    llvmIR.NewBlock(funcd, ++max_label);
    int right_block_label = max_label;
    int end_block_label = true_label;

    lorexp->true_label = end_block_label;
    lorexp->false_label = right_block_label;
    lorexp->codeIR();

    LLVMBlock B1 = llvmIR.GetBlock(funcd, funcd_label);

    int temreg1=global_reg_counter-1;
    IRgenTypeConverse(B1, lorexp->attribute.T.type, Type::BOOL, temreg1, global_reg_counter++);
    IRgenBrCond(B1, global_reg_counter - 1, end_block_label, right_block_label);

    funcd_label = right_block_label;
    landexp->true_label = true_label;
    landexp->false_label = false_label;
    landexp->codeIR();

    LLVMBlock B2 = llvmIR.GetBlock(funcd, funcd_label);

    int temreg2=global_reg_counter-1;
    IRgenTypeConverse(B2, landexp->attribute.T.type, Type::BOOL, temreg2, global_reg_counter++);
}

void ConstExp::codeIR() { addexp->codeIR(); }

void Lval::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);
    std::vector<Operand> arrayindexs;
    if (dims != nullptr) {
        for (auto d : *dims) {
            d->codeIR();
            int temreg = global_reg_counter - 1;
            IRgenTypeConverse(B, d->attribute.T.type, Type::INT, temreg, global_reg_counter++);
            arrayindexs.push_back(GetNewRegOperand(global_reg_counter - 1));
        }
    }
    Operand ptr_operand;
    VarAttribute lval_attribute;
    bool formal_array_tag = false;
    int alloca_reg = irgen_table.symbol_table.lookup(name);
    if (alloca_reg != -1) {    // local, use var's alloca_reg
        ptr_operand = GetNewRegOperand(alloca_reg);
        lval_attribute = reg_Var_Table[alloca_reg];
        formal_array_tag = FormalArrayTable[alloca_reg];
    } else {    // global, use var's name
        ptr_operand = GetNewGlobalOperand(name->get_string());
        lval_attribute = semant_table.GlobalTable[name];
    }

    auto lltype = typeTrans(lval_attribute.type);
    if (arrayindexs.empty() == false ||
        attribute.T.type == Type::PTR) {    // lval is array, first use getelementptr to get address
        if (formal_array_tag) {             // formal array ptr, getelementptr first does not use 0
            IRgenGetElementptrIndexI32(B, lltype, global_reg_counter++, ptr_operand, lval_attribute.dims, arrayindexs);
        } else {    // array ptr, getelementptr first use 0
            arrayindexs.insert(arrayindexs.begin(), new ImmI32Operand(0));
            IRgenGetElementptrIndexI32(B, lltype, global_reg_counter++, ptr_operand, lval_attribute.dims, arrayindexs);
        }
        ptr_operand = GetNewRegOperand(global_reg_counter - 1);    // final address of ptr
    }

    // store the ptr_operand in ptr, if this lval is left value, we can use this to get the address
    // you can see it in assign_stmt::codeIR()
    ptr = ptr_operand;
    if (is_left == false) {                     // lval is right value, use load
        if (attribute.T.type != Type::PTR) {    // not ptr, we need to use load to get the value of the array
            IRgenLoad(B, lltype, global_reg_counter++, ptr_operand);
        }
    }
}

void FuncRParams::codeIR() {}

void Func_call::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);

    if (name->get_string() == "putf") {
        auto params = ((FuncRParams *)funcr_params)->params;
        std::vector<std::pair<BasicInstruction::LLVMType, Operand>> args;

        auto str_param = (*params)[0];
        str_param->codeIR();
        args.push_back({BasicInstruction::PTR, strptr});

        for (int i = 1; i < (*params).size(); ++i) {
            auto param = (*params)[i];
            param->codeIR();

            auto real_type = param->attribute.T.type;
            if (param->attribute.T.type == Type::FLOAT) {
                real_type = Type::DOUBLE;
                int temreg=global_reg_counter - 1;
                IRgenTypeConverse(B, param->attribute.T.type, Type::DOUBLE, temreg,
                                  global_reg_counter++);
            }
            args.push_back({typeTrans(real_type), GetNewRegOperand(global_reg_counter - 1)});
        }
        IRgenCallVoid(B, BasicInstruction::VOID, args, name->get_string());
        return;
    }

    Type::ty return_type = semant_table.FunctionTable[name]->return_type;
    BasicInstruction::LLVMType ret_type = typeTrans(return_type);

    if (funcr_params != nullptr) {
        std::vector<std::pair<BasicInstruction::LLVMType, Operand>> args;
        auto params = ((FuncRParams *)funcr_params)->params;
        auto fparams = semant_table.FunctionTable[name]->formals;
        assert(params->size() == fparams->size());
        for (int i = 0; i < (*params).size(); ++i) {
            auto param = (*params)[i];
            auto fparam = (*fparams)[i];
            param->codeIR();
            int temreg=global_reg_counter - 1;
            IRgenTypeConverse(B, param->attribute.T.type, fparam->attribute.T.type, temreg,
                              global_reg_counter++);
            args.push_back({typeTrans(fparam->attribute.T.type), GetNewRegOperand(global_reg_counter - 1)});
        }
        if (return_type == Type::VOID) {
            IRgenCallVoid(B, ret_type, args, name->get_string());
        } else {
            IRgenCall(B, ret_type, global_reg_counter++, args, name->get_string());
        }
    } else {
        if (return_type == Type::VOID) {
            IRgenCallVoidNoArgs(B, ret_type, name->get_string());
        } else {
            IRgenCallNoArgs(B, ret_type, global_reg_counter++, name->get_string());
        }
    }
    // TODO("FunctionCall CodeIR");
}

void UnaryExp_plus::codeIR() {
    unary_exp->codeIR();
    if (unary_exp->attribute.T.type == Type::BOOL) {
        int reg = global_reg_counter - 1;
        LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);
        IRgenZextI1toI32(B, reg, global_reg_counter++);
    }
}

void UnaryExp_neg::codeIR() {
    unary_exp->codeIR();
    int reg = global_reg_counter - 1;
    LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);
    if (unary_exp->attribute.T.type == Type::INT) {
        IRgenArithmeticI32ImmLeft(B, BasicInstruction::SUB, 0, reg, global_reg_counter++);
    } else if (unary_exp->attribute.T.type == Type::FLOAT) {
        IRgenArithmeticF32ImmLeft(B, BasicInstruction::FSUB, 0, reg, global_reg_counter++);
    } else if (unary_exp->attribute.T.type == Type::BOOL) {
        IRgenZextI1toI32(B, reg, global_reg_counter++);
        int temreg=global_reg_counter - 1;
        IRgenArithmeticI32ImmLeft(B, BasicInstruction::SUB, 0, temreg, global_reg_counter++);
    }
}

void UnaryExp_not::codeIR() {
    unary_exp->codeIR();
    int reg = global_reg_counter - 1;
    LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);
    if (unary_exp->attribute.T.type == Type::INT) {
        IRgenIcmpImmRight(B, BasicInstruction::eq, reg, 0, global_reg_counter++);
    } else if (unary_exp->attribute.T.type == Type::FLOAT) {
        IRgenFcmpImmRight(B, BasicInstruction::OEQ, reg, 0, global_reg_counter);
    } else if (unary_exp->attribute.T.type == Type::BOOL) {
        IRgenZextI1toI32(B, reg, global_reg_counter++);
        int temreg=global_reg_counter - 1;
        IRgenIcmpImmRight(B, BasicInstruction::eq, temreg, 0, global_reg_counter++);
    }
}

void IntConst::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);
    IRgenArithmeticI32ImmAll(B, BasicInstruction::ADD, val, 0, global_reg_counter++);
}

void FloatConst::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);
    IRgenArithmeticF32ImmAll(B, BasicInstruction::FADD, val, 0, global_reg_counter++);
}

void StringConst::codeIR() {}

void PrimaryExp_branch::codeIR() { exp->codeIR(); }

void assign_stmt::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);
    lval->codeIR();
    exp->codeIR();
    int reg = global_reg_counter - 1;
    IRgenTypeConverse(B, exp->attribute.T.type, lval->attribute.T.type, reg, global_reg_counter++);
    IRgenStore(B, typeTrans(lval->attribute.T.type), GetNewRegOperand(global_reg_counter - 1), ((Lval *)lval)->ptr);
}

void expr_stmt::codeIR() { exp->codeIR(); }

void block_stmt::codeIR() {
    irgen_table.symbol_table.enter_scope();
    b->codeIR();
    irgen_table.symbol_table.exit_scope();
}

void ifelse_stmt::codeIR() {
    int if_block_label = llvmIR.NewBlock(funcd, ++max_label)->block_id;
    int else_block_label = llvmIR.NewBlock(funcd, ++max_label)->block_id;
    int end_block_label = llvmIR.NewBlock(funcd, ++max_label)->block_id;

    Cond->true_label = if_block_label;
    Cond->false_label = else_block_label;
    Cond->codeIR();
    LLVMBlock B1 = llvmIR.GetBlock(funcd, funcd_label);
    int temreg=global_reg_counter - 1;
    IRgenTypeConverse(B1, Cond->attribute.T.type, Type::BOOL, temreg, global_reg_counter++);
    IRgenBrCond(B1, global_reg_counter - 1, if_block_label, else_block_label);

    funcd_label=if_block_label;
    ifstmt->codeIR();
    LLVMBlock B2 = llvmIR.GetBlock(funcd, funcd_label);
    IRgenBRUnCond(B2, end_block_label);

    funcd_label=else_block_label;
    elsestmt->codeIR();
    LLVMBlock B3 = llvmIR.GetBlock(funcd, funcd_label);
    IRgenBRUnCond(B3, end_block_label);

    funcd_label = end_block_label;
}

void if_stmt::codeIR() {
    int if_block_label = llvmIR.NewBlock(funcd, ++max_label)->block_id;
    int end_block_label = llvmIR.NewBlock(funcd, ++max_label)->block_id;

    Cond->true_label = if_block_label;
    Cond->false_label = end_block_label;
    Cond->codeIR();
    LLVMBlock B1 = llvmIR.GetBlock(funcd, funcd_label);
    int temreg=global_reg_counter - 1;
    IRgenTypeConverse(B1, Cond->attribute.T.type, Type::BOOL, temreg, global_reg_counter++);
    IRgenBrCond(B1, global_reg_counter - 1, if_block_label, end_block_label);

    funcd_label=if_block_label;
    ifstmt->codeIR();
    LLVMBlock B2 = llvmIR.GetBlock(funcd, funcd_label);
    IRgenBRUnCond(B2, end_block_label);

    funcd_label = end_block_label;
}

// Reference: https://github.com/yuhuifishash/SysY/blob/master/ir_gen/IRgen.cc line416-line446
void while_stmt::codeIR() {
    int cond_block_label = llvmIR.NewBlock(funcd, ++max_label)->block_id;
    int body_block_label = llvmIR.NewBlock(funcd, ++max_label)->block_id;
    int end_block_label = llvmIR.NewBlock(funcd, ++max_label)->block_id;

    int t1 = loop_start_label;
    int t2 = loop_end_label;
    loop_start_label = cond_block_label;
    loop_end_label = end_block_label;

    LLVMBlock B1 = llvmIR.GetBlock(funcd, funcd_label);
    IRgenBRUnCond(B1, cond_block_label);

    funcd_label=cond_block_label;
    Cond->true_label = body_block_label;
    Cond->false_label = end_block_label;
    Cond->codeIR();
    LLVMBlock B2 = llvmIR.GetBlock(funcd, funcd_label);
    int temreg=global_reg_counter - 1;
    IRgenTypeConverse(B2, Cond->attribute.T.type, Type::BOOL, temreg, global_reg_counter++);
    IRgenBrCond(B2, global_reg_counter - 1, body_block_label, end_block_label);

    funcd_label=body_block_label;
    body->codeIR();
    LLVMBlock B3 = llvmIR.GetBlock(funcd, funcd_label);
    IRgenBRUnCond(B3, cond_block_label);

    funcd_label = end_block_label;

    loop_start_label = t1;
    loop_end_label = t2;
}

void continue_stmt::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);
    IRgenBRUnCond(B, loop_start_label);
    funcd_label = llvmIR.NewBlock(funcd, ++max_label)->block_id;
}

void break_stmt::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);
    IRgenBRUnCond(B, loop_end_label);
    funcd_label = llvmIR.NewBlock(funcd, ++max_label)->block_id;
}

void return_stmt::codeIR() {
    return_exp->codeIR();
    LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);
    int temreg = global_reg_counter - 1;
    IRgenTypeConverse(B, return_exp->attribute.T.type, curr_ret, temreg, global_reg_counter++);
    switch (return_exp->attribute.T.type) {
    case Type::VOID:
        IRgenRetReg(B, BasicInstruction::VOID, global_reg_counter - 1);
        break;
    case Type::INT:
        IRgenRetReg(B, BasicInstruction::I32, global_reg_counter - 1);
        break;
    case Type::FLOAT:
        IRgenRetReg(B, BasicInstruction::FLOAT32, global_reg_counter - 1);
        break;
    case Type::BOOL:
        IRgenRetReg(B, BasicInstruction::I1, global_reg_counter - 1);
        break;
    case Type::PTR:
        IRgenRetReg(B, BasicInstruction::PTR, global_reg_counter - 1);
        break;
    case Type::DOUBLE:
        IRgenRetReg(B, BasicInstruction::DOUBLE, global_reg_counter - 1);
        break;
    }
}

void return_stmt_void::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(funcd, funcd_label);
    IRgenRetVoid(B);
}

void ConstInitVal::codeIR() {}

void ConstInitVal_exp::codeIR() { exp->codeIR(); }

void VarInitVal::codeIR() {}

void VarInitVal_exp::codeIR() { exp->codeIR(); }

void VarDef_no_init::codeIR() {}

void VarDef::codeIR() {}

void ConstDef::codeIR() {}

std::vector<int> GetIndexes(std::vector<int> dims, int absoluteIndex) {
    //[3][4]
    // 0-> {0,0}  {absoluteIndex/4,absoluteIndex%4}
    // 1-> {0,1}
    // 2-> {0,2}
    // 3-> {0,3}
    // 4-> {1,0}
    // 5-> {1,1}
    std::vector<int> ret;
    for (std::vector<int>::reverse_iterator it = dims.rbegin(); it != dims.rend(); ++it) {
        int dim = *it;
        ret.insert(ret.begin(), absoluteIndex % dim);
        absoluteIndex /= dim;
    }
    return ret;
}

int FindMinDimStepIR(const std::vector<int> dims, int relativePos, int dimsIdx, int &max_subBlock_sz) {
    int min_dim_step = 1;
    int blockSz = 1;
    for (int i = dimsIdx + 1; i < dims.size(); i++) {
        blockSz *= dims[i];
    }
    while (relativePos % blockSz != 0) {
        min_dim_step++;
        blockSz /= dims[dimsIdx + min_dim_step - 1];
    }
    max_subBlock_sz = blockSz;
    return min_dim_step;
}

void RecursiveArrayInitIR(LLVMBlock block, const std::vector<int> dims, int arrayaddr_reg_no, InitVal init,
                          int beginPos, int endPos, int dimsIdx, Type::ty ArrayType) {
    int pos = beginPos;
    for (InitVal iv : *(init->GetList())) {
        if (iv->IsExp()) {
            iv->codeIR();
            int init_val_reg = global_reg_counter-1;
            IRgenTypeConverse(block, iv->attribute.T.type, ArrayType, init_val_reg,global_reg_counter++);
            init_val_reg = global_reg_counter-1;

            int addr_reg = global_reg_counter++;
            auto gep = new GetElementptrInstruction(typeTrans(ArrayType), GetNewRegOperand(addr_reg),
                                                    GetNewRegOperand(arrayaddr_reg_no), dims, BasicInstruction::I32);
            // pos, dims -> [][][]...
            // gep->pushidx()
            gep->push_idx_imm32(0);
            std::vector<int> indexes = GetIndexes(dims, pos);
            for (int idx : indexes) {
                gep->push_idx_imm32(idx);
            }
            // %addr_reg = getelementptr i32, ptr %arrayaddr_reg_no, i32 0, i32 ...
            block->InsertInstruction(1, gep);
            // store i32 %init_val_reg,ptr %addr_reg
            IRgenStore(block, typeTrans(ArrayType), GetNewRegOperand(init_val_reg), GetNewRegOperand(addr_reg));
            pos++;
        } else {
            int max_subBlock_sz = 0;
            int min_dim_step = FindMinDimStepIR(dims, pos - beginPos, dimsIdx, max_subBlock_sz);
            RecursiveArrayInitIR(block, dims, arrayaddr_reg_no, iv, pos, pos + max_subBlock_sz - 1,
                                 dimsIdx + min_dim_step, ArrayType);
            pos += max_subBlock_sz;
        }
    }
}

void VarDecl::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(funcd, 0);
    LLVMBlock InitB = llvmIR.GetBlock(funcd, funcd_label);
    auto def_vector = *var_def_list;
    for (auto def : def_vector) {
        // VarDef *var_def = dynamic_cast<VarDef *>(def);
        // VarDef *var_def = (VarDef *)def;
        VarAttribute val;
        val.type = type_decl;    // init val.type
        irgen_table.symbol_table.add_Symbol(def->GetName(), global_reg_counter++);
        int alloca_reg = global_reg_counter - 1;

        if (def->GetDims() != nullptr) {    // this var is array
            auto dim_vector = *def->GetDims();
            for (auto d : dim_vector) {    // init val.dims
                val.dims.push_back(d->attribute.V.val.IntVal);
            }
            IRgenAllocaArray(B, typeTrans(type_decl), alloca_reg, val.dims);
            reg_Var_Table[alloca_reg] = val;

            InitVal init = def->GetInit();
            if (init != nullptr) {
                int array_sz = 1;
                for (auto d : val.dims) {
                    array_sz *= d;
                }

                CallInstruction *memsetCall =
                new CallInstruction(BasicInstruction::LLVMType::VOID, nullptr, std::string("llvm.memset.p0.i32"));
                memsetCall->push_back_Parameter(BasicInstruction::LLVMType::PTR,
                                                GetNewRegOperand(alloca_reg));    // array address
                memsetCall->push_back_Parameter(BasicInstruction::LLVMType::I8, new ImmI32Operand(0));
                memsetCall->push_back_Parameter(BasicInstruction::LLVMType::I32,
                                                new ImmI32Operand(array_sz * sizeof(int)));
                memsetCall->push_back_Parameter(BasicInstruction::LLVMType::I1, new ImmI32Operand(0));
                llvmIR.function_block_map[funcd][funcd_label]->InsertInstruction(1, memsetCall);
                // recursive_Array_Init_IR
                RecursiveArrayInitIR(InitB, val.dims, alloca_reg, init, 0, array_sz - 1, 0, type_decl);
            }
        } else {    // this var is not array
            IRgenAlloca(B, typeTrans(type_decl), alloca_reg);
            reg_Var_Table[alloca_reg] = val;

            Operand val_operand;

            InitVal init = def->GetInit();
            if (init != nullptr) {
                // VarInitVal_exp *initExp = dynamic_cast<VarInitVal_exp *>(init);
                Expression initexp = init->GetExp();
                initexp->codeIR();
                int temreg = global_reg_counter - 1;
                IRgenTypeConverse(InitB, initexp->attribute.T.type, type_decl, temreg, global_reg_counter++);
                val_operand = GetNewRegOperand(global_reg_counter - 1);
            } else {    // we consume that no init will be 0 by default
                if (type_decl == Type::INT) {
                    IRgenArithmeticI32ImmAll(InitB, BasicInstruction::LLVMIROpcode::ADD, 0, 0, global_reg_counter++);
                    val_operand = GetNewRegOperand(global_reg_counter - 1);
                } else if (type_decl == Type::FLOAT) {
                    IRgenArithmeticF32ImmAll(InitB, BasicInstruction::LLVMIROpcode::FADD, 0, 0, global_reg_counter++);
                    val_operand = GetNewRegOperand(global_reg_counter - 1);
                }
            }
            // store the value
            IRgenStore(InitB, typeTrans(type_decl), val_operand, GetNewRegOperand(alloca_reg));
        }
    }
}

void ConstDecl::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(funcd, 0);
    LLVMBlock InitB = llvmIR.GetBlock(funcd, funcd_label);
    auto def_vector = *var_def_list;
    for (auto def : def_vector) {

        VarAttribute val;
        val.type = type_decl;    // init val.type
        irgen_table.symbol_table.add_Symbol(def->GetName(), global_reg_counter++);
        int alloca_reg = global_reg_counter - 1;

        if (def->GetDims() != nullptr) {    // this var is array
            auto dim_vector = *def->GetDims();
            for (auto d : dim_vector) {    // init val.dims
                val.dims.push_back(d->attribute.V.val.IntVal);
            }
            IRgenAllocaArray(B, typeTrans(type_decl), alloca_reg, val.dims);
            reg_Var_Table[alloca_reg]=val;

            InitVal init = def->GetInit();
            if (init != nullptr) {
                int array_sz = 1;
                for (auto d : val.dims) {
                    array_sz *= d;
                }

                CallInstruction *memsetCall =
                new CallInstruction(BasicInstruction::LLVMType::VOID, nullptr, std::string("llvm.memset.p0.i32"));
                memsetCall->push_back_Parameter(BasicInstruction::LLVMType::PTR,
                                                GetNewRegOperand(alloca_reg));    // array address
                memsetCall->push_back_Parameter(BasicInstruction::LLVMType::I8, new ImmI32Operand(0));
                memsetCall->push_back_Parameter(BasicInstruction::LLVMType::I32,
                                                new ImmI32Operand(array_sz * sizeof(int)));
                memsetCall->push_back_Parameter(BasicInstruction::LLVMType::I1, new ImmI32Operand(0));
                llvmIR.function_block_map[funcd][funcd_label]->InsertInstruction(1, memsetCall);
                // recursive_Array_Init_IR
                RecursiveArrayInitIR(InitB, val.dims, alloca_reg, init, 0, array_sz - 1, 0, type_decl);
            }
        } else {    // this var is not array
            IRgenAlloca(B, typeTrans(type_decl), alloca_reg);
            reg_Var_Table[alloca_reg] = val;

            Operand val_operand;

            InitVal init = def->GetInit();
            assert(init != nullptr);
            Expression initExp = init->GetExp();
            initExp->codeIR();
            int temreg = global_reg_counter - 1;
            IRgenTypeConverse(InitB, initExp->attribute.T.type, type_decl, temreg, global_reg_counter++);

            val_operand = GetNewRegOperand(global_reg_counter - 1);
            // store the value
            IRgenStore(InitB, typeTrans(type_decl), val_operand, GetNewRegOperand(alloca_reg));
        }
    }
}

void BlockItem_Decl::codeIR() { decl->codeIR(); }

void BlockItem_Stmt::codeIR() { stmt->codeIR(); }

void __Block::codeIR() {
    irgen_table.symbol_table.enter_scope();

    auto item_vector = *item_list;
    for (auto item : item_vector) {
        item->codeIR();
    }

    irgen_table.symbol_table.exit_scope();
}

void __FuncFParam::codeIR() {}

void __FuncDef::codeIR() {    // add FuncDef llvm Instructions
    irgen_table.symbol_table.enter_scope();

    BasicInstruction::LLVMType FuncDeclRetType = typeTrans(return_type);
    FuncDefInstruction FuncDefIns = new FunctionDefineInstruction(FuncDeclRetType, name->get_string());

    global_reg_counter = 0;

    reg_Var_Table.clear();
    FormalArrayTable.clear();

    funcd_label = 0;
    max_label = -1;
    funcd = FuncDefIns;
    curr_ret = return_type;

    llvmIR.NewFunction(funcd);
    LLVMBlock B = llvmIR.NewBlock(funcd, ++max_label);
    auto formal_vector = *formals;
    global_reg_counter = formal_vector.size();
    for (int i = 0; i < formal_vector.size(); ++i) {
        auto formal = formal_vector[i];
        VarAttribute val;
        val.type = formal->type_decl;
        BasicInstruction::LLVMType lltype = typeTrans(formal->type_decl);
        if (formal->dims != nullptr) {    // formal is array
            // in SysY, we can assume that we can not modify the array address, so we do not need alloca
            FuncDefIns->InsertFormal(BasicInstruction::LLVMType::PTR);

            for (int i = 1; i < formal->dims->size(); ++i) {    // in IRgen, we ignore the first dim of the
                                                                // formal
                auto d = formal->dims->at(i);
                val.dims.push_back(d->attribute.V.val.IntVal);
            }
            FormalArrayTable[i] = 1;
            irgen_table.symbol_table.add_Symbol(formal->name, i);
            reg_Var_Table[i] = val;
        } else {    // formal is not array
            FuncDefIns->InsertFormal(lltype);
            IRgenAlloca(B, lltype, global_reg_counter++);
            IRgenStore(B, lltype, GetNewRegOperand(i), GetNewRegOperand(global_reg_counter - 1));
            irgen_table.symbol_table.add_Symbol(formal->name, global_reg_counter - 1);
            reg_Var_Table[global_reg_counter - 1] = val;
        }
    }
    IRgenBRUnCond(B, 1);

    B = llvmIR.NewBlock(funcd, ++max_label);
    funcd_label = max_label;
    block->codeIR();

    AddNoReturnBlock();

    max_reg_map[FuncDefIns] = global_reg_counter-1;
    max_label_map[FuncDefIns] = max_label;
    irgen_table.symbol_table.exit_scope();
}

void CompUnit_Decl::codeIR() {//decl->codeIR();
}

void CompUnit_FuncDef::codeIR() { func_def->codeIR(); }

void AddLibFunctionDeclare() {
    FunctionDeclareInstruction *getint = new FunctionDeclareInstruction(BasicInstruction::I32, "getint");
    llvmIR.function_declare.push_back(getint);

    FunctionDeclareInstruction *getchar = new FunctionDeclareInstruction(BasicInstruction::I32, "getch");
    llvmIR.function_declare.push_back(getchar);

    FunctionDeclareInstruction *getfloat = new FunctionDeclareInstruction(BasicInstruction::FLOAT32, "getfloat");
    llvmIR.function_declare.push_back(getfloat);

    FunctionDeclareInstruction *getarray = new FunctionDeclareInstruction(BasicInstruction::I32, "getarray");
    getarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(getarray);

    FunctionDeclareInstruction *getfloatarray = new FunctionDeclareInstruction(BasicInstruction::I32, "getfarray");
    getfloatarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(getfloatarray);

    FunctionDeclareInstruction *putint = new FunctionDeclareInstruction(BasicInstruction::VOID, "putint");
    putint->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(putint);

    FunctionDeclareInstruction *putch = new FunctionDeclareInstruction(BasicInstruction::VOID, "putch");
    putch->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(putch);

    FunctionDeclareInstruction *putfloat = new FunctionDeclareInstruction(BasicInstruction::VOID, "putfloat");
    putfloat->InsertFormal(BasicInstruction::FLOAT32);
    llvmIR.function_declare.push_back(putfloat);

    FunctionDeclareInstruction *putarray = new FunctionDeclareInstruction(BasicInstruction::VOID, "putarray");
    putarray->InsertFormal(BasicInstruction::I32);
    putarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(putarray);

    FunctionDeclareInstruction *putfarray = new FunctionDeclareInstruction(BasicInstruction::VOID, "putfarray");
    putfarray->InsertFormal(BasicInstruction::I32);
    putfarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(putfarray);

    FunctionDeclareInstruction *starttime = new FunctionDeclareInstruction(BasicInstruction::VOID, "_sysy_starttime");
    starttime->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(starttime);

    FunctionDeclareInstruction *stoptime = new FunctionDeclareInstruction(BasicInstruction::VOID, "_sysy_stoptime");
    stoptime->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(stoptime);

    // 一些llvm自带的函数，也许会为你的优化提供帮助
    FunctionDeclareInstruction *llvm_memset =
    new FunctionDeclareInstruction(BasicInstruction::VOID, "llvm.memset.p0.i32");
    llvm_memset->InsertFormal(BasicInstruction::PTR);
    llvm_memset->InsertFormal(BasicInstruction::I8);
    llvm_memset->InsertFormal(BasicInstruction::I32);
    llvm_memset->InsertFormal(BasicInstruction::I1);
    llvmIR.function_declare.push_back(llvm_memset);

    FunctionDeclareInstruction *llvm_umax = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.umax.i32");
    llvm_umax->InsertFormal(BasicInstruction::I32);
    llvm_umax->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_umax);

    FunctionDeclareInstruction *llvm_umin = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.umin.i32");
    llvm_umin->InsertFormal(BasicInstruction::I32);
    llvm_umin->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_umin);

    FunctionDeclareInstruction *llvm_smax = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.smax.i32");
    llvm_smax->InsertFormal(BasicInstruction::I32);
    llvm_smax->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_smax);

    FunctionDeclareInstruction *llvm_smin = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.smin.i32");
    llvm_smin->InsertFormal(BasicInstruction::I32);
    llvm_smin->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_smin);
}

