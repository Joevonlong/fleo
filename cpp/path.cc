#include "path.h"
#include "routing.h"
#include "flowchannel.h"

typedef cTopology::Node Node;

cModule* getSourceModule(Flow *flow) {
    return flow->path[0]->getModule();
}

void printPath(Path path) {
    for (Path::iterator it = path.begin(); it != path.end(); it++) {
        EV << (*it)->getModule()->getFullPath() << " > ";
    }
    EV << endl;
}
void printPaths(PathList paths) {
    for (PathList::iterator it = paths.begin(); it != paths.end(); it++) {
        printPath(*it);
    }
}

// algo modified from http://mathoverflow.net/a/18634
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
        EV << "Path found: ";
        printPath(path);
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
    printPaths(paths);
    return PathList(paths); // return a copy
}

PathList getShortestPaths(PathList paths) {
    /**
     * Returns PathList with the least number of Nodes.
     * TODO check bandwidth availability?
     */
    // initialisation
    unsigned int minHops = UINT_MAX;
    PathList shortest = PathList();
    // for each path, if shorter than current shortest, become new shortest
    for (PathList::iterator it = paths.begin(); it != paths.end(); it++) {
        if (it->size() < minHops) {
            minHops = it->size();
            shortest.clear();
            shortest.push_back(*it);
        }
        else if (it->size() == minHops) {
            shortest.push_back(*it);
        }
    }
    return shortest;
}

bool _getAvailablePathsHelper(Node *from, Node *to, double datarate) {
    /**
     * Checks if datarate is available between two adjacent nodes
     */
    for (int i = from->getNumOutLinks()-1; i>=0; i--) { // try each link
        if (from->getLinkOut(i)->getRemoteNode() == to) { // until the other node is found
            if (((FlowChannel*)from->getLinkOut(i)->getLocalGate()->getTransmissionChannel())->getAvailableBps() >= datarate) { // then check if bandwidth is available
                return true;
            }
        }
    }
    return false;
}
PathList getAvailablePaths(PathList paths, double datarate) {
    /**
     * Filters given paths to only those that have the datarate in all links
     */
    // initialisation
    PathList available = PathList();
    bool possible = false;
    // for each path, if all links have at least datarate, copy onto available
    for (PathList::iterator outer_it = paths.begin(); outer_it != paths.end(); outer_it++) { // for each path
        possible = true;
        for (Path::iterator inner_it = outer_it->begin(); inner_it != outer_it->end()-1; inner_it++) { // starting from the first node
            if (!_getAvailablePathsHelper(*inner_it, *(inner_it+1), datarate)) { // check if datarate is available on each link
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

// alternatively, add method to return path's bandwidth instead?

Flow createFlow(Path path, double bps) {
    /**
     * Increments used bandwidth for all gates along path.
     * (Currently only in outgoing direction.)
     */
    for (Path::iterator it = path.begin(); it != path.end()-1; it++) {
        for (int i = (*it)->getNumOutLinks()-1; i>=0; i--) { // try each link
            if ((*it)->getLinkOut(i)->getRemoteNode() == *(it+1)) { // until the other node is found
                cChannel *ch = (*it)->getLinkOut(i)->getLocalGate()->getTransmissionChannel();
                ((FlowChannel*)ch)->addUsedBps(bps);
                break;
            }
        }
        // should break before this else nodes were not adjacent
        for (int i = (*it)->getNumInLinks()-1; i>=0; i--) { // try each link
            if ((*it)->getLinkIn(i)->getRemoteNode() == *(it+1)) { // until the other node is found
                cChannel *ch = (*it)->getLinkIn(i)->getRemoteGate()->getTransmissionChannel();
                ((FlowChannel*)ch)->addUsedBps(bps);
                break;
            }
        }
    }
    Flow flow;
    flow.path = path;
    flow.bps = bps;
    return flow;
}

bool revokeFlow(Flow flow) {
    /**
     * Decrements used bandwidth for all gates along path.
     */
    for (Path::iterator it = flow.path.begin(); it != flow.path.end()-1; it++) {
        for (int i = (*it)->getNumOutLinks()-1; i>=0; i--) { // try each link
            if ((*it)->getLinkOut(i)->getRemoteNode() == *(it+1)) { // until the other node is found
                cChannel *ch = (*it)->getLinkOut(i)->getLocalGate()->getTransmissionChannel();
                ((FlowChannel*)ch)->addUsedBps(-flow.bps);
                break;
            }
        }
        // should break before this else nodes were not adjacent
        for (int i = (*it)->getNumInLinks()-1; i>=0; i--) { // try each link
            if ((*it)->getLinkIn(i)->getRemoteNode() == *(it+1)) { // until the other node is found
                cChannel *ch = (*it)->getLinkIn(i)->getRemoteGate()->getTransmissionChannel();
                ((FlowChannel*)ch)->addUsedBps(-flow.bps);
                break;
            }
        }
    }
    return true;
}
