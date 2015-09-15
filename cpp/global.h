#pragma once
#include <omnetpp.h>

extern simsignal_t idleSignal;
extern simsignal_t requestSignal;
extern simsignal_t videoLengthSignal;

extern int requestKind;
extern int replyKind;
extern int64_t headerBitLength;

// MyPacket states
extern short stateStart;
//extern short stateEnd;
extern short stateTransfer;
//extern short stateAck;
extern short stateStream;
extern short flowStart;

extern std::vector<int> cacheIDs;
extern std::vector<int> completeCacheIDs;
extern std::map<std::string, int> locCaches;

class Global : public cSimpleModule
{
public:
    void recordUserD2C(double distToCache);
    void recordIdleTime(simtime_t t);
    void recordRequestedLength(double len);
    void recordPriority(int p, bool add);
    void recordFlowSuccess(bool successful);
    void recordCacheHit(bool hit);
    void recordNetLoad(double delta);
    void recordStartupDelay(simtime_t delay);
    void recordStartupDelayL20(simtime_t delay);
    void recordHops(short hops);
    void recordUnderflow();
    long getBufferBlock();
    long getBufferMin();
protected:
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    cOutVector userD2CVec; cDoubleHistogram userD2CHist;
    cOutVector idleTimeVec; cDoubleHistogram idleTimeHist;
    cOutVector requestedLengthVec; cDoubleHistogram requestedLengthHist;
    cOutVector highPriorityVec; double highP;
    cOutVector medPriorityVec; double medP;
    cOutVector lowPriorityVec; double lowP;
    cOutVector flowSuccessVec;
    cOutVector cacheHitVec;
    cOutVector netLoadVec; double currentNetLoad;
    cOutVector startupDelayVec; cDoubleHistogram startupDelayHist;
    cOutVector startupDelayL20Vec; cDoubleHistogram startupDelayL20Hist;
    cOutVector hopsVec; cDoubleHistogram hopsHist;
    cOutVector underflowVec;
    virtual void finish();
private:
    void loadAllLocs();
    void printCacheLocs();
    void buildCacheVector();
    long bufferBlock;
    long bufferMin;
};
