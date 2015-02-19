#include <list>
#include "request_m.h"
#include "global.h"
#include "cache.h"
#include "parse.h"

Define_Module(Cache);
// record request statistics

const double noCache = 0;
const double origin = -1;

void Cache::initialize() {
    capacity = par("capacity").doubleValue(); //pow(10, 3*4) * 12;
    diskUsed = 0;
}

void Cache::handleMessage(cMessage* msg) {
}

bool Cache::hasCache() {
    return capacity != noCache;
}
bool Cache::isOrigin() {
    return capacity == origin;
}

bool Cache::isCached(int customID) {
    /**
     * Assumes presence in mapping means cached.
     * Will set item to most-recently-used.
     */
    if (capacity == origin) {return true;}
    if (capacity == noCache) {error("checking content in node without cache");}
    std::map<int, std::list<int>::iterator>::iterator it = idToIndex.find(customID); // find customID in mapping
    if (it != idToIndex.end()) {
        // move checked ID to the front of leastRecent
        leastRecent.erase(it->second);
        leastRecent.push_back(customID);
        // and then update mapping
        idToIndex[customID] = leastRecent.end();
        --idToIndex[customID]; // end-1
        return true;
    }
    else {return false;}
}

void Cache::setCached(int customID, bool b) {
    Cache::setCached(customID, b, false);
}

void Cache::setCached(int customID, bool b, bool force) {
    if (capacity == origin) {error("setCached used on origin server");}
    if (capacity == noCache) {error("setCached used on node without cache");}
    if (getVideoBitSize(customID) > capacity) {
        EV << "Item #" << customID << " larger than cache. Refusing.\n";
        error("single item larger than cache.");
        return;
    }
    if (b == true) {
        if (idToIndex.count(customID) == 1) {error("inserting item that is already cached");}
        else {push(customID);}
        while (diskUsed > capacity) {
            // assume LRU replacement
            EV << "Cache full. Evicting by LRU ";
            removeLeastRecent();
        }
    }
    else { // set false
        if (idToIndex.count(customID) == 0) {error("removing item that is not cached");}
        else {erase(customID);}
    }
}

void Cache::push(int customID) {
    /** assumes not already present */
    diskUsed += getVideoBitSize(customID);
    leastRecent.push_back(customID);
    idToIndex[customID] = leastRecent.end();
    --idToIndex[customID]; // end-1
    EV << "Cached item #" << customID
       << " of size " << getVideoBitSize(customID) << endl;
}
void Cache::erase(int customID) {
    /** assumes already present */
    diskUsed -= getVideoBitSize(customID);
    leastRecent.erase(idToIndex[customID]);
    idToIndex.erase(customID);
    EV << "Erased item #" << customID
       << " of size " << getVideoBitSize(customID) << endl;
}

void Cache::removeLeastRecent() {
    // should be faster than calling erase(front) because pop avoids referencing back and forth.
    int removed = leastRecent.front();
    leastRecent.pop_front();
    idToIndex.erase(removed);
    diskUsed -=getVideoBitSize(removed);
    EV << "Evicted item #" << removed
       << " of size " << getVideoBitSize(removed) << endl;
    // add eviction statistics?
}
