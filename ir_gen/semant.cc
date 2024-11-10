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
static bool MainFlag = false;
bool inside_loop = false;
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
    //没有main函数
    if (!MainFlag) {
        error_msgs.push_back("main function does not exist.\n");
    }
    semant_table.symbol_table.exit_scope();
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

    //TODO("BinaryExp Semant");
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

    //TODO("BinaryExp Semant");
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

    //TODO("BinaryExp Semant");
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
            return;
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

    //TODO("BinaryExp Semant");
}

void MulExp_mod::TypeCheck() {
    mulexp->TypeCheck();
    unary_exp->TypeCheck();

    // 检查运算数类型是否为void或float（模运算不支持float）
    if (mulexp->attribute.T.type == Type::VOID || unary_exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("Modulo error on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT;
        return;
    } else if (mulexp->attribute.T.type == Type::FLOAT || unary_exp->attribute.T.type == Type::FLOAT) {
        error_msgs.push_back("Modulo error on float type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT;
        return;
    } else {
        attribute.T.type = Type::INT;
    }

    // 检查除以0的情况（模运算除数为0）
    if (unary_exp->attribute.V.ConstTag && unary_exp->attribute.V.val.IntVal == 0) {
        error_msgs.push_back("Modulo by zero at line " + std::to_string(line_number) + "\n");
        return;
    }

    // 常量折叠
    attribute.V.ConstTag = mulexp->attribute.V.ConstTag && unary_exp->attribute.V.ConstTag;
    if (attribute.V.ConstTag) {
        attribute.V.val.IntVal = mulexp->attribute.V.val.IntVal % unary_exp->attribute.V.val.IntVal;
    }

    //TODO("BinaryExp Semant");
}

void RelExp_leq::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();

    // 获取左、右表达式的类型
    Type::ty leftType = relexp->attribute.T.type;
    Type::ty rightType = addexp->attribute.T.type;

    // 初始化比较结果的类型为布尔型
    attribute.T.type = Type::BOOL;

    // 处理相同类型的情况
    if (leftType == rightType) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，则直接计算结果
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal <= addexp->attribute.V.val.IntVal;
            } else if (leftType == Type::FLOAT) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal <= addexp->attribute.V.val.FloatVal;
            } else if (leftType == Type::BOOL) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.BoolVal <= addexp->attribute.V.val.BoolVal;
            }
        }
    }
    // 处理 int 和 float 的隐式转换
    else if ((leftType == Type::INT && rightType == Type::FLOAT) || (leftType == Type::FLOAT && rightType == Type::INT)) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，则进行计算
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                float leftVal = static_cast<float>(relexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = leftVal <= addexp->attribute.V.val.FloatVal;
            } else {
                float rightVal = static_cast<float>(addexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal <= rightVal;
            }
        }
    }
    // 处理 bool 和 int 的隐式转换
    else if ((leftType == Type::BOOL && rightType == Type::INT) || (leftType == Type::INT && rightType == Type::BOOL)) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，直接进行隐式转换并计算结果
        if (attribute.V.ConstTag) {
            int leftVal = (leftType == Type::BOOL) ? static_cast<int>(relexp->attribute.V.val.BoolVal) : relexp->attribute.V.val.IntVal;
            int rightVal = (rightType == Type::BOOL) ? static_cast<int>(addexp->attribute.V.val.BoolVal) : addexp->attribute.V.val.IntVal;
            attribute.V.val.BoolVal = leftVal <= rightVal;
        }
    }
    // 处理不兼容的类型组合
    else {
        error_msgs.push_back("Error: Type mismatch in <= operation at line " + std::to_string(line_number));
        attribute.T.type = Type::VOID; // 出错时设置类型为 void
    }
    //TODO("BinaryExp Semant");
}

void RelExp_lt::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();

    // 获取左、右表达式的类型
    Type::ty leftType = relexp->attribute.T.type;
    Type::ty rightType = addexp->attribute.T.type;

    // 初始化比较结果的类型为布尔型
    attribute.T.type = Type::BOOL;

    // 处理相同类型的情况
    if (leftType == rightType) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，则直接计算结果
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal < addexp->attribute.V.val.IntVal;
            } else if (leftType == Type::FLOAT) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal < addexp->attribute.V.val.FloatVal;
            } else if (leftType == Type::BOOL) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.BoolVal < addexp->attribute.V.val.BoolVal;
            } else {
                error_msgs.push_back("Error: Unsupported type for < operation at line " + std::to_string(line_number));
                attribute.T.type = Type::VOID; // 设置类型为 void 表示出错
            }
        }
    }
    // 处理 int 和 float 的隐式转换
    else if ((leftType == Type::INT && rightType == Type::FLOAT) || (leftType == Type::FLOAT && rightType == Type::INT)) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，则进行计算
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                float leftVal = static_cast<float>(relexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = leftVal < addexp->attribute.V.val.FloatVal;
            } else {
                float rightVal = static_cast<float>(addexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal < rightVal;
            }
        }
    }
    // 处理 bool 和 int 的隐式转换
    else if ((leftType == Type::BOOL && rightType == Type::INT) || (leftType == Type::INT && rightType == Type::BOOL)) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，直接进行隐式转换并计算结果
        if (attribute.V.ConstTag) {
            int leftVal = (leftType == Type::BOOL) ? static_cast<int>(relexp->attribute.V.val.BoolVal) : relexp->attribute.V.val.IntVal;
            int rightVal = (rightType == Type::BOOL) ? static_cast<int>(addexp->attribute.V.val.BoolVal) : addexp->attribute.V.val.IntVal;
            attribute.V.val.BoolVal = leftVal < rightVal;
        }
    }
    // 处理不兼容的类型组合
    else {
        error_msgs.push_back("Error: Type mismatch in < operation at line " + std::to_string(line_number));
        attribute.T.type = Type::VOID; // 出错时设置类型为 void
    }
    //TODO("BinaryExp Semant");
}

