#include "request_m.h"
#include "global.h"
#include "cache.h"
#include "parse.h"

Define_Module(Cache);
// record request statistics

void Cache::initialize() {
    cacheSize = pow(10, 3*4) * 8; // 1TB
    diskUsed = 0;
    std::map<int, std::deque<int>::iterator> lruMap;
    std::deque<int> cacheOrder; // = new cQueue("cache insert order"); // for LRU replacement
}

void Cache::handleMessage(cMessage* msg) {
}

bool Cache::isCached(int customID) {
    if (getParentModule()->par("completeCache").boolValue() == true) {
        return true;
    }
    if (cached[customID]) { // key added if it doesn't exist, and maps to false.
        // refresh item access to front of cacheOrder
//        cacheOrder.erase(lruMap[customID]); // bug line
//        cacheOrder.push_back(customID);
//        lruMap[customID] = cacheOrder.end()-1; // -1 for last element
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
        error("item larger than cache.");
        return;
    }
    cached[customID] = b;
    if (b == true) {
        diskUsed += getVideoBitSize(customID);
//        cacheOrder.push_back(customID);
//        lruMap[customID] = cacheOrder.end()-1; // -1 for last element
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
            int evicted = cacheOrder.front();
            cacheOrder.pop_front();
            diskUsed -=getVideoBitSize(evicted);
            EV << "Evicted item #" << evicted
               << " of size " << getVideoBitSize(evicted) << endl;
            // add eviction statistics?
        }
    }
}

