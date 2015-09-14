#include "controller.h"
#include "parse.h"
#include "path.h"
#include "flowchannel.h"

Define_Module(Controller);

int Controller::numInitStages() const {
    return 1;
}

void Controller::initialize(int stage) {
}

void Controller::handleMessage(cMessage *msg) {
    EV << "Controller::handleMessage\n";
    if (endFlows.count(msg) == 1) {
        return; // deprecate
        end(msg);
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

// helper function
bool pathAvailable(Path path, uint64_t bps, Priority p) {
    for (Path::iterator node = path.begin(); node != path.end()-1; ++node) {
        if (!availableNodePair(*node, *(node+1), bps, p)) {
            return false;
        }
    }
    // no node pairs returned false, thus the path is available
    return true;
}
// helper function
std::pair<bool, Path> Controller::waypointsAvailable(Path waypoints, uint64_t bps, Priority p) {
    Path fullPath;
    for (Path::iterator wp_it = waypoints.begin(); wp_it != waypoints.end()-1; ++wp_it) { // for each waypoint up till 2nd last
        bool waypointsLinked = false;
        for (int i=0; i<par("detourAttempts").longValue(); ++i) { // for some number of attempts
            Path det = getDetour(*wp_it, *(wp_it+1), i); // get next detour
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

Flow* findSingleDisplaceable(Priority p) {
    return NULL;
}
std::pair<bool, std::list<Flow*> > findDisplaceList(uint64_t bps, Priority p) {
    return std::make_pair(false, std::list<Flow*>());
}
void combineDisplaceLists(uint64_t bps, Priority p) {
    return;
}
bool Controller::requestVID(Path waypoints, int vID) {
    Enter_Method("requestVID()");
    // initialise stream
    Stream* vdl = new Stream;
    vdl->setViewtime(getVideoSeconds(vID));
    // get priority levels to assign to subflows
    std::vector<uint64_t> bitrates = getBitRates(vID);
    Priority baseFlowPriority = bitrates.size(); // magic-y number
    // check which subflows can be established
    for (size_t i=0; i<bitrates.size(); ++i) {
        std::pair<bool, Path> res = waypointsAvailable(waypoints, bitrates[i], baseFlowPriority-i);
        Flow* subflow = new Flow;
        subflow->setBps(bitrates[i]);
        subflow->setPriority(baseFlowPriority-i);
        if (res.first) {
            subflow->setActive(true);
        }
        else {
            if (i==0) { // not even lowest quality subflow
                delete subflow;
                delete vdl;
                return false;
            }
            subflow->setActive(false);
        }
        subflow->setPath(res.second);
        vdl->addSubflow(*subflow);
    }
    // set up active subflows (bit contradictory to call them active already)
    for (std::list<Flow*>::const_iterator sf_it  = vdl->getSubflows().begin();
                                          sf_it != vdl->getSubflows().end();
                                        ++sf_it) {
        // go through each active channel in *sf_it
        if (!(*sf_it)->isActive()) {continue;}
        for (std::vector<cChannel*>::const_iterator ch_it  = (*sf_it)->getChannels().begin();
                                                    ch_it != (*sf_it)->getChannels().end();
                                                  ++ch_it) {
            // reserve/add flow
            check_and_cast<FlowChannel*>(*ch_it)->addFlow(*sf_it);
        }
    }
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

/*
void Controller::end(cMessage* endMsg) {
    //Enter_Method("end()");
    Flow *f = endFlows[endMsg];
    // since we assume shortest path only, we need only look at first channel
    FlowChannel *fc1 = (FlowChannel*)f->channels.front();
    fc1->shareBWexcept(flowEnds, f);
    fc1->spreadUpdates();
    // remove flow from its channels
    for (std::vector<cChannel*>::iterator c_it = f->channels.begin(); c_it != f->channels.end(); ++c_it) {
        ((FlowChannel*)(*c_it))->removeFlowV2(f);
    }
    // delete flow and associated self-timer
    endFlows.erase(endMsg);
    flowEnds.erase(f);
    // pass msg to user (last module in path) to record transfer time
    sendDirect(endMsg, (*(f->path.end()-1))->getModule(), "directInput", -1);
    // delete endMsg;
    delete f;
    //
    rescheduleEnds();
    // record statistics
    // inform user of completion? already called from user
}
*/

void Controller::endStream(cMessage* endMsg) {
    Stream *stream = endStreams[endMsg];
    // remove active subflows from each of their channels
    for (std::list<Flow*>::const_iterator sf_it  = stream->getSubflows().begin();
                                          sf_it != stream->getSubflows().end();
                                        ++sf_it) {
        // go through each active channel in *sf_it
        if (!(*sf_it)->isActive()) {continue;}
        for (std::vector<cChannel*>::const_iterator ch_it  = (*sf_it)->getChannels().begin();
                                                    ch_it != (*sf_it)->getChannels().end();
                                                  ++ch_it) {
            // reserve/add flow
            check_and_cast<FlowChannel*>(*ch_it)->removeFlow(*sf_it);
        }
        delete *sf_it;
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
