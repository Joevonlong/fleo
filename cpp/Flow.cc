/*
 * Flow.cc
 *
 *  Created on: Sep 11, 2015
 *      Author: gang
 */

#include <Flow.h>
#include "flowchannel.h"

Flow::Flow() {
    active = false;
    updated = false;
}

Flow::~Flow() {
}

bool Flow::isActive() const {
    return active;
}
void Flow::setActive(bool active) {
    this->active = active;
}

const std::set<cModule*>& Flow::getModules() {
    update();
    return modules;
}
void Flow::setPath(const Path& path) {
    this->path = path;
    setChannels(FlowChannels());
    addChannels(path);
    updated = false;
}

const FlowChannels& Flow::getChannels() {
    return channels;
}

void Flow::setChannels(const FlowChannels& channels) {
    this->channels = channels;
    updated = false;
}

void Flow::addChannels(const FlowChannels& channels) {
    for (FlowChannels::iterator fc_it  = channels.begin();
                                fc_it != channels.end();
                              ++fc_it) {
        this->channels.insert(*fc_it);
    }
}
void Flow::addChannels(Flow& flow) {
    addChannels(flow.getChannels());
}
void Flow::addChannels(Path path) {
    if (path.size() == 0) {
        throw cRuntimeError("Flow::addChannels: Adding channels for empty path.");
    }
    for (Path::iterator p_it = path.begin(); p_it != path.end()-1; ++p_it) {
        int i;
        for (i=0; i<(*p_it)->getNumOutLinks(); ++i) {
            if ((*p_it)->getLinkOut(i)->getRemoteNode() == *(p_it+1)) {
                channels.insert(
                    check_and_cast<FlowChannel*>(
                        (*p_it)->getLinkOut(i)->getLocalGate()->getTransmissionChannel()
                    )
                );
                break;
            }
        }
        if (i == (*p_it)->getNumOutLinks()) {
            throw cRuntimeError("Flow::addChannels: path argument is not fully linked");
        }
    }
}

const simtime_t& Flow::getLag() {
    update();
    return lag;
}

uint64_t Flow::getBps() const {
    return bps;
}
void Flow::setBps(uint64_t bps) {
    this->bps = bps;
}

uint64_t Flow::getBpsMin() const {
    return bpsMin;
}
void Flow::setBpsMin(uint64_t bpsMin) {
    this->bpsMin = bpsMin;
}

uint64_t Flow::getBitsLeft() const {
    return bits_left;
}
void Flow::setBitsLeft(uint64_t bitsLeft) {
    bits_left = bitsLeft;
}

Priority Flow::getPriority() const {
    return priority;
}
void Flow::setPriority(Priority priority) {
    this->priority = priority;
}

void Flow::update() {
    if (updated) {return;}
    updateModules();
    updateLag();
    last_updated = simTime();
    updated = true;
}

void Flow::updateModules() {
    modules.clear();
    for (FlowChannels::iterator fc_it = channels.begin(); fc_it != channels.end(); ++fc_it) {
        modules.insert((*fc_it)->getSourceGate()->getPathStartGate()->getOwnerModule());
        modules.insert((*fc_it)->getSourceGate()->getPathEndGate()->getOwnerModule());
    }
}

void Flow::updateLag() {
    lag = 0;
    for (FlowChannels::iterator fc_it = channels.begin(); fc_it != channels.end(); ++fc_it) {
        lag += (*fc_it)->getDelay();
    }
}
