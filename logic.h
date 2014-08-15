#pragma once
#include <omnetpp.h>
#include "global.h"

class Logic : public cSimpleModule
{
protected:
public:
  std::map<int, cGate*> nextGate; // maps user index to next cGate*
};

