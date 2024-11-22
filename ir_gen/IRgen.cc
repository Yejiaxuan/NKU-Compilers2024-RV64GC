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
int max_reg = -1;
int label_count = -1;
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
// 修改后的函数定义
void IRgenTypeConverse(LLVMBlock B, Type::ty type_src, Type::ty type_dst, int src) {
    if (type_src == type_dst) {
        return;
    } else {
        //irgen_table.register_counter++;
        //int dst = irgen_table.register_counter; // 自动分配一个新的寄存器作为目标寄存器
        if (type_src == Type::FLOAT && type_dst == Type::INT) {
            IRgenFptosi(B, src, ++irgen_table.register_counter);
        } else if (type_src == Type::INT && type_dst == Type::FLOAT) {
            IRgenSitofp(B, src, ++irgen_table.register_counter);
        } else if (type_src == Type::BOOL && type_dst == Type::INT) {
            IRgenZextI1toI32(B, src, ++irgen_table.register_counter);
        } else if (type_src == Type::INT && type_dst == Type::BOOL) {
            IRgenIcmpImmRight(B, BasicInstruction::IcmpCond::ne, src, 0, ++irgen_table.register_counter);
        } else if (type_src == Type::FLOAT && type_dst == Type::BOOL) {
            IRgenFcmpImmRight(B, BasicInstruction::FcmpCond::ONE, src, 0.0f, ++irgen_table.register_counter);
        } else if (type_src == Type::BOOL && type_dst == Type::FLOAT) {
            // BOOL to FLOAT conversion (true to 1.0, false to 0.0)
            IRgenZextI1toI32(B, src, ++irgen_table.register_counter);
            src = irgen_table.register_counter;
            //irgen_table.register_counter++;
            //dst = irgen_table.register_counter;
            IRgenSitofp(B, src, ++irgen_table.register_counter);
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

void GenerateBinaryOperation(tree_node* current_node, tree_node* left, tree_node* right, NodeAttribute::opcode op, LLVMBlock B) {
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
        type2 = Type::INT;
    } else if (type1 == Type::INT && type2 == Type::FLOAT) {
        IRgenSitofp(B, reg1, ++irgen_table.register_counter);
        reg1 = irgen_table.register_counter;
        type1 = Type::FLOAT;
    } else if (type1 == Type::FLOAT && type2 == Type::BOOL) {
        IRgenZextI1toI32(B, reg2, ++irgen_table.register_counter);
        reg2 = irgen_table.register_counter;
        IRgenSitofp(B, reg2, ++irgen_table.register_counter);
        reg2 = irgen_table.register_counter;
        type2 = Type::FLOAT;
    } else if (type1 == Type::FLOAT && type2 == Type::INT) {
        IRgenSitofp(B, reg2, ++irgen_table.register_counter);
        reg2 = irgen_table.register_counter;
        type2 = Type::FLOAT;
    } else if (type1 == Type::BOOL && type2 == Type::BOOL) {
        IRgenZextI1toI32(B, reg1, ++irgen_table.register_counter);
        reg1 = irgen_table.register_counter;
        type1 = Type::INT;
        IRgenZextI1toI32(B, reg2, ++irgen_table.register_counter);
        reg2 = irgen_table.register_counter;
        type2 = Type::INT;
    } else if (type1 == Type::BOOL && type2 == Type::INT) {
        IRgenZextI1toI32(B, reg1, ++irgen_table.register_counter);
        reg1 = irgen_table.register_counter;
        type1 = Type::INT;
    } else if (type1 == Type::BOOL && type2 == Type::FLOAT) {
        IRgenZextI1toI32(B, reg1, ++irgen_table.register_counter);
        reg1 = irgen_table.register_counter;
        IRgenSitofp(B, reg1, ++irgen_table.register_counter);
        reg1 = irgen_table.register_counter;
        type1 = Type::FLOAT;
    } 

    // 判断操作符是否为关系运算
    if (int_cmp_ops.find(op) != int_cmp_ops.end()) {
        // 关系运算符
        if (type1 == Type::INT && type2 == Type::INT) {
            IRgenIcmp(B, int_cmp_ops.at(op), reg1, reg2, ++irgen_table.register_counter);
            current_node->attribute.T.type = Type::BOOL;  // 关系运算结果为布尔类型
        } else if (type1 == Type::FLOAT && type2 == Type::FLOAT) {
            IRgenFcmp(B, float_cmp_ops.at(op), reg1, reg2, ++irgen_table.register_counter);
            current_node->attribute.T.type = Type::BOOL;
        } else {
            assert(false && "Unsupported type combination for relational operation");
        }
    } else if (int_ops.find(op) != int_ops.end()) {
        // 算术运算符
        if (type1 == Type::INT && type2 == Type::INT) {
            IRgenArithmeticI32(B, int_ops.at(op), reg1, reg2, ++irgen_table.register_counter);
            current_node->attribute.T.type = Type::INT;
        } else if (type1 == Type::FLOAT && type2 == Type::FLOAT) {
            IRgenArithmeticF32(B, float_ops.at(op), reg1, reg2, ++irgen_table.register_counter);
            current_node->attribute.T.type = Type::FLOAT;
        } else {
            assert(false && "Unsupported type combination for arithmetic operation");
        }
    } else {
        // 不支持的操作符
        assert(false && "Unsupported operation");
    }

    // 更新当前节点的结果寄存器
    current_node->attribute.result_reg = irgen_table.register_counter;
}

void AddExp_plus::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(this, addexp, mulexp, NodeAttribute::ADD, B);
    //TODO("BinaryExp CodeIR"); 
}

void AddExp_sub::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(this, addexp, mulexp, NodeAttribute::SUB, B);
    //TODO("BinaryExp CodeIR"); 
}

