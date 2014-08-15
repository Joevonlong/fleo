#include <omnetpp.h>
#include "global.h"

class User : public cSimpleModule
{
public:
  virtual ~User();
private:
  cMessage* idleTimer;
  void idle();
  void sendRequest();
  uint64_t requestingBits;
protected:
  cVarHistogram requestHistogram;
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
  virtual void finish();
};

