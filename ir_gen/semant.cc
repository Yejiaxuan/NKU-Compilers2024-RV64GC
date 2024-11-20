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
    attribute.V.ConstTag = mulexp->attribute.V.ConstTag & unary_exp->attribute.V.ConstTag;

    switch(mulexp->attribute.T.type) {
        case Type::INT:
            switch(unary_exp->attribute.T.type) {
                case Type::INT:    // int / int
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        // 除数为0检查
                        if (unary_exp->attribute.V.val.IntVal == 0) {
                            error_msgs.push_back("Division by zero in line " + 
                                               std::to_string(line_number) + "\n");
                            attribute.T.type = Type::VOID;
                        } else {
                            attribute.V.val.IntVal = mulexp->attribute.V.val.IntVal / 
                                                   unary_exp->attribute.V.val.IntVal;
                        }
                    }
                    break;
                    
                case Type::FLOAT:  // int / float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        if (unary_exp->attribute.V.val.FloatVal == 0.0f) {
                            error_msgs.push_back("Division by zero in line " + 
                                               std::to_string(line_number) + "\n");
                            attribute.T.type = Type::VOID;
                        } else {
                            attribute.V.val.FloatVal = (float)mulexp->attribute.V.val.IntVal / 
                                                     unary_exp->attribute.V.val.FloatVal;
                        }
                    }
                    break;
                    
                case Type::BOOL:   // int / bool
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        if (!unary_exp->attribute.V.val.BoolVal) {
                            error_msgs.push_back("Division by zero in line " + 
                                               std::to_string(line_number) + "\n");
                            attribute.T.type = Type::VOID;
                        } else {
                            attribute.V.val.IntVal = mulexp->attribute.V.val.IntVal;
                        }
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
                        if (unary_exp->attribute.V.val.IntVal == 0) {
                            error_msgs.push_back("Division by zero in line " + 
                                               std::to_string(line_number) + "\n");
                            attribute.T.type = Type::VOID;
                        } else {
                            attribute.V.val.FloatVal = mulexp->attribute.V.val.FloatVal / 
                                                     (float)unary_exp->attribute.V.val.IntVal;
                        }
                    }
                    break;
                    
                case Type::FLOAT:  // float / float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        if (unary_exp->attribute.V.val.FloatVal == 0.0f) {
                            error_msgs.push_back("Division by zero in line " + 
                                               std::to_string(line_number) + "\n");
                            attribute.T.type = Type::VOID;
                        } else {
                            attribute.V.val.FloatVal = mulexp->attribute.V.val.FloatVal / 
                                                     unary_exp->attribute.V.val.FloatVal;
                        }
                    }
                    break;
                    
                case Type::BOOL:   // float / bool
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        if (!unary_exp->attribute.V.val.BoolVal) {
                            error_msgs.push_back("Division by zero in line " + 
                                               std::to_string(line_number) + "\n");
                            attribute.T.type = Type::VOID;
                        } else {
                            attribute.V.val.FloatVal = mulexp->attribute.V.val.FloatVal;
                        }
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
                        if (unary_exp->attribute.V.val.IntVal == 0) {
                            error_msgs.push_back("Division by zero in line " + 
                                               std::to_string(line_number) + "\n");
                            attribute.T.type = Type::VOID;
                        } else {
                            attribute.V.val.IntVal = (mulexp->attribute.V.val.BoolVal ? 1 : 0) / 
                                                   unary_exp->attribute.V.val.IntVal;
                        }
                    }
                    break;
                    
                case Type::FLOAT:  // bool / float
                    attribute.T.type = Type::FLOAT;
                    if (attribute.V.ConstTag) {
                        if (unary_exp->attribute.V.val.FloatVal == 0.0f) {
                            error_msgs.push_back("Division by zero in line " + 
                                               std::to_string(line_number) + "\n");
                            attribute.T.type = Type::VOID;
                        } else {
                            attribute.V.val.FloatVal = (mulexp->attribute.V.val.BoolVal ? 1.0f : 0.0f) / 
                                                     unary_exp->attribute.V.val.FloatVal;
                        }
                    }
                    break;
                    
                case Type::BOOL:   // bool / bool
                    attribute.T.type = Type::INT;
                    if (attribute.V.ConstTag) {
                        if (!unary_exp->attribute.V.val.BoolVal) {
                            error_msgs.push_back("Division by zero in line " + 
                                               std::to_string(line_number) + "\n");
                            attribute.T.type = Type::VOID;
                        } else {
                            attribute.V.val.IntVal = mulexp->attribute.V.val.BoolVal ? 1 : 0;
                        }
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
    attribute.V.ConstTag = mulexp->attribute.V.ConstTag & unary_exp->attribute.V.ConstTag;

    switch(mulexp->attribute.T.type) {
        case Type::INT:
            switch(unary_exp->attribute.T.type) {
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
                    
                case Type::FLOAT:  // int % float -> error
                case Type::BOOL:   // int % bool -> error
                case Type::PTR:    // int % ptr -> error
                case Type::VOID:   // int % void -> error
                    attribute.T.type = Type::VOID;
                    error_msgs.push_back("Invalid operands for % in line " + 
                                       std::to_string(line_number) + "\n");
                    break;
            }
            break;
            
        case Type::FLOAT:  // float % any -> error
        case Type::BOOL:   // bool % any -> error
        case Type::PTR:    // ptr % any -> error
        case Type::VOID:   // void % any -> error
            attribute.T.type = Type::VOID;
            error_msgs.push_back("Invalid operands for % in line " + 
                               std::to_string(line_number) + "\n");
            break;
    }
    //TODO("BinaryExp Semant");
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


int GetArrayIntVal(VarAttribute &val, std::vector<int> &indexs) {
    //[i] + i
    //[i][j] + i*dim[1] + j
    //[i][j][k] + i*dim[1]*dim[2] + j*dim[2] + k
    //[i][j][k][w] + i*dim[1]*dim[2]*dim[3] + j*dim[2]*dim[3] + k*dim[3] + w
    int idx = 0;
    for (int curIndex = 0; curIndex < indexs.size(); curIndex++) {
        idx *= val.dims[curIndex];
        idx += indexs[curIndex];
    }
    return val.IntInitVals[idx];
}

float GetArrayFloatVal(VarAttribute &val, std::vector<int> &indexs) {
    int idx = 0;
    for (int curIndex = 0; curIndex < indexs.size(); curIndex++) {
        idx *= val.dims[curIndex];
        idx += indexs[curIndex];
    }
    return val.FloatInitVals[idx];
}

void Lval::TypeCheck() { 
    is_left = false;
    std::vector<int> arrayindexs;
    bool arrayindexConstTag = true;
    if (dims != nullptr) {
        for (auto d : *dims) {
            d->TypeCheck();
            if (d->attribute.T.type == Type::VOID) {
                error_msgs.push_back("Array Dim can not be void in line " + std::to_string(line_number) + "\n");
            } else if (d->attribute.T.type == Type::FLOAT) {
                error_msgs.push_back("Array Dim can not be float in line " + std::to_string(line_number) + "\n");
            }
            arrayindexs.push_back(d->attribute.V.val.IntVal);
            arrayindexConstTag &= d->attribute.V.ConstTag;
        }
    }

    VarAttribute val = semant_table.symbol_table.lookup_val(name);
    if (val.type != Type::VOID) {    // local var
        scope = semant_table.symbol_table.lookup_scope(name);
    } else if (semant_table.GlobalTable.find(name) != semant_table.GlobalTable.end()) {    // global var
        val = semant_table.GlobalTable[name];
        scope = 0;
    } else {
        error_msgs.push_back("Undefined var in line " + std::to_string(line_number) + "\n");
        return;
    }

    if (arrayindexs.size() == val.dims.size()) {    // lval is a number(not a array)
        attribute.V.ConstTag = val.ConstTag & arrayindexConstTag;
        attribute.T.type = val.type;
        if (attribute.V.ConstTag) {
            if (attribute.T.type == Type::INT) {
                attribute.V.val.IntVal = GetArrayIntVal(val, arrayindexs);
            } else if (attribute.T.type == Type::FLOAT) {
                attribute.V.val.FloatVal = GetArrayFloatVal(val, arrayindexs);
            }
        }
    } else if (arrayindexs.size() < val.dims.size()) {    // lval is a array
        attribute.V.ConstTag = false;
        attribute.T.type = Type::PTR;
    } else {
        error_msgs.push_back("Array is unmatched in line " + std::to_string(line_number) + "\n");
    }
    //TODO("Lval Semant"); 
}

void FuncRParams::TypeCheck() { 
    if (params != nullptr) {
        for (auto param : *params) {
            param->TypeCheck();
            if (param->attribute.T.type == Type::VOID) {
                error_msgs.push_back("Function parameter cannot be void type in line " + 
                    std::to_string(param->GetLineNumber()) + "\n");
            }
        }
    }
    //TODO("FuncRParams Semant"); 
}

void Func_call::TypeCheck() {
    // 1. 检查和处理参数
    if (funcr_params != nullptr) {
        funcr_params->TypeCheck();
    }
    
    int param_count = (funcr_params == nullptr) ? 0 : 
        ((FuncRParams*)funcr_params)->params->size();

    // 2. 处理内置函数putf
    if (name->get_string() == "putf") {
        if (param_count < 1) {
            error_msgs.push_back("putf requires at least one parameter in line " + 
                std::to_string(line_number) + "\n");
        }
        return;
    }

    // 3. 检查函数定义
    auto func_it = semant_table.FunctionTable.find(name);
    if (func_it == semant_table.FunctionTable.end()) {
        error_msgs.push_back("Undefined function " + name->get_string() + 
            " in line " + std::to_string(line_number) + "\n");
        return;
    }

    // 4. 检查参数个数匹配
    FuncDef func_def = func_it->second;
    if (func_def->formals->size() != param_count) {
        error_msgs.push_back("Parameter count mismatch for function " + name->get_string() + 
            " in line " + std::to_string(line_number) + "\n");
    }

    // 5. 设置返回值类型
    attribute.T.type = func_def->return_type;
    attribute.V.ConstTag = false;
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
    GlobalStrCnt += 1;
    auto GlobalStrI = new GlobalStringConstInstruction(str->get_string(), ".str" + std::to_string(GlobalStrCnt));
    llvmIR.global_def.push_back(GlobalStrI);
    semant_table.GlobalStrTable[str] = GlobalStrCnt;
    //TODO("StringConst Semant"); 
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
    ((Lval *)lval)->is_left = true; 
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
    Cond->TypeCheck();

    if (Cond->attribute.T.type == Type::VOID) {
        error_msgs.push_back("while cond type is invalid " + std::to_string(line_number) + "\n");
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

// Type::VOID -> VOID    Type::Int -> I32    Type::FLOAT -> FLOAT32
BasicInstruction::LLVMType Type2LLvm[6] = {BasicInstruction::LLVMType::VOID, BasicInstruction::LLVMType::I32, BasicInstruction::LLVMType::FLOAT32,
                         BasicInstruction::LLVMType::I1,   BasicInstruction::LLVMType::PTR, BasicInstruction::LLVMType::DOUBLE};

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
        
        /*// 检查每个初始化值的类型是否匹配变量的类型（例如int、float等）
        if (init_val->attribute.T.type != attribute.T.type) {
            error_msgs.push_back("Error: Type mismatch in VarInitVal initializer at line " + std::to_string(line_number) + "\n");
        }*/
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

        // add Global Decl llvm ins
        BasicInstruction::LLVMType lltype = Type2LLvm[type_decl];

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