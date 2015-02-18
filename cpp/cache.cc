#include "request_m.h"
#include "global.h"
#include "cache.h"
#include "parse.h"

Define_Module(Cache);
// record request statistics

void Cache::initialize() {
    cacheSize = par("capacity").doubleValue() * pow(2,30); //pow(10, 3*4) * 12;
    diskUsed = 0;
    std::map<int, std::list<int>::iterator> idToIndex;
    std::list<int> leastRecent; // = new cQueue("cache insert order"); // for LRU replacement
}

void Cache::handleMessage(cMessage* msg) {
}

bool Cache::isCached(int customID) {
    if (getParentModule()->par("completeCache").boolValue() == true) {
        return true;
    }
    if (cached[customID]) { // key added if it doesn't exist, and maps to false.
        // refresh item access to front of cacheOrder
        leastRecent.erase(idToIndex[customID]); // bug line
        leastRecent.push_back(customID);
        idToIndex[customID] = leastRecent.end();
        --idToIndex[customID]; // -- for last element
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
        leastRecent.push_back(customID);
        idToIndex[customID] = leastRecent.end();
        --idToIndex[customID]; // -- for last element
        if (!force) {
            EV << "Cached item #" << customID
               << " of size " << getVideoBitSize(customID) << endl;
       }
    }
    else {
        error("setCached(false) not yet implemented.");
//        diskUsed -= getVideoBitSize(customID);
//        cacheOrder->remove((cObject*)customID);
//        EV << "Evicted item #" << customID
//           << " of size " << getVideoBitSize(customID) << endl;
    }
    // check if full
    if (force == false) {
        while (diskUsed > cacheSize) {
            EV << "Cache full. ";
            // assume LRU replacement
            int evicted = leastRecent.front();
            leastRecent.pop_front();
            diskUsed -=getVideoBitSize(evicted);
            cached[evicted] = false;
            EV << "Evicted item #" << evicted
               << " of size " << getVideoBitSize(evicted) << endl;
            // add eviction statistics?
        }
    } // else force insert ignoring space limit
}

