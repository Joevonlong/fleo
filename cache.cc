#include "request_m.h"
#include "global.h"
#include "cache.h"

Define_Module(Cache);
// record request statistics
void Cache::initialize() {
    //cached = std
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
}

//virtual cModule * 	getParentModule () const
//cModule * 	getSubmodule (const char *submodname, int idx=-1)

