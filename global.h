#pragma once
#include <omnetpp.h>

extern simsignal_t idleSignal;
extern simsignal_t requestSignal;

class Global : public cSimpleModule
{
public:
protected:
    virtual void initialize();
};

