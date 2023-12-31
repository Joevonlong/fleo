#include "logic.h"
#include "global.h"
#include "parse.h"
#include "request_m.h"
#include "reply_m.h"
#include "mypacket_m.h"
#include "routing.h"
#include "cache.h"

Define_Module(Logic);

const int64_t noCache = -2;
const int64_t notCached = -1;
const uint64_t packetBitSize = pow(10, 3*2) * 8; // 10MB: takes 0.13s for OC12

int Logic::numInitStages() const {return 5;}

void Logic::initialize(int stage) {
    if (stage == 0) {
        global = (Global*)getParentModule()->getParentModule()
            ->getSubmodule("global");
    }
    else if (stage == 1) {
        registerSelfIfCache();
    }
    else if (stage == 3) {
        if (hasCache()) {
            // find nearest origin server
            if (!isOrigin()) {
                nearestCompleteCache = getNearestID(getId(), completeCacheIDs);
                distToCompleteCache = getDistanceBetween(getId(), nearestCompleteCache);
                EV << "Master cache for " << getFullPath() << "("
                   << getParentModule()->par("loc").stringValue() << ") is "
                   << simulation.getModule(nearestCompleteCache)->getFullPath()
                   << "(" << simulation.getModule(nearestCompleteCache)
                      ->getParentModule()->par("loc").stringValue() << ").\n";
            }
            else {
                nearestCompleteCache = getId();
                distToCompleteCache = 0;
            }
        }
    }
    else if (stage == 4) {
        if (isOrigin()) {
            nearestCache = getId();
            return;
        }
        if (hasCache()) {
            // find replica with smallest detour from origin
            double pathLength;
            double shortestPathLength = DBL_MAX;
            nearestCache = -1;
            for (std::vector<int>::iterator id = cacheIDs.begin();
                id != cacheIDs.end(); id++) {
                if (*id == getId()) {continue;}
                else if (simulation.getModule(*id)->getParentModule()
                    ->par("completeCache").boolValue() == false) {
                    pathLength = getDistanceBetween(getId(), *id)
                        + ((Logic*)simulation.getModule(*id))->distToCompleteCache;
                    if (pathLength < shortestPathLength) {
                        shortestPathLength = pathLength;
                        nearestCache = *id;
                    }
                }
            }
            EV << "Secondary cache for " << getFullPath() << "("
               << getParentModule()->par("loc").stringValue() << ") is "
               << simulation.getModule(nearestCache)->getFullPath()
               << "(" << simulation.getModule(nearestCache)
                  ->getParentModule()->par("loc").stringValue() << ").\n";
        }
    }
}

