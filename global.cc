#include "global.h"
#include "routing.h"
#include "parse.h"

Define_Module(Global);

void Global::initialize()
{
    idleSignal = registerSignal("idle"); // name assigned to signal ID
    requestSignal = registerSignal("request"); // name assigned to signal ID
    topoSetup();
    loadVideoLengthFile();
    EV << static_cast<double>(UINT64_MAX) << endl;
}

