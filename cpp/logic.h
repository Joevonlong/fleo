#pragma once
#include <omnetpp.h>
#include "global.h"
#include "user.h"

class Logic : public cSimpleModule
{
public:
    std::map<int, cGate*> nextGate; // maps user index to next cGate*
    //void setupFlowFrom(User *user);
    std::vector<int> findAvailablePathFrom(User *user, double bpsWanted);
protected:
    Global *global;
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    cTopology topo;
    int nearestCache;
    int nearestCompleteCache;
    double distToCompleteCache;
    void registerSelfIfCache();
    int64_t checkCache(int customID);
    void requestFromCache(int cacheID, int customID);
private:
};