void Logic::handleMessage(cMessage *msg) {
    MyPacket *pkt = check_and_cast<MyPacket*>(msg);
    if (pkt->getDestinationID() == getId()) { // destination reached
        if (pkt->getState() == stateStart) {
            // increment hops
            pkt->setHops(pkt->getHops()+1);
            // check cache
            int64_t vidBitSize = checkCache(pkt->getCustomID());
            if (vidBitSize == noCache) {
                error("Content requested from node without a cache.");
            }
            else if (vidBitSize == notCached) {
                MyPacket *outerPkt = new MyPacket("Request");
                outerPkt->setBitLength(headerBitLength); // assume no transmission delay
                outerPkt->setSourceID(getId());
                outerPkt->setState(stateStart);
                outerPkt->setHops(pkt->getHops());
                outerPkt->setCacheTries(pkt->getCacheTries()-1);
                outerPkt->setCustomID(pkt->getCustomID());
                if (outerPkt->getCacheTries() > 0) { // check secondary
                    outerPkt->setDestinationID(nearestCache);
                    EV << "Checking secondary cache: ";
                }
                else if (outerPkt->getCacheTries() == 0) { // get from master
                    outerPkt->setDestinationID(nearestCompleteCache);
                    EV << "Checking master cache: ";
                }
                else {
                    EV << "cacheTries = " << outerPkt->getCacheTries() << endl;
                    error("Invalid cacheTries value.");
                }
                EV << simulation.getModule(outerPkt->getDestinationID())
                    ->getFullPath() << endl;
                outerPkt->encapsulate(pkt);
                cGate* outGate = NULL;//getNextGate(this, outerPkt);
                send(outerPkt, outGate);
                return;
            }
            else if (vidBitSize > 0) { // cached
                pkt->setDestinationID(pkt->getSourceID());
                pkt->setSourceID(getId());
                uint64_t vidlen = vidBitSize/bitRate;
                pkt->setVideoLength(vidlen);
                pkt->setVideoSegmentsPending(
                    vidlen / global->getBufferBlock()
                    + (vidlen % global->getBufferBlock() != 0)
                    -1);
                if (pkt->getVideoSegmentsPending() > 0) {
                    pkt->setVideoSegmentLength(global->getBufferBlock());
                    pkt->setBitLength(global->getBufferBlock()*bitRate);
                }
                else if (pkt->getVideoSegmentsPending() == 0){
                    pkt->setVideoSegmentLength(vidlen);
                    pkt->setBitLength(vidlen*bitRate);
                }
                else {error("Initial videoSegmentsPending < 0");}
                EV << "Initial bitLength: " << pkt->getBitLength() << endl;
                pkt->setVideoLengthPending(vidlen-pkt->getVideoSegmentLength());
                pkt->setState(stateTransfer);
                EV << "Requested item #" << pkt->getCustomID()
                   << " is cached. Sending reply.\n";
                global->recordRequestedLength(pkt->getVideoLength());
                cGate* outGate = NULL;// getNextGate(this, pkt);
                send(pkt, outGate);
                return;
            }
            else {error("Invalid return from checkCache.");}
        } // end if stateStart
        else if (pkt->getState() == stateStream) {
            int64_t vidBitSize = checkCache(pkt->getCustomID());
            if (vidBitSize == noCache) {
                error("Content streamed from node without a cache.");
            }
            else if (vidBitSize == notCached) {
                MyPacket *outerPkt = new MyPacket("Request");
                outerPkt->setBitLength(headerBitLength);
                outerPkt->setSourceID(getId());
                outerPkt->setState(stateStream);
                outerPkt->setCacheTries(pkt->getCacheTries()-1);
                outerPkt->setCustomID(pkt->getCustomID());
                outerPkt->setVideoLength(pkt->getVideoLength());
                outerPkt->setVideoSegmentLength(pkt->getVideoSegmentLength());
                outerPkt->setVideoSegmentsPending(pkt->getVideoSegmentsPending());
                outerPkt->setVideoLengthPending(pkt->getVideoLengthPending());
                if (outerPkt->getCacheTries() > 0) { // check secondary
                    outerPkt->setDestinationID(nearestCache);
                    EV << "Checking secondary cache: ";
                }
                else if (outerPkt->getCacheTries() == 0) { // get from master
                    outerPkt->setDestinationID(nearestCompleteCache);
                    EV << "Checking master cache: ";
                }
                else {
                    EV << "cacheTries = " << outerPkt->getCacheTries() << endl;
                    error("Invalid cacheTries value.");
                }
                EV << simulation.getModule(outerPkt->getDestinationID())
                    ->getFullPath() << endl;
                outerPkt->encapsulate(pkt);
                cGate* outGate = NULL;// getNextGate(this, outerPkt);
                send(outerPkt, outGate);
                return;
            }
            else if (vidBitSize > 0) { // cached
                pkt->setDestinationID(pkt->getSourceID());
                pkt->setSourceID(getId());
                pkt->setState(stateTransfer);
                pkt->setVideoSegmentsPending(
                    pkt->getVideoSegmentsPending()-1);
                if (pkt->getVideoSegmentsPending() > 0) {
                    pkt->setVideoSegmentLength(global->getBufferBlock());
                }
                else if (pkt->getVideoSegmentsPending() == 0){
                    pkt->setVideoSegmentLength(
                        pkt->getVideoLength() % global->getBufferBlock());
                }
                else {error("videoSegmentsPending < 0 while streaming");}
                pkt->setBitLength(pkt->getVideoSegmentLength()*bitRate);
                pkt->setVideoLengthPending(
                    pkt->getVideoSegmentLength() - pkt->getVideoSegmentLength());
                cGate* outGate = NULL;// getNextGate(this, pkt);
                send(pkt, outGate);
                return;
            }
            else {error("Invalid return from checkCache.");}
        } // end if stateStream
        else if (pkt->getState() == stateTransfer) {
            if (pkt->getVideoSegmentsPending() == 0) {
                EV << "Fetch from other cache completed.\n";
                // assume all misses are cached (as in LRU?)
                ((Cache*)getParentModule()->getSubmodule("cache"))->setCached(
                    pkt->getCustomID(), true);
            }
            else {/*do nothing*/}
            // decapsulate
            if (pkt->hasEncapsulatedPacket() == true) {
                MyPacket *innerPkt = (MyPacket*)pkt->decapsulate();
                innerPkt->setDestinationID(innerPkt->getSourceID());
                innerPkt->setSourceID(getId());
                innerPkt->setHops(pkt->getHops());
                innerPkt->setState(stateTransfer);
                innerPkt->setVideoLength(pkt->getVideoLength());
                innerPkt->setVideoSegmentLength(pkt->getVideoSegmentLength());
                innerPkt->setVideoSegmentsPending(pkt->getVideoSegmentsPending());
                innerPkt->setVideoLengthPending(pkt->getVideoLengthPending());
                innerPkt->setBitLength(pkt->getBitLength());
                // not using bitsPending
                cGate* outGate = NULL;// getNextGate(this, innerPkt);
                send(innerPkt, outGate);
                delete pkt;
                return;
            }
            else {error("Cache-to-cache transfer without user request");}
        }
        else {error("Invalid packet state");}
    }
    else { // destination not reached: forward
        // increment hops first
        if (pkt->getState() == stateStart) {
            pkt->setHops(pkt->getHops()+1);
        }
        // ---
        cGate* outGate = NULL;// getNextGate(this, pkt);
        send(pkt, outGate);
        EV << "Forwarding request via " << outGate->getFullName() << endl;
        return;
    }
}

