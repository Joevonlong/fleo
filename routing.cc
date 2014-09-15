#include "global.h"
#include "routing.h"
#include "request_m.h"
#include "reply_m.h"

cTopology topo;

cGate* getNextGate(Logic* current, cMessage* msg) {
    // get destination module ID
    int destID;
    if (msg->getKind() == requestKind) {
        destID = (check_and_cast<Request*>(msg))->getDestinationID();
    }
    else if (msg->getKind() == replyKind) {
        destID = (check_and_cast<Reply*>(msg))->getDestinationID();
    }
    else {
        EV << "Error in getNextGate : unknown message kind\n";
    }

    // check if answer is not cached
    if (!current->nextGate[destID]) {
        EV << "next gate not cached\n";
        cTopology::Node *destNode =
            topo.getNodeFor(simulation.getModule(destID));
        EV << "destnode " << destNode << endl;
        topo.calculateUnweightedSingleShortestPathsTo(destNode);
        cTopology::Node *currentNode = topo.getNodeFor(current);
        //EV << "currentnode " << currentNode << endl;
        cTopology::LinkOut *next = currentNode->getPath(0);
        //EV << "next " << next << endl;

        // cache the answer
        current->nextGate[destID] = next->getLocalGate();
    }
    else {
        EV << "next gate is cached\n";
    }
    return current->nextGate[destID];
}

// returns ID closest to ID given in arg1, excluding itself.
int getNearestCacheID(int userID) {
    double shortestDist = DBL_MAX;
    int nearestCacheID = -1;
    cTopology::Node *destNode =
        topo.getNodeFor(simulation.getModule(userID));
    topo.calculateUnweightedSingleShortestPathsTo(destNode);
    for (std::vector<int>::iterator id = cacheIDs.begin();
        id != cacheIDs.end(); id++) {
        if (*id != userID) {
            cTopology::Node *cacheNode = topo.getNodeFor(simulation.getModule(*id));
            if (cacheNode->getDistanceToTarget() < shortestDist) {
                nearestCacheID = *id;
                shortestDist = cacheNode->getDistanceToTarget();
            }
        }
    }
    return nearestCacheID;
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
  topo.extractByNedTypeName(cStringTokenizer("Buffer PoPLogic CoreLogic BeyondLogic Logic User").asVector());
  //topo.extractByProperty("display");
  //topo.extractFromNetwork(selectFunction, NULL);
  EV << topo.getNumNodes() << " nodes in routing topology\n";
  /*
  EV << "topo nodes: " << topo.getNumNodes() << endl;
  for (int i=0; i<topo.getNumNodes(); i++)
    {
      cTopology::Node *node = topo.getNode(i);
      ev << "Node i=" << i << " is " << node->getModule()->getFullPath() << endl;
      ev << " It has " << node->getNumOutLinks() << " conns to other nodes\n";
      ev << " and " << node->getNumInLinks() << " conns from other nodes\n";

      ev << " Connections to other modules are:\n";
      for (int j=0; j<node->getNumOutLinks(); j++)
        {
          cTopology::Node *neighbour = node->getLinkOut(j)->getRemoteNode();
          cGate *gate = node->getLinkOut(j)->getLocalGate();
          ev << " " << neighbour->getModule()->getFullPath()
             << " through gate " << gate->getFullName() << endl;
        }
    }
  */
}

