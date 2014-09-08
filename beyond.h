#include "logic.h"

class BeyondLogic : public Logic
{
protected:
    int numInitStages() const;
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
};

