/*
 * ChannelTree.cc
 *
 *  Created on: Feb 14, 2016
 *      Author: gang
 */

#include <ChannelTree.h>

ChannelTree::ChannelTree() {
    active = false;
    updated = false;
}

ChannelTree::~ChannelTree() {
}

bool ChannelTree::isActive() const {
    return active;
}

void ChannelTree::setActive(bool active) {
    this->active = active;
}

const std::vector<cTopology::Node*>& ChannelTree::getPath() {
    updateNodes();
    return path;
}

const FlowChannelTree& ChannelTree::getChannels() const {
    return channels;
}

void ChannelTree::setChannels(const FlowChannelTree& channels) {
    this->channels = channels;
    updated = false;
}

void ChannelTree::addChannels(const FlowChannelTree& channels) {
    for (FlowChannelTree::iterator fct_it = channels.begin(); fct_it != channels.end(); ++fct_it) {
        this->channels.insert(*fct_it);
    }
}

void ChannelTree::addChannels(Flow& flow) {
    addChannels(flow.getChannels());
}

void ChannelTree::addChannels(const std::vector<cChannel*>& channels) {
    for (std::vector<cChannel*>::const_iterator cv_it = channels.begin(); cv_it != channels.end(); ++cv_it) {
        this->channels.insert(check_and_cast<FlowChannel*>(*cv_it));
    }
}

const simtime_t& ChannelTree::getLag() {
    updateLag();
    return lag;
}

uint64_t ChannelTree::getBps() const {
    return bps;
}

void ChannelTree::setBps(uint64_t bps) {
    this->bps = bps;
}

uint64_t ChannelTree::getBpsMin() const {
    return bpsMin;
}

void ChannelTree::setBpsMin(uint64_t bpsMin) {
    this->bpsMin = bpsMin;
}

uint64_t ChannelTree::getBitsLeft() const {
    return bits_left;
}

void ChannelTree::setBitsLeft(uint64_t bitsLeft) {
    bits_left = bitsLeft;
}

Priority ChannelTree::getPriority() const {
    return priority;
}

void ChannelTree::setPriority(Priority priority) {
    this->priority = priority;
}

void ChannelTree::update() {
    if (updated) {return;}
    updateNodes();
    updateLag();
    last_updated = simTime();
    updated = true;
}

void ChannelTree::updateNodes() {
    // NYI
}

void ChannelTree::updateLag() {
    // NYI
    /*lag = 0;
    for (FlowChannelTree::iterator fct_it = channels.begin(); fct_it != channels.end()-1; ++fct_it) {
        //lag += (check_and_cast<FlowChannel*>(*fct_it))->getDelay();
    }*/
}
