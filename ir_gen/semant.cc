#include "semant.h"
#include "../include/SysY_tree.h"
#include "../include/ir.h"
#include "../include/type.h"
/*
    语义分析阶段需要对语法树节点上的类型和常量等信息进行标注, 即NodeAttribute类
    同时还需要标注每个变量的作用域信息，即部分语法树节点中的scope变量
    你可以在utils/ast_out.cc的输出函数中找到你需要关注哪些语法树节点中的NodeAttribute类及其他变量
    以及对语义错误的代码输出报错信息
*/

/*
    错误检查的基本要求:
    • 检查 main 函数是否存在 (根据SysY定义，如果不存在main函数应当报错)；
    • 检查未声明变量，及在同一作用域下重复声明的变量；
    • 条件判断和运算表达式：int 和 bool 隐式类型转换（例如int a=5，return a+!a）；
    • 数值运算表达式：运算数类型是否正确 (如返回值为 void 的函数调用结果是否参与了其他表达式的计算)；
    • 检查未声明函数，及函数形参是否与实参类型及数目匹配；
    • 检查是否存在整型变量除以整型常量0的情况 (如对于表达式a/(5-4-1)，编译器应当给出警告或者直接报错)；

    错误检查的进阶要求:
    • 对数组维度进行相应的类型检查 (例如维度是否有浮点数，定义维度时是否为常量等)；
    • 对float进行隐式类型转换以及其他float相关的检查 (例如模运算中是否有浮点类型变量等)；
*/
extern LLVMIR llvmIR;

SemantTable semant_table;
std::vector<std::string> error_msgs{}; // 将语义错误信息保存到该变量中

void __Program::TypeCheck() {
    semant_table.symbol_table.enter_scope();
    auto comp_vector = *comp_list;
    for (auto comp : comp_vector) {
        comp->TypeCheck();
    }
}

void Exp::TypeCheck() {
    addexp->TypeCheck();

    attribute = addexp->attribute;
}

