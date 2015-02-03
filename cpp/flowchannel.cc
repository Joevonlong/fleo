#include "flowchannel.h"

Define_Channel(FlowChannel);

void FlowChannel::initialize() {
    bpsLeftAtPriority[INT_MAX] = par("datarate").doubleValue(); // getDatarate() returns 0 during initialization
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
double FlowChannel::getAvailableBps() {
    // should be equal to getAvailableBW(INT_MIN)
    return getDatarate() - par("used").doubleValue();
}

double FlowChannel::getAvailableBps(Priority p) {
    return bpsLeftAtPriority.lower_bound(p)->second;
}

double FlowChannel::getUsedBps() {
    return par("used").doubleValue();
}

void FlowChannel::setUsedBps(double bps) {
    if (bps > getDatarate()) {
        throw cRuntimeError("Using more bandwidth than total channel capacity.");
    }
    else if (bps < 0) {
        throw cRuntimeError("Setting bandwidth usage to negative.");
    }
    par("used").setDoubleValue(bps);
}

void FlowChannel::addUsedBps(double bps) {
    setUsedBps(par("used").doubleValue() + bps);
}
//

// Flow methods:
void FlowChannel::addFlow(Flow* f) {
    // check input
    std::map<Priority, double>::iterator it =
        bpsLeftAtPriority.lower_bound(f->priority);
    if (f->bps > it->second) {
        throw cRuntimeError("Insufficient bandwidth to add flow at given priority");
    }
    // add to list of tracked flows
    currentFlows.insert(f);
    // reserve bandwidth for the flow (to deprecate?)
    addUsedBps(f->bps);
    // update table of available bandwidths:
    std::map<Priority, double>::iterator subtractBpsUpTo =
        bpsLeftAtPriority.insert(
            std::pair<Priority, double>(f->priority, bpsLeftAtPriority[it->first])
        ).first; // insert priority key (if needed) and get iterator to that mapping
    // then subtract available bandwidth from all priorities up to and including itself
    for (; subtractBpsUpTo != bpsLeftAtPriority.begin(); --subtractBpsUpTo) {
        subtractBpsUpTo->second -= f->bps;
    } subtractBpsUpTo->second -= f->bps; // because loop doesnt act on first element.
}

void FlowChannel::removeFlow(Flow* f) {
    // check input
    std::map<Priority, double>::iterator it =
        bpsLeftAtPriority.find(f->priority);
    if (it == bpsLeftAtPriority.end()) {
        throw cRuntimeError("Priority of flow to remove not found");
    }
    if (f->bps + it->second > getDatarate()) {
        throw cRuntimeError("Resultant available bandwidth exceeds original capacity");
    }
    // remove from list of tracked flows
    if (currentFlows.erase(f) != 1) {
        throw cRuntimeError("Flow to remove not found");
    }
    // release bandwidth for the flow (to deprecate?)
    addUsedBps(-f->bps);
    // update table of available bandwidths:
    double bpsAfterRemoval = bpsLeftAtPriority[f->priority] +f->bps;
    if (++it != bpsLeftAtPriority.end()) {
        if (bpsAfterRemoval > it->second) {
            throw cRuntimeError("More bandwidth will be available at lower priority");
        }
        else if (bpsAfterRemoval == it->second) {
            // key no longer necessary if value is equal to the next one
            bpsLeftAtPriority.erase(f->priority);
        }
    }
    std::map<Priority, double>::iterator addBpsBefore = it;
    // now add available bandwidth to all lower priorities
    for (it = bpsLeftAtPriority.begin(); it != addBpsBefore; ++it) {
        it->second += f->bps;
    }
}
//

// for debugging output
void FlowChannel::printBpsLeftAtPriority() {
    for (std::map<Priority, double>::iterator it = bpsLeftAtPriority.begin();
        it != bpsLeftAtPriority.end(); ++it) {
        EV << it->second << " bps left at priority " << it->first << endl;
    }
}

bool FlowChannel::isFlowPossible(double bps, Priority p) {
    return bps <= getAvailableBps(p);
}
bool FlowChannel::isFlowPossible(Flow* f) {
    return f->bps <= getAvailableBps(f->priority);
}
