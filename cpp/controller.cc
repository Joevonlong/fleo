#include "controller.h"
#include "parse.h"
#include "routing.h"
#include "flowchannel.h"
#include "logic.h"
#include "cache.h"

Define_Module(Controller);

int Controller::numInitStages() const {
    return 1;
}

void Controller::initialize(int stage) {
    g = (Global*)getParentModule()->getSubmodule("global");;
    detourAttempts = par("detourAttempts").longValue();
    multicast = par("multicast").boolValue();
    branchPriorityModifier = par("branchPriorityModifier").longValue();
}

void Controller::handleMessage(cMessage *msg) {
    EV << "Controller::handleMessage\n";
    if (endFlows.count(msg) == 1) {
        return; // deprecate
    }
    else if (endStreams.count(msg) == 1) {
        endStream(msg);
    }
    else {
        error("Controller::handleMessage: unhandled message");
    }
}

void Controller::finish() {
}

// Returns true if path has bandwidth bps at priority p available
bool pathAvailable(Path path, uint64_t bps, Priority p) {
    for (Path::iterator node = path.begin(); node != path.end()-1; ++node) {
        if (!availableNodePair(*node, *(node+1), bps, p)) {
            return false;
        }
    }
    // no node pairs returned false, thus the path is available
    return true;
}
/**
 * Searches for a path with bandwidth bps and priority p, that runs though the
 * given waypoints in order.
 * Tries a number of paths equal to detourAttempts to link each pair of
 * waypoints.
 * Returns a pair <true, path> if successful, and <false, _> otherwise.
 */
std::pair<bool, Path> Controller::waypointsAvailable(Path waypoints, uint64_t bps, Priority p) {
    // checking arguments
    if (waypoints.size() < 2) {
        error("Controller::waypointsAvailable: given less than 2 waypoints");
    }
    //
    Path fullPath;
    for (Path::iterator wp_it = waypoints.begin(); wp_it != waypoints.end()-1; ++wp_it) { // for each waypoint up till 2nd last
        bool waypointsLinked = false;
        for (int i=0; i<detourAttempts; ++i) { // for some number of attempts
            Path det = getDetour(*wp_it, *(wp_it+1), i); // get next detour
            if (det.size() == 0) {break;} // no more detours
            // and check for BW availability
            if (pathAvailable(det, bps, p)) {
                // save this sub-path and set up full path if other WPs can also be linked
                fullPath.insert(fullPath.end(), det.begin(), det.end()-1); // remember to add last node after all waypoints
                waypointsLinked = true; break;
            }
        }
        if (!waypointsLinked) { // did not find BW in given attempts
            return std::make_pair(false, fullPath);
        }
    }
    fullPath.push_back(waypoints.back()); // add last node
    return std::make_pair(true, fullPath);
}

/**
 * Merges FlowChannel's traversed by Path into FlowChannels
 */
void Controller::MergePathIntoFlowChannels(FlowChannels* fcs, Path* path) {
    for (Path::iterator p_it = path->begin(); p_it != path->end()-1; ++p_it) {
        for (int i=0; i<(*p_it)->getNumOutLinks(); ++i) {
            if ((*p_it)->getLinkOut(i)->getRemoteNode() == *(p_it+1)) {
                fcs->insert(
                    check_and_cast<FlowChannel*>(
                        (*p_it)->getLinkOut(i)->getLocalGate()->getTransmissionChannel()
                    )
                );
                break;
            }
        }
    }
}

/**
 * Returns the (heuristically) shallowest multicast tree
 * that has the required bandwidth at the given priority
 * between the given source and destinations.
 */
std::pair<bool, FlowChannels> Controller::treeAvailable(Node *root, std::vector<Node*> leaves, uint64_t bps, Priority p) {
    FlowChannels chs;
    Path st;
    for (std::vector<Node*>::iterator l_it = leaves.begin(); l_it != leaves.end(); ++l_it) {
        st.clear();
        st.push_back(root); st.push_back(*l_it);
        std::pair<bool, Path> res = waypointsAvailable(st, bps, p); // retries in here
        if (!res.first) { // could not reach a leaf
            return std::make_pair(false, chs);
        }
        MergePathIntoFlowChannels(&chs, &res.second);
    }
    return std::make_pair(true, chs);
}

