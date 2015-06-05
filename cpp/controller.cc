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
    EV << "euroieuroiu";
    if (endFlows.count(msg) == 1) {
        end(msg);
    }
    else {
        error("Controller::handleMessage: unhandled message");
    }
}

void Controller::finish() {
}

cMessage* Controller::userCallsThis(Path path, int vID) {
    Enter_Method("userCallsThis()");
    // initialise flow properties
    Flow* f = new Flow;
    f->path = path;
    f->channels = getChannels(f->path);
    f->lag = pathLag(path);
    f->bps = 0; // pending
    f->bpsMin = 0; // not currently used; for QoS?
    f->bits_left = getVideoBitSize(vID);
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
    fc1->shareBW(flowEnds);
    rescheduleEnds();

    return endMsg;
}

void Controller::end(cMessage* endMsg) {
    //Enter_Method("end()");
    Flow *f = endFlows[endMsg];
    // remove flow from all channels
    for (std::vector<cChannel*>::iterator c_it = f->channels.begin(); c_it != f->channels.end(); ++c_it) {
        ((FlowChannel*)(*c_it))->removeFlowV2(f);
    }
    // since we assume shortest path only, we need only look at first channel
    FlowChannel *fc1 = (FlowChannel*)f->channels.front();
    fc1->shareBW(flowEnds);
    // delete flow and associated self-timer
    flowEnds.erase(f);
    endFlows.erase(endMsg);
    delete f;
    delete endMsg;
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
