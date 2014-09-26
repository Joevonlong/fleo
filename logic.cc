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

int Logic::numInitStages() const {return 4;}

void Logic::initialize(int stage) {
    if (stage == 0) {
        global = (Global*)getParentModule()->getParentModule()
            ->getSubmodule("global");
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
                cGate* outGate = getNextGate(this, pkt);
                send(pkt, outGate);
                return;
            }
            else {
                EV << "checkCache -> " << vidBitSize << endl;
                EV << "custom id: " << pkt->getCustomID() << endl;
                EV << getVideoBitSize(pkt->getCustomID()) << endl;
                error("Invalid return from checkCache.");
            }
        } // end if stateStart
        else if (pkt->getState() == stateStream) {
            error("here we are");
        }
        else if (pkt->getState() == stateEnd) {
            error("not currently used");
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
                error("Cache-to-cache transfer without user request");
            }
        }
        else if (pkt->getState() == stateTransfer) {
            if (pkt->getVideoSegmentsPending() == 0) {
                EV << "Fetch from other cache completed.\n";
                // assume all misses are cached (as in LRU?)
                ((Cache*)getParentModule()->getSubmodule("cache"))->setCached(
                    pkt->getCustomID(), true);
            }
            else {
                // do nothing
            }
            // decapsulate
            if (pkt->hasEncapsulatedPacket() == true) {
                MyPacket *innerPkt = (MyPacket*)pkt->decapsulate();
                innerPkt->setDestinationID(innerPkt->getSourceID());
                innerPkt->setSourceID(getId());
                innerPkt->setVideoLength(pkt->getVideoLength());
                innerPkt->setVideoSegmentLength(pkt->getVideoSegmentLength());
                innerPkt->setVideoSegmentsPending(pkt->getVideoSegmentsPending());
                innerPkt->setVideoLengthPending(pkt->getVideoLengthPending());
                innerPkt->setState(stateTransfer);
                // not using bitsPending
                cGate* outGate = getNextGate(this, innerPkt);
                send(innerPkt, outGate);
                delete pkt;
                return;
            }
            else {error("Cache-to-cache transfer without user request");}
        }
        else if (pkt->getState() == stateAck) { // continue transfer
            error("not currently used");
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

