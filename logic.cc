#include "cache.h"
#include "logic.h"
#include "parse.h"

Define_Module(Logic);

int64_t Logic::checkCache(int customID) {
    Cache* cache = (Cache*)(getParentModule()->getSubmodule("cache"));
    if (cache == NULL) {
        return -1; // module has no cache
    }
    else {
        if (cache->isCached(customID)) {
            return getVideoBitSize(customID); // should be video's bitsize
        }
        else {
            return 0; // requested item is not cached
        }
    }
}

