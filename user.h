#include <omnetpp.h>
#include "global.h"

class User : public cSimpleModule
{
public:
    virtual ~User();
private:
    cMessage* idleTimer;
    void idle();
    void sendRequest();
    uint64_t requestingBits;
    int nearestCache;
protected:
    cDoubleHistogram requestHistogram;
//    cOutVector completionVector;
    cDoubleHistogram completionHistogram;
    cOutVector lagVector;
    virtual int numInitStages() const;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    virtual void finish();
};

