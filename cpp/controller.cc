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
        end(msg);
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
bool Controller::userCallsThisFixedBw(Path waypoints, uint64_t bits, uint64_t bps, Priority p) {
    Enter_Method("userCallsThisFixedBw()");
    // check each consecutive waypoint pair has available bw
        // number of attempts is from cpar
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
        if (!waypointsLinked) {return false;} // did not find BW in given attempts
    }
    fullPath.push_back(waypoints.back()); // add last node
    // Valid fullPath found. Set it up:
    // initialise flow
    Flow* f = new Flow;
    f->path = fullPath;
    f->channels = getChannels(f->path);
    f->lag = pathLag(f->path);
    f->bps = bps;
    f->bpsMin = bps; // not currently used; for QoS?
    f->bits_left = bits;
    f->lastUpdate = simTime();
    f->priority = p;
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

bool Controller::userCallsThis(Path path, uint64_t bits) {
    Enter_Method("userCallsThis()");
    // initialise flow properties
    Flow* f = new Flow;
    f->path = path;
    f->channels = getChannels(f->path);
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

Flow* Controller::createFlow(Path path, uint64_t bps, Priority p) {
    return new Flow;
}
Flow* Controller::createFlow(Flow* f) {
    return new Flow;
}

bool Controller::revokeFlow(Flow* f) {
    return false;
}
