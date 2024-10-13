%option noyywrap
%{
#include <cmath>
#include <vector>
#include <cstring>
#include "../include/symtab.h"
#include "../parser/SysY_parser.tab.h"

extern int line_number;
extern int col_number;
extern int cur_col_number;
extern IdTable id_table;
void yyerror(char* s, ...);
%}

%x COMMENT

%%
    /* TODO():增加处理注释的代码*/
    /* 处理注释的代码 */
"//"[^\n]* {
    col_number += strlen(yytext);  // 更新列号
}
"/*" {
    cur_col_number = col_number;
    BEGIN(COMMENT);
    col_number += 2; // 计入 '/*' 的两个字符
}
"*/" {
    cur_col_number = col_number;
    col_number += 2;
    yylval.error_msg = "Unmatched */";
    return ERROR;
}
<COMMENT>{
    "*/" {
        BEGIN(INITIAL);
        col_number += 2;  // 计入 '*/' 的两个字符
    }
    "\n" {
        ++line_number;
        col_number = 0;
    }
    . {
        col_number += strlen(yytext);
    }
    <<EOF>> {
        BEGIN(INITIAL);
        yylval.error_msg = "Unmatched /*";
        return ERROR;
    }
}

    /* TODO():增加处理列号的代码(cur_col_number表示当前token开始位置, col_number表示当前token结束位置) */
"<=" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return LEQ;
}   

">=" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return GEQ;
}    

"==" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return EQ;
}        

"!=" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return NE;
}

"&&" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return AND;
}         

"||" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return OR;
}

"const" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return CONST;
}     

"if" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return IF;
}       

"else" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return ELSE;
}  

"while" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return WHILE;
}

"void" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return NONE_TYPE;
}    

"int" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return INT;
}    

"float" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return FLOAT;
}      

"return" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return RETURN;
}    

"break" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return BREAK;
}     

"continue" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return CONTINUE;
}

    /*for*/
"for" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return FOR;
}

    /*TODO*/
"TODO" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return TODO;
}

\n {++line_number;col_number = 0;}

[ \t\f\r\v] {col_number += strlen(yytext);}

[\.\+\-\*\/\=\<\!\%\>] {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return yytext[0];}

[\{\}\;\(\)\,\[\]] {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    return yytext[0];}

[_a-zA-Z][_a-zA-Z0-9]* {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.symbol_token = id_table.add_id(std::string(yytext));
    return IDENT;
}
([1-9][0-9]*)|0 {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    // yylval.int_token = stoi(std::string(yytext));
    yylval.int_token = 0;
    for(int i = 0;yytext[i];i++){
        yylval.int_token *= 10;
        yylval.int_token += yytext[i] - '0';
    }
    return INT_CONST;
}

    /*
      在词法分析中，你只需要考虑yylval的三种类型，分别为int_token, float_token, symbol_token
      之所以要定义这三种类型，是因为我们只返回这个token的类型是不够的，我们还需要知道这个token的属性
      例如我们需要知道INT_CONST具体的值是多少
    */
    /*TODO():参考SysY2022定义, 处理更多的词法, 如浮点数，十六进制数等*/

    /*八进制*/
"0"[0-7]+ {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.int_token = 0;

    // 将八进制字符串逐个字符解析成十进制整数
    for (int i = 1; yytext[i]; i++) {  // 从下标1开始，忽略前导0
        yylval.int_token = yylval.int_token * 8 + (yytext[i] - '0');
    }

    return INT_CONST;  // 返回整数常量类型
}

    /*十六进制*/
"0"[xX][0-9a-fA-F]+ {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.int_token = 0;

    // 将十六进制字符串逐个字符解析成十进制整数
    for (int i = 2; yytext[i]; i++) {  // 从下标2开始，忽略0x或0X前缀
        if (yytext[i] >= '0' && yytext[i] <= '9') {
            yylval.int_token = yylval.int_token * 16 + (yytext[i] - '0');
        } else if (yytext[i] >= 'a' && yytext[i] <= 'f') {
            yylval.int_token = yylval.int_token * 16 + (yytext[i] - 'a' + 10);
        } else if (yytext[i] >= 'A' && yytext[i] <= 'F') {
            yylval.int_token = yylval.int_token * 16 + (yytext[i] - 'A' + 10);
        }
    }

    return INT_CONST;  // 返回整数常量类型
}

([0-9]+\.[0-9]*|[0-9]*\.[0-9]+)([eE][+-]?[0-9]+)?|[0-9]+[eE][+-]?[0-9]+ {
    cur_col_number = col_number;
    col_number += strlen(yytext);

    const char* p = yytext;

    double integer_part = 0.0;
    double fraction_part = 0.0;
    int exponent = 0;
    int exponent_sign = 1;
    int num_fraction_digits = 0;
    int state = 0; // 0: 整数部分, 1: 小数部分

    // 解析整数和小数部分
    while (*p != '\0' && *p != 'e' && *p != 'E') {
        if (*p == '.') {
            if (state == 0) {
                state = 1;
            } else {
                yylval.error_msg = "Invalid float format";
                return ERROR;
            }
            p++;
            continue;
        }
        if ('0' <= *p && *p <= '9') {
            if (state == 0) {
                integer_part = integer_part * 10 + (*p - '0');
            } else if (state == 1) {
                fraction_part = fraction_part * 10 + (*p - '0');
                num_fraction_digits++;
            }
            p++;
        } else {
            yylval.error_msg = "Invalid character in float";
            return ERROR;
        }
    }

    // 合并整数和小数部分
    double result = integer_part + fraction_part / pow(10.0, num_fraction_digits);

    // 解析指数部分
    if (*p == 'e' || *p == 'E') {
        p++;
        // 处理指数的正负号
        if (*p == '+') {
            p++;
        } else if (*p == '-') {
            exponent_sign = -1;
            p++;
        }
        // 解析指数值
        while (*p != '\0') {
            if ('0' <= *p && *p <= '9') {
                exponent = exponent * 10 + (*p - '0');
                p++;
            } else {
                yylval.error_msg = "Invalid character in exponent";
                return ERROR;
            }
        }
    }

    // 计算指数因子
    double exp_result = pow(10.0, exponent_sign * exponent);

    // 计算最终结果
    result *= exp_result;

    yylval.float_token = result;
    return FLOAT_CONST;
}


