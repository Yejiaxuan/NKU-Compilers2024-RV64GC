#ifndef INLINE_H
#define INLINE_H

#include "../../include/ir.h"
#include <map>
#include <unordered_map>

// 主要的函数内联接口
void FunctionInline(LLVMIR *IR);

// 内联分析和转换函数
void InlineDFS(CFG *uCFG);
bool IsInlineBetter(CFG *uCFG, CFG *vCFG);
CFG *CopyCFG(CFG *uCFG);
void InlineCFG(CFG *uCFG, CFG *vCFG, uint32_t CallINo);
Operand InlineCFG(CFG *uCFG, CFG *vCFG, LLVMBlock StartBB, LLVMBlock EndBB, 
                  std::map<int, int> &reg_replace_map, 
                  std::map<int, int> &label_replace_map);

#endif // INLINE_H