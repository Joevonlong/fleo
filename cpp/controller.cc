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

bool Controller::userCallsThisFixedBw(Path waypoints, uint64_t bps, Priority p) {
    Enter_Method("userCallsThisFixedBw()");
    // check each consecutive wp pair has available bw
        // number of attempts is from cpar
    for (Path::iterator wp_it = waypoints.begin(); wp_it != waypoints.end()-1; ++wp_it) { // for each waypoint up till 2nd last
        bool waypointsLinked = false;
        for (int i=0; i<par("detourAttempts").longValue(); ++i) { // for some number of attempts
            Path det = getDetour(*wp_it, *(wp_it+1), i); // get next detour
            // and check for BW availability
            bool detPossible = true;
            for (Path::iterator det_it = det.begin(); det_it != det.end()-1; ++det_it) {
                if (!availableNodePair(*det_it, *(det_it+1), bps, p)) {
                    detPossible = false; break;
                }
            }
            if (detPossible) {
                ;//save this path and set them all up if other WPs can also be linked
            }
        }
        if (!waypointsLinked) {return false;} // did not find BW in given attempts
        break;

        //template start: this code is to be adapted
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
        // template end
    }
    // if available, set flows up
    // else
    return false;
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
