#include <omnetpp.h>
#include "logic.h"

class InternalLogic : public Logic
{
protected:
    int numInitStages() const;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
};
class CoreLogic : public InternalLogic{};
class PoPLogic : public InternalLogic{};

