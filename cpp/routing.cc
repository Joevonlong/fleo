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

// algo taken from http://mathoverflow.net/a/18634
// also consider: An algorithm for computing all paths in a graph
// [http://link.springer.com/article/10.1007%2FBF01966095]
Node *source;
Node *target;
Path path; // used as a stack, but vector provides iterator and random access
std::set<Node*> seen;
PathList paths;
bool _stuck(Node *n) { // helper function
    if (n == target) {return false;}
    for (int i = n->getNumOutLinks()-1; i>=0; i--) {
        EV << "in stuck trying numout #" << i << endl;
        Node *m = n->getLinkOut(i)->getRemoteNode(); // for each node beside head
        std::pair<std::set<Node*>::iterator,bool> ret = seen.insert(m); // try add it to seen
        if (ret.second) { // if inserted i.e. m was not in seen
            if (!_stuck(m)) { // and if it is not stuck
                return false; // this head is also not stuck
            }
        }
    }
    return true;
}
void _search(Node *n) { // helper function
    EV << "searching @ " << n->getModule()->getFullPath() << endl;
    if (n == target) {
        // found a path
        EV << "Path found:";
        for (Path::iterator it = path.begin(); it != path.end(); it++) {
            EV << " > " << (*it)->getModule()->getFullPath();
        }
        EV << endl;
        paths.push_back(Path(path));
    }
    // check if stuck
    seen = std::set<Node*>(path.begin(), path.end()); // copy path to seen
    if (_stuck(n)) {return;}
    // run search on each neighbour of n
    for (int i = n->getNumOutLinks()-1; i>=0 ; i--) {
        EV << "search:numout: " << i << endl;
        Node *m = n->getLinkOut(i)->getRemoteNode();
        // do not go backwards:
        bool neighbour_in_path = false;
        for (int j = path.size()-1; j >= 0; j--) {
            if (path[j] == m) {neighbour_in_path = true;}
        }
        if (neighbour_in_path) {continue;}
        // ----------
        path.push_back(m);
        _search(m);
        path.pop_back(); // popped item should be m
    }
}
PathList calculatePathsBetween(cModule *srcMod, cModule *dstMod) {
    // initialise source & target nodes
    source = topo.getNodeFor(srcMod);
    target = topo.getNodeFor(dstMod);
    // (re-)initialise stack
    path = Path();
    path.push_back(source);
    // (re-)initialise path list
    paths = PathList();
    // begin search
    _search(source);
    // relist all found paths
    EV << "Relisting paths found...\n";
    for (PathList::iterator outer_it = paths.begin(); outer_it != paths.end(); outer_it++) {
        EV << "path: ";
        for (Path::iterator inner_it = (*outer_it).begin(); inner_it != (*outer_it).end(); inner_it++) {
            EV << (*inner_it)->getModule()->getFullPath() << " > ";
        }
        EV << endl;
    }
    return PathList(paths); // return a copy
}

Path getShortestPath(PathList paths) {
    /**
     * Returns Path with the least number of Nodes.
     * TODO check bandwidth availability
     **/
    // initialisation
    unsigned int minHops = UINT_MAX;
    Path shortest = Path();
    // for each path, if shorter than current shortest, become new shortest
    for (PathList::iterator it = paths.begin(); it != paths.end(); it++) {
        if (it->size() < minHops) {
            minHops = it->size();
            shortest = Path(*it);
        }
    }
    return shortest;
}

bool _getAvailablePathsHelper(Node *from, Node *to, double datarate) {
    /**
     * Checks if datarate is available between from and to
     **/
    for (int i = from->getNumOutLinks()-1; i>=0; i--) { // try each link
        if (from->getLinkOut(i)->getRemoteNode() == to) { // until the other node is found
            if (from->getLinkOut(i)->getLocalGate()->getTransmissionChannel()->getNominalDatarate() >= datarate) { // then check if bandwidth is available
                return true;
            }
        }
    }
    return false;
}
PathList getAvailablePaths(PathList paths, double datarate) {
    /**
     * Filters given paths to only those that have the datarate in all links
     **/
    // initialisation
    PathList available = PathList();
    bool possible = false;
    // for each path, if all links have at least datarate, copy onto available
    for (PathList::iterator outer_it = paths.begin(); outer_it != paths.end(); outer_it++) { // for each path
        possible = true;
        for (Path::iterator inner_it = outer_it->begin(); inner_it != outer_it->end()-1; inner_it++) { // starting from the first node
            if (!_getAvailablePathsHelper(*inner_it, *(inner_it+1), datarate)) {
                possible = false;
                break;
            }
            // assert(possible);
        }
        if (possible) {
            available.push_back(Path(*outer_it));
        }
    }
    return available;
}