void RelExp_geq::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();
    // 获取左、右表达式的类型
    Type::ty leftType = relexp->attribute.T.type;
    Type::ty rightType = addexp->attribute.T.type;

    // 初始化比较结果的类型为布尔型
    attribute.T.type = Type::BOOL;

    // 处理相同类型的情况
    if (leftType == rightType) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，则直接计算结果
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal >= addexp->attribute.V.val.IntVal;
            } else if (leftType == Type::FLOAT) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal >= addexp->attribute.V.val.FloatVal;
            } else if (leftType == Type::BOOL) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.BoolVal >= addexp->attribute.V.val.BoolVal;
            } else {
                error_msgs.push_back("Error: Unsupported type for >= operation at line " + std::to_string(line_number));
                attribute.T.type = Type::VOID; // 设置类型为 void 表示出错
            }
        }
    }
    // 处理 int 和 float 的隐式转换
    else if ((leftType == Type::INT && rightType == Type::FLOAT) || (leftType == Type::FLOAT && rightType == Type::INT)) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，则进行计算
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                float leftVal = static_cast<float>(relexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = leftVal >= addexp->attribute.V.val.FloatVal;
            } else {
                float rightVal = static_cast<float>(addexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal >= rightVal;
            }
        }
    }
    // 处理 bool 和 int 的隐式转换
    else if ((leftType == Type::BOOL && rightType == Type::INT) || (leftType == Type::INT && rightType == Type::BOOL)) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，直接进行隐式转换并计算结果
        if (attribute.V.ConstTag) {
            int leftVal = (leftType == Type::BOOL) ? static_cast<int>(relexp->attribute.V.val.BoolVal) : relexp->attribute.V.val.IntVal;
            int rightVal = (rightType == Type::BOOL) ? static_cast<int>(addexp->attribute.V.val.BoolVal) : addexp->attribute.V.val.IntVal;
            attribute.V.val.BoolVal = leftVal >= rightVal;
        }
    }
    // 处理不兼容的类型组合
    else {
        error_msgs.push_back("Error: Type mismatch in >= operation at line " + std::to_string(line_number));
        attribute.T.type = Type::VOID; // 出错时设置类型为 void
    }
    //TODO("BinaryExp Semant");
}

void RelExp_gt::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();
    // 获取左、右表达式的类型
    Type::ty leftType = relexp->attribute.T.type;
    Type::ty rightType = addexp->attribute.T.type;

    // 初始化比较结果的类型为布尔型
    attribute.T.type = Type::BOOL;

    // 处理相同类型的情况
    if (leftType == rightType) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，则直接计算结果
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal > addexp->attribute.V.val.IntVal;
            } else if (leftType == Type::FLOAT) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal > addexp->attribute.V.val.FloatVal;
            } else if (leftType == Type::BOOL) {
                attribute.V.val.BoolVal = relexp->attribute.V.val.BoolVal > addexp->attribute.V.val.BoolVal;
            } else {
                error_msgs.push_back("Error: Unsupported type for > operation at line " + std::to_string(line_number));
                attribute.T.type = Type::VOID; // 设置类型为 void 表示出错
            }
        }
    }
    // 处理 int 和 float 的隐式转换
    else if ((leftType == Type::INT && rightType == Type::FLOAT) || (leftType == Type::FLOAT && rightType == Type::INT)) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，则进行计算
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                float leftVal = static_cast<float>(relexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = leftVal > addexp->attribute.V.val.FloatVal;
            } else {
                float rightVal = static_cast<float>(addexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal > rightVal;
            }
        }
    }
    // 处理 bool 和 int 的隐式转换
    else if ((leftType == Type::BOOL && rightType == Type::INT) || (leftType == Type::INT && rightType == Type::BOOL)) {
        attribute.V.ConstTag = relexp->attribute.V.ConstTag && addexp->attribute.V.ConstTag;

        // 如果是常量表达式，直接进行隐式转换并计算结果
        if (attribute.V.ConstTag) {
            int leftVal = (leftType == Type::BOOL) ? static_cast<int>(relexp->attribute.V.val.BoolVal) : relexp->attribute.V.val.IntVal;
            int rightVal = (rightType == Type::BOOL) ? static_cast<int>(addexp->attribute.V.val.BoolVal) : addexp->attribute.V.val.IntVal;
            attribute.V.val.BoolVal = leftVal > rightVal;
        }
    }
    // 处理不兼容的类型组合
    else {
        error_msgs.push_back("Error: Type mismatch in > operation at line " + std::to_string(line_number));
        attribute.T.type = Type::VOID; // 出错时设置类型为 void
    }
    //TODO("BinaryExp Semant");
}

