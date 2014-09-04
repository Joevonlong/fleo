#pragma once
#include <omnetpp.h>

extern simsignal_t idleSignal;
extern simsignal_t requestSignal;

extern int requestKind;
extern int replyKind;

class Global : public cSimpleModule
{
protected:
    virtual void initialize();
};

