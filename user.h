#include <omnetpp.h>

class User : public cSimpleModule
{
public:
  virtual ~User();
private:
  simsignal_t idleSignal;
  cMessage* idleTimer;
  void idle();
  void sendRequest();
  uint64_t requestingBits;
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

