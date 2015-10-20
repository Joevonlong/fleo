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
    if (!getParentModule()->par("hasCache").boolValue()) {
        par("capacity").setDoubleValue(noCache);
    }
    capacity = par("capacity").doubleValue();
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

/**
 * Assumes presence in mapping means cached.
 * Does not affect LRU queue.
 */
bool Cache::isCached(int customID) {
    if (capacity == origin) {return true;}
    if (capacity == noCache) {error("checking content in node without cache");}
    if (idToIndex.count(customID)) {return true;}
    else {return false;}
}

void Cache::setCached(int customID, bool b) {
    if (capacity == origin) {return; error("setCached used on origin server");}
    if (capacity == noCache) {error("setCached used on node without cache");}
    if (getVideoBitSize(customID) > capacity) {
        EV << "Item #" << customID << " of size " << getVideoBitSize(customID)
           << " is larger than cache (" << capacity << "). Refusing.\n";
        return;
        error("single item larger than cache.");
    }
    if (b == true) {
        std::map<int, std::list<int>::iterator>::iterator it = idToIndex.find(customID); // find customID in mapping
        if (it != idToIndex.end()) { // found: move to back of LRU
            leastRecent.erase(it->second);
            leastRecent.push_back(customID);
            // and then update mapping
            idToIndex[customID] = leastRecent.end();
            --idToIndex[customID]; // end-1
        }
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
