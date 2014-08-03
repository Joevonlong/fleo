#include <omnetpp.h>

class Logic : public cSimpleModule
{
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

