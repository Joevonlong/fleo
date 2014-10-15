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

extern std::vector<int> cacheIDs;
extern std::vector<int> completeCacheIDs;
extern std::map<std::string, int> locCaches;

class Global : public cSimpleModule
{
public:
    void recordIdleTime(simtime_t t);
    void recordRequestedLength(double len);
    void recordStartupDelay(simtime_t delay);
    void recordUnderflow();
    long getBufferBlock();
    long getBufferMin();
protected:
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    cOutVector idleTimeVec;
    cDoubleHistogram idleTimeHist;
    cOutVector requestedLengthVec;
    cDoubleHistogram requestedLengthHist;
    cOutVector startupDelayVec;
    cDoubleHistogram startupDelayHist;
    cOutVector underflowVec;
    virtual void finish();
private:
    void loadAllLocs();
    void printCacheLocs();
    void buildCacheVector();
    long bufferBlock;
    long bufferMin;
};