void MulExp_mul::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(this, mulexp, unary_exp, NodeAttribute::MUL, B);
    //TODO("BinaryExp CodeIR");
}

void MulExp_div::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(this, mulexp, unary_exp, NodeAttribute::DIV, B);
    //TODO("BinaryExp CodeIR"); 
}

void MulExp_mod::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(this, mulexp, unary_exp, NodeAttribute::MOD, B);
    //TODO("BinaryExp CodeIR"); 
}

void RelExp_leq::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(this, relexp, addexp, NodeAttribute::LEQ, B);
    //TODO("BinaryExp CodeIR"); 
}

void RelExp_lt::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(this, relexp, addexp, NodeAttribute::LT, B);
    //TODO("BinaryExp CodeIR"); 
}

void RelExp_geq::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(this, relexp, addexp, NodeAttribute::GEQ, B);
    //TODO("BinaryExp CodeIR"); 
}

void RelExp_gt::codeIR() {
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(this, relexp, addexp, NodeAttribute::GT, B);
    //TODO("BinaryExp CodeIR"); 
}

void EqExp_eq::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(this, eqexp, relexp, NodeAttribute::EQ, B);
    //TODO("BinaryExp CodeIR"); 
}

void EqExp_neq::codeIR() { 
    LLVMBlock B = llvmIR.GetBlock(this_function, function_label);
    GenerateBinaryOperation(this, eqexp, relexp, NodeAttribute::NEQ, B);
    //TODO("BinaryExp CodeIR"); 
}

// short circuit &&
void LAndExp_and::codeIR() {
    // 创建新标签
    int left_label = llvmIR.NewBlock(this_function, ++label_count)->block_id;
    // 生成左操作数的代码
    landexp->true_label = left_label;
    landexp->false_label = this->false_label;
    landexp->codeIR();
    // 获取当前函数的基本块
    LLVMBlock current_block = llvmIR.GetBlock(this_function, function_label);
    IRgenTypeConverse(current_block, landexp->attribute.T.type, Type::BOOL, irgen_table.register_counter);

    // 如果左操作数为 false，跳转到结束块（短路逻辑）
    IRgenBrCond(current_block, irgen_table.register_counter, left_label, this->false_label);

    // 处理 true_label，生成右操作数的代码
    function_label = left_label;
    LLVMBlock true_block = llvmIR.GetBlock(this_function, function_label);
    eqexp->true_label = this->true_label;
    eqexp->false_label = this->false_label;
    eqexp->codeIR();
    IRgenTypeConverse(true_block, eqexp->attribute.T.type, Type::BOOL, irgen_table.register_counter);

    /*// 跳转到结束块
    IRgenBRUnCond(true_block, end_label);

    // 处理结束块，将最终结果保存在 attribute.result_reg
    function_label = end_label;
    LLVMBlock end_block = llvmIR.GetBlock(this_function, end_label);
    attribute.result_reg = irgen_table.register_counter;
    //this->attribute.result_reg = final_result;
    this->attribute.T.type = Type::BOOL;*/
    // 设置最终结果
    this->attribute.result_reg = eqexp->attribute.result_reg;
    this->attribute.T.type = Type::BOOL;
    // TODO("LAndExpAnd CodeIR");
}

