#include <omnetpp.h>
#include "router.h"

class InternalRouter : public Router
{
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};
class Core : public InternalRouter{};
class PoP : public InternalRouter{};