void EqExp_eq::TypeCheck() {
    eqexp->TypeCheck();
    relexp->TypeCheck();
    // 获取左、右表达式的类型
    Type::ty leftType = eqexp->attribute.T.type;
    Type::ty rightType = relexp->attribute.T.type;

    // 初始化比较结果的类型为布尔型
    attribute.T.type = Type::BOOL;

    // 处理相同类型的情况
    if (leftType == rightType) {
        attribute.V.ConstTag = eqexp->attribute.V.ConstTag && relexp->attribute.V.ConstTag;

        // 如果是常量表达式，则直接计算结果
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                attribute.V.val.BoolVal = eqexp->attribute.V.val.IntVal == relexp->attribute.V.val.IntVal;
            } else if (leftType == Type::FLOAT) {
                attribute.V.val.BoolVal = eqexp->attribute.V.val.FloatVal == relexp->attribute.V.val.FloatVal;
            } else if (leftType == Type::BOOL) {
                attribute.V.val.BoolVal = eqexp->attribute.V.val.BoolVal == relexp->attribute.V.val.BoolVal;
            } else {
                error_msgs.push_back("Error: Unsupported type for == operation at line " + std::to_string(line_number));
                attribute.T.type = Type::VOID; // 设置类型为 void 表示出错
            }
        }
    }
    // 处理 int 和 float 的隐式转换
    else if ((leftType == Type::INT && rightType == Type::FLOAT) || (leftType == Type::FLOAT && rightType == Type::INT)) {
        attribute.V.ConstTag = eqexp->attribute.V.ConstTag && relexp->attribute.V.ConstTag;

        // 如果是常量表达式，则进行计算
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                float leftVal = static_cast<float>(eqexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = leftVal == relexp->attribute.V.val.FloatVal;
            } else {
                float rightVal = static_cast<float>(relexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = eqexp->attribute.V.val.FloatVal == rightVal;
            }
        }
    }
    // 处理 bool 和 int 的隐式转换
    else if ((leftType == Type::BOOL && rightType == Type::INT) || (leftType == Type::INT && rightType == Type::BOOL)) {
        attribute.V.ConstTag = eqexp->attribute.V.ConstTag && relexp->attribute.V.ConstTag;

        // 如果是常量表达式，直接进行隐式转换并计算结果
        if (attribute.V.ConstTag) {
            int leftVal = (leftType == Type::BOOL) ? static_cast<int>(eqexp->attribute.V.val.BoolVal) : eqexp->attribute.V.val.IntVal;
            int rightVal = (rightType == Type::BOOL) ? static_cast<int>(relexp->attribute.V.val.BoolVal) : relexp->attribute.V.val.IntVal;
            attribute.V.val.BoolVal = leftVal == rightVal;
        }
    }
    // 处理不兼容的类型组合
    else {
        error_msgs.push_back("Error: Type mismatch in == operation at line " + std::to_string(line_number));
        attribute.T.type = Type::VOID; // 出错时设置类型为 void
    }
    //TODO("BinaryExp Semant");
}

void EqExp_neq::TypeCheck() {
    eqexp->TypeCheck();
    relexp->TypeCheck();
    // 获取左、右表达式的类型
    Type::ty leftType = eqexp->attribute.T.type;
    Type::ty rightType = relexp->attribute.T.type;

    // 初始化比较结果的类型为布尔型
    attribute.T.type = Type::BOOL;

    // 处理相同类型的情况
    if (leftType == rightType) {
        attribute.V.ConstTag = eqexp->attribute.V.ConstTag && relexp->attribute.V.ConstTag;

        // 如果是常量表达式，则直接计算结果
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                attribute.V.val.BoolVal = eqexp->attribute.V.val.IntVal != relexp->attribute.V.val.IntVal;
            } else if (leftType == Type::FLOAT) {
                attribute.V.val.BoolVal = eqexp->attribute.V.val.FloatVal != relexp->attribute.V.val.FloatVal;
            } else if (leftType == Type::BOOL) {
                attribute.V.val.BoolVal = eqexp->attribute.V.val.BoolVal != relexp->attribute.V.val.BoolVal;
            } else {
                error_msgs.push_back("Error: Unsupported type for != operation at line " + std::to_string(line_number));
                attribute.T.type = Type::VOID; // 设置类型为 void 表示出错
            }
        }
    }
    // 处理 int 和 float 的隐式转换
    else if ((leftType == Type::INT && rightType == Type::FLOAT) || (leftType == Type::FLOAT && rightType == Type::INT)) {
        attribute.V.ConstTag = eqexp->attribute.V.ConstTag && relexp->attribute.V.ConstTag;

        // 如果是常量表达式，则进行计算
        if (attribute.V.ConstTag) {
            if (leftType == Type::INT) {
                float leftVal = static_cast<float>(eqexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = leftVal != relexp->attribute.V.val.FloatVal;
            } else {
                float rightVal = static_cast<float>(relexp->attribute.V.val.IntVal);
                attribute.V.val.BoolVal = eqexp->attribute.V.val.FloatVal != rightVal;
            }
        }
    }
    // 处理 bool 和 int 的隐式转换
    else if ((leftType == Type::BOOL && rightType == Type::INT) || (leftType == Type::INT && rightType == Type::BOOL)) {
        attribute.V.ConstTag = eqexp->attribute.V.ConstTag && relexp->attribute.V.ConstTag;

        // 如果是常量表达式，直接进行隐式转换并计算结果
        if (attribute.V.ConstTag) {
            int leftVal = (leftType == Type::BOOL) ? static_cast<int>(eqexp->attribute.V.val.BoolVal) : eqexp->attribute.V.val.IntVal;
            int rightVal = (rightType == Type::BOOL) ? static_cast<int>(relexp->attribute.V.val.BoolVal) : relexp->attribute.V.val.IntVal;
            attribute.V.val.BoolVal = leftVal != rightVal;
        }
    }
    // 处理不兼容的类型组合
    else {
        error_msgs.push_back("Error: Type mismatch in != operation at line " + std::to_string(line_number));
        attribute.T.type = Type::VOID; // 出错时设置类型为 void
    }
    //TODO("BinaryExp Semant");
}