// short circuit ||
void LOrExp_or::codeIR() {
    // 创建新标签
    int left_label = llvmIR.NewBlock(this_function, ++label_count)->block_id;

    // 生成左操作数的代码
    lorexp->true_label = this->true_label;
    lorexp->false_label = left_label;
    lorexp->codeIR();
    // 获取当前函数的基本块
    LLVMBlock current_block = llvmIR.GetBlock(this_function, function_label);
    IRgenTypeConverse(current_block, lorexp->attribute.T.type, Type::BOOL, irgen_table.register_counter);

    // 如果左操作数为 true，跳转到结束块（短路逻辑）
    IRgenBrCond(current_block, irgen_table.register_counter, this->true_label, left_label);

    // 处理 false_label，生成右操作数的代码
    function_label = left_label;
    LLVMBlock false_block = llvmIR.GetBlock(this_function, function_label);
    landexp->true_label = this->true_label;
    landexp->false_label = this->false_label;
    landexp->codeIR();
    IRgenTypeConverse(false_block, landexp->attribute.T.type, Type::BOOL, irgen_table.register_counter);

    /*// 跳转到结束块
    IRgenBRUnCond(false_block, end_label);

    // 处理结束块，将最终结果保存在 attribute.result_reg
    function_label = end_label;
    LLVMBlock end_block = llvmIR.GetBlock(this_function, end_label);
    attribute.result_reg = irgen_table.register_counter;
    //this->attribute.result_reg = final_result;
    this->attribute.T.type = Type::BOOL;*/
    // 设置最终结果
    this->attribute.result_reg = landexp->attribute.result_reg;
    this->attribute.T.type = Type::BOOL;
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
        // 处理数组索引
        std::vector<Operand> array_indexs;
        // 对每个维度的索引表达式进行 codeIR，并进行类型转换
        if(dims != nullptr){
            for (auto dim_exp : *dims) {
                dim_exp->codeIR();
                int idx_reg = irgen_table.register_counter;
                if (dim_exp->attribute.T.type != Type::INT) {
                    // 需要进行类型转换
                    IRgenTypeConverse(B, dim_exp->attribute.T.type, Type::INT, idx_reg);
                    int converted_reg = irgen_table.register_counter; // 转换后的寄存器编号
                    array_indexs.push_back(GetNewRegOperand(converted_reg));
                } else {
                    // 不需要类型转换，直接使用原寄存器
                    array_indexs.push_back(GetNewRegOperand(idx_reg));
                }
            }
        }

        // 如果是形参数组，第一个索引不需要 0
        if (formal_array_tag) {
            // 形参数组，第一个索引不需要 0
            IRgenGetElementptrIndexI32(B, lltype, ++irgen_table.register_counter, ptr_operand, lval_attribute.dims, array_indexs);
        } else {
            // 普通数组，需要在索引前加一个 0
            array_indexs.insert(array_indexs.begin(), new ImmI32Operand(0));
            IRgenGetElementptrIndexI32(B, lltype, ++irgen_table.register_counter, ptr_operand, lval_attribute.dims, array_indexs);
        }
        ptr_operand = GetNewRegOperand(irgen_table.register_counter);
    }

    // 存储指针操作数
    ptr = ptr_operand;

    if (!is_left) {
        if (attribute.T.type != Type::PTR) {
            IRgenLoad(B, lltype, ++irgen_table.register_counter, ptr_operand);
        }
    }
    // TODO("Lval CodeIR");
}

