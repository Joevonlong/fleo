#include "request_m.h"
#include "reply_m.h"
#include "logic.h"

extern cTopology topo;
extern std::string beyondPath;
extern void topoSetup();
extern cGate* getNextGate(Logic* current, cMessage* msg);
extern int getNearestCacheID(int userID);
extern int getNearestID(int originID, std::vector<int> candidateIDs);

