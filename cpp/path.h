#include <omnetpp.h>
#include "flow.h"

// TODO typedef std::pair<std::queue<Path>, PathList> DetourSearchState;
typedef cTopology::Node Node;

extern cModule* getSourceModule(Flow *flow);
extern void printPath(Path path);
extern void printPaths(PathList paths);

extern Path getShortestPathDijkstra(cModule *srcMod, cModule *dstMod);
extern Path getShortestPathBfs(Node *srcNode, Node *dstNode);
extern Path getShortestPathBfs(cModule *srcMod, cModule *dstMod);

extern PathList getPathsAroundShortest(Node *srcNode, Node *dstNode);
extern PathList getPathsAroundShortest(cModule *srcMod, cModule *dstMod);
extern PathList calculatePathsBetween(cModule *srcMod, cModule *dstMod); // DFS

extern PathList getShortestPaths(PathList paths);
extern PathList getAvailablePaths(PathList paths, double bps, Priority p);
extern PathList waypointsToAvailablePaths(Path waypoints, double bps, Priority p);

extern Flow* createFlow(Path path, double bps, Priority p);
extern Flow* createFlow(Flow* f);
extern bool revokeFlow(Flow* f);
