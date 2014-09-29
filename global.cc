#include "global.h"
#include "routing.h"
#include "parse.h"

Define_Module(Global);

const int defaultLoc = -1;

simsignal_t idleSignal;
simsignal_t requestSignal;
simsignal_t videoLengthSignal;
simsignal_t startupDelaySignal;
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
short stateStream = 4;

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

        // user idle time between videos
        idleTimeVec.setName("idle time vector");
        idleTimeHist.setName("idle time histogram");
        idleTimeHist.setRangeAutoUpper(0, 10000);
        idleTimeHist.setNumCells(1000);

        // video lengths
        requestedLengthVec.setName("video length vector");
        requestedLengthHist.setName("video length histogram");
        requestedLengthHist.setRangeAutoUpper(0, 1000);
        requestedLengthHist.setNumCells(1000);

        // startup delays
        //startupDelaySignal = registerSignal("startup delay");
        startupDelayVec.setName("startup delay vector");
        startupDelayHist.setName("startup delay histogram");
        startupDelayHist.setRangeAutoUpper(0);
        startupDelayHist.setNumCells(200);

        // underflows (buffer runs out)
        underflowVec.setName("underflow vector");

        bufferBlock = par("bufferBlock").longValue();
        bufferMin = par("bufferMin").longValue();

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

long Global::getBufferBlock() {
    return bufferBlock;
}
long Global::getBufferMin() {
    return bufferMin;
}

void Global::recordIdleTime(simtime_t t) {
    idleTimeVec.record(t);
    idleTimeHist.collect(t);
}

void Global::recordRequestedLength(double len) {
    requestedLengthVec.record(len);
    requestedLengthHist.collect(len);
}

void Global::recordStartupDelay(simtime_t delay) {
    startupDelayVec.record(delay);
    startupDelayHist.collect(delay);
}

void Global::recordUnderflow() {
    underflowVec.record(1); // only want to record timestamp
}

void Global::finish() {
    idleTimeHist.record();
    requestedLengthHist.record();
    startupDelayHist.record();
}

