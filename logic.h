#pragma once
#include <omnetpp.h>
#include "global.h"

class Logic : public cSimpleModule
{
public:
    std::map<int, cGate*> nextGate; // maps user index to next cGate*
protected:
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    int64_t checkCache(int customID);
    void registerSelfIfCache();
private:
    int nearestCache;
};