bool Logic::hasCache() {
    return check_and_cast<Cache*>(getParentModule()->getSubmodule("cache"))->hasCache();
}
bool Logic::isOrigin() {
    return check_and_cast<Cache*>(getParentModule()->getSubmodule("cache"))->isOrigin();
}
void Logic::setCached(int customID, bool b) {
    check_and_cast<Cache*>(getParentModule()->getSubmodule("cache"))->setCached(customID, b);
}

// inserts logic's ID as value with location as key,
// as well as if cache is a complete one.
void Logic::registerSelfIfCache() {
    if (hasCache()) {
        locCaches[getParentModule()->par("loc").stringValue()] = getId();
        if (isOrigin()) {
            completeCacheIDs.push_back(getId());
        }
        else {
            incompleteCacheIDs.push_back(getId());
        }
    }
}

int64_t Logic::checkCache(int customID) {
    if (!hasCache()) {
        return noCache; // module has no cache
    }
    else if (isOrigin()) {
        return getVideoBitSize(customID);
    }
    else if (((Cache*)(getParentModule()->getSubmodule("cache")))->isCached(customID)) {
        global->recordCacheHit(true);
        return getVideoBitSize(customID); // should be video's bitsize
    }
    else {
        global->recordCacheHit(false);
        return notCached; // requested item is not cached
    }
}

void requestFromCache(int cacheID, int customID) {
    EV << "Requesting "<< customID << " from secondary cache.\n";
    Request *req = new Request("cache to cache", requestKind);
    req->setBitLength(1);
}

std::deque<Logic*> Logic::getRequestWaypoints(int vID, int tries) {
    /**
     * Limit tries to at most 2 for now sinca replicas only know one other replica.
     * TODO: for this reason, this function should be moved to controller.
     */
    int64_t bitsize = checkCache(vID);
    tries -= 1;
    if (bitsize == noCache) {error("user request not sent to cache");}
    else if (bitsize == notCached){
        // assume cache content since LRU. FUTURE: determine if content should be cached
        // add waypoint if so and forward request to next cache
        std::deque<Logic*> ret;
        if (tries > 0) {
            ret = ((Logic*)simulation.getModule(nearestCache))->getRequestWaypoints(vID, tries);
        }
        else {
            ret = ((Logic*)simulation.getModule(nearestCompleteCache))->getRequestWaypoints(vID, tries);
        }
        ret.push_front(this);
        return ret;
    }
    else if (bitsize < 0) {error("invalid checkCache result");}
    else {
        // content is cached. return self as last waypoint.
        return std::deque<Logic*>(1, this);
    }
    error("Logic::getRequestWaypoints: should not reach this");
    return std::deque<Logic*>(); // just to remove warning
}
