#pragma once
#include <omnetpp.h>

extern simsignal_t idleSignal;
extern simsignal_t requestSignal;

extern int requestKind;
extern int replyKind;

extern std::map<std::string, std::string> locCaches;

class Global : public cSimpleModule
{
public:
    int numInitStages () const;
protected:
    virtual void initialize(int stage);
private:
    void loadCacheLocs();
};

