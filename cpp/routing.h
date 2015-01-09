#include "request_m.h"
#include "reply_m.h"
#include "logic.h"

extern cTopology topo;
extern std::string beyondPath;
extern void topoSetup();
extern cGate* getNextGate(Logic* current, cMessage* msg);
extern double getDistanceBetween(int originID, int destID);
extern int getNearestCacheID(int userID);
extern int getNearestID(int originID, std::vector<int> candidateIDs);

typedef std::vector<cTopology::Node*> Path;
typedef std::vector<Path> PathList;
extern void printPath(Path path);
extern void printPaths(PathList paths);
extern PathList calculatePathsBetween(cModule *srcMod, cModule *dstMod);
extern Path getShortestPath(PathList paths);
extern PathList getAvailablePaths(PathList paths, double datarate);
