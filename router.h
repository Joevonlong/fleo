#include <omnetpp.h>

class Router : public cSimpleModule
{
protected:
  simsignal_t requestSignal;
  cPacketQueue *queue;
};

