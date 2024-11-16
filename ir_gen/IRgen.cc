#include "IRgen.h"
#include "../include/ir.h"
#include "semant.h"

extern SemantTable semant_table;    // 也许你会需要一些语义分析的信息

IRgenTable irgen_table;    // 中间代码生成的辅助变量
LLVMIR llvmIR;             // 我们需要在这个变量中生成中间代码
static FuncDefInstruction this_function;
static int function_label = 0;
Type::ty current_type_decl;
static Type::ty function_returntype = Type::VOID;
Operand current_strptr = nullptr;
std::map<Symbol, int> GlobalTable;
std::map<int, VarAttribute> RegTable;
std::map<int, int> FormalArrayTable;
std::map<FuncDefInstruction, int> max_label_map{};
std::map<FuncDefInstruction, int> max_reg_map{};
int max_reg = -1;
int max_label = -1;
void AddLibFunctionDeclare();

BasicInstruction::LLVMType Type2LLVM(Type::ty type) {
    switch (type) {
        case Type::VOID:
            return BasicInstruction::LLVMType::VOID;
        case Type::INT:
            return BasicInstruction::LLVMType::I32;
        case Type::FLOAT:
            return BasicInstruction::LLVMType::FLOAT32;
        case Type::BOOL:
            return BasicInstruction::LLVMType::I1;
        case Type::PTR:
            return BasicInstruction::LLVMType::PTR;
        case Type::DOUBLE:
            return BasicInstruction::LLVMType::DOUBLE;
        default:
            // 可以根据需要处理未知类型
            return BasicInstruction::LLVMType::VOID;
    }
}

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

RegOperand *GetNewRegOperand(int RegNo);

// generate TypeConverse Instructions from type_src to type_dst
// eg. you can use fptosi instruction to converse float to int
// eg. you can use zext instruction to converse bool to int
// 修改后的函数声明
void IRgenTypeConverse(LLVMBlock B, Type::ty type_src, Type::ty type_dst, int src);

