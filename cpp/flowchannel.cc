#include "flowchannel.h"

Define_Channel(FlowChannel);

void FlowChannel::initialize() {
    bandwidthLeftAtPriority[INT_MAX] = par("datarate").doubleValue(); // getDatarate() returns 0 during initialization
}

bool FlowChannel::isTransmissionChannel() const {
    return true;
}

simtime_t FlowChannel::getTransmissionFinishTime() const {
    return txfinishtime;
}

void FlowChannel::processMessage(cMessage *msg, simtime_t t, result_t &result) {
    // if channel is disabled, signal that message should be deleted
    if (isDisabled) {
        result.discard = true;
        return;
    }

    // datarate modelling
    if (getDatarate()!=0 && msg->isPacket()) {
        simtime_t duration = ((cPacket *)msg)->getBitLength() / getDatarate();
        result.duration = duration;
        txfinishtime = t + duration;
    }
    else {
        txfinishtime = t;
    }

    // propagation delay modelling
    result.delay = delay;

    // bit error modeling
    if ((ber!=0 || per!=0) && msg->isPacket()) {
        cPacket *pkt = (cPacket *)msg;
        if (ber!=0 && dblrand() < 1.0 - pow(1.0-ber, (double)pkt->getBitLength()))
            pkt->setBitError(true);
        if (per!=0 && dblrand() < per)
            pkt->setBitError(true);
    }
}

//~ void FlowChannel::handleParameterChange (const char *parname) {
//~ }

// Bandwidth methods:
double FlowChannel::getAvailableBW() {
    // should be equal to getAvailableBW(INT_MIN)
    return getDatarate() - par("used").doubleValue();
}

double FlowChannel::getAvailableBW(Priority p) {
    return bandwidthLeftAtPriority.lower_bound(p)->second;
}

double FlowChannel::getUsedBW() {
    return par("used").doubleValue();
}

void FlowChannel::setUsedBW(double bps) {
    if (bps > getDatarate()) {
        throw cRuntimeError("Using more bandwidth than channel is capable of.");
    }
    else if (bps < 0) {
        throw cRuntimeError("Setting bandwidth usage to negative.");
    }
    par("used").setDoubleValue(bps);
}

void FlowChannel::addUsedBW(double bps) {
    setUsedBW(par("used").doubleValue() + bps);
}
//

// Flow methods:
void FlowChannel::addFlow(Flow* f) {
    // check input
    std::map<Priority, double>::iterator it =
        bandwidthLeftAtPriority.lower_bound(f->priority);
    if (f->bps > it->second) {
        throw cRuntimeError("Insufficient bandwidth to add flow at given priority");
    }
    // add to list of tracked flows
    currentFlows.insert(f);
    // reserve bandwidth for the flow
    addUsedBW(f->bps);
    // update table of available bandwidths:
    std::map<Priority, double>::iterator subtractBpsUpTo =
        bandwidthLeftAtPriority.insert(
            std::pair<Priority, double>(f->priority, bandwidthLeftAtPriority[it->first])
        ).first; // insert priority key (if needed) and get iterator to that mapping
    // then subtract available bandwidth from all priorities up to and including itself
    for (; subtractBpsUpTo != bandwidthLeftAtPriority.begin(); subtractBpsUpTo--) {
        subtractBpsUpTo->second -= f->bps;
    } subtractBpsUpTo->second -= f->bps; // because loop doesnt subtract first value.
}

void FlowChannel::removeFlow(Flow* f) {
    // remove from list of tracked flows
    if(currentFlows.erase(f) != 1) {
        throw cRuntimeError("Flow to remove not found");
    }
    // release bandwidth for the flow
    addUsedBW(-f->bps);
    // update priority mapping
    if (priorityUsage.count(f->priority)) {
        priorityUsage[f->priority] = priorityUsage[f->priority] -1;
        if (priorityUsage[f->priority] == 0) {
            
        }
    }
    else {
        throw cRuntimeError("Priority of removed flow not tracked");
    }
}
//

std::set<Priority> FlowChannel::getPrioritySet() {
    std::set<Priority> prioritySet;
    for (std::set<Flow*>::iterator it = currentFlows.begin(); it != currentFlows.end(); it++) {
        prioritySet.insert((*it)->priority);
    }
    return prioritySet;
}
