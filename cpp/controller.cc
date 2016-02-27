#include "controller.h"
#include "parse.h"
#include "routing.h"
#include "flowchannel.h"
#include "logic.h"

Define_Module(Controller);

int Controller::numInitStages() const {
    return 1;
}

void Controller::initialize(int stage) {
    g = (Global*)getParentModule()->getSubmodule("global");;
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
        for (int i=0; i<par("detourAttempts").longValue(); ++i) { // for some number of attempts
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
    // walk FlowChannels and setCached at their output gates
    for (FlowChannels::const_iterator ch_it  = f->getChannels().begin();
                                      ch_it != f->getChannels().end();
                                    ++ch_it) {
        FlowChannel *fc = *ch_it;
        cModule* fcDest = fc->getSourceGate()->getPathEndGate()->getOwnerModule();
        // if Logic, setCached; elif User, ignore; else, should not reach this error
        if (fcDest->getNedTypeName() == std::string("Logic")) {
            checkAndCache(fcDest, vID);
        }
        else if (fcDest->getNedTypeName() == std::string("User")) {
            // terminates at end-user and not cache: do nothing
        }
        else {
            error("Controller::setupSubflow: unknown NED type: %s", fcDest->getNedTypeName());
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
    if (par("multicast").boolValue()) {
        // TODO check if content is already cached (can assume all caches return the same result)

        // begin multicast flow setup:
        // assume source is first origin:
        Node* rootNode = topo.getNodeFor(simulation.getModule(completeCacheIDs.front()));
        // assume all replica locations are destinations:
        std::vector<Node*> leafNodes;
        for (std::vector<int>::iterator it = incompleteCacheIDs.begin();
                it != incompleteCacheIDs.end(); ++it) {
            leafNodes.push_back(topo.getNodeFor(simulation.getModule(*it)));
        } // Cannot simply add user as leaf since it might be routed directly to root,
        // so keep only last 2 waypoints: to user from its cache... (1)
        waypoints.erase(waypoints.begin(), waypoints.begin()+waypoints.size()-2);
        // check availability of multicast substreams
        for (size_t i=0; i<bitrates.size(); ++i) {
            std::pair<bool, FlowChannels> res = treeAvailable(rootNode, leafNodes, bitrates[i], baseFlowPriority-i);
            // (1)... so that we can add the unicast route from user to its cache.
            std::pair<bool, Path> res2 = waypointsAvailable(waypoints, bitrates[i], baseFlowPriority-i);
            //
            //printFlowChannels(res.second);
            Flow* subflow = new Flow;
            subflow->setBps(bitrates[i]);
            subflow->setPriority(baseFlowPriority-i);
            vdl->addSubflow(*subflow);
            if (res.first && res2.first) {
                subflow->setChannels(res.second);
                subflow->addChannels(res2.second);
                printFlowChannels(subflow->getChannels());
                subflow->setActive(true);
                setupSubflow(subflow, vID);
                SubflowStreams[subflow] = vdl;
            }
            else {
                if (i==0) { //not even lowest quality -> whole req fails
                    delete subflow;
                    delete vdl;
                    return false;
                }
                subflow->setActive(false);
                break; // do not attempt lower priority subflows
            }
        }
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
    } // endif unicast
    // add event to queue
    cMessage* endMsg = new cMessage("end-of-stream");
    scheduleAt(simTime()+vdl->getViewtime(), endMsg);
    // and point it back to the request
    endStreams[endMsg] = vdl;
    // success
    return true;
}

/*
bool Controller::userCallsThis_FixedBw(Path waypoints, uint64_t bits, uint64_t bps) {
    Enter_Method("userCallsThis_FixedBw()");
    // check each consecutive waypoint pair has available bw
        // number of attempts is from cpar
    Priority baseFlowPriority = 3; // magic number
    std::pair<bool, Path> res = waypointsAvailable(waypoints, bps, baseFlowPriority);
    if (!res.first) {
        return false; // waypoints could not be linked
    }
    // else valid fullPath found. Initialise 3 subflows:
    for (int i=0; i<3; ++i) {

    }

    // initialise flow
    Flow* f = new Flow;
    f->path = res.second;
    fillChannels(f);
    f->lag = pathLag(f->path);
    f->bps = bps;
    f->bpsMin = bps; // not currently used; for QoS?
    f->bits_left = bits;
    f->lastUpdate = simTime();
    f->priority = baseFlowPriority;
    // attach timer to flow
    cMessage* endMsg = new cMessage("flow ends");
    flowEnds[f] = endMsg;
    endFlows[endMsg] = f;
    // add flow ref to its channels
    for (std::vector<cChannel*>::iterator c_it = f->channels.begin(); c_it != f->channels.end(); ++c_it) {
        ((FlowChannel*)(*c_it))->addFlow(f);
    }
    return true; // signifies successful setup
}
*/

/*
bool Controller::userCallsThis(Path path, uint64_t bits) {
    Enter_Method("userCallsThis()");
    // initialise flow properties
    Flow* f = new Flow;
    f->path = path;
    fillChannels(f);
    f->lag = pathLag(path);
    f->bps = 0; // pending
    f->bpsMin = 0; // not currently used; for QoS?
    f->bits_left = bits;
    EV << "bitsize: " << f->bits_left << endl;
    f->lastUpdate = simTime();
    f->priority = 0;

    // attach timer to flow
    cMessage* endMsg = new cMessage("flow ends");
    flowEnds[f] = endMsg;
    endFlows[endMsg] = f;

    // add new flow to its channels
    for (std::vector<cChannel*>::iterator c_it = f->channels.begin(); c_it != f->channels.end(); ++c_it) {
        ((FlowChannel*)(*c_it))->addFlowV2(f);
    }
    // since we assume shortest path only, we need only look at first channel
    FlowChannel *fc1 = (FlowChannel*)f->channels.front();
    fc1->shareBWexcept(flowEnds, NULL);
    fc1->spreadUpdates();
    rescheduleEnds();

    return true;
}
*/

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
