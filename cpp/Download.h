/*
 * Download.h
 *
 *  Created on: Sep 10, 2015
 *      Author: gang
 */

#ifndef DOWNLOAD_H_
#define DOWNLOAD_H_

#include <omnetpp.h>
#include "Flow.h"
#include "ChannelTree.h"

class Download {
public:
    Download();
    virtual ~Download();
    const std::list<Flow*>& getSubflows() const;
    void setSubflows(const std::list<Flow*>& subflows);
    void addSubflow(Flow& subflow);
    const std::list<ChannelTree*>& getSubtrees() const;
    void setSubtrees(const std::list<ChannelTree*>& subtrees);
    void addSubtree(ChannelTree& subtree);
protected:
    std::list<Flow*> subflows;
    std::list<ChannelTree*> subtrees;
    uint64_t bits;
    uint64_t bitsTransferred;
    uint64_t bitsRemaining;
    // data maintenance:
    bool updated;
    simtime_t last_updated;
    void update();
};
typedef Download DL;

#endif /* DOWNLOAD_H_ */
