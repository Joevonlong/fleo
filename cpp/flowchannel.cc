#include "flowchannel.h"

Define_Channel(FlowChannel);

//~ void FlowChannel::initialize() {
    //~ EV << datarate << endl;
//~ }

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

double FlowChannel::getAvailableBW() {
    return getDatarate() - par("used").doubleValue();
}

double FlowChannel::getAvailableBW(int priority) {
    std::set<int> prioritySet = getPrioritySet();
    // iterating through flows
    // if priority is higher than that requested
    // subtract from total available
    // return remainder
    //...
    return -1;
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

std::set<int> FlowChannel::getPrioritySet() {
    std::set<int> prioritySet;
    for (std::vector<Flow*>::iterator it = currentFlows.begin(); it != currentFlows.end(); it++) {
        prioritySet.insert((*it)->priority);
    }
    return prioritySet;
}