void FuncRParams::codeIR() {
    LLVMBlock current_block = llvmIR.GetBlock(this_function, function_label);

    // 获取形式参数
    auto formal_params = semant_table.FunctionTable[function_name]->formals;
    assert(params->size() == formal_params->size()); // 参数数量必须匹配

    for (size_t i = 0; i < params->size(); ++i) {
        auto param = params->at(i);
        auto formal_param = formal_params->at(i);

        param->codeIR();  // 生成参数的中间代码
        
        // 类型检查并进行必要的类型转换
        if (formal_param->type_decl == Type::PTR) {
            // 处理指针类型参数
            if (param->attribute.T.type != Type::PTR) {
                IRgenGetElementptrIndexI32(
                    current_block, Type2LLVM(param->attribute.T.type), ++irgen_table.register_counter,
                    GetNewRegOperand(param->attribute.result_reg), {}, {}
                );
                call_args.emplace_back(BasicInstruction::PTR, GetNewRegOperand(irgen_table.register_counter));
            } else {
                call_args.emplace_back(BasicInstruction::PTR, GetNewRegOperand(param->attribute.result_reg));
            }
        } else {
            // 处理非指针类型参数
            int src = irgen_table.register_counter;
            if (param->attribute.T.type == Type::FLOAT && formal_param->type_decl == Type::INT) {
                IRgenFptosi(current_block, src, ++irgen_table.register_counter);
                param->attribute.T.type = formal_param->type_decl;
            } else if (param->attribute.T.type == Type::INT && formal_param->type_decl == Type::FLOAT) {
                IRgenSitofp(current_block, src, ++irgen_table.register_counter);
                param->attribute.T.type = formal_param->type_decl;
            } else if (param->attribute.T.type == Type::BOOL && formal_param->type_decl == Type::INT) {
                IRgenZextI1toI32(current_block, src, ++irgen_table.register_counter);
                param->attribute.T.type = formal_param->type_decl;
            } else if (param->attribute.T.type == Type::INT && formal_param->type_decl == Type::BOOL) {
                IRgenIcmpImmRight(current_block, BasicInstruction::IcmpCond::ne, src, 0, ++irgen_table.register_counter);
                param->attribute.T.type = formal_param->type_decl;
            } else if (param->attribute.T.type == Type::FLOAT && formal_param->type_decl == Type::BOOL) {
                IRgenFcmpImmRight(current_block, BasicInstruction::FcmpCond::ONE, src, 0.0f, ++irgen_table.register_counter);
                param->attribute.T.type = formal_param->type_decl;
            } else if (param->attribute.T.type == Type::BOOL && formal_param->type_decl == Type::FLOAT) {
                IRgenZextI1toI32(current_block, src, ++irgen_table.register_counter);
                src = irgen_table.register_counter;
                IRgenSitofp(current_block, src, ++irgen_table.register_counter);
                param->attribute.T.type = formal_param->type_decl;
            }
            call_args.emplace_back(Type2LLVM(param->attribute.T.type), GetNewRegOperand(irgen_table.register_counter));
        }
    }
    // TODO("FuncRParams CodeIR");
}

void Func_call::codeIR() {
    // 获取当前块和函数上下文
    LLVMBlock current_block = llvmIR.GetBlock(this_function, function_label);
    auto return_type = semant_table.FunctionTable[name]->return_type;
    auto llvm_ret_type = Type2LLVM(return_type);

    std::vector<std::pair<BasicInstruction::LLVMType, Operand>> call_args;

    // 检查并处理函数参数
    if (funcr_params != nullptr) {
        auto func_params = dynamic_cast<FuncRParams*>(funcr_params);

        if (func_params) {
            func_params->function_name = name; // 设置函数名上下文
            func_params->codeIR(); // 调用 FuncRParams::codeIR
            call_args = func_params->call_args; // 获取处理后的参数
        }
    }

    // 根据返回类型生成调用指令
    if (return_type == Type::VOID) {
        if (call_args.empty()) {
            IRgenCallVoidNoArgs(current_block, llvm_ret_type, name->get_string());
        } else {
            IRgenCallVoid(current_block, llvm_ret_type, call_args, name->get_string());
        }
    } else {
        if (call_args.empty()) {
            IRgenCallNoArgs(current_block, llvm_ret_type, ++irgen_table.register_counter, name->get_string());
        } else {
            IRgenCall(current_block, llvm_ret_type, ++irgen_table.register_counter, call_args, name->get_string());
        }

        // 更新返回值寄存器
        this->attribute.result_reg = irgen_table.register_counter;
        this->attribute.T.type = return_type;
    }
}


