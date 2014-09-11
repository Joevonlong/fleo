#include "request_m.h"
#include "reply_m.h"
#include "logic.h"

extern cTopology topo;
cGate* getNextGate(Logic* current, cMessage* msg);
extern std::string beyondPath;
void topoSetup();

