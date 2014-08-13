#include "global.h"
#include "routing.h"
#include "parse.h"

Define_Module(Global);

void Global::initialize()
{
    topoSetup();
    loadVideoLengthFile();
    EV << std::max(2,5) << endl;
}