// 修改后的函数定义
void IRgenTypeConverse(LLVMBlock B, Type::ty type_src, Type::ty type_dst, int src) {
    if (type_src == type_dst) {
        return;
    } else {
        int dst = ++irgen_table.register_counter; // 自动分配一个新的寄存器作为目标寄存器
        if (type_src == Type::FLOAT && type_dst == Type::INT) {
            IRgenFptosi(B, src, dst);
        } else if (type_src == Type::INT && type_dst == Type::FLOAT) {
            IRgenSitofp(B, src, dst);
        } else if (type_src == Type::BOOL && type_dst == Type::INT) {
            IRgenZextI1toI32(B, src, dst);
        } else if (type_src == Type::INT && type_dst == Type::BOOL) {
            // INT to BOOL conversion (non-zero to true, zero to false)
            IRgenIcmpImmRight(B, BasicInstruction::IcmpCond::ne, src, 0, dst);
        } else if (type_src == Type::FLOAT && type_dst == Type::BOOL) {
            // FLOAT to BOOL conversion (non-zero to true, zero to false)
            IRgenFcmpImmRight(B, BasicInstruction::FcmpCond::ONE, src, 0.0f, dst);
        } else if (type_src == Type::BOOL && type_dst == Type::FLOAT) {
            // BOOL to FLOAT conversion (true to 1.0, false to 0.0)
            IRgenZextI1toI32(B, src, dst);
            src = irgen_table.register_counter;
            irgen_table.register_counter++;
            dst = irgen_table.register_counter;
            IRgenSitofp(B, src, dst);
        } 
    }
    // TODO: 处理其他类型转换
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

void __Program::codeIR() {
    AddLibFunctionDeclare();
    auto comp_vector = *comp_list;
    for (auto comp : comp_vector) {
        comp->codeIR();
    }
}

void Exp::codeIR() { addexp->codeIR(); }

void GenerateBinaryOperation(tree_node* left, tree_node* right, NodeAttribute::opcode op, LLVMBlock B) {
    left->codeIR();
    int reg1 = irgen_table.register_counter;
    right->codeIR();
    int reg2 = irgen_table.register_counter;

    Type::ty type1 = left->attribute.T.type;
    Type::ty type2 = right->attribute.T.type;

    // 确定操作码和类型映射表
    std::map<NodeAttribute::opcode, BasicInstruction::LLVMIROpcode> int_ops = {
        {NodeAttribute::ADD, BasicInstruction::ADD},
        {NodeAttribute::SUB, BasicInstruction::SUB},
        {NodeAttribute::MUL, BasicInstruction::MUL},
        {NodeAttribute::DIV, BasicInstruction::DIV},
        {NodeAttribute::MOD, BasicInstruction::MOD}
    };

    std::map<NodeAttribute::opcode, BasicInstruction::LLVMIROpcode> float_ops = {
        {NodeAttribute::ADD, BasicInstruction::FADD},
        {NodeAttribute::SUB, BasicInstruction::FSUB},
        {NodeAttribute::MUL, BasicInstruction::FMUL},
        {NodeAttribute::DIV, BasicInstruction::FDIV}
    };

    std::map<NodeAttribute::opcode, BasicInstruction::IcmpCond> int_cmp_ops = {
        {NodeAttribute::LEQ, BasicInstruction::sle},
        {NodeAttribute::LT, BasicInstruction::slt},
        {NodeAttribute::GEQ, BasicInstruction::sge},
        {NodeAttribute::GT, BasicInstruction::sgt},
        {NodeAttribute::EQ, BasicInstruction::eq},
        {NodeAttribute::NEQ, BasicInstruction::ne}
    };

    std::map<NodeAttribute::opcode, BasicInstruction::FcmpCond> float_cmp_ops = {
        {NodeAttribute::LEQ, BasicInstruction::OLE},
        {NodeAttribute::LT, BasicInstruction::OLT},
        {NodeAttribute::GEQ, BasicInstruction::OGE},
        {NodeAttribute::GT, BasicInstruction::OGT},
        {NodeAttribute::EQ, BasicInstruction::OEQ},
        {NodeAttribute::NEQ, BasicInstruction::ONE}
    };
    // 检查并处理类型转换
    if (type1 == Type::INT && type2 == Type::BOOL){
        IRgenZextI1toI32(B, reg2, ++irgen_table.register_counter);
        reg2 = irgen_table.register_counter;
    } else if (type1 == Type::INT && type2 == Type::FLOAT) {
        IRgenSitofp(B, reg1, ++irgen_table.register_counter);
        reg1 = irgen_table.register_counter;
    } else if (type1 == Type::FLOAT && type2 == Type::BOOL) {
        IRgenZextI1toI32(B, reg2, ++irgen_table.register_counter);
        reg2 = irgen_table.register_counter;
        IRgenSitofp(B, reg2, ++irgen_table.register_counter);
        reg2 = irgen_table.register_counter;
    } else if (type1 == Type::FLOAT && type2 == Type::INT) {
        IRgenSitofp(B, reg2, ++irgen_table.register_counter);
        reg2 = irgen_table.register_counter;
    } else if (type1 == Type::BOOL && type2 == Type::BOOL) {
        IRgenZextI1toI32(B, reg1, ++irgen_table.register_counter);
        reg1 = irgen_table.register_counter;
        IRgenZextI1toI32(B, reg2, ++irgen_table.register_counter);
        reg2 = irgen_table.register_counter;
    } else if (type1 == Type::BOOL && type2 == Type::INT) {
        IRgenZextI1toI32(B, reg1, ++irgen_table.register_counter);
        reg1 = irgen_table.register_counter;
    } else if (type1 == Type::BOOL && type2 == Type::FLOAT) {
        IRgenZextI1toI32(B, reg1, ++irgen_table.register_counter);
        reg1 = irgen_table.register_counter;
        IRgenSitofp(B, reg1, ++irgen_table.register_counter);
        reg1 = irgen_table.register_counter;
    } 

    // 判断操作符是否为关系运算
    if (int_cmp_ops.find(op) != int_cmp_ops.end()) {
        if (type1 == Type::INT && type2 == Type::INT) {
            IRgenIcmp(B, int_cmp_ops.at(op), reg1, reg2, ++irgen_table.register_counter);
        } else if (type1 == Type::FLOAT && type2 == Type::FLOAT) {
            IRgenFcmp(B, float_cmp_ops.at(op), reg1, reg2, ++irgen_table.register_counter);
        } else {
            assert(false && "Unsupported type combination for relational operation");
        }
    } else if (type1 == Type::INT && type2 == Type::INT) {
        // 算术运算（整数）
        IRgenArithmeticI32(B, int_ops.at(op), reg1, reg2, ++irgen_table.register_counter);
    } else if (type1 == Type::FLOAT && type2 == Type::FLOAT) {
        // 算术运算（浮点数）
        IRgenArithmeticF32(B, float_ops.at(op), reg1, reg2, ++irgen_table.register_counter);
    } else {
        assert(false && "Unsupported operation");
    }
}

void AddExp_plus::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(addexp, mulexp, NodeAttribute::ADD, B);

    // 设置当前节点的结果寄存器
    this->attribute.result_reg = irgen_table.register_counter;
    /*this->attribute.T = (addexp->attribute.T.type == Type::FLOAT || mulexp->attribute.T.type == Type::FLOAT) 
                        ? Type(Type::FLOAT) 
                        : Type(Type::INT);  // 选择最终类型*/
    //TODO("BinaryExp CodeIR"); 
}

