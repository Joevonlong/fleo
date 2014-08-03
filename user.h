#include <omnetpp.h>

class User : public cSimpleModule
{
public:
  virtual ~User();
private:
  simsignal_t idleSignal;
  cMessage* idleTimer;
  virtual void idle();
  cPacketQueue *queue;
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

