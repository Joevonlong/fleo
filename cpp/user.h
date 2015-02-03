#pragma once
#include <omnetpp.h>
#include "global.h"
//#include "logic.h"
#include "path.h"

class User : public cSimpleModule
{
public:
    virtual ~User();
protected:
    cMessage* idleTimer;
    simtime_t idleTime;
    void idle();
    void idle(simtime_t t);
    short cacheTries;
    simtime_t playbackStart;
    simtime_t playbackTimeDownloaded;
    void startPlayback(simtime_t remaining);
    // also record request start - playback start time
    cMessage* underflowTimer;
    cOutVector underflowVector;
    
    void sendRequest();
    //void endRequest(MyPacket *pkt);
    simtime_t requestStartTime;
    bool playingBack;
    simtime_t playBackStart;
    uint64_t requestingBits;
    int nearestCache;
    Global *global;
    cDoubleHistogram requestHistogram;
//    cOutVector completionVector;
    cDoubleHistogram completionHistogram;
    cOutVector lagVector;
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    //void setupFlowTo(int destID);
    virtual void finish();
    
    // for flow-based
    std::map<cMessage*, Flow*> flowMap;
};