void AddExp_plus::TypeCheck() {
    addexp->TypeCheck();
    mulexp->TypeCheck();

    // 检查运算数类型是否为void
    if (addexp->attribute.T.type == Type::VOID || mulexp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("Addition error on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT; // 继续分析，假设结果类型为int
    } else {
        // 类型提升（int和float）
        if (addexp->attribute.T.type == Type::FLOAT || mulexp->attribute.T.type == Type::FLOAT) {
            attribute.T.type = Type::FLOAT;
        } else {
            attribute.T.type = Type::INT;
        }
    }

    // 处理常量折叠
    attribute.V.ConstTag = addexp->attribute.V.ConstTag && mulexp->attribute.V.ConstTag;
    if (attribute.V.ConstTag) {
        if (attribute.T.type == Type::INT) {
            attribute.V.val.IntVal = addexp->attribute.V.val.IntVal + mulexp->attribute.V.val.IntVal;
        } else {
            attribute.V.val.FloatVal = (addexp->attribute.T.type == Type::INT ? addexp->attribute.V.val.IntVal : addexp->attribute.V.val.FloatVal)
                                     + (mulexp->attribute.T.type == Type::INT ? mulexp->attribute.V.val.IntVal : mulexp->attribute.V.val.FloatVal);
        }
    }

    TODO("BinaryExp Semant");
}

void AddExp_sub::TypeCheck() {
    addexp->TypeCheck();
    mulexp->TypeCheck();

    // 检查运算数类型是否为void
    if (addexp->attribute.T.type == Type::VOID || mulexp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("Subtraction error on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT;
    } else {
        // 类型提升
        if (addexp->attribute.T.type == Type::FLOAT || mulexp->attribute.T.type == Type::FLOAT) {
            attribute.T.type = Type::FLOAT;
        } else {
            attribute.T.type = Type::INT;
        }
    }

    // 常量折叠
    attribute.V.ConstTag = addexp->attribute.V.ConstTag && mulexp->attribute.V.ConstTag;
    if (attribute.V.ConstTag) {
        if (attribute.T.type == Type::INT) {
            attribute.V.val.IntVal = addexp->attribute.V.val.IntVal - mulexp->attribute.V.val.IntVal;
        } else {
            attribute.V.val.FloatVal = (addexp->attribute.T.type == Type::INT ? addexp->attribute.V.val.IntVal : addexp->attribute.V.val.FloatVal)
                                     - (mulexp->attribute.T.type == Type::INT ? mulexp->attribute.V.val.IntVal : mulexp->attribute.V.val.FloatVal);
        }
    }

    TODO("BinaryExp Semant");
}

void MulExp_mul::TypeCheck() {
    mulexp->TypeCheck();
    unary_exp->TypeCheck();

    // 检查运算数类型是否为void
    if (mulexp->attribute.T.type == Type::VOID || unary_exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("Multiplication error on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT;
    } else {
        // 类型提升
        if (mulexp->attribute.T.type == Type::FLOAT || unary_exp->attribute.T.type == Type::FLOAT) {
            attribute.T.type = Type::FLOAT;
        } else {
            attribute.T.type = Type::INT;
        }
    }

    // 常量折叠
    attribute.V.ConstTag = mulexp->attribute.V.ConstTag && unary_exp->attribute.V.ConstTag;
    if (attribute.V.ConstTag) {
        if (attribute.T.type == Type::INT) {
            attribute.V.val.IntVal = mulexp->attribute.V.val.IntVal * unary_exp->attribute.V.val.IntVal;
        } else {
            attribute.V.val.FloatVal = (mulexp->attribute.T.type == Type::INT ? mulexp->attribute.V.val.IntVal : mulexp->attribute.V.val.FloatVal)
                                     * (unary_exp->attribute.T.type == Type::INT ? unary_exp->attribute.V.val.IntVal : unary_exp->attribute.V.val.FloatVal);
        }
    }

    TODO("BinaryExp Semant");
}

void MulExp_div::TypeCheck() {
    mulexp->TypeCheck();
    unary_exp->TypeCheck();

    // 检查运算数类型是否为void
    if (mulexp->attribute.T.type == Type::VOID || unary_exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("Division error on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT;
    } else {
        // 类型提升
        if (mulexp->attribute.T.type == Type::FLOAT || unary_exp->attribute.T.type == Type::FLOAT) {
            attribute.T.type = Type::FLOAT;
        } else {
            attribute.T.type = Type::INT;
        }
    }

    // 检查除以0的情况
    if (unary_exp->attribute.V.ConstTag) {
        if ((unary_exp->attribute.T.type == Type::INT && unary_exp->attribute.V.val.IntVal == 0) ||
            (unary_exp->attribute.T.type == Type::FLOAT && unary_exp->attribute.V.val.FloatVal == 0.0f)) {
            error_msgs.push_back("Division by zero at line " + std::to_string(line_number) + "\n");
        }
    }

    // 常量折叠
    attribute.V.ConstTag = mulexp->attribute.V.ConstTag && unary_exp->attribute.V.ConstTag;
    if (attribute.V.ConstTag) {
        if (attribute.T.type == Type::INT) {
            attribute.V.val.IntVal = mulexp->attribute.V.val.IntVal / unary_exp->attribute.V.val.IntVal;
        } else {
            attribute.V.val.FloatVal = (mulexp->attribute.T.type == Type::INT ? mulexp->attribute.V.val.IntVal : mulexp->attribute.V.val.FloatVal)
                                     / (unary_exp->attribute.T.type == Type::INT ? unary_exp->attribute.V.val.IntVal : unary_exp->attribute.V.val.FloatVal);
        }
    }

    TODO("BinaryExp Semant");
}

void MulExp_mod::TypeCheck() {
    mulexp->TypeCheck();
    unary_exp->TypeCheck();

    // 检查运算数类型是否为void或float（模运算不支持float）
    if (mulexp->attribute.T.type == Type::VOID || unary_exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("Modulo error on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT;
    } else if (mulexp->attribute.T.type == Type::FLOAT || unary_exp->attribute.T.type == Type::FLOAT) {
        error_msgs.push_back("Modulo error on float type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT;
    } else {
        attribute.T.type = Type::INT;
    }

    // 检查除以0的情况（模运算除数为0）
    if (unary_exp->attribute.V.ConstTag && unary_exp->attribute.V.val.IntVal == 0) {
        error_msgs.push_back("Modulo by zero at line " + std::to_string(line_number) + "\n");
    }

    // 常量折叠
    attribute.V.ConstTag = mulexp->attribute.V.ConstTag && unary_exp->attribute.V.ConstTag;
    if (attribute.V.ConstTag) {
        attribute.V.val.IntVal = mulexp->attribute.V.val.IntVal % unary_exp->attribute.V.val.IntVal;
    }

    TODO("BinaryExp Semant");
}

void RelExp_leq::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();

    TODO("BinaryExp Semant");
}

void RelExp_lt::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();

    TODO("BinaryExp Semant");
}

void RelExp_geq::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();

    TODO("BinaryExp Semant");
}

void RelExp_gt::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();

    TODO("BinaryExp Semant");
}

void EqExp_eq::TypeCheck() {
    eqexp->TypeCheck();
    relexp->TypeCheck();

    TODO("BinaryExp Semant");
}

void EqExp_neq::TypeCheck() {
    eqexp->TypeCheck();
    relexp->TypeCheck();

    TODO("BinaryExp Semant");
}

void LAndExp_and::TypeCheck() {
    landexp->TypeCheck();
    eqexp->TypeCheck();

    TODO("BinaryExp Semant");
}

void LOrExp_or::TypeCheck() {
    lorexp->TypeCheck();
    landexp->TypeCheck();

    TODO("BinaryExp Semant");
}

void ConstExp::TypeCheck() {
    addexp->TypeCheck();
    attribute = addexp->attribute;
    if (!attribute.V.ConstTag) {    // addexp is not const
        error_msgs.push_back("Expression is not const " + std::to_string(line_number) + "\n");
    }
}

void Lval::TypeCheck() { TODO("Lval Semant"); }

void FuncRParams::TypeCheck() { 
    auto param_vector = *params;
    for (auto param : param_vector) {
        param->TypeCheck();
    }
    
    TODO("FuncRParams Semant"); 
}

// 辅助函数，将 Type::ty 枚举转换为字符串表示
std::string type_to_string(Type::ty type) {
    switch (type) {
        case Type::VOID: return "void";
        case Type::INT: return "int";
        case Type::FLOAT: return "float";
        case Type::BOOL: return "bool";
        case Type::PTR: return "ptr";
        case Type::DOUBLE: return "double";
        default: return "unknown";
    }
}

void Func_call::TypeCheck() {
    // 获取函数的定义
    auto it = semant_table.FunctionTable.find(name);
    if (it == semant_table.FunctionTable.end()) {
        error_msgs.push_back("Error: Function '" + name->get_string() + "' is not defined at line " +
                             std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT; // 假设返回类型为 int，继续分析
        return;
    }

    auto func_def = it->second; // 获取函数定义

    // 检查实参数量是否匹配
    size_t params_count = 0;
    if (funcr_params != nullptr) {
        auto func_r_params = static_cast<FuncRParams*>(funcr_params);
        if (func_r_params->params != nullptr) {
            params_count = func_r_params->params->size(); // 获取参数数量

            if (params_count != func_def->formals->size()) {
                error_msgs.push_back("Error: Argument count mismatch in function call '" + name->get_string() + 
                                     "' at line " + std::to_string(line_number) + "\n");
            }

            // 遍历实参并进行类型匹配
            for (size_t i = 0; i < params_count; ++i) {
                // 对实参进行类型检查
                (*func_r_params->params)[i]->TypeCheck(); // 访问实际参数

                auto expected_type = func_def->formals->at(i)->attribute.T; // 获取形参的完整 Type 对象
                auto actual_type = (*func_r_params->params)[i]->attribute.T;

                // 检查实参类型与形参类型是否一致
                if (actual_type.type != expected_type.type) {
                    error_msgs.push_back("Error: Type mismatch for argument " + std::to_string(i) +
                                         " in function call '" + name->get_string() + "' (expected " + 
                                         type_to_string(expected_type.type) + ", got " + 
                                         type_to_string(actual_type.type) + ") at line " + 
                                         std::to_string(line_number) + "\n");
                }
            }
        }
    }

    // 设置返回类型为函数定义的返回类型
    attribute.T.type = func_def->attribute.T.type;
}


void UnaryExp_plus::TypeCheck() { 

    // 检查运算数类型是否为void
    if (unary_exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("UnaryExp_plus error on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT;
    } else {
        attribute.T.type = unary_exp->attribute.T.type;
    }

    // 常量折叠
    attribute.V = unary_exp->attribute.V;

    TODO("UnaryExp Semant"); 
}

void UnaryExp_neg::TypeCheck() { 
    // 检查运算数类型是否为void
    if (unary_exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("UnaryExp_neg error on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT;
    } else {
        attribute.T.type = unary_exp->attribute.T.type;
    }

    // 常量折叠
    attribute.V.ConstTag = unary_exp->attribute.V.ConstTag;
    if (attribute.V.ConstTag) {
        if (attribute.T.type == Type::INT) {
            attribute.V.val.IntVal = -unary_exp->attribute.V.val.IntVal;
        } else {
            attribute.V.val.FloatVal = -unary_exp->attribute.V.val.FloatVal;
        }
    }

    TODO("UnaryExp Semant"); 
}

void UnaryExp_not::TypeCheck() { 
    // 检查运算数类型是否为void
    if (unary_exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("UnaryExp_not error not on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT;
    } else {
        attribute.T.type = Type::INT; // 逻辑非运算结果为int类型
    }

    // 常量折叠
    attribute.V.ConstTag = unary_exp->attribute.V.ConstTag;
    if (attribute.V.ConstTag) {
        attribute.V.val.IntVal = !unary_exp->attribute.V.val.IntVal;
    }
    
    TODO("UnaryExp Semant"); 
}

void IntConst::TypeCheck() {
    attribute.T.type = Type::INT;
    attribute.V.ConstTag = true;
    attribute.V.val.IntVal = val;
}

void FloatConst::TypeCheck() {
    attribute.T.type = Type::FLOAT;
    attribute.V.ConstTag = true;
    attribute.V.val.FloatVal = val;
}

void StringConst::TypeCheck() { TODO("StringConst Semant"); }

void PrimaryExp_branch::TypeCheck() {
    exp->TypeCheck();
    attribute = exp->attribute;
}

void assign_stmt::TypeCheck() { TODO("AssignStmt Semant"); }

void expr_stmt::TypeCheck() {
    exp->TypeCheck();
    attribute = exp->attribute;
}

void block_stmt::TypeCheck() { b->TypeCheck(); }

void ifelse_stmt::TypeCheck() {
    Cond->TypeCheck();
    if (Cond->attribute.T.type == Type::VOID) {
        error_msgs.push_back("if cond type is invalid " + std::to_string(line_number) + "\n");
    }
    ifstmt->TypeCheck();
    elsestmt->TypeCheck();
}

void if_stmt::TypeCheck() {
    Cond->TypeCheck();
    if (Cond->attribute.T.type == Type::VOID) {
        error_msgs.push_back("if cond type is invalid " + std::to_string(line_number) + "\n");
    }
    ifstmt->TypeCheck();
}

void while_stmt::TypeCheck() { TODO("WhileStmt Semant"); }

void continue_stmt::TypeCheck() { TODO("ContinueStmt Semant"); }

void break_stmt::TypeCheck() { TODO("BreakStmt Semant"); }

void return_stmt::TypeCheck() { return_exp->TypeCheck(); }

void return_stmt_void::TypeCheck() {}

void ConstInitVal::TypeCheck() { TODO("ConstInitVal Semant"); }

void ConstInitVal_exp::TypeCheck() { TODO("ConstInitValExp Semant"); }

void VarInitVal::TypeCheck() { TODO("VarInitVal Semant"); }

void VarInitVal_exp::TypeCheck() { TODO("VarInitValExp Semant"); }

void VarDef_no_init::TypeCheck() { TODO("VarDefNoInit Semant"); }

void VarDef::TypeCheck() { TODO("VarDef Semant"); }

void ConstDef::TypeCheck() { TODO("ConstDef Semant"); }

void VarDecl::TypeCheck() { TODO("VarDecl Semant"); }

void ConstDecl::TypeCheck() { TODO("ConstDecl Semant"); }

void BlockItem_Decl::TypeCheck() { decl->TypeCheck(); }

void BlockItem_Stmt::TypeCheck() { stmt->TypeCheck(); }

void __Block::TypeCheck() {
    semant_table.symbol_table.enter_scope();
    auto item_vector = *item_list;
    for (auto item : item_vector) {
        item->TypeCheck();
    }
    semant_table.symbol_table.exit_scope();
}

void __FuncFParam::TypeCheck() {
    VarAttribute val;
    val.ConstTag = false;
    val.type = type_decl;
    scope = 1;

    // 如果dims为nullptr, 表示该变量不含数组下标, 如果你在语法分析中采用了其他方式处理，这里也需要更改
    if (dims != nullptr) {    
        auto dim_vector = *dims;

        // the fisrt dim of FuncFParam is empty
        // eg. int f(int A[][30][40])
        val.dims.push_back(-1);    // 这里用-1表示empty，你也可以使用其他方式
        for (int i = 1; i < dim_vector.size(); ++i) {
            auto d = dim_vector[i];
            d->TypeCheck();
            if (d->attribute.V.ConstTag == false) {
                error_msgs.push_back("Array Dim must be const expression in line " + std::to_string(line_number) +
                                     "\n");
            }
            if (d->attribute.T.type == Type::FLOAT) {
                error_msgs.push_back("Array Dim can not be float in line " + std::to_string(line_number) + "\n");
            }
            val.dims.push_back(d->attribute.V.val.IntVal);
        }
        attribute.T.type = Type::PTR;
    } else {
        attribute.T.type = type_decl;
    }

    if (name != nullptr) {
        if (semant_table.symbol_table.lookup_scope(name) != -1) {
            error_msgs.push_back("multiple difinitions of formals in function " + name->get_string() + " in line " +
                                 std::to_string(line_number) + "\n");
        }
        semant_table.symbol_table.add_Symbol(name, val);
    }
}

void __FuncDef::TypeCheck() {
    semant_table.symbol_table.enter_scope();

    semant_table.FunctionTable[name] = this;

    auto formal_vector = *formals;
    for (auto formal : formal_vector) {
        formal->TypeCheck();
    }

    // block TypeCheck
    if (block != nullptr) {
        auto item_vector = *(block->item_list);
        for (auto item : item_vector) {
            item->TypeCheck();
        }
    }

    semant_table.symbol_table.exit_scope();
}

void CompUnit_Decl::TypeCheck() { TODO("CompUnitDecl Semant"); }

void CompUnit_FuncDef::TypeCheck() { func_def->TypeCheck(); }