void AddExp_sub::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(addexp, mulexp, NodeAttribute::SUB, B);

    // 设置当前节点的结果寄存器
    this->attribute.result_reg = irgen_table.register_counter;
    //TODO("BinaryExp CodeIR"); 
}

void MulExp_mul::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(mulexp, unary_exp, NodeAttribute::MUL, B);

    // 设置当前节点的结果寄存器
    this->attribute.result_reg = irgen_table.register_counter; 
    //TODO("BinaryExp CodeIR");
}

void MulExp_div::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(mulexp, unary_exp, NodeAttribute::DIV, B);

    // 设置当前节点的结果寄存器
    this->attribute.result_reg = irgen_table.register_counter;  
    //TODO("BinaryExp CodeIR"); 
}

void MulExp_mod::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(mulexp, unary_exp, NodeAttribute::MOD, B);

    // 设置当前节点的结果寄存器
    this->attribute.result_reg = irgen_table.register_counter;  
    //TODO("BinaryExp CodeIR"); 
}

void RelExp_leq::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(relexp, addexp, NodeAttribute::LEQ, B);

    // 设置当前节点的结果寄存器
    this->attribute.result_reg = irgen_table.register_counter;  
    //TODO("BinaryExp CodeIR"); 
}

void RelExp_lt::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(relexp, addexp, NodeAttribute::LT, B);

    // 设置当前节点的结果寄存器
    this->attribute.result_reg = irgen_table.register_counter;   
    //TODO("BinaryExp CodeIR"); 
}

void RelExp_geq::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(relexp, addexp, NodeAttribute::GEQ, B);

    // 设置当前节点的结果寄存器
    this->attribute.result_reg = irgen_table.register_counter;   
    //TODO("BinaryExp CodeIR"); 
}

void RelExp_gt::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(relexp, addexp, NodeAttribute::GT, B);

    // 设置当前节点的结果寄存器
    this->attribute.result_reg = irgen_table.register_counter;    
    //TODO("BinaryExp CodeIR"); 
}

void EqExp_eq::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(eqexp, relexp, NodeAttribute::EQ, B);

    // 设置当前节点的结果寄存器
    this->attribute.result_reg = irgen_table.register_counter;
    //TODO("BinaryExp CodeIR"); 
}

void EqExp_neq::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(eqexp, relexp, NodeAttribute::NEQ, B);

    // 设置当前节点的结果寄存器
    this->attribute.result_reg = irgen_table.register_counter;   
    //TODO("BinaryExp CodeIR"); 
}

// short circuit &&
void LAndExp_and::codeIR() {
    // 获取当前函数的基本块
    LLVMBlock current_block = llvmIR.GetBlock(this_function, function_label);

    // 创建新标签
    int true_label = llvmIR.NewBlock(this_function, ++function_label)->block_id;
    int end_label = llvmIR.NewBlock(this_function, ++function_label)->block_id;

    // 生成左操作数的代码
    landexp->codeIR();
    IRgenTypeConverse(current_block, landexp->attribute.T.type, Type::BOOL, irgen_table.register_counter);

    // 如果左操作数为 false，跳转到结束块（短路逻辑）
    IRgenBrCond(current_block, irgen_table.register_counter, true_label, end_label);

    // 处理 true_label，生成右操作数的代码
    function_label = true_label;
    LLVMBlock true_block = llvmIR.GetBlock(this_function, true_label);
    eqexp->codeIR();
    IRgenTypeConverse(true_block, eqexp->attribute.T.type, Type::BOOL, irgen_table.register_counter);

    // 跳转到结束块
    IRgenBRUnCond(true_block, end_label);

    // 处理结束块，将最终结果保存在 attribute.result_reg
    function_label = end_label;
    LLVMBlock end_block = llvmIR.GetBlock(this_function, end_label);
    attribute.result_reg = irgen_table.register_counter;
    // TODO("LAndExpAnd CodeIR");
}

