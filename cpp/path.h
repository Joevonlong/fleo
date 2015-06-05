#include <omnetpp.h>
#include <queue>
#include "flow.h"

// TODO typedef std::pair<std::queue<Path>, PathList> DetourSearchState;
typedef cTopology::Node Node;
struct searchState {
    std::queue<Path> searchingQ;
    std::set<Path> searchingSet;
    PathList searched;
    std::set<Path> searchedSet;
};

extern cModule* getSourceModule(Flow *flow);
extern void printPath(Path path);
extern void printPaths(PathList paths);
extern simtime_t pathLag(Path path);
extern std::vector<cChannel*> getChannels(Path path);

extern Path getShortestPathDijkstra(cModule *srcMod, cModule *dstMod);
extern Path getShortestPathBfs(Node *srcNode, Node *dstNode);
extern Path getShortestPathBfs(cModule *srcMod, cModule *dstMod);

extern Path getNextDetour(Node *srcNode, Node *dstNode);
extern Path getNextDetour(cModule *srcMod, cModule *dstMod);
extern PathList getPathsAroundShortest(Node *srcNode, Node *dstNode);
extern PathList getPathsAroundShortest(cModule *srcMod, cModule *dstMod);
extern PathList calculatePathsBetween(cModule *srcMod, cModule *dstMod); // DFS

extern PathList getShortestPaths(PathList paths);
extern PathList getAvailablePaths(PathList paths, uint64_t bps, Priority p);
extern PathList waypointsToAvailablePaths(Path waypoints, uint64_t bps, Priority p);

extern Flow* createFlow(Path path, uint64_t bps, Priority p);
extern Flow* createFlow(Flow* f);
extern bool revokeFlow(Flow* f);
