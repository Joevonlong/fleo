#include "request_m.h"
#include "cache.h"

Define_Module(Cache);
// record request statistics
void Cache::initialize() {
    //cached = std
}

void Cache::handleMessage(cMessage* msg) {
    if (msg->getKind() == 123) {
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

