#ifndef SIMPLE_ADCE_H
#define SIMPLE_ADCE_H

#include "../../include/ir.h"
#include "../pass.h"
#include <string>
#include <map>
#include <unordered_set>
#include <vector>

/*
激进死代码删除（ADCE）：
1. 首先通过对函数控制流图(CFG)的遍历，确定哪些基本块是从入口可达的。
2. 对不可达基本块中的指令直接删除。
3. 对可达基本块执行类似DCE的流程，但在此过程中，所有不影响程序外部行为（包括无副作用指令且其结果寄存器不被最终使用）的指令都将被删除。
*/

class SimpleADCEPass : public IRPass {
private:
    void EliminateDeadCode(CFG *C);

public:
    SimpleADCEPass(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
};

#endif