// short circuit ||
void LOrExp_or::codeIR() {
    // 获取当前函数的基本块
    LLVMBlock current_block = llvmIR.GetBlock(this_function, function_label);

    // 创建新标签
    int false_label = llvmIR.NewBlock(this_function, ++function_label)->block_id;
    int end_label = llvmIR.NewBlock(this_function, ++function_label)->block_id;

    // 生成左操作数的代码
    lorexp->codeIR();
    IRgenTypeConverse(current_block, lorexp->attribute.T.type, Type::BOOL, irgen_table.register_counter);

    // 如果左操作数为 true，跳转到结束块（短路逻辑）
    IRgenBrCond(current_block, irgen_table.register_counter, end_label, false_label);

    // 处理 false_label，生成右操作数的代码
    function_label = false_label;
    LLVMBlock false_block = llvmIR.GetBlock(this_function, false_label);
    landexp->codeIR();
    IRgenTypeConverse(false_block, landexp->attribute.T.type, Type::BOOL, irgen_table.register_counter);

    // 跳转到结束块
    IRgenBRUnCond(false_block, end_label);

    // 处理结束块，将最终结果保存在 attribute.result_reg
    function_label = end_label;
    LLVMBlock end_block = llvmIR.GetBlock(this_function, end_label);
    attribute.result_reg = irgen_table.register_counter;
    // TODO("LOrExpOr CodeIR");
}

void ConstExp::codeIR() { addexp->codeIR(); }

void Lval::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    Operand ptr_operand;
    VarAttribute lval_attribute;
    bool formal_array_tag = false;
    int alloca_reg = irgen_table.symbol_table.lookup(name);

    // 判断是全局变量还是局部变量
    if (alloca_reg == -1) {
        ptr_operand = GetNewGlobalOperand(name->get_string());
        lval_attribute = semant_table.GlobalTable[name];
    } else {
        ptr_operand = GetNewRegOperand(alloca_reg);
        lval_attribute = RegTable[alloca_reg];
        formal_array_tag = FormalArrayTable[alloca_reg];
    }

    auto lltype = Type2LLVM(lval_attribute.type);

    // 保留数组条件判断，但暂时不处理数组
    if (attribute.T.type == Type::PTR || dims != nullptr) {
        // TODO: 处理数组索引
    }

    // 存储指针操作数
    ptr = ptr_operand;

    // 如果是右值，加载值
    if (!is_left) {
        if (attribute.T.type != Type::PTR) {
            IRgenLoad(B, lltype, ++irgen_table.register_counter, ptr_operand);
        }
    }
    // TODO("Lval CodeIR");
}

void FuncRParams::codeIR() {

    // TODO("FuncRParams CodeIR");
}

void Func_call::codeIR() {
    LLVMBlock block = llvmIR.GetBlock(this_function, function_label);

    // 检查是否调用的是 `putf` 函数
    if (name->get_string() == "putf") {
        auto params = ((FuncRParams *)funcr_params)->params;
        std::vector<std::pair<BasicInstruction::LLVMType, Operand>> args;

        // 第一个参数是字符串指针
        auto str_param = (*params)[0];
        str_param->codeIR();
        args.emplace_back(BasicInstruction::PTR, irgen_table.string_operand);

        // 从第二个参数开始处理数值类型的参数
        for (size_t i = 1; i < params->size(); ++i) {
            auto param = (*params)[i];
            param->codeIR();

            Type::ty param_type = param->attribute.T.type;
            if (param_type == Type::FLOAT) {
                // `putf` 需要 DOUBLE 类型的参数，将 FLOAT 转换为 DOUBLE
                IRgenTypeConverse(block, param_type, Type::DOUBLE, irgen_table.register_counter);
                param_type = Type::DOUBLE;
            }
            args.emplace_back(Type2LLVM(param_type), GetNewRegOperand(irgen_table.register_counter));
        }

        // 生成 `putf` 函数的调用
        IRgenCallVoid(block, BasicInstruction::VOID, args, name->get_string());
        return;
    }

    // 获取函数的返回类型
    Type::ty return_type = semant_table.FunctionTable[name]->return_type;
    BasicInstruction::LLVMType llvm_ret_type = Type2LLVM(return_type);

    // 函数参数处理
    std::vector<std::pair<BasicInstruction::LLVMType, Operand>> args;
    if (funcr_params != nullptr) {
        auto params = ((FuncRParams *)funcr_params)->params;
        auto func_params = semant_table.FunctionTable[name]->formals;
        assert(params->size() == func_params->size());

        for (size_t i = 0; i < params->size(); ++i) {
            auto param = (*params)[i];
            auto expected_type = (*func_params)[i]->attribute.T.type;

            param->codeIR();
            IRgenTypeConverse(block, param->attribute.T.type, expected_type, irgen_table.register_counter);
            args.emplace_back(Type2LLVM(expected_type), GetNewRegOperand(irgen_table.register_counter));
        }
    }

    // 根据返回类型生成不同的调用指令
    if (return_type == Type::VOID) {
        if (args.empty()) {
            IRgenCallVoidNoArgs(block, llvm_ret_type, name->get_string());
        } else {
            IRgenCallVoid(block, llvm_ret_type, args, name->get_string());
        }
    } else {
        if (args.empty()) {
            IRgenCallNoArgs(block, llvm_ret_type, ++irgen_table.register_counter, name->get_string());
        } else {
            IRgenCall(block, llvm_ret_type, ++irgen_table.register_counter, args, name->get_string());
        }
        // 设置当前节点的结果寄存器
        this->attribute.result_reg = irgen_table.register_counter;
        this->attribute.T.type = return_type;
    }
    // TODO("FunctionCall CodeIR");
}

