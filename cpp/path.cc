#include "path.h"
#include "routing.h"

typedef cTopology::Node Node;

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

// alternatively, add method to return path's bandwidth instead?