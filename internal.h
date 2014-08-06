#include <omnetpp.h>
#include "logic.h"

class InternalLogic : public Logic
{
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};
class CoreLogic : public InternalLogic{};
class PoPLogic : public InternalLogic{};