void LAndExp_and::TypeCheck() {
    landexp->TypeCheck();
    eqexp->TypeCheck();
    // 获取左右操作数的类型
    Type::ty leftType = landexp->attribute.T.type;
    Type::ty rightType = eqexp->attribute.T.type;

    // 初始化结果类型为布尔型
    attribute.T.type = Type::BOOL;

    // 处理 bool 类型
    if (leftType == Type::BOOL && rightType == Type::BOOL) {
        attribute.V.ConstTag = landexp->attribute.V.ConstTag && eqexp->attribute.V.ConstTag;

        // 如果是常量表达式，则直接计算结果
        if (attribute.V.ConstTag) {
            attribute.V.val.BoolVal = landexp->attribute.V.val.BoolVal && eqexp->attribute.V.val.BoolVal;
        }
    }
    // 处理 int 和 bool 的隐式转换
    else if ((leftType == Type::INT && rightType == Type::BOOL) || (leftType == Type::BOOL && rightType == Type::INT)) {
        attribute.V.ConstTag = landexp->attribute.V.ConstTag && eqexp->attribute.V.ConstTag;

        // 如果是常量表达式，直接进行隐式转换并计算结果
        if (attribute.V.ConstTag) {
            int leftVal = (leftType == Type::BOOL) ? static_cast<int>(landexp->attribute.V.val.BoolVal) : landexp->attribute.V.val.IntVal;
            int rightVal = (rightType == Type::BOOL) ? static_cast<int>(eqexp->attribute.V.val.BoolVal) : eqexp->attribute.V.val.IntVal;
            attribute.V.val.BoolVal = (leftVal != 0) && (rightVal != 0);
        }
    }
    // 处理 int 和 float 的隐式转换
    else if ((leftType == Type::INT && rightType == Type::FLOAT) || (leftType == Type::FLOAT && rightType == Type::INT)) {
        attribute.V.ConstTag = landexp->attribute.V.ConstTag && eqexp->attribute.V.ConstTag;

        // 如果是常量表达式，则进行计算
        if (attribute.V.ConstTag) {
            float leftVal = (leftType == Type::INT) ? static_cast<float>(landexp->attribute.V.val.IntVal) : landexp->attribute.V.val.FloatVal;
            float rightVal = (rightType == Type::INT) ? static_cast<float>(eqexp->attribute.V.val.IntVal) : eqexp->attribute.V.val.FloatVal;
            attribute.V.val.BoolVal = (leftVal != 0.0f) && (rightVal != 0.0f);
        }
    }
    // 处理不兼容的类型组合
    else if ((leftType == Type::INT && rightType == Type::INT) || (leftType == Type::FLOAT && rightType == Type::FLOAT)) {
        attribute.V.ConstTag = landexp->attribute.V.ConstTag && eqexp->attribute.V.ConstTag;

        // 如果是常量表达式，直接进行计算
        if (attribute.V.ConstTag) {
            int leftVal = (leftType == Type::INT) ? landexp->attribute.V.val.IntVal : static_cast<int>(landexp->attribute.V.val.FloatVal);
            int rightVal = (rightType == Type::INT) ? eqexp->attribute.V.val.IntVal : static_cast<int>(eqexp->attribute.V.val.FloatVal);
            attribute.V.val.BoolVal = (leftVal != 0) && (rightVal != 0);
        }
    }
    // 处理不兼容的类型组合
    else {
        error_msgs.push_back("Error: Both operands of && must be of type bool at line " + std::to_string(line_number));
        attribute.T.type = Type::VOID; // 出错时设置类型为 void
    }
    //TODO("BinaryExp Semant");
}

void LOrExp_or::TypeCheck() {
    lorexp->TypeCheck();
    landexp->TypeCheck();
    // 获取左右操作数的类型
    Type::ty leftType = lorexp->attribute.T.type;
    Type::ty rightType = landexp->attribute.T.type;

    // 初始化结果类型为布尔型
    attribute.T.type = Type::BOOL;

    // 处理 bool 类型
    if (leftType == Type::BOOL && rightType == Type::BOOL) {
        attribute.V.ConstTag = lorexp->attribute.V.ConstTag && landexp->attribute.V.ConstTag;

        // 如果是常量表达式，则直接计算结果
        if (attribute.V.ConstTag) {
            attribute.V.val.BoolVal = lorexp->attribute.V.val.BoolVal || landexp->attribute.V.val.BoolVal;
        }
    }
    // 处理 int 和 bool 的隐式转换
    else if ((leftType == Type::INT && rightType == Type::BOOL) || (leftType == Type::BOOL && rightType == Type::INT)) {
        attribute.V.ConstTag = lorexp->attribute.V.ConstTag && landexp->attribute.V.ConstTag;

        // 如果是常量表达式，直接进行隐式转换并计算结果
        if (attribute.V.ConstTag) {
            int leftVal = (leftType == Type::BOOL) ? static_cast<int>(lorexp->attribute.V.val.BoolVal) : lorexp->attribute.V.val.IntVal;
            int rightVal = (rightType == Type::BOOL) ? static_cast<int>(landexp->attribute.V.val.BoolVal) : landexp->attribute.V.val.IntVal;
            attribute.V.val.BoolVal = (leftVal != 0) || (rightVal != 0); // 非零视为 true
        }
    }
    // 处理 int 和 float 的隐式转换
    else if ((leftType == Type::INT && rightType == Type::FLOAT) || (leftType == Type::FLOAT && rightType == Type::INT)) {
        attribute.V.ConstTag = lorexp->attribute.V.ConstTag && landexp->attribute.V.ConstTag;

        // 如果是常量表达式，则进行计算
        if (attribute.V.ConstTag) {
            float leftVal = (leftType == Type::INT) ? static_cast<float>(lorexp->attribute.V.val.IntVal) : lorexp->attribute.V.val.FloatVal;
            float rightVal = (rightType == Type::INT) ? static_cast<float>(landexp->attribute.V.val.IntVal) : landexp->attribute.V.val.FloatVal;
            attribute.V.val.BoolVal = (leftVal != 0.0f) || (rightVal != 0.0f); // 非零视为 true
        }
    }
    // 处理不兼容的类型组合
    else {
        error_msgs.push_back("Error: Both operands of || must be of type bool at line " + std::to_string(line_number));
        attribute.T.type = Type::VOID; // 出错时设置类型为 void
    }
    //TODO("BinaryExp Semant");
}

