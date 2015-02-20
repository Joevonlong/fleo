#include <omnetpp.h>
#include "flow.h"

// TODO typedef std::pair<std::queue<Path>, PathList> DetourSearchState;

extern cModule* getSourceModule(Flow *flow);
extern void printPath(Path path);
extern void printNodeDeque(NodeDeque nd);
extern void printPaths(PathList paths);
extern Path getShortestPathDijkstra(cModule *srcMod, cModule *dstMod);
extern Path getShortestPathBfs(cModule *srcMod, cModule *dstMod);

extern PathList getPathsAroundShortest(cModule *srcMod, cModule *dstMod);
extern PathList calculatePathsBetween(cModule *srcMod, cModule *dstMod); // DFS
extern PathList getShortestPaths(PathList paths);
extern PathList getAvailablePaths(PathList paths, double bps, Priority p);
extern Flow* createFlow(Path path, double bps, Priority p);
extern bool revokeFlow(Flow* f);
