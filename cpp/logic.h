#pragma once
#include <omnetpp.h>
#include <deque>
#include "global.h"
#include "user.h"

class Logic : public cSimpleModule
{
public:
    std::map<int, cGate*> nextGate; // maps user index to next cGate*
    //void setupFlowFrom(User *user);
    std::vector<int> findAvailablePathFrom(User *user, double bpsWanted);
    std::deque<Logic*> getRequestWaypoints(int vID, int tries);
    bool hasCache();
    bool isOrigin();
    int64_t checkCache(int customID);
    void setCached(int customID, bool b);
protected:
    Global *global;
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    // DELME cTopology topo;
    // cache-related
    int nearestCache;
    int nearestCompleteCache;
    double distToCompleteCache;
    void registerSelfIfCache();
    void requestFromCache(int cacheID, int customID);
    //
private:
};
