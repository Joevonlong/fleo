#include "request_m.h"
#include "reply_m.h"
#include "logic.h"

extern cTopology topo;
cGate* getNextGate(Logic* current, Request* request);
cGate* getNextGate(Logic* current, Reply* reply);
extern std::string beyondPath;
void topoSetup();

