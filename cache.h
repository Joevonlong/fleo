#include <omnetpp.h>

class Cache : public cSimpleModule
{
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    std::map<int, bool> cached; // maps custom video ID to cache status
};

