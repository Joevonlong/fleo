#include <omnetpp.h>
#include <deque>
#include "Flow.h"

// TODO typedef std::pair<std::queue<Path>, PathList> DetourSearchState;
typedef cTopology::Node Node;
typedef struct {
    std::deque<Path> searchingQ;
    std::set<Path> searchingSet;
    PathList searched;
    std::set<Path> searchedSet;
} searchState;

//extern cModule* getSourceModule(Flow *flow);
extern void printPath(Path path);
extern void printPaths(PathList paths);
//extern bool fillChannels(Flow* f);
//extern simtime_t pathLag(Path path);

extern Path getShortestPathDijkstra(cModule *srcMod, cModule *dstMod);
extern Path getShortestPathBfs(Node *srcNode, Node *dstNode);
extern Path getShortestPathBfs(cModule *srcMod, cModule *dstMod);

extern Path getDetour(Node *srcNode, Node *dstNode, size_t index);
extern Path getDetour(cModule *srcMod, cModule *dstMod, size_t index);
extern PathList getPathsAroundShortest(Node *srcNode, Node *dstNode);
extern PathList getPathsAroundShortest(cModule *srcMod, cModule *dstMod);
extern PathList calculatePathsBetween(cModule *srcMod, cModule *dstMod); // DFS

extern PathList getShortestPaths(PathList paths);
extern bool availableNodePair(Node *from, Node *to, uint64_t bps, Priority p);
extern PathList getAvailablePaths(PathList paths, uint64_t bps, Priority p);
extern PathList waypointsToAvailablePaths(Path waypoints, uint64_t bps, Priority p);

//extern Flow* createFlow(Path path, uint64_t bps, Priority p);
//extern Flow* createFlow(Flow* f);
extern bool revokeFlow(Flow* f);