void UnaryExp_plus::codeIR() {

    // TODO("UnaryExpPlus CodeIR");
}

void UnaryExp_neg::codeIR() {

    // TODO("UnaryExpNeg CodeIR");
}

void UnaryExp_not::codeIR() {
    
    // TODO("UnaryExpNot CodeIR");
}

void IntConst::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    int result_reg = ++irgen_table.register_counter;
    IRgenArithmeticI32ImmAll(B, BasicInstruction::LLVMIROpcode::ADD, val, 0, result_reg);
    // TODO("IntConst CodeIR");
}

void FloatConst::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    int result_reg = ++irgen_table.register_counter;
    IRgenArithmeticF32ImmAll(B, BasicInstruction::LLVMIROpcode::FADD, val, 0.0f, result_reg);
    // TODO("FloatConst CodeIR");
}

void StringConst::codeIR() {
    int id = GlobalTable[str];
    current_strptr = GetNewGlobalOperand(".str" + std::to_string(id));
    // TODO("StringConst CodeIR");
}

void PrimaryExp_branch::codeIR() { exp->codeIR(); }

void assign_stmt::codeIR() {
    LLVMBlock block = llvmIR.GetBlock(this_function, function_label);
    lval->codeIR();
    exp->codeIR();
    int reg = irgen_table.register_counter;
    IRgenTypeConverse(block, exp->attribute.T.type, lval->attribute.T.type, reg);
    IRgenStore(block, Type2LLVM(lval->attribute.T.type), GetNewRegOperand(reg), ((Lval *)lval)->ptr);
    // TODO("AssignStmt CodeIR");
}

void expr_stmt::codeIR() {
    exp->codeIR();
}

void block_stmt::codeIR() {
    irgen_table.symbol_table.enter_scope();
    b->codeIR();
    irgen_table.symbol_table.exit_scope();
    // TODO("BlockStmt CodeIR");
}

void ifelse_stmt::codeIR() {
    int ifLabel = llvmIR.NewBlock(this_function, function_label)->block_id;
    int elseLabel = llvmIR.NewBlock(this_function, function_label)->block_id;
    int endLabel = llvmIR.NewBlock(this_function, function_label)->block_id;

    Cond->true_label = ifLabel;
    Cond->false_label = elseLabel;
    Cond->codeIR();
    LLVMBlock block1 = llvmIR.GetBlock(this_function, function_label);
    IRgenTypeConverse(block1, Cond->attribute.T.type, Type::BOOL, irgen_table.register_counter);
    IRgenBrCond(block1, irgen_table.register_counter, ifLabel, elseLabel);

    function_label = ifLabel;
    ifstmt->codeIR();
    LLVMBlock block2 = llvmIR.GetBlock(this_function, function_label);
    IRgenBRUnCond(block2, endLabel);

    function_label = elseLabel;
    elsestmt->codeIR();
    LLVMBlock block3 = llvmIR.GetBlock(this_function, function_label);
    IRgenBRUnCond(block3, endLabel);

    function_label = endLabel;
    // TODO("IfElseStmt CodeIR");
}

