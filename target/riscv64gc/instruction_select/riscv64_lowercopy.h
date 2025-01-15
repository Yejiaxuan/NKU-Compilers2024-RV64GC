#ifndef RV64_PHI_DESTRUCTION_H
#define RV64_PHI_DESTRUCTION_H

#include "../../common/machine_passes/machine_phi_destruction.h"
#include "../riscv64.h" // 添加此行以包含 RiscV64InstructionConstructor 的定义

// 基类，用于共享辅助函数
class RiscV64LowerBase : public MachinePass {
protected:
    // 辅助函数：处理寄存器拷贝
    void HandleRegisterCopy(MachineCopyInstruction *m_copy,
                            std::list<MachineBaseInstruction *>::iterator &it,
                            MachineBlock *block,
                            MachineFunction *function);
    
    // 辅助函数：处理立即数拷贝
    void HandleImmediateCopy(MachineCopyInstruction *m_copy,
                             std::list<MachineBaseInstruction *>::iterator &it,
                             MachineBlock *block,
                             MachineFunction *function);
    
public:
    RiscV64LowerBase(MachineUnit *m_unit) : MachinePass(m_unit) {}
    virtual void Execute() = 0; // 纯虚函数，子类需实现
};

// 处理通用拷贝
class RiscV64LowerCopy : public RiscV64LowerBase {
public:
    RiscV64LowerCopy(MachineUnit *m_unit) : RiscV64LowerBase(m_unit) {}
    void Execute() override;
};

// 处理浮点立即数拷贝
class RiscV64LowerFImmCopy : public RiscV64LowerBase {
public:
    RiscV64LowerFImmCopy(MachineUnit *m_unit) : RiscV64LowerBase(m_unit) {}
    void Execute() override;
};

// 处理整数立即数拷贝
class RiscV64LowerIImmCopy : public RiscV64LowerBase {
public:
    RiscV64LowerIImmCopy(MachineUnit *m_unit) : RiscV64LowerBase(m_unit) {}
    void Execute() override;
};

#endif

