#include <algorithm> // for std::find
#include <queue>
#include "path.h"
#include "routing.h"
#include "flowchannel.h"

typedef cTopology::Node Node;

cModule* getSourceModule(Flow *flow) {
    return flow->path[0]->getModule();
}

void printPath(Path path) {
    for (Path::iterator it = path.begin(); it != path.end(); ++it) {
        EV << (*it)->getModule()->getFullPath() << " > ";
    }
    EV << endl;
}
void printPaths(PathList paths) {
    for (PathList::iterator it = paths.begin(); it != paths.end(); ++it) {
        printPath(*it);
    }
}

Path getShortestPathDijkstra(cModule *srcMod, cModule *dstMod) {
    /**
     * useful for checking correctness of my BFS function
     */
    // initialise
    Path ret;
    Node* current = topo.getNodeFor(srcMod);
    Node* target = topo.getNodeFor(dstMod);
    // run Dijkstra's algorithm
    topo.calculateUnweightedSingleShortestPathsTo(target);
    while (current != target) {
        ret.push_back(current);
        current = current->getPath(0)->getRemoteNode();
    } ret.push_back(current); // once more to push target onto path
    return ret;
}
Path getShortestPathBfs(cModule *srcMod, cModule *dstMod) {
    /**
     * seems to process about 2/3 more paths than getShortestPathDijkstra
     */
    // initialise
    Path path; Node* n; Node* m; int i; // minor speedup observed from initialising out of loop
    std::deque<Path> paths; // faster than flipping two vectors
    Node* current = topo.getNodeFor(srcMod);
    Node* target = topo.getNodeFor(dstMod);
    std::set<Node*> seen; seen.insert(current);
    paths.push_back(Path(1, current));
    // breadth-first search
    while (paths.size() != 0) {
        path = paths.front(); // taking the least recent partial-path
        n = path.back(); // go to its tail ie. furthest from source
        for (i = n->getNumOutLinks()-1; i>=0; --i) {
            m = n->getLinkOut(i)->getRemoteNode(); // and for each adjacent node
            if (seen.count(m) == 0) { // if it has not been seen before
                // add new partial-path with this node to queue
                path.push_back(m);
                if (m == target) {return path;}
                seen.insert(m);
                paths.push_back(path);
                path.pop_back();
            }
            // else ignore node
        }
        paths.pop_front();
    }
    // nothing found (should not reach here)
    throw cRuntimeError("BFS did not find any paths.");
    return Path();
}