void GenerateUnaryOperation(tree_node* current_node, tree_node* operand, NodeAttribute::opcode op, LLVMBlock B) {
    // 生成操作数的中间代码
    operand->codeIR();
    int reg = irgen_table.register_counter; // 操作数所在的寄存器

    Type::ty type = operand->attribute.T.type;

    // 如果是布尔值，且是加减运算，需要扩展为整型
    if (type == Type::BOOL && (op == NodeAttribute::ADD || op == NodeAttribute::SUB)) {
        IRgenZextI1toI32(B, reg, ++irgen_table.register_counter);
        reg = irgen_table.register_counter;
        type = Type::INT;  // 更新类型为整型
    }

    if (op == NodeAttribute::ADD) {
        // 单目加号不做任何操作
        current_node->attribute.result_reg = reg;
        current_node->attribute.T.type = type;
        return;
    } else if (op == NodeAttribute::SUB) {
        // 处理单目减号
        if (type == Type::INT) {
            // 整型取负操作：0 - reg
            IRgenArithmeticI32ImmLeft(B, BasicInstruction::SUB, 0, reg, ++irgen_table.register_counter);
            current_node->attribute.result_reg = irgen_table.register_counter;
            current_node->attribute.T.type = Type::INT;
        } else if (type == Type::FLOAT) {
            // 浮点数取负操作：0.0 - reg
            IRgenArithmeticF32ImmLeft(B, BasicInstruction::FSUB, 0.0f, reg, ++irgen_table.register_counter);
            current_node->attribute.result_reg = irgen_table.register_counter;
            current_node->attribute.T.type = Type::FLOAT;
        } else {
            assert(false && "Unsupported type for SUB operation");
        }
        return;
    } else if (op == NodeAttribute::NOT) {
        // 处理逻辑非操作
        if (type == Type::BOOL) {
            // 对布尔值执行非运算：先将 i1 扩展为 i32
            IRgenZextI1toI32(B, reg, ++irgen_table.register_counter);
            reg = irgen_table.register_counter;
            // 然后执行比较操作
            IRgenIcmpImmRight(B, BasicInstruction::IcmpCond::eq, reg, 0, ++irgen_table.register_counter);
            current_node->attribute.result_reg = irgen_table.register_counter;
            current_node->attribute.T.type = Type::BOOL;
        } else if (type == Type::INT) {
            // 对整型执行非运算：reg == 0
            IRgenIcmpImmRight(B, BasicInstruction::IcmpCond::eq, reg, 0, ++irgen_table.register_counter);
            current_node->attribute.result_reg = irgen_table.register_counter;
            current_node->attribute.T.type = Type::BOOL;
        } else if (type == Type::FLOAT) {
            // 对浮点数执行非运算：reg == 0.0
            IRgenFcmpImmRight(B, BasicInstruction::FcmpCond::OEQ, reg, 0.0f, ++irgen_table.register_counter);
            current_node->attribute.result_reg = irgen_table.register_counter;
            current_node->attribute.T.type = Type::BOOL;
        } else {
            assert(false && "Unsupported type for NOT operation");
        }
        return;
    }

    // 如果遇到不支持的操作符，抛出错误
    assert(false && "Unsupported unary operation");
}


void UnaryExp_plus::codeIR() {
    LLVMBlock block = llvmIR.GetBlock(this_function, function_label);
    GenerateUnaryOperation(this, unary_exp, NodeAttribute::ADD, block);
    this->attribute.result_reg = irgen_table.register_counter;
    // TODO("UnaryExpPlus CodeIR");
}

void UnaryExp_neg::codeIR() {
    LLVMBlock block = llvmIR.GetBlock(this_function, function_label);
    GenerateUnaryOperation(this, unary_exp, NodeAttribute::SUB, block);
    this->attribute.result_reg = irgen_table.register_counter;
    // TODO("UnaryExpNeg CodeIR");
}

void UnaryExp_not::codeIR() {
    LLVMBlock block = llvmIR.GetBlock(this_function, function_label);
    GenerateUnaryOperation(this, unary_exp, NodeAttribute::NOT, block);
    this->attribute.result_reg = irgen_table.register_counter;
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
    reg = irgen_table.register_counter;
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
    // 创建基本块
    int ifLabel = llvmIR.NewBlock(this_function, ++label_count)->block_id;    // if 分支块
    int elseLabel = llvmIR.NewBlock(this_function, ++label_count)->block_id; // else 分支块
    int endLabel = llvmIR.NewBlock(this_function, ++label_count)->block_id;  // 合并块

    // 条件判断部分
    Cond->true_label = ifLabel;
    Cond->false_label = elseLabel;
    Cond->codeIR(); // 生成条件表达式的中间代码
    LLVMBlock currentBlock = llvmIR.GetBlock(this_function, function_label);
    IRgenTypeConverse(currentBlock, Cond->attribute.T.type, Type::BOOL, irgen_table.register_counter); // 转换为布尔类型
    IRgenBrCond(currentBlock, irgen_table.register_counter, ifLabel, elseLabel); // 根据条件跳转

    // if 分支块生成
    function_label = ifLabel;
    ifstmt->codeIR(); // 生成 if 语句块的中间代码
    LLVMBlock ifBlock = llvmIR.GetBlock(this_function, function_label);
    IRgenBRUnCond(ifBlock, endLabel); // if 块结束后跳转到合并块

    // else 分支块生成
    function_label = elseLabel;
    elsestmt->codeIR(); // 生成 else 语句块的中间代码
    LLVMBlock elseBlock = llvmIR.GetBlock(this_function, function_label);
    IRgenBRUnCond(elseBlock, endLabel); // else 块结束后跳转到合并块

    // 合并块生成
    function_label = endLabel;
    LLVMBlock endBlock = llvmIR.GetBlock(this_function, function_label);

    // TODO("IfElseStmt CodeIR");
}

