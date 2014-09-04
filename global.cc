#include "global.h"
#include "routing.h"
#include "parse.h"

Define_Module(Global);

simsignal_t idleSignal;
simsignal_t requestSignal;
int requestKind;
int replyKind;

void Global::initialize()
{
    idleSignal = registerSignal("idle"); // name assigned to signal ID
    requestSignal = registerSignal("request"); // name assigned to signal ID
    requestKind = 123;
    replyKind = 321;
    topoSetup();
    loadVideoLengthFile();
    EV << static_cast<double>(UINT64_MAX) << endl;
}

