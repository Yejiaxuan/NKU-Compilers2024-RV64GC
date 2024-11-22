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
int InWhileCount = 0;
static int GlobalStrCnt = 0;
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
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = addexp->attribute.V.ConstTag & mulexp->attribute.V.ConstTag;

    switch(addexp->attribute.T.type) {
        case Type::INT:
            switch(mulexp->attribute.T.type) {
                case Type::INT:    // int + int
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = addexp->attribute.V.val.IntVal + 
                                               mulexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int + float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = (float)addexp->attribute.V.val.IntVal + 
                                                 mulexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int + bool
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = addexp->attribute.V.val.IntVal + 
                                               (mulexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                case Type::PTR:    // int + ptr
                case Type::VOID:   // int + void
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for + in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(mulexp->attribute.T.type) {
                case Type::INT:    // float + int
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = addexp->attribute.V.val.FloatVal + 
                                                 (float)mulexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float + float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = addexp->attribute.V.val.FloatVal + 
                                                 mulexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float + bool
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = addexp->attribute.V.val.FloatVal + 
                                                 (mulexp->attribute.V.val.BoolVal ? 1.0f : 0.0f);
                    }
                    break;
                    
                case Type::PTR:    // float + ptr
                case Type::VOID:   // float + void
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for + in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(mulexp->attribute.T.type) {
                case Type::INT:    // bool + int
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = (addexp->attribute.V.val.BoolVal ? 1 : 0) + 
                                               mulexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool + float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = (addexp->attribute.V.val.BoolVal ? 1.0f : 0.0f) + 
                                                 mulexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool + bool
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = (addexp->attribute.V.val.BoolVal ? 1 : 0) + 
                                               (mulexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                case Type::PTR:    // bool + ptr
                case Type::VOID:   // bool + void
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for + in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::PTR:           // ptr + any
        case Type::VOID:          // void + any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for + in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
}

void AddExp_sub::TypeCheck() {
    addexp->TypeCheck();
    mulexp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = addexp->attribute.V.ConstTag & mulexp->attribute.V.ConstTag;

    switch(addexp->attribute.T.type) {
        case Type::INT:
            switch(mulexp->attribute.T.type) {
                case Type::INT:    // int - int
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = addexp->attribute.V.val.IntVal - 
                                               mulexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int - float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = (float)addexp->attribute.V.val.IntVal - 
                                                 mulexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int - bool
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = addexp->attribute.V.val.IntVal - 
                                               (mulexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // int - (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for - in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(mulexp->attribute.T.type) {
                case Type::INT:    // float - int
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = addexp->attribute.V.val.FloatVal - 
                                                 (float)mulexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float - float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = addexp->attribute.V.val.FloatVal - 
                                                 mulexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float - bool
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = addexp->attribute.V.val.FloatVal - 
                                                 (mulexp->attribute.V.val.BoolVal ? 1.0f : 0.0f);
                    }
                    break;
                    
                default:           // float - (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for - in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(mulexp->attribute.T.type) {
                case Type::INT:    // bool - int
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = (addexp->attribute.V.val.BoolVal ? 1 : 0) - 
                                               mulexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool - float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = (addexp->attribute.V.val.BoolVal ? 1.0f : 0.0f) - 
                                                 mulexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool - bool
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = (addexp->attribute.V.val.BoolVal ? 1 : 0) - 
                                               (mulexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // bool - (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for - in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        default:                   // (ptr/void) - any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for - in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }

    //TODO("BinaryExp Semant");
}

void MulExp_mul::TypeCheck() {
    mulexp->TypeCheck();
    unary_exp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = mulexp->attribute.V.ConstTag & unary_exp->attribute.V.ConstTag;

    switch(mulexp->attribute.T.type) {
        case Type::INT:
            switch(unary_exp->attribute.T.type) {
                case Type::INT:    // int * int
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = mulexp->attribute.V.val.IntVal * 
                                               unary_exp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int * float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = (float)mulexp->attribute.V.val.IntVal * 
                                                 unary_exp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int * bool
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = mulexp->attribute.V.val.IntVal * 
                                               (unary_exp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // int * (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for * in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(unary_exp->attribute.T.type) {
                case Type::INT:    // float * int
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = mulexp->attribute.V.val.FloatVal * 
                                                 (float)unary_exp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float * float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = mulexp->attribute.V.val.FloatVal * 
                                                 unary_exp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float * bool
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = mulexp->attribute.V.val.FloatVal * 
                                                 (unary_exp->attribute.V.val.BoolVal ? 1.0f : 0.0f);
                    }
                    break;
                    
                default:           // float * (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for * in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(unary_exp->attribute.T.type) {
                case Type::INT:    // bool * int
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = (mulexp->attribute.V.val.BoolVal ? 1 : 0) * 
                                               unary_exp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool * float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = (mulexp->attribute.V.val.BoolVal ? 1.0f : 0.0f) * 
                                                 unary_exp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool * bool
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = (mulexp->attribute.V.val.BoolVal ? 1 : 0) * 
                                               (unary_exp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // bool * (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for * in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        default:                   // (ptr/void) * any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for * in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("BinaryExp Semant");
}

void MulExp_div::TypeCheck() {
    mulexp->TypeCheck();
    unary_exp->TypeCheck();

    attribute.line_number = line_number;
    // 被除数和除数是否为常量
    bool numeratorConst = mulexp->attribute.V.ConstTag;
    bool denominatorConst = unary_exp->attribute.V.ConstTag;

    // 检查除数是否为常量且为0
    if (denominatorConst) {
        if (unary_exp->attribute.T.type == Type::INT && unary_exp->attribute.V.val.IntVal == 0) {
            error_msgs.push_back("Division by zero in line " + 
                               std::to_string(line_number) + "\n");
            attribute.T.type = Type::VOID;
            return;
        }
        if (unary_exp->attribute.T.type == Type::FLOAT && unary_exp->attribute.V.val.FloatVal == 0.0f) {
            error_msgs.push_back("Division by zero in line " + 
                               std::to_string(line_number) + "\n");
            attribute.T.type = Type::VOID;
            return;
        }
        if (unary_exp->attribute.T.type == Type::BOOL && !unary_exp->attribute.V.val.BoolVal) {
            error_msgs.push_back("Division by zero in line " + 
                               std::to_string(line_number) + "\n");
            attribute.T.type = Type::VOID;
            return;
        }
    }

    // 更新 ConstTag 仅在被除数和除数都是常量时为真
    attribute.V.ConstTag = numeratorConst && denominatorConst;

    switch(mulexp->attribute.T.type) {
        case Type::INT:
            switch(unary_exp->attribute.T.type) {
                case Type::INT:    // int / int
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = mulexp->attribute.V.val.IntVal / 
                                               unary_exp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int / float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = (float)mulexp->attribute.V.val.IntVal / 
                                                 unary_exp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int / bool
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = mulexp->attribute.V.val.IntVal / 
                                               (unary_exp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // int / (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for / in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(unary_exp->attribute.T.type) {
                case Type::INT:    // float / int
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = mulexp->attribute.V.val.FloatVal / 
                                                 (float)unary_exp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float / float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = mulexp->attribute.V.val.FloatVal / 
                                                 unary_exp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float / bool
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = mulexp->attribute.V.val.FloatVal / 
                                                 (unary_exp->attribute.V.val.BoolVal ? 1.0f : 0.0f);
                    }
                    break;
                    
                default:           // float / (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for / in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(unary_exp->attribute.T.type) {
                case Type::INT:    // bool / int
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = (mulexp->attribute.V.val.BoolVal ? 1 : 0) / 
                                               unary_exp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool / float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.FloatVal = (mulexp->attribute.V.val.BoolVal ? 1.0f : 0.0f) / 
                                                 unary_exp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool / bool
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.IntVal = (mulexp->attribute.V.val.BoolVal ? 1 : 0) / 
                                               (unary_exp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // bool / (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for / in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        default:                   // (ptr/void) / any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for / in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("BinaryExp Semant");
}

void MulExp_mod::TypeCheck() {
    mulexp->TypeCheck();
    unary_exp->TypeCheck();

    attribute.line_number = line_number;

    // 被除数和除数是否为常量
    bool numeratorConst = mulexp->attribute.V.ConstTag;
    bool denominatorConst = unary_exp->attribute.V.ConstTag;

    // 检查除数是否为常量且为 0
    if (denominatorConst) {
        if (unary_exp->attribute.T.type == Type::INT && unary_exp->attribute.V.val.IntVal == 0) {
            error_msgs.push_back("Modulo by zero in line " +
                                 std::to_string(line_number) + "\n");
            attribute.T.type = Type::VOID;
            return;
        }
    }

    // 更新 ConstTag 仅在被除数和除数都是常量时为真
    attribute.V.ConstTag = numeratorConst && denominatorConst;

    switch (mulexp->attribute.T.type) {
        case Type::INT:
            switch (unary_exp->attribute.T.type) {
                case Type::INT:    // int % int
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        if (unary_exp->attribute.V.val.IntVal == 0) {
                            error_msgs.push_back("Modulo by zero in line " +
                                                 std::to_string(line_number) + "\n");
                            attribute.T.type = Type::VOID;
                        } else {
                            attribute.V.val.IntVal = mulexp->attribute.V.val.IntVal %
                                                     unary_exp->attribute.V.val.IntVal;
                        }
                    }
                    break;

                default:           // int % 非整数
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for % in line " +
                                         std::to_string(line_number) + "\n");
                    break;
            }
            break;

        default:  // 非整数左操作数
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for % in line " +
                                 std::to_string(line_number) + "\n");
            break;
    }
}


void RelExp_leq::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = relexp->attribute.V.ConstTag & addexp->attribute.V.ConstTag;

    switch(relexp->attribute.T.type) {
        case Type::INT:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // int <= int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal <= 
                                                addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int <= float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (float)relexp->attribute.V.val.IntVal <= 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int <= bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal <= 
                                                (addexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // int <= (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for <= in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // float <= int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal <= 
                                                (float)addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float <= float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal <= 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float <= bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal <= 
                                                (addexp->attribute.V.val.BoolVal ? 1.0f : 0.0f);
                    }
                    break;
                    
                default:           // float <= (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for <= in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // bool <= int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1 : 0) <= 
                                                addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool <= float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1.0f : 0.0f) <= 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool <= bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1 : 0) <= 
                                                (addexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // bool <= (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for <= in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        default:                   // (ptr/void) <= any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for <= in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("BinaryExp Semant");
}

void RelExp_lt::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = relexp->attribute.V.ConstTag & addexp->attribute.V.ConstTag;

    switch(relexp->attribute.T.type) {
        case Type::INT:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // int < int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal < 
                                                addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int < float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (float)relexp->attribute.V.val.IntVal < 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int < bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal < 
                                                (addexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // int < (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for < in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // float < int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal < 
                                                (float)addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float < float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal < 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float < bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal < 
                                                (addexp->attribute.V.val.BoolVal ? 1.0f : 0.0f);
                    }
                    break;
                    
                default:           // float < (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for < in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // bool < int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1 : 0) < 
                                                addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool < float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1.0f : 0.0f) < 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool < bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1 : 0) < 
                                                (addexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // bool < (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for < in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        default:                   // (ptr/void) < any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for < in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("BinaryExp Semant");
}

void RelExp_geq::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = relexp->attribute.V.ConstTag & addexp->attribute.V.ConstTag;

    switch(relexp->attribute.T.type) {
        case Type::INT:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // int >= int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal >= 
                                                addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int >= float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (float)relexp->attribute.V.val.IntVal >= 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int >= bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal >= 
                                                (addexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // int >= (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for >= in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // float >= int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal >= 
                                                (float)addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float >= float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal >= 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float >= bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal >= 
                                                (addexp->attribute.V.val.BoolVal ? 1.0f : 0.0f);
                    }
                    break;
                    
                default:           // float >= (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for >= in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // bool >= int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1 : 0) >= 
                                                addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool >= float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1.0f : 0.0f) >= 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool >= bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1 : 0) >= 
                                                (addexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // bool >= (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for >= in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        default:                   // (ptr/void) >= any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for >= in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("BinaryExp Semant");
}

void RelExp_gt::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = relexp->attribute.V.ConstTag & addexp->attribute.V.ConstTag;

    switch(relexp->attribute.T.type) {
        case Type::INT:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // int > int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal > 
                                                addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int > float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (float)relexp->attribute.V.val.IntVal > 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int > bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.IntVal > 
                                                (addexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // int > (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for > in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // float > int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal > 
                                                (float)addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float > float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal > 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float > bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = relexp->attribute.V.val.FloatVal > 
                                                (addexp->attribute.V.val.BoolVal ? 1.0f : 0.0f);
                    }
                    break;
                    
                default:           // float > (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for > in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(addexp->attribute.T.type) {
                case Type::INT:    // bool > int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1 : 0) > 
                                                addexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool > float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1.0f : 0.0f) > 
                                                addexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool > bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (relexp->attribute.V.val.BoolVal ? 1 : 0) > 
                                                (addexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // bool > (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for > in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        default:                   // (ptr/void) > any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for > in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("BinaryExp Semant");
}

void EqExp_eq::TypeCheck() {
    eqexp->TypeCheck();
    relexp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = eqexp->attribute.V.ConstTag & relexp->attribute.V.ConstTag;

    switch(eqexp->attribute.T.type) {
        case Type::INT:
            switch(relexp->attribute.T.type) {
                case Type::INT:    // int == int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = eqexp->attribute.V.val.IntVal == 
                                                relexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int == float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (float)eqexp->attribute.V.val.IntVal == 
                                                relexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int == bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = eqexp->attribute.V.val.IntVal == 
                                                (relexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // int == (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for == in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(relexp->attribute.T.type) {
                case Type::INT:    // float == int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = eqexp->attribute.V.val.FloatVal == 
                                                (float)relexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float == float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = eqexp->attribute.V.val.FloatVal == 
                                                relexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float == bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = eqexp->attribute.V.val.FloatVal == 
                                                (relexp->attribute.V.val.BoolVal ? 1.0f : 0.0f);
                    }
                    break;
                    
                default:           // float == (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for == in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(relexp->attribute.T.type) {
                case Type::INT:    // bool == int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (eqexp->attribute.V.val.BoolVal ? 1 : 0) == 
                                                relexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool == float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (eqexp->attribute.V.val.BoolVal ? 1.0f : 0.0f) == 
                                                relexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool == bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (eqexp->attribute.V.val.BoolVal ? 1 : 0) == 
                                                (relexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // bool == (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for == in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        default:                   // (ptr/void) == any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for == in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("BinaryExp Semant");
}

void EqExp_neq::TypeCheck() {
    eqexp->TypeCheck();
    relexp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = eqexp->attribute.V.ConstTag & relexp->attribute.V.ConstTag;

    switch(eqexp->attribute.T.type) {
        case Type::INT:
            switch(relexp->attribute.T.type) {
                case Type::INT:    // int != int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = eqexp->attribute.V.val.IntVal != 
                                                relexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int != float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (float)eqexp->attribute.V.val.IntVal != 
                                                relexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int != bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = eqexp->attribute.V.val.IntVal != 
                                                (relexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // int != (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for != in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(relexp->attribute.T.type) {
                case Type::INT:    // float != int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = eqexp->attribute.V.val.FloatVal != 
                                                (float)relexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float != float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = eqexp->attribute.V.val.FloatVal != 
                                                relexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float != bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = eqexp->attribute.V.val.FloatVal != 
                                                (relexp->attribute.V.val.BoolVal ? 1.0f : 0.0f);
                    }
                    break;
                    
                default:           // float != (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for != in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(relexp->attribute.T.type) {
                case Type::INT:    // bool != int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (eqexp->attribute.V.val.BoolVal ? 1 : 0) != 
                                                relexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool != float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (eqexp->attribute.V.val.BoolVal ? 1.0f : 0.0f) != 
                                                relexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool != bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (eqexp->attribute.V.val.BoolVal ? 1 : 0) != 
                                                (relexp->attribute.V.val.BoolVal ? 1 : 0);
                    }
                    break;
                    
                default:           // bool != (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for != in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        default:                   // (ptr/void) != any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for != in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("BinaryExp Semant");
}

void LAndExp_and::TypeCheck() {
    landexp->TypeCheck();
    eqexp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = landexp->attribute.V.ConstTag & eqexp->attribute.V.ConstTag;

    switch(landexp->attribute.T.type) {
        case Type::INT:
            switch(eqexp->attribute.T.type) {
                case Type::INT:    // int && int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = landexp->attribute.V.val.IntVal && 
                                                eqexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int && float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (float)landexp->attribute.V.val.IntVal && 
                                                eqexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int && bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = landexp->attribute.V.val.IntVal && 
                                                eqexp->attribute.V.val.BoolVal;
                    }
                    break;
                    
                default:           // int && (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for && in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(eqexp->attribute.T.type) {
                case Type::INT:    // float && int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = landexp->attribute.V.val.FloatVal && 
                                                (float)eqexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float && float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = landexp->attribute.V.val.FloatVal && 
                                                eqexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float && bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = landexp->attribute.V.val.FloatVal && 
                                                eqexp->attribute.V.val.BoolVal;
                    }
                    break;
                    
                default:           // float && (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for && in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(eqexp->attribute.T.type) {
                case Type::INT:    // bool && int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = landexp->attribute.V.val.BoolVal && 
                                                eqexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool && float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = landexp->attribute.V.val.BoolVal && 
                                                eqexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool && bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = landexp->attribute.V.val.BoolVal && 
                                                eqexp->attribute.V.val.BoolVal;
                    }
                    break;
                    
                default:           // bool && (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for && in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        default:                   // (ptr/void) && any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for && in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("BinaryExp Semant");
}

void LOrExp_or::TypeCheck() {
    lorexp->TypeCheck();
    landexp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = lorexp->attribute.V.ConstTag & landexp->attribute.V.ConstTag;

    switch(lorexp->attribute.T.type) {
        case Type::INT:
            switch(landexp->attribute.T.type) {
                case Type::INT:    // int || int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = lorexp->attribute.V.val.IntVal || 
                                                landexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // int || float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = (float)lorexp->attribute.V.val.IntVal || 
                                                landexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // int || bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = lorexp->attribute.V.val.IntVal || 
                                                landexp->attribute.V.val.BoolVal;
                    }
                    break;
                    
                default:           // int || (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for || in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:
            switch(landexp->attribute.T.type) {
                case Type::INT:    // float || int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = lorexp->attribute.V.val.FloatVal || 
                                                (float)landexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // float || float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = lorexp->attribute.V.val.FloatVal || 
                                                landexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // float || bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = lorexp->attribute.V.val.FloatVal || 
                                                landexp->attribute.V.val.BoolVal;
                    }
                    break;
                    
                default:           // float || (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for || in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::BOOL:
            switch(landexp->attribute.T.type) {
                case Type::INT:    // bool || int
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = lorexp->attribute.V.val.BoolVal || 
                                                landexp->attribute.V.val.IntVal;
                    }
                    break;
                    
                case Type::FLOAT:  // bool || float
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = lorexp->attribute.V.val.BoolVal || 
                                                landexp->attribute.V.val.FloatVal;
                    }
                    break;
                    
                case Type::BOOL:   // bool || bool
                    attribute.T.type = Type::BOOL;
                    if (attribute.V.ConstTag) {
                        attribute.V.val.BoolVal = lorexp->attribute.V.val.BoolVal || 
                                                landexp->attribute.V.val.BoolVal;
                    }
                    break;
                    
                default:           // bool || (ptr/void)
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for || in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        default:                   // (ptr/void) || any
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for || in line " + 
                               std::to_string(line_number) + "\n");
            break;
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

// Reference: https://github.com/yuhuifishash/SysY/blob/master/ir_gen/IRgen.cc line169-line189
// 通过偏移量计算多维数组元素的索引值
int GetArrayIntVal(VarAttribute &val, std::vector<int> &indices) {
    // 计算最终的线性偏移量
    int offset = 0;
    int totalDimSize = 1;
    
    // 从最右维度开始,依次计算每个维度的贡献:
    // 最右维度直接加上索引值 
    // 其他维度要先乘以右边维度的总大小再加上索引值
    for(int i = indices.size() - 1; i >= 0; i--) {
        offset += indices[i] * totalDimSize;
        if(i > 0) {
            totalDimSize *= val.dims[i];
        }
    }
    
    return val.IntInitVals[offset];
}

float GetArrayFloatVal(VarAttribute &val, std::vector<int> &indices) {
    // 使用子区间大小的方式计算索引
    int finalIndex = 0;
    int subSize = 1;
    
    // 从左到右计算每个维度
    // 每个维度的索引值乘以剩余维度的大小之积
    for(int i = indices.size() - 1; i >= 0; i--) {
        // 计算当前维右边所有维度的元素个数之积
        subSize = 1;
        for(int j = i + 1; j < val.dims.size(); j++) {
            subSize *= val.dims[j];
        }
        // 累加当前维度对最终索引的贡献
        finalIndex += indices[i] * subSize;
    }
    
    return val.FloatInitVals[finalIndex];
}

void Lval::TypeCheck() { 
    // 初始状态设置
    struct IndexInfo {
        std::vector<int> values;
        bool isAllConst;
    };
    
    // 第一步：处理索引信息 
    auto processIndices = [this]() -> IndexInfo {
        IndexInfo result{{}, true};
        if (!dims) return result;
        
        for (const auto& dim : *dims) {
            dim->TypeCheck();
            switch (dim->attribute.T.type) {
                case Type::VOID:
                case Type::FLOAT:
                    error_msgs.push_back(
                        std::string("Invalid array dimension type (") + 
                        (dim->attribute.T.type == Type::VOID ? "void" : "float") +
                        ") at line " + std::to_string(line_number) + "\n"
                    );
                    break;
                default:
                    result.values.push_back(dim->attribute.V.val.IntVal);
                    result.isAllConst &= dim->attribute.V.ConstTag;
            }
        }
        return result;
    };

    // 第二步：查找变量定义
    auto lookupVariable = [this]() -> std::pair<VarAttribute, int> {
        auto localVar = semant_table.symbol_table.lookup_val(name);
        if (localVar.type != Type::VOID) {
            return {localVar, semant_table.symbol_table.lookup_scope(name)};
        }
        
        auto globalIt = semant_table.GlobalTable.find(name);
        if (globalIt != semant_table.GlobalTable.end()) {
            return {globalIt->second, 0};
        }
        
        error_msgs.push_back("Use of undeclared identifier '" + 
                           std::string(name->get_string()) + 
                           "' at line " + std::to_string(line_number) + "\n");
        return {VarAttribute(), -1};
    };

    // 第三步：设置属性
    auto setAttributes = [this](const VarAttribute& var, const IndexInfo& idx) {
        size_t idxCount = idx.values.size();
        size_t dimCount = var.dims.size();
        
        if (idxCount > dimCount) {
            error_msgs.push_back("Too many indices for array at line " + 
                               std::to_string(line_number) + "\n");
            return;
        }
        
        if (idxCount == dimCount) {
            attribute.T.type = var.type;
            attribute.V.ConstTag = var.ConstTag && idx.isAllConst;
            
            if (attribute.V.ConstTag) {
                // 创建非const临时变量用于调用GetArray*Val
                VarAttribute tempVar = var;
                std::vector<int> tempIndices = idx.values;
                
                if (var.type == Type::INT) {
                    attribute.V.val.IntVal = GetArrayIntVal(tempVar, tempIndices);
                } else {
                    attribute.V.val.FloatVal = GetArrayFloatVal(tempVar, tempIndices);
                }
            }
        } else {
            attribute.T.type = Type::PTR;
            attribute.V.ConstTag = false;
        }
    };

    // 执行主逻辑
    is_left = false;
    auto indices = processIndices();
    auto [varAttr, varScope] = lookupVariable();
    
    if (varScope != -1) {
        scope = varScope;
        setAttributes(varAttr, indices);
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

        // 隐式类型转换
        if (actual_type.type == Type::BOOL && expected_type == Type::INT) {
            actual_type.type = Type::INT;
        } else if (actual_type.type == Type::INT && expected_type == Type::FLOAT) {
            actual_type.type = Type::FLOAT;
        } else if (actual_type.type == Type::BOOL && expected_type == Type::FLOAT) {
            actual_type.type = Type::FLOAT;
        } else if (actual_type.type == Type::FLOAT && expected_type == Type::INT) {
            actual_type.type = Type::INT;
        }

        // 检查实参类型是否与形参类型一致
        if (actual_type.type != expected_type && actual_type.type != Type::PTR) {
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
    unary_exp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = unary_exp->attribute.V.ConstTag;

    switch(unary_exp->attribute.T.type) {
        case Type::INT:    // +int
            attribute.T.type = Type::INT;
            if (attribute.V.ConstTag) {
                attribute.V.val.IntVal = unary_exp->attribute.V.val.IntVal;
            }
            break;
            
        case Type::FLOAT:  // +float
            attribute.T.type = Type::FLOAT;
            if (attribute.V.ConstTag) {
                attribute.V.val.FloatVal = unary_exp->attribute.V.val.FloatVal;
            }
            break;
            
        case Type::BOOL:   // +bool -> int
            attribute.T.type = Type::INT;
            if (attribute.V.ConstTag) {
                attribute.V.val.IntVal = unary_exp->attribute.V.val.BoolVal ? 1 : 0;
            }
            break;
            
        default:           // +(ptr/void)
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operand for unary + in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("unary_exp Semant"); 
}

void UnaryExp_neg::TypeCheck() { 
    unary_exp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = unary_exp->attribute.V.ConstTag;

    switch(unary_exp->attribute.T.type) {
        case Type::INT:    // -int
            attribute.T.type = Type::INT;
            if (attribute.V.ConstTag) {
                attribute.V.val.IntVal = -unary_exp->attribute.V.val.IntVal;
            }
            break;
            
        case Type::FLOAT:  // -float
            attribute.T.type = Type::FLOAT;
            if (attribute.V.ConstTag) {
                attribute.V.val.FloatVal = -unary_exp->attribute.V.val.FloatVal;
            }
            break;
            
        case Type::BOOL:   // -bool -> -int
            attribute.T.type = Type::INT;
            if (attribute.V.ConstTag) {
                attribute.V.val.IntVal = -(unary_exp->attribute.V.val.BoolVal ? 1 : 0);
            }
            break;
            
        default:           // -(ptr/void)
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operand for unary - in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("unary_exp Semant"); 
}

void UnaryExp_not::TypeCheck() { 
    unary_exp->TypeCheck();
    
    attribute.line_number = line_number;
    attribute.V.ConstTag = unary_exp->attribute.V.ConstTag;

    switch(unary_exp->attribute.T.type) {
        case Type::INT:    // !int
            attribute.T.type = Type::BOOL;
            if (attribute.V.ConstTag) {
                attribute.V.val.BoolVal = !unary_exp->attribute.V.val.IntVal;
            }
            break;
            
        case Type::FLOAT:  // !float
            attribute.T.type = Type::BOOL;
            if (attribute.V.ConstTag) {
                attribute.V.val.BoolVal = !unary_exp->attribute.V.val.FloatVal;
            }
            break;
            
        case Type::BOOL:   // !bool
            attribute.T.type = Type::BOOL;
            if (attribute.V.ConstTag) {
                attribute.V.val.BoolVal = !unary_exp->attribute.V.val.BoolVal;
            }
            break;
            
        default:           // !(ptr/void)
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operand for unary ! in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("unary_exp Semant"); 
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

void StringConst::TypeCheck() { 
    attribute.T.type = Type::PTR;
    std::string label = ".str" + std::to_string(++GlobalStrCnt);
    GlobalStringConstInstruction* strInst = nullptr;
    if (str && str->get_string().length() > 0) {
        strInst = new GlobalStringConstInstruction(str->get_string(),label);
    } else {
        strInst = new GlobalStringConstInstruction("",label);
    }
    if (strInst) {
        llvmIR.global_def.push_back(strInst);
    }
    if (str) {
        semant_table.GlobalStrTable.insert(
            std::make_pair(str, GlobalStrCnt)
        );
    }
    //TODO("StringConst Semant"); 
}

void PrimaryExp_branch::TypeCheck() {
    exp->TypeCheck();
    attribute = exp->attribute;
}

void assign_stmt::TypeCheck() { 
    lval->TypeCheck();
    exp->TypeCheck();
    ((Lval *)lval)->is_left = true;
    if (exp->attribute.T.type == Type::VOID) {
        error_msgs.push_back("void type can not be assign_stmt's expression " + std::to_string(line_number) + "\n");
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
    Cond->TypeCheck();

    if (Cond->attribute.T.type == Type::VOID) {
        error_msgs.push_back("while cond type is invalid " + std::to_string(Cond->GetLineNumber()) + "\n");
    }

    InWhileCount++;
    body->TypeCheck();
    InWhileCount--;
    //TODO("WhileStmt Semant"); 
}

void continue_stmt::TypeCheck() {
    if (!InWhileCount) {
        error_msgs.push_back("continue is not in while stmt in line " + std::to_string(line_number) + "\n");
    }
    //TODO("ContinueStmt Semant"); 
}

void break_stmt::TypeCheck() { 
    if (!InWhileCount) {
        error_msgs.push_back("break is not in while stmt in line " + std::to_string(line_number) + "\n");
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

std::map<std::string, VarAttribute> ConstGlobalMap;
std::map<std::string, VarAttribute> StaticGlobalMap; 

extern BasicInstruction::LLVMType Type2LLVM(Type::ty type);


// Reference: https://github.com/yuhuifishash/SysY/blob/master/ir_gen/semant.cc line52-line166
int FindMinDimStep(const VarAttribute &val, int relativePos, int dimsIdx, int &max_subBlock_sz) {
    int min_dim_step = 1;
    int blockSz = 1;
    for (int i = dimsIdx + 1; i < val.dims.size(); i++) {
        blockSz *= val.dims[i];
    }
    while (relativePos % blockSz != 0) {
        min_dim_step++;
        blockSz /= val.dims[dimsIdx + min_dim_step - 1];
    }
    max_subBlock_sz = blockSz;
    return min_dim_step;
}

void RecursiveArrayInit(InitVal init, VarAttribute &val, int begPos, int endPos, int dimsIdx) {
    // dimsIdx from 0
    int pos = begPos;

    // Old Policy: One { } for one dim

    for (InitVal iv : *(init->GetList())) {
        if (iv->IsExp()) {
            if (iv->attribute.T.type == Type::VOID) {
                error_msgs.push_back("exp can not be void in initval in line " + std::to_string(init->GetLineNumber()) +
                                     "\n");
            }
            if (val.type == Type::INT) {
                if (iv->attribute.T.type == Type::INT) {
                    val.IntInitVals[pos] = iv->attribute.V.val.IntVal;
                } else if (iv->attribute.T.type == Type::FLOAT) {
                    val.IntInitVals[pos] = iv->attribute.V.val.FloatVal;
                }
            }
            if (val.type == Type::FLOAT) {
                if (iv->attribute.T.type == Type::INT) {
                    val.FloatInitVals[pos] = iv->attribute.V.val.IntVal;
                } else if (iv->attribute.T.type == Type::FLOAT) {
                    val.FloatInitVals[pos] = iv->attribute.V.val.FloatVal;
                }
            }
            pos++;
        } else {
            // New Policy: One { } for the max align-able dim
            // More informations see comments above FindMinDimStep
            int max_subBlock_sz = 0;
            int min_dim_step = FindMinDimStep(val, pos - begPos, dimsIdx, max_subBlock_sz);
            RecursiveArrayInit(iv, val, pos, pos + max_subBlock_sz - 1, dimsIdx + min_dim_step);
            pos += max_subBlock_sz;
        }
    }
}

void SolveIntInitVal(InitVal init, VarAttribute &val)    // used for global or const
{
    val.type = Type::INT;
    int arraySz = 1;
    for (auto d : val.dims) {
        arraySz *= d;
    }
    val.IntInitVals.resize(arraySz, 0);
    if (val.dims.empty()) {
        if (init->GetExp() != nullptr) {
            if (init->GetExp()->attribute.T.type == Type::VOID) {
                error_msgs.push_back("Expression can not be void in initval in line " +
                                     std::to_string(init->GetLineNumber()) + "\n");
            } else if (init->GetExp()->attribute.T.type == Type::INT) {
                val.IntInitVals[0] = init->GetExp()->attribute.V.val.IntVal;
            } else if (init->GetExp()->attribute.T.type == Type::FLOAT) {
                val.IntInitVals[0] = init->GetExp()->attribute.V.val.FloatVal;
            }
        }
        return;
    } else {
        if (init->IsExp()) {
            if ((init)->GetExp() != nullptr) {
                error_msgs.push_back("InitVal can not be exp in line " + std::to_string(init->GetLineNumber()) + "\n");
            }
            return;
        } else {
            RecursiveArrayInit(init, val, 0, arraySz - 1, 0);
        }
    }
}

void SolveFloatInitVal(InitVal init, VarAttribute &val)    // used for global or const
{
    val.type = Type::FLOAT;
    int arraySz = 1;
    for (auto d : val.dims) {
        arraySz *= d;
    }
    val.FloatInitVals.resize(arraySz, 0);
    if (val.dims.empty()) {
        if (init->GetExp() != nullptr) {
            if (init->GetExp()->attribute.T.type == Type::VOID) {
                error_msgs.push_back("exp can not be void in initval in line " + std::to_string(init->GetLineNumber()) +
                                     "\n");
            } else if (init->GetExp()->attribute.T.type == Type::FLOAT) {
                val.FloatInitVals[0] = init->GetExp()->attribute.V.val.FloatVal;
            } else if (init->GetExp()->attribute.T.type == Type::INT) {
                val.FloatInitVals[0] = init->GetExp()->attribute.V.val.IntVal;
            }
        }
        return;
    } else {
        if (init->IsExp()) {
            if ((init)->GetExp() != nullptr) {
                error_msgs.push_back("InitVal can not be exp in line " + std::to_string(init->GetLineNumber()) + "\n");
            }
            return;
        } else {
            RecursiveArrayInit(init, val, 0, arraySz - 1, 0);
        }
    }
}

void ConstInitVal::TypeCheck() { 
    for (auto init_val : *initval) {
        init_val->TypeCheck();
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
    // 检查重复定义
    if (semant_table.symbol_table.lookup_scope(name) == semant_table.symbol_table.get_current_scope()) {
        error_msgs.push_back("multiple definition of " + name->get_string() + " exists in line " + 
                            std::to_string(line_number) + "\n");
        return;
    }

    VarAttribute var_attr;
    var_attr.type = attribute.T.type;
    var_attr.ConstTag = false;
    scope = semant_table.symbol_table.get_current_scope();

    // 检查数组维度
    if (dims != nullptr) {
        for (auto d : *dims) {
            d->TypeCheck();
            if (!d->attribute.V.ConstTag) {
                error_msgs.push_back("Array Dim must be const expression in line " + 
                                   std::to_string(line_number) + "\n");
            }
            if (d->attribute.T.type == Type::FLOAT) {
                error_msgs.push_back("Array Dim can not be float in line " + 
                                   std::to_string(line_number) + "\n");
            }
            var_attr.dims.push_back(d->attribute.V.val.IntVal);
        }
    }

    // 处理初始化
    if (init != nullptr) {
        init->TypeCheck();
    }

    // 添加到符号表
    semant_table.symbol_table.add_Symbol(name, var_attr);

    //TODO("VarDef Semant"); 
}

void ConstDef::TypeCheck() { 
    // 检查重复定义
    if (semant_table.symbol_table.lookup_scope(name) == semant_table.symbol_table.get_current_scope()) {
        error_msgs.push_back("multiple definition of " + name->get_string() + " in line " + 
                            std::to_string(line_number) + "\n");
        return;
    }

    VarAttribute const_attr;
    const_attr.type = attribute.T.type; 
    const_attr.ConstTag = true;
    scope = semant_table.symbol_table.get_current_scope();

    // 检查数组维度
    if (dims != nullptr) {
        for (auto d : *dims) {
            d->TypeCheck();
            if (!d->attribute.V.ConstTag) {
                error_msgs.push_back("Array Dim must be const expression in line " + 
                                   std::to_string(line_number) + "\n");
            }
            if (d->attribute.T.type == Type::FLOAT) {
                error_msgs.push_back("Array Dim can not be float in line " + 
                                   std::to_string(line_number) + "\n");
            }
            const_attr.dims.push_back(d->attribute.V.val.IntVal);
        }
    }

    // 处理初始化
    if (init != nullptr) {
        init->TypeCheck();
        if (attribute.T.type == Type::INT) {
            SolveIntInitVal(init, const_attr);
        } else if (attribute.T.type == Type::FLOAT) {
            SolveFloatInitVal(init, const_attr);
        }
    }

    // 添加到符号表
    semant_table.symbol_table.add_Symbol(name, const_attr);

    //TODO("ConstDef Semant");
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

    if (dims != nullptr) {
        auto dim_vector = *dims;
        val.dims.push_back(-1);
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
    Type::ty type_decl = decl->GetTypedecl();
    auto def_vector = *decl->GetDefs();
    for (auto def : def_vector) {

        if (semant_table.GlobalTable.find(def->GetName()) != semant_table.GlobalTable.end()) {
            error_msgs.push_back("multilpe difinitions of vars in line " + std::to_string(line_number) + "\n");
        }

        VarAttribute val;
        val.ConstTag = def->IsConst();
        val.type = (Type::ty)type_decl;
        def->scope = 0;

        if (def->GetDims() != nullptr) {
            auto dim_vector = *def->GetDims();
            for (auto d : dim_vector) {
                d->TypeCheck();
                if (d->attribute.V.ConstTag == false) {
                    error_msgs.push_back("Array Dim must be const expression " + std::to_string(line_number) + "\n");
                }
                if (d->attribute.T.type == Type::FLOAT) {
                    error_msgs.push_back("Array Dim can not be float in line " + std::to_string(line_number) + "\n");
                }
            }
            for (auto d : dim_vector) {
                val.dims.push_back(d->attribute.V.val.IntVal);
            }
        }

        InitVal init = def->GetInit();
        if (init != nullptr) {
            init->TypeCheck();
            if (type_decl == Type::INT) {
                SolveIntInitVal(init, val);
            } else if (type_decl == Type::FLOAT) {
                SolveFloatInitVal(init, val);
            }
        }

        if (def->IsConst()) {
            ConstGlobalMap[def->GetName()->get_string()] = val;
        }

        StaticGlobalMap[def->GetName()->get_string()] = val;
        semant_table.GlobalTable[def->GetName()] = val;

        BasicInstruction::LLVMType lltype = Type2LLVM(type_decl);

        Instruction globalDecl;
        if (def->GetDims() != nullptr) {
            globalDecl = new GlobalVarDefineInstruction(def->GetName()->get_string(), lltype, val);
        } else if (init == nullptr) {
            globalDecl = new GlobalVarDefineInstruction(def->GetName()->get_string(), lltype, nullptr);
        } else if (lltype == BasicInstruction::I32) {
            globalDecl =
            new GlobalVarDefineInstruction(def->GetName()->get_string(), lltype, new ImmI32Operand(val.IntInitVals[0]));
        } else if (lltype == BasicInstruction::FLOAT32) {
            globalDecl = new GlobalVarDefineInstruction(def->GetName()->get_string(), lltype,
                                                        new ImmF32Operand(val.FloatInitVals[0]));
        }
        llvmIR.global_def.push_back(globalDecl);
    }
    //TODO("CompUnitDecl Semant");
}

void CompUnit_FuncDef::TypeCheck() { func_def->TypeCheck(); }
