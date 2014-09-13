#include "logic.h"
#include "global.h"
#include "parse.h"
#include "request_m.h"
#include "reply_m.h"
#include "routing.h"
#include "cache.h"

Define_Module(Logic);

int Logic::numInitStages() const {
    return 2;
}

void Logic::initialize(int stage) {
    if (stage == 0) {
    }
    else if (stage == 1) {
        registerSelfIfCache();
        // populate cache with all content
        Cache* cache = (Cache*)(getParentModule()->getSubmodule("cache"));
        for (unsigned long maxID = getMaxCustomVideoID(); maxID != ULONG_MAX; maxID--) {
            cache->setCached(maxID, true);
        }
    }
    else {
        EV << "Error in Logic::initialize\n";
    }
}

void Logic::handleMessage(cMessage *msg) {
    if (msg->getKind() == requestKind) {
        Request *req = check_and_cast<Request*>(msg);
        if (req->getDestinationID() == getId()) { // reached destination
            int64_t vidBitSize = checkCache(req->getCustomID());
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

void Logic::registerSelfIfCache() {
    if (getParentModule()->par("hasCache").boolValue() == true) {
        locCaches[getParentModule()->par("loc").stringValue()] = getId();
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