bool _hasLoop(Path p) {
    return std::set<Node*>(p.begin(), p.end()).size() != p.size();
}
PathList getPathsAroundShortest(cModule *srcMod, cModule *dstMod) {
    /**
     * Starts with shortest path, then detours in a FIFO order.
     * Thus the Paths returned should be some of the shortest.
     */
    std::queue<Path> searchingQ;
    PathList searched;
    std::set<Path> searchingSet, searchedSet;
    searchingQ.push(getShortestPathBfs(srcMod, dstMod)); // size is now 1
    searchingSet.insert(searchingQ.front());
    //std::set<Node*> inAPath(searched[0].begin(), searched[0].end()); // ???
    while (searchingQ.size() != 0 /*&& (searched.size() < 100)*/) { // until no new paths are found
        //prevSize = searched.size();
        Path path = searchingQ.front(); { // for each known path,
            for (Path::iterator branch_it = path.begin(); branch_it != path.end()-1; ++branch_it) { // for each of its nodes except the last (destinataion),
                for (int i = (*branch_it)->getNumOutLinks()-1; i>=0; --i) { // for each neighbour...
                    Node* detour = (*branch_it)->getLinkOut(i)->getRemoteNode();
                    if (std::find(path.begin(), path.end(), detour) == path.end()) { // ... that is not also in the path,
                        for (int j = detour->getNumOutLinks()-1; j>=0; --j) { // for each of that neighbour's neighbours
                            Path::iterator merge_it = std::find(path.begin(), path.end(), detour->getLinkOut(j)->getRemoteNode()); // look for a node in the aforementioned known path
                            if (merge_it == path.end()) {continue;} // must rejoin path
                            if (std::find(path.begin(), branch_it+1, detour->getLinkOut(j)->getRemoteNode()) != branch_it+1) {continue;} // and after branching point
                            else { // if so, that forms a new path
                                Path tmp(path.begin(), branch_it); // note: does not include n itself
                                tmp.push_back(*branch_it); // add node at start of detour
                                tmp.push_back(detour); // add detour node
                                tmp.insert(tmp.end(), merge_it, path.end()); // add node where detour rejoins path and remainder
                                if (searchedSet.count(tmp)) {continue;}
                                if (searchingSet.insert(tmp).second) { // but it is not necessarily found uniquely
                                    if (_hasLoop(tmp)){
                                        EV << "loop found. based on "; printPath(path);
                                        EV << "we get "; printPath(tmp);
                                        cRuntimeError("");
                                    }
                                    searchingQ.push(tmp);
                                }
                                //~ if (searchedSet.size() +searchAroundSet.size()> 10000) {
                                    //~ goto end;
                                //~ }
                            }
                        }
                    }
                }
            }
        }
        searched.push_back(path); searchedSet.insert(path);
        searchingQ.pop(); searchingSet.erase(path);
    }
    end:
    return searched;
    /*
     * [insight] if a node has only 2 neighbours, it is traversed only by paths going to the other side
     * []may be optimisations exploiting the fact that input path is shortest
     * make set from shortest path
     * add all nodes adjacent to the unstuck set to another set (nodes to try)
     * loop {
     *  for each node to try
     *      if 2 adj, and both unstuck: self is unstuck and not node to try anymore
     *      else 2 adj, and one unstuck
     *      (only other case is both not unstuck
     * } until no new nodes are added to the unstuck set
     */
}

