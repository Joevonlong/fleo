#include "global.h"
#include "routing.h"
#include "request_m.h"
#include "reply_m.h"
#include "mypacket_m.h"
#include "flowchannel.h"

cTopology topo;

// returns number of hops between the two module IDs
double getDistanceBetween(int originID, int destID) {
    cTopology::Node *originNode =
        topo.getNodeFor(simulation.getModule(originID));
    cTopology::Node *destNode =
        topo.getNodeFor(simulation.getModule(destID));
    topo.calculateUnweightedSingleShortestPathsTo(destNode);
    return originNode->getDistanceToTarget();
}

// returns cacheID closest to ID given in arg1, excluding itself.
int getNearestCacheID(int userID) {
    return getNearestID(userID, cacheIDs);
}

// returns candidateID closest to originID, excluding itself.
int getNearestID(int originID, std::vector<int> candidateIDs) {
    double shortestDist = DBL_MAX;
    int nearestID = -1;
    cTopology::Node *originNode =
        topo.getNodeFor(simulation.getModule(originID));
    topo.calculateUnweightedSingleShortestPathsTo(originNode);
    for (std::vector<int>::iterator id = candidateIDs.begin();
        id != candidateIDs.end(); ++id) {
        if (*id != originID) {
            cTopology::Node *candidateNode =
                topo.getNodeFor(simulation.getModule(*id));
            if (candidateNode->getDistanceToTarget() < shortestDist) {
                nearestID = *id;
                shortestDist = candidateNode->getDistanceToTarget();
            }
        }
    }
    return nearestID;
}

bool selectFunction(cModule *mod, void *)
{
  // EV << "selfunc debug:" <<  mod->getFullName() << mod->getParentModule() << simulation.getSystemModule() << endl;
  return mod->getParentModule() == simulation.getSystemModule();
}

// called from global during init
void topoSetup()
{
  topo.clear();
  // 4 methods:
  //topo.extractByModulePath(cStringTokenizer("*").asVector());
  topo.extractByNedTypeName(cStringTokenizer("PoPLogic CoreLogic BeyondLogic Logic User").asVector());
  //topo.extractByProperty("display");
  //topo.extractFromNetwork(selectFunction, NULL);
  EV << topo.getNumNodes() << " nodes in routing topology\n";
}