void if_stmt::codeIR() {
    int ifLabel = llvmIR.NewBlock(this_function, ++label_count)->block_id;
    int endLabel = llvmIR.NewBlock(this_function, ++label_count)->block_id;

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
    int judgeLabel = llvmIR.NewBlock(this_function, ++label_count)->block_id;
    int bodyLabel = llvmIR.NewBlock(this_function, ++label_count)->block_id;
    int endLabel = llvmIR.NewBlock(this_function, ++label_count)->block_id;

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
    function_label = llvmIR.NewBlock(this_function, ++label_count)->block_id;
    // TODO("ContinueStmt CodeIR");
}

void break_stmt::codeIR() {
    LLVMBlock block = llvmIR.GetBlock(this_function, function_label);
    IRgenBRUnCond(block, irgen_table.loop_end_label);
    function_label = llvmIR.NewBlock(this_function, ++label_count)->block_id;
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

int FindMinDimStepIR(const std::vector<int>& dims, int relativePos, int dimsIdx, int& max_subBlock_sz) {
    int min_dim_step = 1;
    int blockSz = 1;
    for (size_t i = dimsIdx + 1; i < dims.size(); i++) {
        blockSz *= dims[i];
    }
    while (relativePos % blockSz != 0) {
        min_dim_step++;
        blockSz /= dims[dimsIdx + min_dim_step - 1];
    }
    max_subBlock_sz = blockSz;
    return min_dim_step;
}

std::vector<int> GetIndexes(const std::vector<int>& dims, int absoluteIndex) {
    std::vector<int> ret;
    for (auto it = dims.rbegin(); it != dims.rend(); ++it) {
        int dim = *it;
        ret.insert(ret.begin(), absoluteIndex % dim);
        absoluteIndex /= dim;
    }
    return ret;
}

void RecursiveArrayInitIR(LLVMBlock block, const std::vector<int>& dims, int arrayaddr_reg_no, InitVal init,
                          int beginPos, int endPos, int dimsIdx, Type::ty ArrayType) {
    int pos = beginPos;
    for (InitVal iv : *(init->GetList())) {
        if (iv->IsExp()) {
            iv->codeIR();
            int init_val_reg = irgen_table.register_counter;
            IRgenTypeConverse(block, iv->attribute.T.type, ArrayType, init_val_reg);
            init_val_reg = irgen_table.register_counter;

            int addr_reg = ++irgen_table.register_counter;
            auto gep = new GetElementptrInstruction(Type2LLVM(ArrayType), GetNewRegOperand(addr_reg),
                                                    GetNewRegOperand(arrayaddr_reg_no), dims,
                                                    BasicInstruction::I32);
            gep->push_idx_imm32(0);
            std::vector<int> indexes = GetIndexes(dims, pos);
            for (int idx : indexes) {
                gep->push_idx_imm32(idx);
            }
            block->InsertInstruction(1, gep);
            IRgenStore(block, Type2LLVM(ArrayType), GetNewRegOperand(init_val_reg), GetNewRegOperand(addr_reg));
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
            IRgenArithmeticF32ImmAll(InitB, BasicInstruction::FADD, 0.0f, 0.0f, ++irgen_table.register_counter);
            value_operand = GetNewRegOperand(irgen_table.register_counter);
        }
        IRgenStore(InitB, Type2LLVM(current_type_decl), value_operand, GetNewRegOperand(allocation_register));
    } else {
        // 处理数组变量
        // 获取维度信息
        auto dims_vector = *GetDims();
        for (auto dim : dims_vector) {
            dim->codeIR();
            attribute.dims.push_back(dim->attribute.V.val.IntVal);
        }

        // 在符号表中记录维度信息
        RegTable[allocation_register] = attribute;

        // 使用 Alloca 指令为数组分配内存
        IRgenAllocaArray(B, Type2LLVM(current_type_decl), allocation_register, attribute.dims);

        // 初始化数组为零（可选）
        int array_size = 1;
        for (int dim_size : attribute.dims) {
            array_size *= dim_size;
        }
        CallInstruction *memsetCall = new CallInstruction(BasicInstruction::VOID, nullptr, "llvm.memset.p0.i32");
        memsetCall->push_back_Parameter(BasicInstruction::PTR, GetNewRegOperand(allocation_register)); // 数组地址
        memsetCall->push_back_Parameter(BasicInstruction::I8, new ImmI32Operand(0));                   // 初始化值 0
        memsetCall->push_back_Parameter(BasicInstruction::I32, new ImmI32Operand(array_size * 4));     // 数组字节大小
        memsetCall->push_back_Parameter(BasicInstruction::I1, new ImmI32Operand(0));                   // 非 volatile
        InitB->InsertInstruction(1, memsetCall);
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

        InitVal initializer = GetInit();
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
                IRgenArithmeticF32ImmAll(InitB, BasicInstruction::FADD, 0.0f, 0.0f, ++irgen_table.register_counter); 
                value_operand = GetNewRegOperand(irgen_table.register_counter);
            }
            IRgenStore(InitB, Type2LLVM(current_type_decl), value_operand, GetNewRegOperand(allocation_register));
        }
    } else {
        // 数组变量
        // 获取维度信息
        auto dims_vector = *GetDims();
        for (auto dim : dims_vector) {
            dim->codeIR();
            attribute.dims.push_back(dim->attribute.V.val.IntVal);
        }

        // 在符号表中记录维度信息
        RegTable[allocation_register] = attribute;

        // 使用 Alloca 指令为数组分配内存
        IRgenAllocaArray(B, Type2LLVM(current_type_decl), allocation_register, attribute.dims);

        // 初始化数组（递归初始化）
        InitVal initializer = GetInit();
        if (initializer != nullptr) {
            int array_size = 1;
            for (int dim_size : attribute.dims) {
                array_size *= dim_size;
            }

            // 先将数组内存清零
            CallInstruction *memsetCall = new CallInstruction(BasicInstruction::VOID, nullptr, "llvm.memset.p0.i32");
            memsetCall->push_back_Parameter(BasicInstruction::PTR, GetNewRegOperand(allocation_register)); // 数组地址
            memsetCall->push_back_Parameter(BasicInstruction::I8, new ImmI32Operand(0));                   // 初始化值 0
            memsetCall->push_back_Parameter(BasicInstruction::I32, new ImmI32Operand(array_size * 4));     // 数组字节大小
            memsetCall->push_back_Parameter(BasicInstruction::I1, new ImmI32Operand(0));                   // 非 volatile
            InitB->InsertInstruction(1, memsetCall);

            // 递归初始化数组
            RecursiveArrayInitIR(InitB, attribute.dims, allocation_register, initializer, 0, array_size - 1, 0, current_type_decl);
        } else {
            int array_size = 1;
            for (int dim_size : attribute.dims) {
                array_size *= dim_size;
            }
            CallInstruction *memsetCall = new CallInstruction(BasicInstruction::VOID, nullptr, "llvm.memset.p0.i32");
            memsetCall->push_back_Parameter(BasicInstruction::PTR, GetNewRegOperand(allocation_register)); // 数组地址
            memsetCall->push_back_Parameter(BasicInstruction::I8, new ImmI32Operand(0));                   // 初始化值 0
            memsetCall->push_back_Parameter(BasicInstruction::I32, new ImmI32Operand(array_size * 4));     // 数组字节大小
            memsetCall->push_back_Parameter(BasicInstruction::I1, new ImmI32Operand(0));                   // 非 volatile
            InitB->InsertInstruction(1, memsetCall);
        }
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

        InitVal initializer = GetInit();
        assert(initializer != nullptr);

        initializer->codeIR();
        IRgenTypeConverse(InitB, initializer->attribute.T.type, current_type_decl, irgen_table.register_counter);
        Operand value_operand = GetNewRegOperand(irgen_table.register_counter);
        IRgenStore(InitB, Type2LLVM(current_type_decl), value_operand, GetNewRegOperand(allocation_register));
    } else {
        // 数组常量
        // 获取维度信息
        auto dims_vector = *GetDims();
        for (auto dim : dims_vector) {
            dim->codeIR();
            attribute.dims.push_back(dim->attribute.V.val.IntVal);
        }

        // 在符号表中记录维度信息
        RegTable[allocation_register] = attribute;

        // 使用 Alloca 指令为数组分配内存
        IRgenAllocaArray(B, Type2LLVM(current_type_decl), allocation_register, attribute.dims);

        // 初始化数组（递归初始化）
        InitVal initializer = GetInit();
        assert(initializer != nullptr);

        int array_size = 1;
        for (int dim_size : attribute.dims) {
            array_size *= dim_size;
        }

        // 先将数组内存清零
        CallInstruction *memsetCall = new CallInstruction(BasicInstruction::VOID, nullptr, "llvm.memset.p0.i32");
        memsetCall->push_back_Parameter(BasicInstruction::PTR, GetNewRegOperand(allocation_register)); // 数组地址
        memsetCall->push_back_Parameter(BasicInstruction::I8, new ImmI32Operand(0));                   // 初始化值 0
        memsetCall->push_back_Parameter(BasicInstruction::I32, new ImmI32Operand(array_size * 4));     // 数组字节大小
        memsetCall->push_back_Parameter(BasicInstruction::I1, new ImmI32Operand(0));                   // 非 volatile
        InitB->InsertInstruction(1, memsetCall);

        // 递归初始化数组
        RecursiveArrayInitIR(InitB, attribute.dims, allocation_register, initializer, 0, array_size - 1, 0, current_type_decl);
    }
    // TODO("ConstDef CodeIR");
}

