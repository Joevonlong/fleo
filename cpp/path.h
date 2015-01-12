#include <omnetpp.h>

typedef std::vector<cTopology::Node*> Path;
typedef std::vector<Path> PathList;
struct Flow {
    Path path;
    // remember gates too?
    double bps;
    // TODO int priority;
    // MAYBE cMessage* completionTimer;
    // MAYBE uint64_t videoSize/videoLength;
    // MAYBE uint64_t transferred/completed;
    // MAYBE simtime_t lastTransferredUpdateTime; //(currentTime-lastupdateTime)*bitrate = additional transferred bits
    // not necessary? since higher qualities simulated using separate flows so each flow has a fixed bitrate (though aggregate varies)
};

extern void printPath(Path path);
extern void printPaths(PathList paths);
extern PathList calculatePathsBetween(cModule *srcMod, cModule *dstMod);
extern Path getShortestPath(PathList paths);
extern PathList getAvailablePaths(PathList paths, double datarate);
extern Flow createFlow(Path path, double bps);
extern bool revokeFlow(Flow flow);