void Controller::checkAndCache(cModule* mod, int vID) {
    Logic* l = check_and_cast<Logic*>(mod);
    if (l->hasCache()){
        if (!l->isOrigin()) {
            l->setCached(vID, true);
        }
    }
}
// helperhelper
void Controller::deactivateSubflow(Flow* f) {
    //Stream* s = SubflowStreams[f]; // TODO change to DL
    f->setActive(false);
    g->recordPriority(f->getPriority(), false);
    for (FlowChannels::const_iterator c_it  = f->getChannels().begin();
                                      c_it != f->getChannels().end();
                                    ++c_it) {
        check_and_cast<FlowChannel*>(*c_it)->removeFlow(f);
    }
}
// helper
void Controller::setupSubflow(Flow* f, int vID) {
    g->recordPriority(f->getPriority(), true);
    // follow FlowChannels and add flows
    for (FlowChannels::const_iterator ch_it  = f->getChannels().begin();
                                      ch_it != f->getChannels().end();
                                    ++ch_it) {
        FlowChannel *fc = *ch_it;
        // cancel lower priority flows if necessary
        while (fc->getAvailableBps() < f->getBps()) {
            if (fc->getLowestPriorityFlow()->getPriority() >= f->getPriority()) {
                throw cRuntimeError("revoked flow of higher priority than new flow");
            }
            deactivateSubflow(fc->getLowestPriorityFlow());
        }
        // add new flow
        fc->addFlow(f);
    }
    // try setCached at all modules
    for (std::set<cModule*>::const_iterator m_it  = f->getModules().begin();
                                            m_it != f->getModules().end();
                                          ++m_it) {
        if ((*m_it)->getNedTypeName() == std::string("Logic")) {
            checkAndCache(*m_it, vID);
        }
        else if ((*m_it)->getNedTypeName() == std::string("User")) {
            // terminates at end-user and not cache: do nothing
        }
        else {
            error("Controller::setupSubflow: unknown NED type: %s", (*m_it)->getNedTypeName());
        }
    }
    return;
}
bool Controller::requestVID(Path waypoints, int vID) {
    Enter_Method_Silent("requestVID()");
    // checking arguments
    if (waypoints.size() < 2) {
        error("Controller::requestVID: given less than 2 waypoints");
    }
    // initialise stream
    Stream* vdl = new Stream;
    vdl->setViewtime(getVideoSeconds(vID));
    // get priority levels to assign to subflows
    std::vector<uint64_t> bitrates = getBitRates(vID);
    Priority baseFlowPriority = bitrates.size(); // magic-y number
    // do parameter check for multicast vs unicast
    if (multicast) { // begin multicast flow setup:
        Stream* trunkVDL = new Stream;
        Stream* branchVDL = new Stream;
        // only last 2 waypoints are now needed: to user from its cache
        waypoints.erase(waypoints.begin(), waypoints.begin()+waypoints.size()-2);
        // assume source is first origin:
        Node* rootNode = topo.getNodeFor(simulation.getModule(completeCacheIDs.front()));
        std::vector<Node*> leafNodes;
        // check if content is already cached, assuming the cache we're given return the same result as all others
        bool cached = check_and_cast<Cache*>(check_and_cast<Logic*>(waypoints.front()->getModule())->getParentModule()->getSubmodule("cache"))->isCached(vID);
        if (!cached) {
            // assume all replica locations are destinations:
            for (std::vector<int>::iterator it = incompleteCacheIDs.begin();
                    it != incompleteCacheIDs.end(); ++it) {
                leafNodes.push_back(topo.getNodeFor(simulation.getModule(*it)));
            } // Cannot simply add user as leaf since it might be routed directly to root... (1)
        }
        // check availability of multicast substreams
        for (size_t i=0; i<bitrates.size(); ++i) {
            std::pair<bool, FlowChannels> branchRes = treeAvailable(rootNode, leafNodes, bitrates[i], baseFlowPriority-i);
            std::pair<bool, FlowChannels> trunkRes; trunkRes.first = branchRes.first;
            if (!cached) { // tree isn't empty: subtract trunk
                trunkRes = treeAvailable(rootNode, std::vector<Node*>(1,waypoints.front()), bitrates[i], baseFlowPriority-i);
                branchRes.second.erase(trunkRes.second.begin(), trunkRes.second.end());
                throw cRuntimeError("breakpoint");
            }
            // (1)... so we add the unicast route from user to its cache.
            std::pair<bool, Path> localRes = waypointsAvailable(waypoints, bitrates[i], baseFlowPriority-i);
            //
            Flow* trunkSubflow = new Flow;
            Flow* branchSubflow = new Flow;
            trunkSubflow->setBps(bitrates[i]);
            branchSubflow->setBps(bitrates[i]);
            trunkSubflow->setPriority(baseFlowPriority-i);
            branchSubflow->setPriority(baseFlowPriority-i+branchPriorityModifier);
            trunkVDL->addSubflow(*trunkSubflow);
            branchVDL->addSubflow(*branchSubflow);
            if (branchRes.first && localRes.first) {
                if (!cached) {
                    trunkSubflow->setChannels(trunkRes.second);
                }
                trunkSubflow->addChannels(localRes.second);
                branchSubflow->setChannels(branchRes.second);
                printFlowChannels(trunkSubflow->getChannels());
                printFlowChannels(branchSubflow->getChannels());
                trunkSubflow->setActive(true);
                branchSubflow->setActive(true);
                setupSubflow(trunkSubflow, vID);
                setupSubflow(branchSubflow, vID);
                SubflowStreams[trunkSubflow] = trunkVDL;
                SubflowStreams[branchSubflow] = branchVDL;
            }
            else {
                if (i==0) { //not even lowest quality -> whole req fails
                    delete trunkSubflow; delete branchSubflow;
                    delete trunkVDL; delete branchVDL;
                    delete vdl;
                    return false;
                }
                trunkSubflow->setActive(false);
                branchSubflow->setActive(false);
                break; // do not attempt lower priority subflows
            }
        }
        // add event to queue
        cMessage* trunkEndMsg = new cMessage("end-of-stream");
        cMessage* branchEndMsg = new cMessage("end-of-stream");
        scheduleAt(simTime()+trunkVDL->getViewtime(), trunkEndMsg);
        scheduleAt(simTime()+branchVDL->getViewtime(), branchEndMsg);
        // and point it back to the request
        endStreams[trunkEndMsg] = trunkVDL;
        endStreams[branchEndMsg] = branchVDL;
        // success
        return true;
    } // endif multicast
    else { // begin unicast flow setup:
        // check which subflows can be established
        for (size_t i=0; i<bitrates.size(); ++i) {
            std::pair<bool, Path> res = waypointsAvailable(waypoints, bitrates[i], baseFlowPriority-i);
            Flow* subflow = new Flow;
            subflow->setBps(bitrates[i]);
            subflow->setPriority(baseFlowPriority-i);
            vdl->addSubflow(*subflow);
            if (res.first) {
                subflow->setPath(res.second);
                subflow->setActive(true);
                setupSubflow(subflow, vID);
                SubflowStreams[subflow] = vdl;
            }
            else {
                if (i==0) { // not even lowest quality subflow
                    delete subflow;
                    delete vdl;
                    return false;
                }
                subflow->setActive(false);
                break; // do not attempt lower priority subflows
            }
        }
        // add event to queue
        cMessage* endMsg = new cMessage("end-of-stream");
        scheduleAt(simTime()+vdl->getViewtime(), endMsg);
        // and point it back to the request
        endStreams[endMsg] = vdl;
        // success
        return true;
    } // endif unicast
}