void if_stmt::codeIR() {
    int ifLabel = llvmIR.NewBlock(this_function, function_label)->block_id;
    int endLabel = llvmIR.NewBlock(this_function, function_label)->block_id;

    Cond->true_label = ifLabel;
    Cond->false_label = endLabel;
    Cond->codeIR();
    LLVMBlock block1 = llvmIR.GetBlock(this_function, function_label);
    IRgenTypeConverse(block1, Cond->attribute.T.type, Type::BOOL, irgen_table.register_counter);
    IRgenBrCond(block1, irgen_table.register_counter, ifLabel, endLabel);

    function_label = ifLabel;
    ifstmt->codeIR();
    LLVMBlock block2 = llvmIR.GetBlock(this_function, function_label);
    IRgenBRUnCond(block2, endLabel);

    function_label = endLabel;
    // TODO("IfStmt CodeIR");
}

void while_stmt::codeIR() {
    int judgeLabel = llvmIR.NewBlock(this_function, function_label)->block_id;
    int bodyLabel = llvmIR.NewBlock(this_function, function_label)->block_id;
    int endLabel = llvmIR.NewBlock(this_function, function_label)->block_id;

    int tempStartLabel = irgen_table.loop_start_label;
    int tempEndLabel = irgen_table.loop_end_label;
    irgen_table.loop_start_label = judgeLabel;
    irgen_table.loop_end_label = endLabel;

    LLVMBlock block1 = llvmIR.GetBlock(this_function, function_label);
    IRgenBRUnCond(block1, judgeLabel);

    function_label = judgeLabel;
    Cond->true_label = bodyLabel;
    Cond->false_label = endLabel;
    Cond->codeIR();
    LLVMBlock block2 = llvmIR.GetBlock(this_function, function_label);
    IRgenTypeConverse(block2, Cond->attribute.T.type, Type::BOOL, irgen_table.register_counter);
    IRgenBrCond(block2, irgen_table.register_counter, bodyLabel, endLabel);

    function_label = bodyLabel;
    body->codeIR();
    LLVMBlock block3 = llvmIR.GetBlock(this_function, function_label);
    IRgenBRUnCond(block3, judgeLabel);

    function_label = endLabel;

    irgen_table.loop_start_label = tempStartLabel;
    irgen_table.loop_end_label = tempEndLabel;
    // TODO("WhileStmt CodeIR");
}

void continue_stmt::codeIR() {
    LLVMBlock block = llvmIR.GetBlock(this_function, function_label);
    IRgenBRUnCond(block, irgen_table.loop_start_label);
    function_label = llvmIR.NewBlock(this_function, function_label)->block_id;
    // TODO("ContinueStmt CodeIR");
}

void break_stmt::codeIR() {
    LLVMBlock block = llvmIR.GetBlock(this_function, function_label);
    IRgenBRUnCond(block, irgen_table.loop_end_label);
    function_label = llvmIR.NewBlock(this_function, function_label)->block_id;
    // TODO("BreakStmt CodeIR");
}

void return_stmt::codeIR() {
    return_exp->codeIR();
    LLVMBlock block = llvmIR.GetBlock(this_function, function_label);
    IRgenTypeConverse(block, return_exp->attribute.T.type, irgen_table.function_returntype, irgen_table.register_counter);
    IRgenRetReg(block, Type2LLVM(irgen_table.function_returntype), irgen_table.register_counter);
    // TODO("ReturnStmt CodeIR");
}

void return_stmt_void::codeIR() {
    LLVMBlock block = llvmIR.GetBlock(this_function, function_label);
    IRgenRetVoid(block);
    // TODO("ReturnStmtVoid CodeIR");
}

void ConstInitVal::codeIR() {
    for (auto initializer : *GetList()) {
         initializer->codeIR();
    }
    // TODO("ConstInitVal CodeIR");
}

void ConstInitVal_exp::codeIR() {
    exp->codeIR();
    // TODO("ConstInitValWithExp CodeIR");
}

void VarInitVal::codeIR() {
    for (auto initializer : *GetList()) {
         initializer->codeIR();
    }
    // TODO("VarInitVal CodeIR");
}

void VarInitVal_exp::codeIR() {
    exp->codeIR();
    // TODO("VarInitValWithExp CodeIR");
}

