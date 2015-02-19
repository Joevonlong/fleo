#include <omnetpp.h>
#include <deque>

class Cache : public cSimpleModule
{
public:
    bool hasCache();
    bool isOrigin();
    bool isCached(int customID);
    void setCached(int customID, bool b);
    void setCached(int customID, bool b, bool force);
protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    double capacity;
    double diskUsed;
    std::list<int> leastRecent; // push does not invalidate list iterators, unlike deque.
    std::map<int, std::list<int>::iterator> idToIndex; // maps video ID to LRU list's index (for speed)
    void push(int customID);
    void erase(int customID);
    void removeLeastRecent();
};
