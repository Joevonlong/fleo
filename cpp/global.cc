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
int64_t headerBitLength = 0;

// MyPacket states
short stateStart = 0;
//short stateEnd = 1;
short stateTransfer = 2;
//short stateAck = 3;
short stateStream = 4;
short flowStart = 5;

std::vector<int> cacheIDs;
std::vector<int> completeCacheIDs;
std::vector<int> incompleteCacheIDs;
std::map<std::string, int> locCaches;

int Global::numInitStages () const {return 3;}

void Global::initialize(int stage)
{
    if (stage == 0) {
        idleSignal = registerSignal("idle"); // name assigned to signal ID
        requestSignal = registerSignal("request"); // name assigned to signal ID
        videoLengthSignal = registerSignal("videoLength");

        // distance from each user to its nearest cache
        userD2CVec.setName("distance to cache vector");
        userD2CHist.setName("distance to cache histogram");
        userD2CHist.setRangeAutoUpper(0, 10000, 1);
        userD2CHist.setNumCells(100);

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

        // flow priorities
        highPriorityVec.setName("high priority flow vector");
        highP = 0;
        medPriorityVec.setName("med priority flow vector");
        medP = 0;
        lowPriorityVec.setName("low priority flow vector");
        lowP = 0;

        // whether at least one candidate path had bandwidth available for a flow
        flowSuccessVec.setName("flow setup success vector");

        // whether content was already cached at replica when requested
        cacheHitVec.setName("cache hit vector");

        // whether content was already cached at replica when requested
        netLoadVec.setName("total bps in use throughout network");
        currentNetLoad = 0;

        // startup delays
        //startupDelaySignal = registerSignal("startup delay");
        startupDelayVec.setName("startup delay vector");
        startupDelayHist.setName("startup delay histogram");
        startupDelayHist.setRangeAutoUpper(0);
        startupDelayHist.setNumCells(200);

        // startup delays for vids shorter than 20s
        //startupDelaySignal = registerSignal("startup delay");
        startupDelayL20Vec.setName("startup delay for short videos vector");
        startupDelayL20Hist.setName("startup delay for short videos histogram");
        startupDelayL20Hist.setRangeAutoUpper(0);
        startupDelayL20Hist.setNumCells(200);

        // hops to cache hit
        hopsVec.setName("hops vector");
        hopsHist.setName("hops histogram");
        hopsHist.setRangeAutoUpper(0);
        hopsHist.setNumCells(20);

        // underflows (buffer runs out)
        underflowVec.setName("underflow vector");

        bufferBlock = par("bufferBlock").longValue();
        bufferMin = par("bufferMin").longValue();

        topoSetup();
        loadVideoLengthFile();
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

void Global::recordUserD2C(double d) {
    userD2CVec.record(d);
    userD2CHist.collect(d);
}

void Global::recordIdleTime(simtime_t t) {
    idleTimeVec.record(t);
    idleTimeHist.collect(t);
}

void Global::recordRequestedLength(double len) {
    requestedLengthVec.record(len);
    requestedLengthHist.collect(len);
}

void Global::recordPriority(int p, bool add) {
    if (p==3) {
        if (add) {highPriorityVec.record(++highP);}
        else {highPriorityVec.record(--highP);}
    }
    else if (p==2) {
        if (add) {medPriorityVec.record(++medP);}
        else {medPriorityVec.record(--medP);}
    }
    else if (p==1) {
        if (add) {lowPriorityVec.record(++lowP);}
        else {lowPriorityVec.record(--lowP);}
    }
    else {
        EV << "Global::recordPriority: unknown priority: " << p << endl;
        //error("Global::recordPriority: unknown priority");
    }
}

void Global::recordFlowSuccess(bool successful) {
    flowSuccessVec.record(successful);
}

void Global::recordCacheHit(bool hit) {
    cacheHitVec.record(hit);
}

void Global::recordNetLoad(double delta) {
    currentNetLoad += delta;
    netLoadVec.record(currentNetLoad);
}

void Global::recordStartupDelay(simtime_t delay) {
    startupDelayVec.record(delay);
    startupDelayHist.collect(delay);
}

void Global::recordStartupDelayL20(simtime_t delay) {
    startupDelayL20Vec.record(delay);
    startupDelayL20Hist.collect(delay);
}

void Global::recordHops(short hops) {
    hopsVec.record(hops);
    hopsHist.collect(hops);
}

void Global::recordUnderflow() {
    underflowVec.record(1); // only want to record timestamp
}

void Global::finish() {
    userD2CHist.record();
    idleTimeHist.record();
    requestedLengthHist.record();
    startupDelayHist.record();
    startupDelayL20Hist.record();
    hopsHist.record();
}
