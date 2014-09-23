#include "request_m.h"
#include "global.h"
#include "cache.h"
#include "parse.h"

Define_Module(Cache);
// record request statistics
void Cache::initialize() {
    cacheSize = pow(10, 2+3*4) * 8; // 100TB
    diskUsed = 0;
    cacheOrder = new cQueue("cache insert order"); // for LRU replacement
}

void Cache::handleMessage(cMessage* msg) {
    if (msg->getKind() == requestKind) {
        Request *req = check_and_cast<Request*>(msg);
        if (cached[req->getCustomID()]) {
            // servefrom cache
        }
        else {
            // not in cache:
            // fetch from upstream, or redirect.
        }
    }
}

bool Cache::isCached(int customID) {
    if (cached[customID]) {
        return true;
    }
    else {
        return false;
    }
}

void Cache::setCached(int customID, bool b) {
    cached[customID] = b;
    if (b == true) {
        diskUsed += getVideoBitSize(customID);
        cacheOrder->insert((cObject*)customID);
    }
    else {
        diskUsed -= getVideoBitSize(customID);
        cacheOrder->remove((cObject*)customID);
    }
    // check if full
    if (diskUsed > cacheSize) {
        // assume LRU replacement
        
    }
}

