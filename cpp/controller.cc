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
    if (flowEnds.count(msg) == 1) {
        // remove flow from all channels
        // record statistics
        // inform user of completion?
        // delete flow
        return;
    }
    else {
        error("Controller::handleMessage: unknown message");
    }
}

void Controller::finish() {
}

void Controller::userCallsThis(Path path, int vID) {
    // initialise flow properties
    Flow* f = new Flow;
    f->path = path;
    f->lag = pathLag(path);
    f->bps = 0; // pending
    f->bpsMin = 0;
    f->bits_left = getVideoBitSize(vID);
    f->priority = 1;

    // attach timer to flow
    cMessage* flowEndMsg = new cMessage("flow ends");
    flowEnds[flowEndMsg] = f;

    // since we assume shortest path only, we need only look at first channel
    std::set<Flow*> firstHopFlows;
    FlowChannel *ch;
    Path::iterator it = path.begin();
    for (int i = (*it)->getNumOutLinks()-1; i>=0; --i) {
        if ((*it)->getLinkOut(i)->getRemoteNode() == *(it+1)) {
            ch = (FlowChannel*)(*it)->getLinkOut(i)->getLocalGate()->getTransmissionChannel();
            firstHopFlows = ch->getFlows();
            break;
        }
    }
    // add new flow to this
    firstHopFlows.insert(f);

    // (re)calculate bandwidth-share of all flows
    // for test assume equal:
    for (std::set<Flow*>::iterator it = firstHopFlows.begin(); it != firstHopFlows.end(); ++it) {
        (*it)->bps = ch->getAvailableBps(INT_MAX) / firstHopFlows.size();
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
