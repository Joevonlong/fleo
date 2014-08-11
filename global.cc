#include "global.h"
#include "routing.h"
#include "parse.h"

Define_Module(Global);

void Global::initialize()
{
    topoSetup();
    loadVideoLengthFile();
}