void VarDecl::codeIR() {
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
    // 获取当前函数入口基本块
    LLVMBlock entry_block = llvmIR.GetBlock(this_function, 0);

    // 获取形式参数的 LLVM 类型
    BasicInstruction::LLVMType llvm_type = Type2LLVM(this->type_decl);

    // 定义形式参数的属性
    VarAttribute param_attr;
    param_attr.type = this->type_decl;

    // 判断是否是数组
    if (this->dims == nullptr) {
        // 普通变量参数处理
        this_function->InsertFormal(llvm_type);

        // 分配寄存器并分配内存
        int param_reg = ++irgen_table.register_counter;
        IRgenAlloca(entry_block, llvm_type, param_reg);

        // 将参数值存储到寄存器
        IRgenStore(entry_block, llvm_type, GetNewRegOperand(index), GetNewRegOperand(param_reg));

        // 更新符号表和寄存器表
        irgen_table.symbol_table.add_Symbol(this->name, param_reg);
        RegTable[param_reg] = param_attr;
    } else {
        // 数组参数处理
        this_function->InsertFormal(AllocaInstruction::LLVMType::PTR);

        // 保存数组的维度信息
        for (size_t i = 1; i < this->dims->size(); ++i) {
            param_attr.dims.push_back(this->dims->at(i)->attribute.V.val.IntVal);
        }

        // 更新符号表和数组表
        //int index = irgen_table.register_counter + 1;  // 当前参数索引
        irgen_table.symbol_table.add_Symbol(this->name, index);
        FormalArrayTable[index] = 1;
        RegTable[index] = param_attr;
    }
    // TODO("FunctionFParam CodeIR");
}

