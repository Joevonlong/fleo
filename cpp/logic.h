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
    Flow* processRequest(int vID, Path waypoints);
protected:
    Global *global;
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    cTopology topo;
    // cache-related
    int nearestCache;
    int nearestCompleteCache;
    double distToCompleteCache;
    void registerSelfIfCache();
    int64_t checkCache(int customID);
    bool hasCache();
    bool isOrigin();
    void requestFromCache(int cacheID, int customID);
    //
private:
};
