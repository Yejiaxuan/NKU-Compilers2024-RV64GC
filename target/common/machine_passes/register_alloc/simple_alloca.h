#ifndef SIMPLE_ALLOCA_H
#define SIMPLE_ALLOCA_H
#include "basic_register_allocation.h"

class SimpleAlloca : public RegisterAllocation {
private:
    void CoalesceInCurrentFunc();

protected:
    bool DoAllocInCurrentFunc();

public:
    SimpleAlloca(MachineUnit *unit, PhysicalRegistersAllocTools *phy, SpillCodeGen *spiller);
};

#endif