0[xX]([0-9a-fA-F]+(\.[0-9a-fA-F]*)?|\.[0-9a-fA-F]+)[pP][+-]?[0-9]+ {
    cur_col_number = col_number;
    col_number += strlen(yytext);

    const char* p = yytext;

    // 检查 '0x' 或 '0X' 前缀
    if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    } else {
        yylval.error_msg = "Invalid hexadecimal float format";
        return ERROR;
    }

    unsigned long long integer_part = 0;
    unsigned long long fraction_part = 0;
    int fraction_digits = 0;
    int exponent = 0;
    int exponent_sign = 1;
    int state = 0; // 0: 整数部分, 1: 小数部分

    // 解析整数部分和小数部分
    while (*p != '\0' && *p != 'p' && *p != 'P') {
        if (*p == '.') {
            if (state == 0) {
                state = 1;
            } else {
                yylval.error_msg = "Invalid hexadecimal float format";
                return ERROR;
            }
            p++;
            continue;
        }
        if (isxdigit(*p)) {
            int digit = 0;
            if ('0' <= *p && *p <= '9') {
                digit = *p - '0';
            } else if ('a' <= *p && *p <= 'f') {
                digit = 10 + (*p - 'a');
            } else if ('A' <= *p && *p <= 'F') {
                digit = 10 + (*p - 'A');
            }
            if (state == 0) {
                integer_part = integer_part * 16 + digit;
            } else if (state == 1) {
                fraction_part = fraction_part * 16 + digit;
                fraction_digits++;
            }
            p++;
        } else {
            yylval.error_msg = "Invalid character in hexadecimal float";
            return ERROR;
        }
    }

    // 计算尾数
    double mantissa = (double)integer_part;
    if (fraction_digits > 0) {
        mantissa += fraction_part / pow(16.0, fraction_digits);
    }

    // 解析指数部分
    if (*p == 'p' || *p == 'P') {
        p++;
        // 处理指数的正负号
        if (*p == '+') {
            p++;
        } else if (*p == '-') {
            exponent_sign = -1;
            p++;
        }
        if (!isdigit(*p)) {
            yylval.error_msg = "Invalid exponent in hexadecimal float";
            return ERROR;
        }
        // 解析指数的值
        while (*p != '\0') {
            if (isdigit(*p)) {
                exponent = exponent * 10 + (*p - '0');
                p++;
            } else {
                yylval.error_msg = "Invalid character in exponent";
                return ERROR;
            }
        }
    } else {
        yylval.error_msg = "Missing exponent in hexadecimal float";
        return ERROR;
    }

    // 计算最终结果
    int total_exponent = exponent_sign * exponent;
    double result = mantissa * pow(2.0, total_exponent);

    yylval.float_token = result;
    return FLOAT_CONST;
}

    /* 字符串常量的检测规则 */
\"(\\.|[^"\\\n])*\" {
    cur_col_number = col_number;
    col_number += strlen(yytext);

    std::string raw_string(yytext);

    std::string content = raw_string.substr(1, raw_string.length() - 2);

    std::string processed_string;
    for (size_t i = 0; i < content.length(); ) {
        if (content[i] == '\\') {
            i++;
            if (i >= content.length()) {
                static char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), "Invalid escape sequence at end of string");
                yylval.error_msg = error_msg;
                return ERROR;
            }
            char esc_char = content[i];
            switch (esc_char) {
                case 'n':
                    processed_string += "\\n";
                    break;
                case 't':
                    processed_string += "\\t";
                    break;
                case 'r':
                    processed_string += "\\r";
                    break;
                case '\\':
                    processed_string += "\\\\";
                    break;
                case '\'':
                    processed_string += "\\'";
                    break;
                case '"':
                    processed_string += "\\\"";
                    break;
                case '0':
                    processed_string += "\\0";
                    break;
                default:
                    {
                        static char error_msg[256];
                        snprintf(error_msg, sizeof(error_msg), "Invalid escape sequence: \\%c", esc_char);
                        yylval.error_msg = error_msg;
                        return ERROR;
                    }
            }
            i++;
        } else {
            processed_string += content[i];
            i++;
        }
    }

    std::string final_string = "\"" + processed_string + "\"";
    yylval.symbol_token = id_table.add_id(final_string);

    return STR_CONST;
}

    /*unknown tokens, return ERROR*/
. {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.error_msg = yytext;
    return ERROR;
}
%%
