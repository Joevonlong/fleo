#include "global.h"
#include "routing.h"
#include "request_m.h"
#include "reply_m.h"
#include "mypacket_m.h"

typedef cTopology::Node Node;
cTopology topo;

cGate* getNextGate(Logic* current, cMessage* msg) {
    // get destination module ID
    int destID = (check_and_cast<MyPacket*>(msg))->getDestinationID();

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
        id != candidateIDs.end(); id++) {
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

Node *source;
Node *target;
std::vector<Node*> path; // used as a stack, but need iterator and access
std::set<Node*> seen;
bool _stuck(Node *n) { // helper function
    if (n == target) {return false;}
    int i = n->getNumOutLinks();
    for (; i>0; i--) {
        Node *m = n->getLinkOut(i)->getRemoteNode();
        if (seen.find(m) == seen.end()) { // i.e. m not in seen
            seen.insert(m);
            if (!_stuck(m)) {
                return false;
            }
        }
    }
    return true;
}
void _search(Node *n) { // helper function
    if (n == target) {
        // found a path. output it somewhere...
    }
    seen = std::set<Node*>(path.begin(), path.end()); // copy path to seen
    if (_stuck(n)) {return;}
    // run search on each neighbour of n
    int i = n->getNumOutLinks();
    for (; i>0; i--) {
        Node *m = n->getLinkOut(i)->getRemoteNode();
        path.push_back(m);
        _search(m);
        path.pop_back(); // should be m
    }
}
void calculatePathsBetween(cModule *srcMod, cModule *dstMod) {
    // initialise source & target nodes
    source = topo.getNodeFor(srcMod);
    target = topo.getNodeFor(dstMod);
    // (re-)initialise stack
    path = std::vector<Node*>();
    path.push_back(source);
    // begin search
    _search(source);
}
