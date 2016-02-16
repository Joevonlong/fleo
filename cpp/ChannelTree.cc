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

const std::vector<cTopology::Node*>& ChannelTree::getPath() const {
    return path;
}

void ChannelTree::setPath(const std::vector<cTopology::Node*>& path) {
    this->path = path;
    updated = false;
}

const std::vector<cChannel*>& ChannelTree::getChannels() {
    update();
    return channels;
}

void ChannelTree::setChannels(const std::vector<cChannel*>& channels) {
    this->channels = channels;
}

const simtime_t& ChannelTree::getLag() {
    update();
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
    updateChannels();
    updateLag();
    last_updated = simTime();
    updated = true;
}

void ChannelTree::updateChannels() {
}

void ChannelTree::updateLag() {
}