void VarDef_no_init::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, 0);
    LLVMBlock InitB = llvmIR.GetBlock(this_function, function_label);

    irgen_table.symbol_table.add_Symbol(GetName(), ++irgen_table.register_counter);
    int allocation_register = irgen_table.register_counter;
    VarAttribute attribute;
    attribute.type = current_type_decl;

    if (GetDims() == nullptr) {
        IRgenAlloca(B, Type2LLVM(current_type_decl), allocation_register);
        RegTable[allocation_register] = attribute;

        Operand value_operand;
        if (current_type_decl == Type::INT) {
            IRgenArithmeticI32ImmAll(InitB, BasicInstruction::LLVMIROpcode::ADD, 0, 0, ++irgen_table.register_counter);
            value_operand = GetNewRegOperand(irgen_table.register_counter);
        } else if (current_type_decl == Type::FLOAT) {
            // TODO: 处理浮点数的默认初始化（后续实现）
        }
        IRgenStore(InitB, Type2LLVM(current_type_decl), value_operand, GetNewRegOperand(allocation_register));
    } else {
        // TODO: 处理数组的内存分配（后续实现）
    }
    // TODO("VarDefNoInit CodeIR");
}

void VarDef::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, 0);
    LLVMBlock InitB = llvmIR.GetBlock(this_function, function_label);

    irgen_table.symbol_table.add_Symbol(GetName(), ++irgen_table.register_counter);
    int allocation_register = irgen_table.register_counter;
    VarAttribute attribute;
    attribute.type = current_type_decl;

    if (GetDims() == nullptr) {
        IRgenAlloca(B, Type2LLVM(current_type_decl), allocation_register);
        RegTable[allocation_register] = attribute;

        VarInitVal *initializer = dynamic_cast<VarInitVal*>(GetInit());
        if (initializer != nullptr) {
            initializer->codeIR();
            IRgenTypeConverse(InitB, initializer->attribute.T.type, current_type_decl, irgen_table.register_counter);
            Operand value_operand = GetNewRegOperand(irgen_table.register_counter);
            IRgenStore(InitB, Type2LLVM(current_type_decl), value_operand, GetNewRegOperand(allocation_register));
        } else {
            Operand value_operand;
            if (current_type_decl == Type::INT) {
                IRgenArithmeticI32ImmAll(InitB, BasicInstruction::LLVMIROpcode::ADD, 0, 0, ++irgen_table.register_counter);
                value_operand = GetNewRegOperand(irgen_table.register_counter);
            } else if (current_type_decl == Type::FLOAT) {
                // TODO: 处理浮点数的默认初始化（后续实现）
            }
            IRgenStore(InitB, Type2LLVM(current_type_decl), value_operand, GetNewRegOperand(allocation_register));
        }
    } else {
        // TODO: 处理数组的内存分配和初始化（后续实现）
    }
    // TODO("VarDef CodeIR");
}

void ConstDef::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, 0);
    LLVMBlock InitB = llvmIR.GetBlock(this_function, function_label);

    irgen_table.symbol_table.add_Symbol(GetName(), ++irgen_table.register_counter);
    int allocation_register = irgen_table.register_counter;
    VarAttribute attribute;
    attribute.type = current_type_decl;

    if (GetDims() == nullptr) {
        IRgenAlloca(B, Type2LLVM(current_type_decl), allocation_register);
        RegTable[allocation_register] = attribute;

        ConstInitVal *initializer = dynamic_cast<ConstInitVal*>(GetInit());
        assert(initializer != nullptr);

        initializer->codeIR();
        IRgenTypeConverse(InitB, initializer->attribute.T.type, current_type_decl, irgen_table.register_counter);
        Operand value_operand = GetNewRegOperand(irgen_table.register_counter);
        IRgenStore(InitB, Type2LLVM(current_type_decl), value_operand, GetNewRegOperand(allocation_register));
    } else {
        // TODO: 处理数组的内存分配和初始化（后续实现）
    }
    // TODO("ConstDef CodeIR");
}

void VarDecl::codeIR() {
    /*LLVMBlock block = llvmIR.GetBlock(this_function, 0);
    LLVMBlock init_block = llvmIR.GetBlock(this_function, function_label);*/
    Type::ty local_type_decl = this->type_decl;
    Type::ty temp = current_type_decl;

    for (auto definition : *var_def_list) {
        current_type_decl = local_type_decl;
        definition->codeIR();
    }
    current_type_decl = temp;
    // TODO("VarDecl CodeIR");
}

void ConstDecl::codeIR() {
    /*LLVMBlock block = llvmIR.GetBlock(this_function, 0);
    LLVMBlock init_block = llvmIR.GetBlock(this_function, function_label);*/
    Type::ty local_type_decl = this->type_decl;
    Type::ty temp = current_type_decl;

    for (auto definition : *var_def_list) {
        current_type_decl = local_type_decl;
        definition->codeIR();
    }
    current_type_decl = temp;
    // TODO("ConstDecl CodeIR");
}