void __FuncDef::codeIR() {
    // 进入符号表作用域
    irgen_table.symbol_table.enter_scope();

    // 清空形式参数数组表
    FormalArrayTable.clear();

    // 初始化标签计数器
    label_count = -1;

    // 创建函数定义指令，包含返回类型和函数名称
    FuncDefInstruction function_instruction = new FunctionDefineInstruction(Type2LLVM(return_type), name->get_string());
   
    // 将当前函数指针设置为新定义的函数
    this_function = function_instruction;

    // 设置当前函数的返回类型
    irgen_table.function_returntype = return_type;
    
    // 新建函数并创建入口块
    llvmIR.NewFunction(this_function);
    LLVMBlock entry_block = llvmIR.NewBlock(this_function, ++label_count);

    // 处理参数
    auto formal_vector = *formals;
    irgen_table.register_counter = formal_vector.size() - 1;
    for (int i = 0; i < formal_vector.size(); ++i) {
        auto formal_param = formal_vector[i];
        formal_param->index = i;
        formal_param->codeIR();
    }

    // 无条件跳转到函数体块
    IRgenBRUnCond(entry_block, 1);

    // 新建函数体块并生成代码
    entry_block = llvmIR.NewBlock(this_function, ++label_count);
    function_label = label_count;
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