// algo modified from http://mathoverflow.net/a/18634
// also consider: An algorithm for computing all paths in a graph
// [http://link.springer.com/article/10.1007%2FBF01966095]
Node *source;
Node *target;
Path path; // used as a stack, but vector provides iterator and random access
std::set<Node*> seen;
std::map<Node*, bool> stuckCache;
PathList paths;
bool _stuck(Node *n) { // helper function
    //if (stuckCache.find(n) != stuckCache.end()) {return stuckCache[n];}
    if (n == target) {return false;}
    for (int i = n->getNumOutLinks()-1; i>=0; --i) {
        // EV << "in stuck trying numout #" << i << endl;
        Node *m = n->getLinkOut(i)->getRemoteNode(); // for each node beside head
        if (seen.insert(m).second == true) { // true means insertion successful i.e. m was not in seen
            if (!_stuck(m)) { // and if it is not stuck
                //stuckCache[m] = false;
                return false; // this head is also not stuck
            }
        }
    }
    //stuckCache[n] = true;
    return true;
}
void _search(Node *n) { // helper function
    // EV << "searching @ " << n->getModule()->getFullPath() << endl;
    if (n == target) {
        // found a path
        // EV << "Path found: ";
        // printPath(path);
        paths.push_back(Path(path));
        return;
    }
    // check if stuck
    //seen = std::set<Node*>(path.begin(), path.end()); // copy path to seen
    //if (_stuck(n)) {return;}
    // run search on each neighbour of n
    for (int i = n->getNumOutLinks()-1; i>=0 ; --i) {
        // EV << "search:numout: " << i << endl;
        Node *m = n->getLinkOut(i)->getRemoteNode();
        // do not go backwards:
        bool neighbour_in_path = false;
        for (int j = path.size()-1; j >= 0; --j) {
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
    // (re-)initialise which nodes are stuck
    stuckCache.clear();
    stuckCache[target] = false;
    // (re-)initialise path list
    paths = PathList();
    // begin search
    _search(source);
    // relist all found paths
    // EV << "Relisting paths found...\n";
    // printPaths(paths);
    return PathList(paths); // return a copy
}

PathList getShortestPaths(PathList paths) {
    /**
     * Returns PathList with the least number of Nodes.
     * TODO? check bandwidth availability
     */
    // initialisation
    unsigned int minHops = UINT_MAX;
    PathList shortest = PathList();
    // for each path, if shorter than current shortest, become new shortest
    for (PathList::iterator it = paths.begin(); it != paths.end(); ++it) {
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

bool _getAvailablePathsHelper(Node *from, Node *to, double bps, Priority p) {
    /**
     * Checks if datarate is available between two adjacent nodes
     */
    for (int i = from->getNumOutLinks()-1; i>=0; --i) { // try each link
        if (from->getLinkOut(i)->getRemoteNode() == to) { // until the other node is found
            if (((FlowChannel*)from->getLinkOut(i)->getLocalGate()->getTransmissionChannel())->isFlowPossible(bps, p)) {
                return true;
            }
        }
    }
    return false;
}
PathList getAvailablePaths(PathList paths, double bps, Priority p) {
    /**
     * Filters given paths to only those that have the datarate in all links
     */
    // initialisation
    PathList available = PathList();
    bool possible = false;
    // for each path, if all links have at least datarate, copy onto available
    for (PathList::iterator path_it = paths.begin(); path_it != paths.end(); ++path_it) { // for each path
        possible = true;
        for (Path::iterator node_it = path_it->begin(); node_it != path_it->end()-1; ++node_it) { // starting from the first node
            if (!_getAvailablePathsHelper(*node_it, *(node_it+1), bps, p)) { // check if datarate is available on each link
                possible = false;
                break;
            }
            // assert(possible);
        }
        if (possible) {
            available.push_back(Path(*path_it));
        }
    }
    return available;
}

// alternatively, add method to return path's bandwidth instead?

Flow* createFlow(Path path, double bps, Priority p) {
    /**
     * Increments used bandwidth for all gates along path.
     */
    Flow* f = new Flow;
    f->path = path;
    f->bps = bps;
    f->priority = p;
    for (Path::iterator it = path.begin(); it != path.end()-1; ++it) { // for each node in the path
        for (int i = (*it)->getNumOutLinks()-1; i>=0; --i) { // try each outgoing link
            if ((*it)->getLinkOut(i)->getRemoteNode() == *(it+1)) { // until the other node is found
                cChannel *ch = (*it)->getLinkOut(i)->getLocalGate()->getTransmissionChannel();
                ((FlowChannel*)ch)->addFlow(f);
                break;
            }
        }
        // should break before this else nodes were not adjacent
        for (int i = (*it)->getNumInLinks()-1; i>=0; --i) { // try each incoming link
            if ((*it)->getLinkIn(i)->getRemoteNode() == *(it+1)) { // until the other node is found
                cChannel *ch = (*it)->getLinkIn(i)->getRemoteGate()->getTransmissionChannel();
                ((FlowChannel*)ch)->addFlow(f);
                break;
            }
        }
    }
    return f;
}

bool revokeFlow(Flow* f) {
    /**
     * Decrements used bandwidth for all gates along path.
     */
    for (Path::iterator it = f->path.begin(); it != f->path.end()-1; ++it) {
        for (int i = (*it)->getNumOutLinks()-1; i>=0; --i) { // try each outgoing link
            if ((*it)->getLinkOut(i)->getRemoteNode() == *(it+1)) { // until the other node is found
                cChannel *ch = (*it)->getLinkOut(i)->getLocalGate()->getTransmissionChannel();
                ((FlowChannel*)ch)->removeFlow(f);
                break;
            }
        }
        // should break before this else nodes were not adjacent
        for (int i = (*it)->getNumInLinks()-1; i>=0; --i) { // try each incoming link
            if ((*it)->getLinkIn(i)->getRemoteNode() == *(it+1)) { // until the other node is found
                cChannel *ch = (*it)->getLinkIn(i)->getRemoteGate()->getTransmissionChannel();
                ((FlowChannel*)ch)->removeFlow(f);
                break;
            }
        }
    }
    delete f;
    return true;
}