void BlockItem_Decl::codeIR() {
    decl->codeIR();
    // TODO("BlockItemDecl CodeIR");
}

void BlockItem_Stmt::codeIR() {
    stmt->codeIR();
    // TODO("BlockItemStmt CodeIR");
}

void __Block::codeIR() {
    irgen_table.symbol_table.enter_scope();
    for (auto item : *item_list) {
        item->codeIR();
    }
    irgen_table.symbol_table.exit_scope();
    // TODO("Block CodeIR");
}

void __FuncFParam::codeIR() {

    // TODO("FunctionFParam CodeIR");
}

void __FuncDef::codeIR() {
    // 进入符号表作用域
    irgen_table.symbol_table.enter_scope();

    // 设置返回类型和函数名称
    BasicInstruction::LLVMType func_ret_type = Type2LLVM(return_type);
    FuncDefInstruction function_instruction = new FunctionDefineInstruction(func_ret_type, name->get_string());
    
    // 清空寄存器和数组表
    irgen_table.register_counter = -1;
    RegTable.clear();
    FormalArrayTable.clear();
    function_label = 0;
    max_label = -1;
    this_function = function_instruction;
    irgen_table.function_returntype = return_type;

    // 新建函数并创建入口块
    llvmIR.NewFunction(this_function);
    LLVMBlock B = llvmIR.NewBlock(this_function, ++max_label);

    /*auto formal_vector = *formals;
    irgen_table.register_counter = formal_vector.size() - 1;
    for (int i = 0; i < formal_vector.size(); ++i) {
        auto formal = formal_vector[i];
        VarAttribute val;
        val.type = formal->type_decl;
        BasicInstruction::LLVMType lltype = Type2LLVM(return_type);
        if (formal->dims != nullptr) {    // formal is array
            // in SysY, we can assume that we can not modify the array address, so we do not need alloca
            function_instruction->InsertFormal(AllocaInstruction::LLVMType::PTR);

            for (int i = 1; i < formal->dims->size(); ++i) {    // in IRgen, we ignore the first dim of the
                                                                // formal
                auto d = formal->dims->at(i);
                val.dims.push_back(d->attribute.V.val.IntVal);
            }

            FormalArrayTable[i] = 1;
            irgen_table.symbol_table.add_Symbol(formal->name, i);
            RegTable[i] = val;
        } else {    // formal is not array
            function_instruction->InsertFormal(lltype);
            IRgenAlloca(entry_block, lltype, ++irgen_table.register_counter);
            IRgenStore(entry_block, lltype, GetNewRegOperand(i), GetNewRegOperand(irgen_table.register_counter));
            irgen_table.symbol_table.add_Symbol(formal->name, irgen_table.register_counter);
            RegTable[irgen_table.register_counter] = val;
        }
    }*/
    // 无条件跳转到函数体块
    IRgenBRUnCond(B, 1);

    // 新建函数体块并生成代码
    B = llvmIR.NewBlock(this_function, max_label);
    function_label = max_label;
    block->codeIR();

    // 处理无返回值情况
    for (auto& block_pair : llvmIR.function_block_map[function_instruction]) {
        LLVMBlock B = block_pair.second;

        // 检查基本块是否为空，或是否缺少返回或跳转指令
        if (B->Instruction_list.empty() || 
            (B->Instruction_list.back()->GetOpcode() != AllocaInstruction::RET && 
             B->Instruction_list.back()->GetOpcode() != AllocaInstruction::BR_COND &&
             B->Instruction_list.back()->GetOpcode() != AllocaInstruction::BR_UNCOND)) {
            
            // 如果没有返回或跳转指令，根据返回类型插入默认返回
            if (return_type == Type::VOID) {
                IRgenRetVoid(B);
            } else if (return_type == Type::INT) {
                IRgenRetImmInt(B, BasicInstruction::I32, 0);
            } else if (return_type == Type::FLOAT) {
                IRgenRetImmFloat(B, BasicInstruction::FLOAT32, 0);
            }
        }
    }
    // 更新最大寄存器号和标签号
    max_reg_map[function_instruction] = irgen_table.register_counter;
    max_label_map[function_instruction] = max_label;

    // 退出符号表作用域
    irgen_table.symbol_table.exit_scope();
    // TODO("FunctionDef CodeIR");
}

void CompUnit_Decl::codeIR() {

    // TODO("CompUnitDecl CodeIR");
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


