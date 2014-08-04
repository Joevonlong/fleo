#pragma once
#include <omnetpp.h>

class Router : public cSimpleModule
{
protected:
  simsignal_t requestSignal;
public:
  std::map<int, cGate*> nextGate; // maps user index to next cGate*
};

