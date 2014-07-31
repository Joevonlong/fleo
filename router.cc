#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"
#include "reply_m.h"

class Router : public cSimpleModule
{
protected:
  simsignal_t requestSignal;
  cPacketQueue *queue;
};

Define_Module(Router);