void ConstExp::TypeCheck() {
    addexp->TypeCheck();
    attribute = addexp->attribute;
    if (!attribute.V.ConstTag) {    // addexp is not const
        error_msgs.push_back("Expression is not const " + std::to_string(line_number) + "\n");
    }
}

void Lval::TypeCheck() { 
    VarAttribute val;
    // 检查变量是否在本地作用域中声明
    auto global_entry = semant_table.GlobalTable.find(name);
    if (global_entry != semant_table.GlobalTable.end()) {
        // 如果在全局作用域中找到变量，则直接使用全局定义
        scope = 0; // 设置作用域为全局作用域
        attribute.V.ConstTag = global_entry->second.ConstTag; // 使用全局常量标记
        val = global_entry->second; // 从全局表中获取变量属性
    } else {
        // 如果全局表中没有该变量，检查本地作用域
        int local_scope = semant_table.symbol_table.lookup_scope(name);
        if (local_scope != -1) {
            // 变量在本地作用域中
            scope = local_scope;
            attribute.V.ConstTag = false; // 本地变量默认不可变
            val = semant_table.symbol_table.lookup_val(name);
        } else {
            // 如果在任何作用域中都未找到，记录错误并返回
            error_msgs.push_back("Error: Variable '" + name->get_string() + "' is not defined at line " + std::to_string(line_number) + "\n");
            attribute.T.type = Type::UNDEFINED;
            return;
        }
    }

    // 设置类型信息
    attribute.T.type = val.type;
    attribute.V.ConstTag = val.ConstTag;
    /*// 检查是否为常量，常量不能作为左值
    if (attribute.V.ConstTag) {
        error_msgs.push_back("Error: Constant '" + name->get_string() + "' cannot be assigned to at line " + std::to_string(line_number) + "\n");
        return;
    }*/

    // 数组维度检查
    if (dims != nullptr) {
        if (val.dims.size() != dims->size()) {
            error_msgs.push_back("Error: Array dimension mismatch for variable '" + name->get_string() + "' at line " + std::to_string(line_number) + "\n");
        } else {
            for (auto dim : *dims) {
                dim->TypeCheck();
                if (!dim->attribute.V.ConstTag || dim->attribute.T.type != Type::INT) {
                    error_msgs.push_back("Error: Array index must be a constant integer for variable '" + name->get_string() + "' at line " + std::to_string(line_number) + "\n");
                }
            }
        }
    } else if (!val.dims.empty()) {
        error_msgs.push_back("Error: Array variable '" + name->get_string() + "' used without index at line " + std::to_string(line_number) + "\n");
    }
    //TODO("Lval Semant"); 
}

void FuncRParams::TypeCheck() { 
    auto param_vector = *params;
    for (auto param : param_vector) {
        param->TypeCheck();
    }
    
    //TODO("FuncRParams Semant"); 
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
        error_msgs.push_back("Error: Function '" + name->get_string() +
                             "' is not defined at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT; // 假设返回类型为 int，继续分析
        return;
    }

    auto func_def = it->second; // 获取函数定义

    // 获取形参数量
    size_t formals_count = func_def->formals->size();

    // 获取实参数量
    size_t params_count = 0;
    FuncRParams* func_r_params = nullptr;
    if (funcr_params != nullptr) {
        func_r_params = static_cast<FuncRParams*>(funcr_params);
        if (func_r_params->params != nullptr) {
            params_count = func_r_params->params->size();
        }
    }

    // 检查实参与形参数量是否匹配
    if (params_count != formals_count) {
        error_msgs.push_back("Error: Argument count mismatch in function call '" + name->get_string() + "' at line " + std::to_string(line_number) + "\n");
    }

    // 检查实参类型与形参类型是否一致
    size_t min_count = std::min(params_count, formals_count);
    for (size_t i = 0; i < min_count; ++i) {
        // 对实参进行类型检查
        (*func_r_params->params)[i]->TypeCheck(); // 访问实际参数

        auto expected_type = func_def->formals->at(i)->type_decl; // 获取形参的类型
        auto actual_type = (*func_r_params->params)[i]->attribute.T;

        // 检查实参类型是否与形参类型一致
        if (actual_type.type != expected_type) {
            error_msgs.push_back("Error: Type mismatch for argument " + std::to_string(i + 1) +
                                 " in function call '" + name->get_string() + "' (expected " +
                                 type_to_string(expected_type) + ", got " +
                                 type_to_string(actual_type.type) + ") at line " +
                                 std::to_string(line_number) + "\n");
        }
    }

    // 设置返回类型为函数定义的返回类型
    attribute.T.type = func_def->return_type;
}




