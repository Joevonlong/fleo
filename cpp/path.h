#include <omnetpp.h>

typedef std::vector<cTopology::Node*> Path;
typedef std::vector<Path> PathList;
struct Flow {
    Path path;
    // remember gates too?
    double bps;
};

extern void printPath(Path path);
extern void printPaths(PathList paths);
extern PathList calculatePathsBetween(cModule *srcMod, cModule *dstMod);
extern Path getShortestPath(PathList paths);
extern PathList getAvailablePaths(PathList paths, double datarate);
extern Flow createFlow(Path path, double bps);
extern bool revokeFlow(Flow flow);
