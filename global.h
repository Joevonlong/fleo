#pragma once
#include <omnetpp.h>

extern simsignal_t idleSignal;
extern simsignal_t requestSignal;

extern int requestKind;
extern int replyKind;

extern std::vector<int> cacheIDs;
extern std::vector<int> completeCacheIDs;
extern std::map<std::string, int> locCaches;

class Global : public cSimpleModule
{
protected:
    virtual int numInitStages() const;
    virtual void initialize(int stage);
private:
    void loadAllLocs();
    void printCacheLocs();
    void buildCacheVector();
};