void Controller::endStream(cMessage* endMsg) {
    Stream *stream = endStreams[endMsg];
    // remove active subflows from each of their channels
    for (std::list<Flow*>::const_iterator sf_it  = stream->getSubflows().begin();
                                          sf_it != stream->getSubflows().end();
                                        ++sf_it) {
        // go through each active channel in *sf_it
        SubflowStreams.erase(*sf_it);
        if ((*sf_it)->isActive()) {
            deactivateSubflow(*sf_it);
            delete *sf_it;
        }
    }
    endStreams.erase(endMsg);
    delete stream;
    delete endMsg;
    // or sendDirect to user?
    // sendDirect(endMsg, (*(stream->getSubflows().front()->getPath().end()-1))->getModule(), "directInput", -1);
    // call separate function? look through current req list and see which can be upgraded
}

void Controller::rescheduleEnds() {
    //Enter_Method("rescheduleEnds()");
    for (std::map<cMessage*, Flow*>::iterator m_it = endFlows.begin(); m_it != endFlows.end(); ++m_it) {
        cancelEvent(m_it->first);
        scheduleAt(m_it->first->getTimestamp(), m_it->first);
    }
}

void Controller::shareBandwidth(std::set<Flow*> flows) {
    // should be a public method of flowchannel probably
}
