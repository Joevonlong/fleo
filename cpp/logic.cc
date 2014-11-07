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
        // build paths to this node
        topo.clear();
        topo.extractByNedTypeName(cStringTokenizer("Buffer PoPLogic CoreLogic BeyondLogic Logic User").asVector());
        cTopology::Node *thisNode =
            topo.getNodeFor(simulation.getModule(getId()));
        topo.calculateUnweightedSingleShortestPathsTo(thisNode);
    }
    else if (stage == 1) {
        registerSelfIfCache();
        // if origin server, populate with all content
        if (getParentModule()->par("completeCache").boolValue() == true) {
            Cache* cache = (Cache*)(getParentModule()->getSubmodule("cache"));
            for (unsigned long maxID = getMaxCustomVideoID(); maxID != ULONG_MAX; maxID--) {
                cache->setCached(maxID, true, true);
            }
        }
    }
    else if (stage == 3) {
        if (getParentModule()->par("hasCache").boolValue() == true) {
            // find nearest origin server
            if (getParentModule()->par("completeCache").boolValue() == false) {
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
        return;
        if (getParentModule()->par("hasCache").boolValue() == true) {
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
                global->recordRequestedLength(pkt->getVideoLength());
                cGate* outGate = getNextGate(this, pkt);
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
                cGate* outGate = getNextGate(this, outerPkt);
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
                cGate* outGate = getNextGate(this, pkt);
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
                cGate* outGate = getNextGate(this, innerPkt);
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
    else if (getParentModule()->par("completeCache").boolValue() == true) {
        return getVideoBitSize(customID);
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

void Logic::setupFlowFrom(User *user) {
    double minDatarate = DBL_MAX; // bit/ss
    // walk from user to this, finding smallest available BW
    cTopology::Node *node = topo.getNodeFor(user);
    if (node == NULL) {
        ev << "We (" << getFullPath() << ") are not included in the topology.\n";
    }
    else if (node->getNumPaths()==0) {
        ev << "No path to destination.\n";
    }
    else {
        while (node != topo.getTargetNode()) {
        //EV << node->getModuleId() << endl;
        //EV << node->getModule()->getFullName() << endl;
            ev << "We are in " << node->getModule()->getFullPath() << endl;
            ev << node->getDistanceToTarget() << " hops to go\n";
            ev << "There are " << node->getNumPaths()
               << " equally good directions, taking the first one\n";
            cTopology::LinkOut *path = node->getPath(0);
            ev << "Taking gate " << path->getLocalGate()->getFullName()
               << " we arrive in " << path->getRemoteNode()->getModule()->getFullPath()
               << " on its gate " << path->getRemoteGate()->getFullName() << endl;

            //EV << path->getLocalGate()->findTransmissionChannel()->info() << endl;//isTransmissionChannel() << endl;
            if (path->getLocalGate()->findTransmissionChannel()) {
                double nextDatarate = path->getLocalGate()->getTransmissionChannel()->getNominalDatarate();
                minDatarate = std::min(minDatarate, nextDatarate);
            }
            else {
                EV << "Not a datarate channel\n";
            }
            node = path->getRemoteNode();
        }
    }
    EV << "min rate is " << minDatarate << endl;
}
