#include "logic.h"
#include "global.h"
#include "parse.h"
#include "request_m.h"
#include "reply_m.h"
#include "routing.h"
#include "cache.h"

Define_Module(Logic);

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
                cache->setCached(maxID, true);
            }
        }
    }
    else if (stage == 3) {
        if (getParentModule()->par("hasCache").boolValue() == true) {
            nearestCache = getNearestCacheID(getId());
            EV << "Secondary cache for " << getFullPath() << "(" << getParentModule()->par("loc").stringValue() << ") is " << simulation.getModule(nearestCache)->getFullPath() << "(" << simulation.getModule(nearestCache)->getParentModule()->par("loc").stringValue() << ")." << endl;
            if (getParentModule()->par("completeCache").boolValue() == false) {
                nearestCompleteCache = getNearestID(getId(), completeCacheIDs);
                EV << "Master cache for " << getFullPath() << "(" << getParentModule()->par("loc").stringValue() << ") is " << simulation.getModule(nearestCompleteCache)->getFullPath() << "(" << simulation.getModule(nearestCompleteCache)->getParentModule()->par("loc").stringValue() << ")." << endl;
            }
            else {
                nearestCompleteCache = getId();
            }
        }
    }
}

void Logic::handleMessage(cMessage *msg) {
    if (msg->getKind() == requestKind) {
        Request *req = check_and_cast<Request*>(msg);
        if (req->getDestinationID() == getId()) { // reached destination
            int64_t vidBitSize = checkCache(req->getCustomID());
            EV << "received request sent at " << req->getCreationTime() << endl;
            // construct reply
            Reply *reply = new Reply("reply", replyKind);
            EV << "reply size: " << vidBitSize << endl;
            reply->setBitLength(vidBitSize);
            reply->setSourceID(getId());
            reply->setDestinationID(req->getSourceID());
            // send reply
            cGate* outGate = getNextGate(this, (cMessage*)reply);
            send(reply, outGate);
            EV << "Sending reply out of " << outGate->getFullName() << endl;
            // cleanup
            delete msg;
        }
        else { // forward request
            cGate* outGate = getNextGate(this, msg);
            send(req, outGate);
            EV << "Forwarding request via " << outGate->getFullName() << endl;
        }
    }
    else if (msg->getKind() == replyKind) { // forward reply
        Reply *reply = check_and_cast<Reply*>(msg);
        cGate* outGate = getNextGate(this, msg);
        send(reply, outGate);
        EV << "Forwarding reply via " << outGate->getFullName() << endl;
    }
    else {
        EV << "Error in Logic::handleMessage : unknown message kind\n";
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
        return -1; // module has no cache
    }
    else if (((Cache*)(getParentModule()->getSubmodule("cache")))->isCached(customID)) {
        return getVideoBitSize(customID); // should be video's bitsize
    }
    else {
        return 0; // requested item is not cached
    }
}

