#include <algorithm> // for std::find
#include <queue>
#include "path.h"
#include "routing.h"
#include "flowchannel.h"

/*
cModule* getSourceModule(Flow *flow) {
    return flow->path[0]->getModule();
}
*/

void printPath(Path path) {
    for (Path::iterator it = path.begin(); it != path.end(); ++it) {
        EV << (*it)->getModule()->getFullPath() << " > ";
    }
    EV << ".\n";
}
void printPaths(PathList paths) {
    for (PathList::iterator it = paths.begin(); it != paths.end(); ++it) {
        printPath(*it);
    }
}

/*
simtime_t pathLag(Path path) {
    simtime_t lag = 0;
    for (Path::iterator p_it = path.begin(); p_it != path.end()-1; ++p_it) {
        for (int i = (*p_it)->getNumOutLinks()-1; i>=0; --i) {
            if ((*p_it)->getLinkOut(i)->getRemoteNode() == *(p_it+1)) {
                lag += ((FlowChannel*)(*p_it)->getLinkOut(i)->getLocalGate()->getTransmissionChannel())->getDelay();
            }
        }
    }
    return lag;
}
*/

/**
 * Replace flow's channels with those linking its current path.
 * Returns true if all such links are found.
 */
/*
bool fillChannels(Flow* f) {
    f->channels.clear();
    std::vector<cChannel*> channels;
    for (Path::iterator p_it = f->path.begin(); p_it != f->path.end()-1; ++p_it) {
        for (int i = (*p_it)->getNumOutLinks()-1; i>=0; --i) {
            if ((*p_it)->getLinkOut(i)->getRemoteNode() == *(p_it+1)) {
                f->channels.push_back((*p_it)->getLinkOut(i)->getLocalGate()->getTransmissionChannel());
                break;
            }
        }
    }
    if (f->channels.size() == f->path.size()-1) {
        return true;
    }
    else {
        return false;
    }
}
*/

