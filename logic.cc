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
const uint64_t packetBitSize = UINT64_MAX; //pow(10, 1+3*2) * 8; // 10MB: takes 0.13s for OC12

int Logic::numInitStages() const {return 4;}

void Logic::initialize(int stage) {
    if (stage == 0) {
    }
    else if (stage == 1) {
        registerSelfIfCache();
        // if master cache, populate with all content
        if (getParentModule()->par("completeCache").boolValue() == true) {
            Cache* cache = (Cache*)(getParentModule()->getSubmodule("cache"));
            for (unsigned long maxID = getMaxCustomVideoID(); maxID != ULONG_MAX; maxID--) {
                cache->setCached(maxID, true, true);
            }
            EV << "asdf"<<((Cache*)getParentModule()->getSubmodule("cache"))->diskUsed << endl;
        }
    }
    else if (stage == 3) {
        if (getParentModule()->par("hasCache").boolValue() == true) {
            nearestCache = getNearestCacheID(getId());
            EV << "Secondary cache for " << getFullPath() << "("
               << getParentModule()->par("loc").stringValue() << ") is "
               << simulation.getModule(nearestCache)->getFullPath()
               << "(" << simulation.getModule(nearestCache)
                  ->getParentModule()->par("loc").stringValue() << ").\n";
            if (getParentModule()->par("completeCache").boolValue() == false) {
                nearestCompleteCache = getNearestID(getId(), completeCacheIDs);
                EV << "Master cache for " << getFullPath() << "("
                   << getParentModule()->par("loc").stringValue() << ") is "
                   << simulation.getModule(nearestCompleteCache)->getFullPath()
                   << "(" << simulation.getModule(nearestCompleteCache)
                      ->getParentModule()->par("loc").stringValue() << ").\n";
            }
            else {
                nearestCompleteCache = getId();
            }
        }
    }
}

void Logic::handleMessage(cMessage *msg) {
    MyPacket *pkt = check_and_cast<MyPacket*>(msg);
    if (pkt->getDestinationID() == getId()) { // destination reached
        if (pkt->getState() == stateStart) {
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
                cGate* outGate = getNextGate(this, outerPkt);
                send(outerPkt, outGate);
                return;
            }
            else if (vidBitSize > 0) { // cached
                pkt->setDestinationID(pkt->getSourceID());
                pkt->setSourceID(getId());
                pkt->setVideoLength(vidBitSize/800000);
                pkt->setBitLength(std::min((uint64_t)vidBitSize,packetBitSize));
                EV << pkt->getBitLength() << endl;
                pkt->setBitsPending(vidBitSize-pkt->getBitLength());
                if (pkt->getBitsPending() == 0) {
                    pkt->setState(stateEnd);
                }
                else {
                    pkt->setState(stateTransfer);
                }
                EV << "Requested item #" << pkt->getCustomID()
                   << " is cached. Sending reply.\n";
                cGate* outGate = getNextGate(this, pkt);
                send(pkt, outGate);
                return;
            }
            else {
                EV << "checkCache -> " << vidBitSize << endl;
                EV << "custom id: " << pkt->getCustomID() << endl;
                EV << getVideoBitSize(pkt->getCustomID()) << endl;
                throw std::runtime_error("Invalid return from checkCache.");
            }
        }
        else if (pkt->getState() == stateEnd) {
            EV << "Fetch from other cache completed.\n";
            // assume all misses are cached (as in LRU?)
            ((Cache*)getParentModule()->getSubmodule("cache"))->setCached(
                pkt->getCustomID(), true);
            if (pkt->hasEncapsulatedPacket() == true) {
                MyPacket *innerPkt = (MyPacket*)pkt->decapsulate();
                innerPkt->setVideoLength(pkt->getVideoLength());
                delete pkt;
                scheduleAt(simTime(), innerPkt);
                return;
            }
            else {
                throw std::runtime_error(
                    "Cache-to-cache transfer without user request");
            }
        }
        else if (pkt->getState() == stateTransfer) {
            // acknowledge transfer
            pkt->setDestinationID(pkt->getSourceID());
            pkt->setSourceID(getId());
            pkt->setState(stateAck);
            pkt->setBitLength(headerBitLength);
            cGate* outGate = getNextGate(this, pkt);
            send(pkt, outGate);
            return;
        }
        else if (pkt->getState() == stateAck) { // continue transfer
            pkt->setDestinationID(pkt->getSourceID());
            pkt->setSourceID(getId());
            pkt->setBitLength(std::min(pkt->getBitsPending(),packetBitSize));
            pkt->setBitsPending(pkt->getBitsPending()-pkt->getBitLength());
            if (pkt->getBitsPending() == 0) {
                pkt->setState(stateEnd);
            }
            else {
                pkt->setState(stateTransfer);
            }
            cGate* outGate = getNextGate(this, pkt);
            send(pkt, outGate);
            return;
        }
        else {
            error("Invalid packet state");
        }
    }
    else { // destination not reached: forward
        cGate* outGate = getNextGate(this, pkt);
        send(pkt, outGate);
        EV << "Forwarding request via " << outGate->getFullName() << endl;
        return;
    }
}

// inserts logic's ID as value with location as key,
// as well as if cache is a complete one.
void Logic::registerSelfIfCache() {
    if (getParentModule()->par("hasCache").boolValue() == true) {
        locCaches[getParentModule()->par("loc").stringValue()] = getId();
        if (getParentModule()->par("completeCache").boolValue() == true) {
            completeCacheIDs.push_back(getId());
        }
    }
}

int64_t Logic::checkCache(int customID) {
    if (getParentModule()->par("hasCache").boolValue() == false) {
        return noCache; // module has no cache
    }
    else if (((Cache*)(getParentModule()->getSubmodule("cache")))->isCached(customID)) {
        return getVideoBitSize(customID); // should be video's bitsize
    }
    else {
        return notCached; // requested item is not cached
    }
}

void requestFromCache(int cacheID, int customID) {
    EV << "Requesting "<< customID << " from secondary cache.\n";
    Request *req = new Request("cache to cache", requestKind);
    req->setBitLength(1);
    //req->
//                Reply *reply = new Reply("reply", replyKind);
//                EV << "reply size: " << vidBitSize << endl;
//                reply->setBitLength(vidBitSize);
//                reply->setSourceID(getId());
//                reply->setDestinationID(req->getSourceID());
}

