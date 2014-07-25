#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"

class Beyond : public cSimpleModule
{
protected:
  virtual void handleMessage(cMessage *msg);
};

class Core : public cSimpleModule
{
protected:
  virtual void handleMessage(cMessage *msg);
};

class PoP : public cSimpleModule
{
protected:
  virtual void handleMessage(cMessage *msg);
};

class User : public cSimpleModule
{
protected:
  virtual void initialize();
};

Define_Module(Beyond);
Define_Module(Core);
Define_Module(PoP);
Define_Module(User);

void User::initialize()
{
  cPacket *pkt = new cPacket("test", 65535, 1);
  send(pkt, "gate$o");
}

void PoP::handleMessage(cMessage *msg)
{
  // forward to first gate (which should be towards the beyond)
  cChannel *upstream =
    gate("gate$o", 0)->
    getTransmissionChannel();
  EV << getFullName();
  if (upstream->isBusy()) {
    EV << "Busy. Scheduled for " << upstream->getTransmissionFinishTime() << "s\n";
    scheduleAt(upstream->getTransmissionFinishTime(), msg);
  }
  else {
    EV << "Upstream free. Forwarding now.\n";
    send(msg, "gate$o", 0);
  }
}

void Core::handleMessage(cMessage *msg)
{
  // forward to first gate (which should be towards the beyond)
  cChannel *upstream =
    gate("gate$o", 0)->
    getTransmissionChannel();
  EV << getFullName();
  if (upstream->isBusy()) {
    EV << "Busy. Scheduled for " << upstream->getTransmissionFinishTime() << "s\n";
    scheduleAt(upstream->getTransmissionFinishTime(), msg);
  }
  else {
    EV << "Upstream free. Forwarding now.\n";
    send(msg, "gate$o", 0);
  }
}

void Beyond::handleMessage(cMessage *msg)
{
  delete msg;
}
