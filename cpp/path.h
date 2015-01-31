#include <omnetpp.h>
#include "flow.h"

extern cModule* getSourceModule(Flow *flow);
extern void printPath(Path path);
extern void printPaths(PathList paths);
extern PathList calculatePathsBetween(cModule *srcMod, cModule *dstMod);
extern PathList getShortestPaths(PathList paths);
extern PathList getAvailablePaths(PathList paths, Flow* f);
extern Flow createFlow(Path path, double bps);
extern bool revokeFlow(Flow flow);
