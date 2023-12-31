/*
 * Download.cc
 *
 *  Created on: Sep 10, 2015
 *      Author: gang
 */

#include <Download.h>

Download::Download() {
    //this->updated = true;
    //this->last_updated = simTime();
}

Download::~Download() {
    // TODO Auto-generated destructor stub
}

const std::list<Flow*>& Download::getSubflows() const {
    return subflows;
}

void Download::setSubflows(const std::list<Flow*>& flows) {
    subflows = flows;
}

void Download::addSubflow(Flow& flow) {
    subflows.push_back(&flow);
}

const std::list<ChannelTree*>& Download::getSubtrees() const {
    return subtrees;
}

void Download::setSubtrees(const std::list<ChannelTree*>& subtrees) {
    this->subtrees = subtrees;
}

void Download::addSubtree(ChannelTree& subtree) {
    subtrees.push_back(&subtree);
}

void Download::update() {
    last_updated = simTime();
    updated = true;
}