void UnaryExp_plus::TypeCheck() { 

    // 检查运算数类型是否为 void
    unary_exp->TypeCheck();  // 确保获取运算数的类型

    if (unary_exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("UnaryExp_plus error on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT; // 这里返回类型也可以为 void，但通常加法结果为 int
    } else {
        attribute.T.type = unary_exp->attribute.T.type; // 设置为运算数的类型
    }

    // 常量折叠
    attribute.V = unary_exp->attribute.V; // 直接赋值
    //TODO("UnaryExp Semant"); 
}

void UnaryExp_neg::TypeCheck() { 
    // 检查运算数类型是否为 void
    unary_exp->TypeCheck();

    if (unary_exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("UnaryExp_neg error on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::INT; // 这里可以设置为 void，但通常返回类型为 int
    } else {
        attribute.T.type = unary_exp->attribute.T.type; // 设置为运算数的类型
    }

    // 常量折叠
    attribute.V.ConstTag = unary_exp->attribute.V.ConstTag;
    if (attribute.V.ConstTag) {
        if (attribute.T.type == Type::INT) {
            attribute.V.val.IntVal = -unary_exp->attribute.V.val.IntVal; // 处理 int 类型
        } else if (attribute.T.type == Type::FLOAT) {
            attribute.V.val.FloatVal = -unary_exp->attribute.V.val.FloatVal; // 处理 float 类型
        }
    }

    //TODO("UnaryExp Semant"); 
}

void UnaryExp_not::TypeCheck() { 
    // 检查运算数类型
    unary_exp->TypeCheck();

    // 检查运算数是否为 void
    if (unary_exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("UnaryExp_not error on void type at line " + std::to_string(line_number) + "\n");
        attribute.T.type = Type::BOOL; // 逻辑非的结果类型应为 bool
    } else {
        // 对于 int 类型的逻辑非处理
        if (unary_exp->attribute.T.type == Type::INT) {
            attribute.T.type = Type::BOOL; // 逻辑非运算结果为 bool
        } else if (unary_exp->attribute.T.type == Type::BOOL) {
            attribute.T.type = Type::BOOL; // 保持 bool 类型
        } else {
            error_msgs.push_back("Error: Unsupported type for unary ! operator at line " + std::to_string(line_number));
            attribute.T.type = Type::VOID; // 设置为 void
            return;
        }
    }

    // 常量折叠
    attribute.V.ConstTag = unary_exp->attribute.V.ConstTag;
    if (attribute.V.ConstTag) {
        if (unary_exp->attribute.T.type == Type::INT) {
            attribute.V.val.BoolVal = (unary_exp->attribute.V.val.IntVal == 0) ? true : false; // 将 int 转为 bool
        } else if (unary_exp->attribute.T.type == Type::BOOL) {
            attribute.V.val.BoolVal = !unary_exp->attribute.V.val.BoolVal; // 直接取反
        }
    }
    
    //TODO("UnaryExp Semant"); 
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

void StringConst::TypeCheck() { //TODO("StringConst Semant"); 
}

void PrimaryExp_branch::TypeCheck() {
    exp->TypeCheck();
    attribute = exp->attribute;
}

void assign_stmt::TypeCheck() { 
    // 检查左值变量是否已声明
    if (lval == nullptr) {
        error_msgs.push_back("Left-hand side of assignment is missing at line " + std::to_string(line_number) + "\n");
        return;
    }

    lval->TypeCheck(); // 对左值变量进行类型检查，确保其已声明并有效

    // 检查右侧表达式是否存在
    if (exp == nullptr) {
        error_msgs.push_back("Right-hand side of assignment is missing at line " + std::to_string(line_number) + "\n");
        return;
    }

    exp->TypeCheck(); // 对右值表达式进行类型检查

    // 设置类型变量，便于后续检查
    Type::ty left_type = lval->attribute.T.type;
    Type::ty right_type = exp->attribute.T.type;

    // 检查左值是否为常量，常量不能被赋值
    if (lval->attribute.V.ConstTag) {
        error_msgs.push_back("Error: Left-hand side cannot be a constant at line " + std::to_string(line_number) + "\n");
        return;
    }

    // 检查右值是否为 VOID 类型
    if (right_type == Type::VOID) {
        error_msgs.push_back("Error: Right-hand side cannot be of type 'void' at line " + std::to_string(line_number) + "\n");
        return;
    }

    // 检查左值和右值的类型是否一致或可以隐式转换
    if (left_type != right_type) {
        // 允许 int 转换为 bool
        if (left_type == Type::BOOL && right_type == Type::INT) {
            // 设置赋值语句类型为左值的类型
            attribute.T.type = left_type;
            attribute.V.ConstTag = false;
        }
        // 允许 bool 转换为 int
        else if (left_type == Type::INT && right_type == Type::BOOL) {
            // 设置赋值语句类型为左值的类型
            attribute.T.type = left_type;
            attribute.V.ConstTag = false;
        } else {
            error_msgs.push_back("Error: Type mismatch in assignment at line " + std::to_string(line_number) + "\n");
            return;
        }
    }

    //TODO("AssignStmt Semant"); 
}

void expr_stmt::TypeCheck() {
    exp->TypeCheck();
    attribute = exp->attribute;
}

void block_stmt::TypeCheck() { b->TypeCheck(); }

void ifelse_stmt::TypeCheck() {
    Cond->TypeCheck();
    if (Cond->attribute.T.type == Type::VOID) {
        error_msgs.push_back("if cond type is invalid " + std::to_string(Cond->GetLineNumber()) + "\n");
    }
    ifstmt->TypeCheck();
    elsestmt->TypeCheck();
}

void if_stmt::TypeCheck() {
    Cond->TypeCheck();
    if (Cond->attribute.T.type == Type::VOID) {
        error_msgs.push_back("if cond type is invalid " + std::to_string(Cond->GetLineNumber()) + "\n");
    }
    ifstmt->TypeCheck();
}

void while_stmt::TypeCheck() {
    inside_loop = true;
    Cond->TypeCheck();
    if (Cond->attribute.T.type == Type::VOID) {
        error_msgs.push_back("while cond type is invalid " + std::to_string(Cond->GetLineNumber()) + "\n");
    }
    body->TypeCheck();
    inside_loop = false;
    //TODO("WhileStmt Semant"); 
}

void continue_stmt::TypeCheck() {
    if (!inside_loop) {
        error_msgs.push_back("continue语句不在循环内 " + std::to_string(line_number) + "\n");
    } 
    //TODO("ContinueStmt Semant"); 
}

void break_stmt::TypeCheck() { 
    if (!inside_loop) {
        error_msgs.push_back("break语句不在循环内 " + std::to_string(line_number) + "\n");
    }
    //TODO("BreakStmt Semant"); 
}

void return_stmt::TypeCheck() { 
    return_exp->TypeCheck();

    if (return_exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("return type is invalid in line " + std::to_string(line_number) + "\n");
    }
}

void return_stmt_void::TypeCheck() {}

void ConstInitVal::TypeCheck() { 
    for (auto init_val : *initval) {
        init_val->TypeCheck();
        
        // 检查每个初始化值是否为常量表达式
        if (!init_val->attribute.V.ConstTag) {
            error_msgs.push_back("Initializer must be a constant expression in ConstInitVal at line " + std::to_string(line_number) + "\n");
        }
    }
    //TODO("ConstInitVal Semant"); 
}

void ConstInitVal_exp::TypeCheck() { 
    if (exp == nullptr) {
        return;
    }
    exp->TypeCheck();
    attribute = exp->attribute;
    // 检查表达式是否为常量
    if (exp->attribute.V.ConstTag == false) {
        error_msgs.push_back("Initializer expression must be a constant in ConstInitVal_exp at line " + std::to_string(line_number) + "\n");
    }
    //TODO("ConstInitValExp Semant"); 
}

void VarInitVal::TypeCheck() { 
    for (auto init_val : *initval) {
        init_val->TypeCheck();
        
        // 检查每个初始化值的类型是否匹配变量的类型（例如int、float等）
        if (init_val->attribute.T.type != attribute.T.type) {
            error_msgs.push_back("Error: Type mismatch in VarInitVal initializer at line " + std::to_string(line_number) + "\n");
        }
    }
    //TODO("VarInitVal Semant"); 
}

void VarInitVal_exp::TypeCheck() { 
    if (exp == nullptr) {
        return;
    }
    exp->TypeCheck();
    attribute = exp->attribute;
    // 检查初始化值的类型是否为 void
    if (attribute.T.type == Type::VOID) {
        error_msgs.push_back("Error: Variable initializer expression cannot be of type void at line " + std::to_string(line_number) + "\n");
    }
    //TODO("VarInitValExp Semant"); 
}

void VarDef_no_init::TypeCheck() { 
    // 检查数组维度是否合法
    if (dims != nullptr) {
        for (auto d : *dims) {
            d->TypeCheck();
            if (!d->attribute.V.ConstTag) {
                error_msgs.push_back("Array dimension must be a constant expression for variable '" + name->get_string() + "' at line " + std::to_string(line_number) + "\n");
            }
            if (d->attribute.T.type == Type::FLOAT) {
                error_msgs.push_back("Array dimension cannot be of type float for variable '" + name->get_string() + "' at line " + std::to_string(line_number) + "\n");
            }
        }
    }

    // 将未初始化的变量添加到符号表
    VarAttribute var_attr;
    var_attr.type = attribute.T.type;
    var_attr.ConstTag = false;
    if (dims != nullptr) {
        for (auto d : *dims) {
            var_attr.dims.push_back(d->attribute.V.val.IntVal); // 存储数组维度
        }
    }

    // 检查是否在当前作用域中重复定义
    if (semant_table.symbol_table.lookup_scope(name) == semant_table.symbol_table.get_current_scope()) {
        error_msgs.push_back("Variable '" + name->get_string() + "' is already defined in the current scope at line " + std::to_string(line_number) + "\n");
    } else {
        semant_table.symbol_table.add_Symbol(name, var_attr);
    }
    //TODO("VarDefNoInit Semant"); 
}

void VarDef::TypeCheck() {
    int current_scope = semant_table.symbol_table.get_current_scope();
    // 检查是否在当前作用域中重复定义
    if (semant_table.symbol_table.lookup_scope(name) == current_scope)  {
        // 错误：全局作用域中已有同名变量
        error_msgs.push_back("Variable '" + name->get_string() + "' is already defined in the current scope at line " + std::to_string(line_number) + "\n");
        return;
    }
    // 检查数组维度是否合法
    if (dims != nullptr) {
        for (auto d : *dims) {
            d->TypeCheck();
            if (!d->attribute.V.ConstTag) {
                error_msgs.push_back("Array dimension must be a constant expression for variable '" + name->get_string() + "' at line " + std::to_string(line_number) + "\n");
            }
            if (d->attribute.T.type == Type::FLOAT) {
                error_msgs.push_back("Array dimension cannot be of type float for variable '" + name->get_string() + "' at line " + std::to_string(line_number) + "\n");
            }
        }
    }

    // 检查并处理初始化值
    if (init != nullptr) {
        init->TypeCheck();

        if (init->attribute.T.type != attribute.T.type && init->attribute.T.type != Type::VOID) {
            error_msgs.push_back("Type mismatch in initializer for variable '" + name->get_string() + "' at line " + std::to_string(line_number) + "\n");
        }
    }

    // 将变量添加到符号表中
    VarAttribute var_attr;
    var_attr.type = attribute.T.type;
    var_attr.ConstTag = false;
    if (dims != nullptr) {
        for (auto d : *dims) {
            var_attr.dims.push_back(d->attribute.V.val.IntVal); // 存储数组维度
        }
    }
    semant_table.symbol_table.add_Symbol(name, var_attr);
    //TODO("VarDef Semant"); 
}

void ConstDef::TypeCheck() { 
    // 检查是否在当前作用域中重复定义
    int current_scope = semant_table.symbol_table.get_current_scope();
    if (semant_table.symbol_table.lookup_scope(name) == current_scope) {
        error_msgs.push_back("Constant '" + name->get_string() + "' is already defined in the current scope at line " + std::to_string(line_number) + "\n");
        return;
    }
    // 检查数组维度是否合法
    if (dims != nullptr) {
        for (auto d : *dims) {
            d->TypeCheck();
            if (!d->attribute.V.ConstTag) {
                error_msgs.push_back("Array dimension must be a constant expression for '" + name->get_string() + "' at line " + std::to_string(line_number) + "\n");
            }
            if (d->attribute.T.type == Type::FLOAT) {
                error_msgs.push_back("Array dimension cannot be of type float for '" + name->get_string() + "' at line " + std::to_string(line_number) + "\n");
            }
        }
    }
    VarAttribute const_attr;
    const_attr.type = attribute.T.type;
    const_attr.ConstTag = true;
    // 如果有初始化值，检查初始化值的类型是否匹配
    if (init != nullptr) {
        init->TypeCheck();

        // 确保初始化值为常量表达式
        if (!init->attribute.V.ConstTag) {
            error_msgs.push_back("Initializer for constant '" + name->get_string() + "' must be a constant expression at line " + std::to_string(line_number) + "\n");
        } else if (init->attribute.T.type != const_attr.type) {
            // 只有在初始化值存在且类型不匹配时，才输出类型不匹配的错误
            error_msgs.push_back("Type mismatch in initializer for constant '" + name->get_string() + "' at line " + std::to_string(line_number) + "\n");
        } else {
            if(init->attribute.T.type == Type::INT) {
                const_attr.IntInitVals.push_back(init->attribute.V.val.IntVal);
            } else if(init->attribute.T.type == Type::FLOAT) {
                const_attr.FloatInitVals.push_back(init->attribute.V.val.FloatVal);
            }
        }
    }

    if (dims != nullptr) {
        for (auto d : *dims) {
            const_attr.dims.push_back(d->attribute.V.val.IntVal); // 存储数组维度
        }
    }
    semant_table.symbol_table.add_Symbol(name, const_attr);
}

void VarDecl::TypeCheck() { 
    // 遍历变量声明列表中的每个变量定义
    for (auto def : *var_def_list) {
        /*auto var_def = dynamic_cast<VarDef*>(def);
        if (!var_def) {
            error_msgs.push_back("Invalid variable definition at line " + std::to_string(line_number) + "\n");
            continue;
        }*/

        // 设置变量定义的类型为 VarDecl 声明的类型
        def->attribute.T.type = type_decl;

        // 调用 VarDef 的 TypeCheck 方法来检查单个变量定义
        def->TypeCheck();
    }
    //TODO("VarDecl Semant"); 
}

void ConstDecl::TypeCheck() { 
    // 遍历常量声明列表中的每个常量定义
    for (auto def : *var_def_list) {
        /*auto const_def = dynamic_cast<ConstDef*>(def);
        if (!const_def) {
            error_msgs.push_back("Invalid constant definition at line " + std::to_string(line_number) + "\n");
            continue;
        }*/
        
        // 设置常量定义的类型为 ConstDecl 声明的类型
        def->attribute.T.type = type_decl;
        
        // 调用 ConstDef 的 TypeCheck 方法来检查单个常量定义
        def->TypeCheck();
    }
    //TODO("ConstDecl Semant"); 
}

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
    attribute.T.type = return_type;
    //main函数检查
    if (name->get_string() == "main") {
        MainFlag = true;
    }
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

void CompUnit_Decl::TypeCheck() { 
    // 首先检查声明类型是否为常量声明或变量声明
    if (auto var_decl = dynamic_cast<VarDecl*>(decl)) {
        var_decl->TypeCheck();
        // 如果是变量声明，检查每个变量的重复声明
        for (auto &var_def : *(var_decl->var_def_list)) {
            auto var_def_cast = dynamic_cast<VarDef*>(var_def);
            if (!var_def_cast) {
                error_msgs.push_back("Invalid variable definition at line " + std::to_string(line_number) + "\n");
                continue;
            }
            Symbol var_name = var_def_cast->name;
            // 检查是否在全局作用域中重复声明
            if (semant_table.GlobalTable.find(var_name) == semant_table.GlobalTable.end()) {
                // 将全局变量添加到 GlobalTable 中
                VarAttribute var_attr;
                var_attr.type = var_def_cast->attribute.T.type;
                var_attr.ConstTag = false; // 标记为非常量
                semant_table.GlobalTable[var_name] = var_attr;
            }

            // 对变量初始化进行类型检查
            //var_def->TypeCheck();
        }
    } else if (auto const_decl = dynamic_cast<ConstDecl*>(decl)) {
        const_decl->TypeCheck();
        // 如果是常量声明，检查每个常量的重复声明
        for (auto const_def : *(const_decl->var_def_list)) {
            auto const_def_cast = dynamic_cast<ConstDef*>(const_def);
            if (!const_def_cast) {
                error_msgs.push_back("Invalid constant definition at line " + std::to_string(line_number) + "\n");
                continue;
            }
            Symbol const_name = const_def_cast->name;

            // 检查是否在全局作用域中重复声明
            if (semant_table.GlobalTable.find(const_name) == semant_table.GlobalTable.end()) {
                // 将全局常量添加到 GlobalTable 中
                VarAttribute const_attr;
                const_attr.type = const_def_cast->attribute.T.type;
                const_attr.ConstTag = true; // 标记为常量
                semant_table.GlobalTable[const_name] = const_attr;
            }

            // 对常量初始化进行类型检查
            //const_def->TypeCheck();
        }
    } else {
        // 如果 decl 既不是变量声明也不是常量声明，报告错误
        error_msgs.push_back("Unknown declaration type at line " + std::to_string(line_number) + "\n");
    }
    //TODO("CompUnitDecl Semant");
}

void CompUnit_FuncDef::TypeCheck() { func_def->TypeCheck(); }

