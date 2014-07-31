#include <omnetpp.h>

class Buffer : public cSimpleModule
{
private:
  int fromLogicID;
  int toLogicID;
  int receiveID;
  int transmitID;
  cMessage* transmitDone;
protected:
  cPacketQueue* queue;
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

