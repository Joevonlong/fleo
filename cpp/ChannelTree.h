/*
 * ChannelTree.h
 *
 *  Created on: Feb 14, 2016
 *      Author: gang
 */

#ifndef CHANNELTREE_H_
#define CHANNELTREE_H_

#include <omnetpp.h>
#include "Flow.h"
#include "flowchannel.h"

typedef std::set<FlowChannel*> FlowChannelTree;

class ChannelTree {
public:
    ChannelTree();
    virtual ~ChannelTree();
    bool isActive() const;
    void setActive(bool active);
    const std::vector<cTopology::Node*>& getPath();
    const FlowChannelTree& getChannels() const;
    void setChannels(const FlowChannelTree& channels);
    // 3 ways to add channels
    void addChannels(const FlowChannelTree& channels);
    void addChannels(Flow& flow);
    void addChannels(Path path);
    void addChannels(const std::vector<cChannel*>& channels);
    const simtime_t& getLag();
    uint64_t getBps() const;
    void setBps(uint64_t bps);
    uint64_t getBpsMin() const;
    void setBpsMin(uint64_t bpsMin);
    uint64_t getBitsLeft() const;
    void setBitsLeft(uint64_t bitsLeft);
    Priority getPriority() const;
    void setPriority(Priority priority);
protected:
    bool active; // whether flow is actually moving
    std::vector<cTopology::Node*> path; // no use anymore? relegate to secondary?
    FlowChannelTree channels;
    simtime_t lag; // round-trip-time / latency / lag / delay
    uint64_t bps;
    uint64_t bpsMin;
    uint64_t bits_left;
    Priority priority; // larger number signifies higher priority eg. 5 overrides 3
    // data maintenance:
    bool updated;
    simtime_t last_updated;
    void update();
    void updateNodes();
    void updateLag();
};

#endif /* CHANNELTREE_H_ */