Path getShortestPathDijkstra(Node *srcNode, Node *dstNode) {
    /**
     * useful for checking correctness of my BFS function
     */
    // initialise
    Path ret;
    // run Dijkstra's algorithm
    topo.calculateUnweightedSingleShortestPathsTo(dstNode);
    while (srcNode != dstNode) {
        ret.push_back(srcNode);
        srcNode = srcNode->getPath(0)->getRemoteNode();
    } ret.push_back(srcNode); // once more to push target onto path
    return ret;
}
Path getShortestPathDijkstra(cModule *srcMod, cModule *dstMod) {
    Node* srcNode = topo.getNodeFor(srcMod);
    Node* dstNode = topo.getNodeFor(dstMod);
    return getShortestPathDijkstra(srcNode, dstNode);
}
Path getShortestPathBfs(Node *srcNode, Node *dstNode) {
    /**
     * seems to process about 2/3 more paths than getShortestPathDijkstra
     */
    // initialise
    Path path; Node* n; Node* m; int i; // minor speedup observed from initialising out of loop
    std::deque<Path> paths; // faster than flipping two vectors
    std::set<Node*> seen; seen.insert(srcNode);
    paths.push_back(Path(1, srcNode));
    // breadth-first search
    while (paths.size() != 0) {
        path = paths.front(); // taking the least recent partial-path
        n = path.back(); // go to its tail ie. furthest from source
        for (i = n->getNumOutLinks()-1; i>=0; --i) {
            m = n->getLinkOut(i)->getRemoteNode(); // and for each adjacent node
            if (seen.count(m) == 0) { // if it has not been seen before
                // add new partial-path with this node to queue
                path.push_back(m);
                if (m == dstNode) {return path;}
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
Path getShortestPathBfs(cModule *srcMod, cModule *dstMod) {
    Node* srcNode = topo.getNodeFor(srcMod);
    Node* dstNode = topo.getNodeFor(dstMod);
    return getShortestPathBfs(srcNode, dstNode);
}

/* return:
 * mtda calls detourfinder with src dst
 *   checks ret against bw/p
 *   if fail retry but retry numb ltd by global car
 *   if success ???
 *   if detourfinder fails or retires capped, ret fail to user
 */

std::map<std::pair<Node*, Node*>, PathList> pathsAroundShortestCache; // TODO find more suitable place for this eg. controller
// search state stored here: mapping from src-dst pair
// to search-state (searching queue+set; searched vector+set)
std::map<std::pair<Node*, Node*>, searchState*> searchStates;

void dequeueAndSearch(searchState *state) {
    Path path = state->searchingQ.front();
    // algorithm begins here:
    for (Path::iterator branch_it = path.begin(); branch_it != path.end()-1; ++branch_it) { // for each of its nodes except the last (destinataion),
        for (int i=0; i<(*branch_it)->getNumOutLinks(); ++i) { // for each neighbour...
            Node* detour = (*branch_it)->getLinkOut(i)->getRemoteNode();
            if (std::find(path.begin(), path.end(), detour) == path.end()) { // ... that is not in the path,
                for (int j=0; j<detour->getNumOutLinks(); ++j) { // for each of that neighbour's neighbours
                    Path::iterator merge_it = std::find(path.begin(), path.end(), detour->getLinkOut(j)->getRemoteNode()); // see if it is in the path
                    if (merge_it == path.end()) {continue;} // (must rejoin path)
                    if (std::find(path.begin(), branch_it+1, detour->getLinkOut(j)->getRemoteNode()) != branch_it+1) {continue;} // (must be after branching point)
                    else { // if so, a new path is found
                        // v: use new? will tmp go out of scope?
                        Path tmp(path.begin(), branch_it); // note: does not include branch node itself
                        tmp.push_back(*branch_it); // so we add branch node
                        tmp.push_back(detour); // add detour node
                        tmp.insert(tmp.end(), merge_it, path.end()); // add node where detour rejoins path, and the remainder.
                        // check for loops (using size of set)
                        if (std::set<Node*>(tmp.begin(), tmp.end()).size() != tmp.size()){
                            EV << "loop found. starting from "; printPath(path);
                            EV << "we get "; printPath(tmp);
                            cRuntimeError("loop found");
                        } // end check for loops
                        if (state->searchedSet.count(tmp)) {continue;} // check that new path has not been searched before...
                        if (state->searchingSet.insert(tmp).second) { // ... nor has it already been queued for searching
                            state->searchingQ.push_back(tmp);
                        }
                        //if (searchingQ.size() + searched.size()> 1000) {goto end;}
                    }
                }
            }
        }
    }
    // lastly, move queue-head into set of searched paths
    state->searched.push_back(path); state->searchedSet.insert(path);
    state->searchingQ.pop_front(); state->searchingSet.erase(path);
    return;
}

/**
 * Begins or resumes a search, returning only the first path in the detour
 * queue, but after queueing all its detours.
 */
Path getDetour(Node *srcNode, Node *dstNode, size_t index) {
    // create mapping if it does not exist
    if (!searchStates.count(std::make_pair(srcNode, dstNode))) {
        searchState *newState = new searchState;
        // starting with the shortest path
        newState->searchingQ.push_back(getShortestPathBfs(srcNode, dstNode));
        newState->searchingSet.insert(newState->searchingQ.front());
        searchStates[std::make_pair(srcNode, dstNode)] = newState;
    }
    // fetch current search-state
    searchState *currentState = searchStates[std::make_pair(srcNode, dstNode)];
    while (!currentState->searchingQ.empty()) {
        // requested index falls within current total (searched+queue)
        if (index < currentState->searchingQ.size() + currentState->searched.size()) {
            break;
        }
        // find detours until total (searched+queue) adds up to requested index
        else {
            EV << "State of detour search: " << currentState->searchedSet.size() << " paths searched with "
               << currentState->searchingSet.size() << " pending. Now searching more...\n";
            dequeueAndSearch(currentState);
            EV << "State of detour search: " << currentState->searchedSet.size() << " paths searched with "
               << currentState->searchingSet.size() << " pending.\n";
        }
        // or out of potential detours to search.
    }
    // start counting from searched...
    if (index < currentState->searched.size()) {
        return currentState->searched[index];
    }
    // ... before searchingQ
    else if (index < currentState->searchingQ.size() + currentState->searched.size()) {
        return currentState->searchingQ[index - currentState->searched.size()];
    }
    // out of detours to search and index not reached: terminate search
    else {
        return Path();
    }
}
Path getDetour(cModule *srcMod, cModule *dstMod, size_t index) {
    Node* srcNode = topo.getNodeFor(srcMod);
    Node* dstNode = topo.getNodeFor(dstMod);
    return getDetour(srcNode, dstNode, index);
}

PathList getPathsAroundShortest(Node *srcNode, Node *dstNode) {
    /**
     * Starts with shortest path, followed by its detours in a FIFO order.
     * Thus the Paths returned should be relatively short.
     * TODO: return "state" of the search: pair<searchingQ, searched>, so that
     * source nodes can cache partial searches and resume with no repetition.
     */
    // check for cached result
    if (pathsAroundShortestCache.count(std::make_pair(srcNode, dstNode))) {
        return pathsAroundShortestCache[std::make_pair(srcNode, dstNode)];
    }
    // end check
    std::queue<Path> searchingQ;
    PathList searched;
    std::set<Path> searchingSet, searchedSet;
    searchingQ.push(getShortestPathBfs(srcNode, dstNode)); // size is now 1
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
                                    if (std::set<Node*>(tmp.begin(), tmp.end()).size() != tmp.size()){ // if non-unique node --> loop exists
                                        EV << "loop found. based on "; printPath(path);
                                        EV << "we get "; printPath(tmp);
                                        cRuntimeError("");
                                    }
                                    searchingQ.push(tmp);
                                }
                                if (searchingQ.size() + searched.size()> 1000) {
                                    goto end;
                                }
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
    pathsAroundShortestCache[std::make_pair(srcNode, dstNode)] = searched; // cache result
    EV << searched.size() << " paths found\n";
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
PathList getPathsAroundShortest(cModule *srcMod, cModule *dstMod) {
    Node* srcNode = topo.getNodeFor(srcMod);
    Node* dstNode = topo.getNodeFor(dstMod);
    return getPathsAroundShortest(srcNode, dstNode);
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

/**
 * Checks if datarate is available between two adjacent nodes
 */
bool availableNodePair(Node *from, Node *to, uint64_t bps, Priority p) {
    for (int i=0; i<from->getNumOutLinks(); ++i) { // try each link
        if (from->getLinkOut(i)->getRemoteNode() == to) { // until the other node is found
            if (((FlowChannel*)from->getLinkOut(i)->getLocalGate()->getTransmissionChannel())->isFlowPossible(bps, p)) {
                return true;
            }
        }
    }
    return false;
}
PathList getAvailablePaths(PathList paths, uint64_t bps, Priority p) {
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
            if (!availableNodePair(*node_it, *(node_it+1), bps, p)) { // check if datarate is available on each link
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

PathList waypointsToAvailablePaths(Path waypoints, uint64_t bps, Priority p) {
    /**
     * Returns Paths that can support bps@p through given waypoints.
     * Failure returns empty container.
     */
    // for each waypoint up till 2nd last
        // for each path towards its next waypoint
            // for each node up till 2nd last
                // check node has bandwidth available to next one, including reservations
            // if all node pairs available
                // reserve bandwidth along this path
                // break
    PathList ret;
    std::map<std::pair<Node*, Node*>, double> reservedBWs;
    for (Path::iterator wp_it = waypoints.begin(); wp_it != waypoints.end()-1; ++wp_it) { // for each waypoint up till 2nd last
        bool waypointsLinked = false;
        PathList tryPaths = getPathsAroundShortest(*wp_it, *(wp_it+1));
        for (PathList::iterator try_it = tryPaths.begin(); try_it != tryPaths.end(); ++try_it) { // for each path towards its next waypoint
            bool pathPossible = true;
            for (Path::iterator p_it = try_it->begin(); p_it != try_it->end()-1; ++p_it) { // for each node up till 2nd last
                // check node has bandwidth available to next one, including reservations
                if (!availableNodePair(*p_it, *(p_it+1),
                    bps + reservedBWs[std::make_pair(*p_it, *(p_it+1))], p)) {
                    pathPossible = false;
                    break;
                }
            }
            if (pathPossible) { // if all node pairs available
                // reserve bandwidth along this path
                for (Path::iterator p_it = try_it->begin(); p_it != try_it->end()-1; ++p_it) {
                    reservedBWs[std::make_pair(*p_it, *(p_it+1))] += bps;
                }
                ret.push_back(*try_it);
                waypointsLinked = true;
                break;
            }
        }
        if (!waypointsLinked) {
            return PathList(); // failure
        }
    }
    return ret;
}

/**
 * Increments used bandwidth for all gates along path.
 */
/*
Flow* createFlow(Path path, uint64_t bps, Priority p) {
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
    }
    return f;
}
Flow* createFlow(Flow* f) {
    return createFlow(f->path, f->bps, f->priority);
}
*/

/**
 * Detach flow from all its channels, then delete it.
 */
bool revokeFlow(Flow* f) {
    for (FlowChannels::const_iterator c_it  = f->getChannels().begin();
                                      c_it != f->getChannels().end();
                                    ++c_it) {
        ((FlowChannel*)*c_it)->removeFlow(f);
    }
    delete f;
    return true;
}
