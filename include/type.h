#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <string>
#include <vector>

class Type {
public:
    // 我们认为数组的类型为PTR
    enum ty { VOID = 0, INT = 1, FLOAT = 2, BOOL = 3, PTR = 4, DOUBLE = 5, UNDEFINED = 6} type;
    std::string GetTypeInfo();
    Type() { type = VOID; }
};

class ConstValue {
public:
    bool ConstTag;
    union ConstVal {
        bool BoolVal;
        int IntVal;
        float FloatVal;
        double DoubleVal;
    } val;
    std::string GetConstValueInfo(Type ty);
    ConstValue() {
        val.IntVal = 0;
        ConstTag = false;
    }
};

// 变量的属性
class VarAttribute {
public:
    Type::ty type;
    bool ConstTag = 0;
    std::vector<int> dims{};    // 存储数组类型的相关信息
    // 对于数组的初始化值，我们将高维数组看作一维后再存储 eg.([3 x [4 x i32]] => [12 x i32])
    std::vector<int> IntInitVals{}; 
    std::vector<float> FloatInitVals{};

    // TODO():也许你需要添加更多变量
    VarAttribute() {
        type = Type::VOID;
        ConstTag = false;
    }
};

// 语法树节点的属性
class NodeAttribute {
public:
    enum opcode {
        ADD = 0,     // +
        SUB = 1,     // -
        MUL = 2,     // *
        DIV = 3,     // /
        MOD = 4,     // %
        LEQ = 5,     // <=
        LT = 6,      // <
        GEQ = 7,     // >=
        GT = 8,      // >
        EQ = 9,      // ==
        NEQ = 10,     // !=
        AND = 11,     // &&
        OR = 12,    // ||
    };
    int line_number = -1;
    Type T;
    ConstValue V;
    int result_reg;
    std::string GetAttributeInfo();
};

#endif
