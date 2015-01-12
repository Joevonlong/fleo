#include <omnetpp.h>

typedef std::vector<cTopology::Node*> Path;
typedef std::vector<Path> PathList;
extern void printPath(Path path);
extern void printPaths(PathList paths);
extern PathList calculatePathsBetween(cModule *srcMod, cModule *dstMod);
extern Path getShortestPath(PathList paths);
extern PathList getAvailablePaths(PathList paths, double datarate);
