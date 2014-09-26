#pragma once
#include <omnetpp.h>

extern simsignal_t idleSignal;
extern simsignal_t requestSignal;
extern simsignal_t videoLengthSignal;
extern simsignal_t completionTimeSignal;
extern simsignal_t effBitRateSignal;

extern int requestKind;
extern int replyKind;
extern int64_t headerBitLength;

// MyPacket states
extern short stateStart;
extern short stateEnd;
extern short stateTransfer;
extern short stateAck;
extern short stateStream;

extern std::vector<int> cacheIDs;
extern std::vector<int> completeCacheIDs;
extern std::map<std::string, int> locCaches;

class Global : public cSimpleModule
{
public:
    void recordCompletionTimeGlobal(simtime_t time);
    void recordEffBitRateGlobal(double d);
    long getBufferBlock();
    long getBufferMin();
protected:
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    simsignal_t completionTimeGlobalSignal;
    simsignal_t effBitRateGlobalSignal;
    cDoubleHistogram effBitRateGlobalHist;
    virtual void finish();
private:
    void loadAllLocs();
    void printCacheLocs();
    void buildCacheVector();
    long bufferBlock;
    long bufferMin;
};

