#include <omnetpp.h>

class Cache : public cSimpleModule
{
public:
    bool isCached(int customID);
    void setCached(int customID, bool b);
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    std::map<int, bool> cached; // maps custom video ID to cache status
    uint64_t cacheSize;
    uint64_t diskUsed;
    cQueue *cacheOrder;
};

