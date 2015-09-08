#include "flowchannel.h"
#include "global.h"

Define_Channel(FlowChannel);

void FlowChannel::initialize() {
    bpsLeftAtPriority[INT_MAX] = par("datarate").doubleValue(); // workaround because getDatarate() returns 0 during initialization
    EV << "flowchannel init: total available bps " << bpsLeftAtPriority[INT_MAX] << endl;
    utilVec.setName("Utilisation fraction of channel");
    prevRecAt = 0; prevBw = 0; cumBwT = 0;
    utilVec.record(0);
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
uint64_t FlowChannel::getAvailableBps() {
    // should be equal to getAvailableBW(INT_MIN)
    EV << "FlowChannel::getAvailableBps " << getDatarate() << "\t"  << par("used").doubleValue() << endl;
    return getDatarate() - par("used").doubleValue();
}

uint64_t FlowChannel::getAvailableBps(Priority p) {
    return bpsLeftAtPriority.lower_bound(p)->second;
}

uint64_t FlowChannel::getUsedBps() {
    EV << "FlowChannel::getUsedBps " << par("used").doubleValue() << endl;
    return par("used").doubleValue();
}

void FlowChannel::setUsedBps(uint64_t bps) {
    if (bps > getDatarate()) {
        throw cRuntimeError("Using more bandwidth than total channel capacity.");
    }
    else if (bps < 0) {
        throw cRuntimeError("Setting bandwidth usage to negative.");
    }
    par("used").setDoubleValue(bps);
    EV << "FlowChannel::setUsedBps " << bps << "\t"  << par("used").doubleValue() << endl;
}

void FlowChannel::addUsedBps(int64_t bps) {
    setUsedBps(par("used").doubleValue() + bps);
    EV << "FlowChannel::addUsedBps " << bps << "\t" << par("used").doubleValue() << endl;
}
//

// Flow methods:
void FlowChannel::addFlow(Flow* f) {
    // check input
    std::map<Priority, uint64_t>::iterator it =
        bpsLeftAtPriority.lower_bound(f->priority);
    if (f->bps > it->second) {
        throw cRuntimeError("Insufficient bandwidth to add flow at given priority");
    }
    // add to list of tracked flows
    currentFlows.insert(f);
    // reserve bandwidth for the flow (to deprecate?)
    addUsedBps(f->bps);
    // update table of available bandwidths:
    std::map<Priority, uint64_t>::iterator subtractBpsUpTo =
        bpsLeftAtPriority.insert(
            std::pair<Priority, uint64_t>(f->priority, bpsLeftAtPriority[it->first])
        ).first; // insert priority key (if needed) and get iterator to that mapping
    // then subtract available bandwidth from all priorities up to and including itself
    for (; subtractBpsUpTo != bpsLeftAtPriority.begin(); --subtractBpsUpTo) {
        subtractBpsUpTo->second -= f->bps;
    } subtractBpsUpTo->second -= f->bps; // because loop doesnt act on first element.
    // now check if lower priority flow was displaced
    std::set<Flow*> flowsToDisplace;
    uint64_t bpsDeficit = -getAvailableBps();
    while (bpsDeficit > 0) {
        bool flowMarked = false;
        // get lowest priority
        Priority p = bpsLeftAtPriority.lower_bound(p)->first;
        // remove a flow at this priority
        // TODO add first-come-first-served priority
        // TODO search more efficiently
        for (std::set<Flow*>::iterator f_it = currentFlows.begin(); f_it != currentFlows.end(); ++f_it) {
            if (flowsToDisplace.count(*f_it)) {
                continue; // skip flows already to be displaced
            }
            if ((*f_it)->priority == p) { // need to check higher priorities that are still lower than new one!
                // add removed flow to return value
                flowsToDisplace.insert(*f_it);
                bpsDeficit -= (*f_it)->bps;
                flowMarked = true; break;
            }
        }
        if (!flowMarked) {
            throw cRuntimeError("FlowChannel::addFlow: no flows to displace");
        }
    }
    //
    recordUtil();
    return flowsToDisplace;
}

void FlowChannel::addFlowV2(Flow* f) {
    currentFlows.insert(f);
}

// these 3 helpers could be merged into pow(-0/-1/-2)
void FlowChannel::shareBwEqual(Flow* except) {
    int numFlows = 0;
    for (std::set<Flow*>::iterator it = currentFlows.begin(); it != currentFlows.end(); ++it) {
        if (*it == except) {continue;}
        ++numFlows;
    }
    for (std::set<Flow*>::iterator it = currentFlows.begin(); it != currentFlows.end(); ++it) {
        if (*it == except) {
            (*it)->bps = 0;
        }
        else {
            (*it)->bps = bpsLeftAtPriority[INT_MAX] / numFlows;
        }
    }
}
void FlowChannel::shareBwRttInverse(Flow* except) {
    double totalSharePoints = 0;
    for (std::set<Flow*>::iterator it = currentFlows.begin(); it != currentFlows.end(); ++it) {
        if (*it == except) {continue;}
        totalSharePoints += 1/(*it)->lag.dbl();
    }
    for (std::set<Flow*>::iterator it = currentFlows.begin(); it != currentFlows.end(); ++it) {
        if (*it == except) {
            (*it)->bps = 0;
        }
        else {
            (*it)->bps = bpsLeftAtPriority[INT_MAX] / totalSharePoints / (*it)->lag.dbl();
        }
    }
}
void FlowChannel::shareBwRtt2Inverse(Flow* except) {
    double totalSharePoints = 0;
    for (std::set<Flow*>::iterator it = currentFlows.begin(); it != currentFlows.end(); ++it) {
        if (*it == except) {continue;}
        totalSharePoints += pow((*it)->lag.dbl(), -2);
    }
    for (std::set<Flow*>::iterator it = currentFlows.begin(); it != currentFlows.end(); ++it) {
        if (*it == except) {
            (*it)->bps = 0;
        }
        else {
            (*it)->bps = bpsLeftAtPriority[INT_MAX] / totalSharePoints * pow((*it)->lag.dbl(), -2);
        }
    }
}
void FlowChannel::shareBWexcept(std::map<Flow*, cMessage*> flowEnds, Flow* except) {
    // update bits remaining for all flows
    for (std::set<Flow*>::iterator it = currentFlows.begin(); it != currentFlows.end(); ++it) {
        EV << (*it)->bits_left << endl;
        (*it)->bits_left -= (*it)->bps * (simTime() - (*it)->lastUpdate).dbl();
        if ((*it)->bits_left < 0) {throw cRuntimeError("FlowChannel::shareBW: flow has negative bits remaining");}
        (*it)->lastUpdate = simTime();
    }
    // assign new bandwidth-share (assume equal split for now)
    shareBwRttInverse(except);
    // update end-timers
    for (std::set<Flow*>::iterator it = currentFlows.begin(); it != currentFlows.end(); ++it) {
        flowEnds[*it]->setTimestamp(simTime() + (double)(*it)->bits_left / (double)(*it)->bps);
        EV << "timestamp: " << flowEnds[*it]->getTimestamp()
           << ", simtime: " << simTime()
           << ", bits left: "<< (*it)->bits_left
           << ", bps: "<< (*it)->bps << endl;
    }
}

void FlowChannel::spreadUpdates() {
    // update utilisations of all affected channels:
    // all channels traversed by each flow in this (first) hop.
    std::set<cChannel*> affectedChs; // to update each channel only once
    for (std::set<Flow*>::iterator f_it = currentFlows.begin(); f_it != currentFlows.end(); ++f_it) {
        affectedChs.insert((*f_it)->channels.begin(), (*f_it)->channels.end());
    }
    for (std::set<cChannel*>::iterator c_it = affectedChs.begin(); c_it != affectedChs.end(); ++c_it) {
        FlowChannel *fc = check_and_cast<FlowChannel*>(*c_it);
        fc->bpsLeftAtPriority[0] = fc->bpsLeftAtPriority[INT_MAX];
        for (std::set<Flow*>::iterator f_it = fc->currentFlows.begin(); f_it != fc->currentFlows.end(); ++f_it) {
            fc->bpsLeftAtPriority[0] -= (*f_it)->bps;
        }
        fc->recordUtil();
    }
}

void FlowChannel::removeFlow(Flow* f) {
    // check input
    std::map<Priority, uint64_t>::iterator it =
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
    uint64_t bpsAfterRemoval = bpsLeftAtPriority[f->priority] +f->bps;
    if (++it != bpsLeftAtPriority.end()) {
        if (bpsAfterRemoval > it->second) {
            throw cRuntimeError("More bandwidth will be available at lower priority");
        }
        else if (bpsAfterRemoval == it->second) {
            // key no longer necessary if value is equal to the next one
            bpsLeftAtPriority.erase(f->priority);
        }
    }
    std::map<Priority, uint64_t>::iterator addBpsBefore = it;
    // now add available bandwidth to all lower priorities
    for (it = bpsLeftAtPriority.begin(); it != addBpsBefore; ++it) {
        it->second += f->bps;
    }
    recordUtil();
}

void FlowChannel::removeFlowV2(Flow* f) {
    currentFlows.erase(f);
}
//

// for debugging output
void FlowChannel::printBpsLeftAtPriority() {
    for (std::map<Priority, uint64_t>::iterator it = bpsLeftAtPriority.begin();
        it != bpsLeftAtPriority.end(); ++it) {
        EV << it->second << " bps left at priority " << it->first << endl;
    }
}

bool FlowChannel::isFlowPossible(uint64_t bps, Priority p) {
    return bps <= getAvailableBps(p);
}
bool FlowChannel::isFlowPossible(Flow* f) {
    return f->bps <= getAvailableBps(f->priority);
}

void FlowChannel::recordUtil() {
    // accumulate just-finished rectangle of bandwidth-time product
    cumBwT += prevBw * (simTime()-prevRecAt).dbl();
    // update global network load stats
    ((Global*)getParentModule()->getSubmodule("global"))->recordNetLoad(getAvailableBps(INT_MIN)-prevBw);
    // update data for new rectangle
    prevBw = getDatarate() - getAvailableBps(INT_MIN);
    prevRecAt = simTime();
    // record current bandwidth usage
    utilVec.record(1 - getAvailableBps(INT_MIN)/getDatarate()); // new prevBw ie. current BW usage
}

void FlowChannel::finish() {
    recordUtil();
    recordScalar("Time-averaged utilisation", cumBwT/(getDatarate()*simTime().dbl()));
}
