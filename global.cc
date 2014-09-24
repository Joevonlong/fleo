#include "global.h"
#include "routing.h"
#include "parse.h"

Define_Module(Global);

const int defaultLoc = -1;

simsignal_t idleSignal;
simsignal_t videoLengthSignal;
simsignal_t requestSignal;
simsignal_t completionTimeSignal;
simsignal_t effBitRateSignal;

int requestKind = 123;
int replyKind = 321;
int64_t headerBitLength = 100;

// MyPacket states
short stateStart = 0;
short stateEnd = 1;
short stateTransfer = 2;
short stateAck = 3;

std::vector<int> cacheIDs;
std::vector<int> completeCacheIDs;
std::map<std::string, int> locCaches;

int Global::numInitStages () const {return 3;}

void Global::initialize(int stage)
{
    if (stage == 0) {
        idleSignal = registerSignal("idle"); // name assigned to signal ID
        requestSignal = registerSignal("request"); // name assigned to signal ID
        videoLengthSignal = registerSignal("videoLength");
        completionTimeSignal = registerSignal("completionTime");
        completionTimeGlobalSignal = registerSignal("completionTimeGlobal");;
        effBitRateSignal = registerSignal("effectiveBitRate");
        topoSetup();
        loadVideoLengthFile();
        EV << static_cast<double>(UINT64_MAX) << endl;
        //loadAllLocs();
    }
    else if (stage == 1) {
    }
    else if (stage == 2) {
        printCacheLocs();
        buildCacheVector();
    }
}

// insert all locations as keys into map with value -1
void Global::loadAllLocs() {
    for (cModule::SubmoduleIterator i(getParentModule()); !i.end(); i++) {
        cModule *subModule = i();
        if (subModule->hasPar("loc")) {
            locCaches[subModule->par("loc").stringValue()] = defaultLoc;
        }
    }
}

void Global::printCacheLocs() {
    EV << "locCaches.size() is " << locCaches.size() << endl;
    for (std::map<std::string, int>::iterator it=locCaches.begin();
        it!=locCaches.end(); it++) {
        EV << it->first << " => " << it->second << endl;
    }
}

void Global::buildCacheVector() {
    for (std::map<std::string, int>::iterator it=locCaches.begin(); it!=locCaches.end(); it++) {
        if (it->second != defaultLoc){
            cacheIDs.push_back(it->second);
        }
    }
}

void Global::recordCompletionTimeGlobal(simtime_t time) {
    emit(completionTimeGlobalSignal, time);
}

