#include "cache.h"
#include "logic.h"
#include "parse.h"
#include "global.h"

Define_Module(Logic);

void Logic::registerSelfIfCache() {
//    if (hasPar("hasCache")) {
        if (getParentModule()->par("hasCache").boolValue() == true) {
            locCaches[getParentModule()->par("loc").stringValue()] = getFullPath();
        }
  //  }
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

