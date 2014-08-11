#include "global.h"
#include "routing.h"
#include "parse.h"

Define_Module(Global);

void Global::initialize()
{
    topoSetup();
    loadVideoLengthFile();
    EV << sizeof(SIM_API int) << endl;
    EV << intRandMax() << endl;
    EV << sizeof(uint64_t) << endl;
    EV << UINT64_MAX << endl;
}

