#include <omnetpp.h>
#include <deque>

class Cache : public cSimpleModule
{
public:
    bool isCached(int customID);
    void setCached(int customID, bool b);
    void setCached(int customID, bool b, bool force);
    uint64_t diskUsed;
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    std::map<int, bool> cached; // maps custom video ID to cache status
    uint64_t cacheSize;
    std::deque<uint64_t> cacheOrder;
};

