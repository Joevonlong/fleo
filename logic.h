#pragma once
#include <omnetpp.h>
#include "global.h"

class Logic : public cSimpleModule
{
public:
    std::map<std::string, cGate*> nextGate; // maps user index to next cGate*
protected:
    int64_t checkCache(int customID);
};

