#include "request_m.h"
#include "global.h"
#include "cache.h"
#include "parse.h"

Define_Module(Cache);
// record request statistics
void Cache::initialize() {
    cacheSize = pow(10, 3*4) * 8; // 1TB
    diskUsed = 0;
    std::queue<uint64_t> cacheOrder; // = new cQueue("cache insert order"); // for LRU replacement
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
    Cache::setCached(customID, b, false);
}

void Cache::setCached(int customID, bool b, bool force) {
    if (getVideoBitSize(customID) > cacheSize) {
        EV << "Item #" << customID << " larger than cache. Refusing.\n";
        return;
    }
    cached[customID] = b;
    if (b == true) {
        diskUsed += getVideoBitSize(customID);
        cacheOrder.push(customID);
        //EV << "Cached item #" << customID
        //   << " of size " << getVideoBitSize(customID) << endl;
    }
    else {
        error("not yet implemented.");
//        diskUsed -= getVideoBitSize(customID);
//        cacheOrder->remove((cObject*)customID);
//        EV << "Evicted item #" << customID
//           << " of size " << getVideoBitSize(customID) << endl;
    }
    // check if full
    if (force == false) {
        while (diskUsed > cacheSize) {
            error("cache full.");
            EV << "Cache full. ";
            // assume LRU replacement
            uint64_t evicted = cacheOrder.front();
            cacheOrder.pop();
            diskUsed -=getVideoBitSize(evicted);
            EV << "Evicted item #" << evicted
               << " of size " << getVideoBitSize(evicted) << endl;
            // add eviction statistics?
        }
    }
}

