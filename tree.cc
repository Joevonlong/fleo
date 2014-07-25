#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"

class Beyond : public cSimpleModule
{
private:
  simsignal_t requestSignal;
protected:
  virtual void handleMessage(cMessage *msg);
};

class Core : public cSimpleModule
{
private:
  simsignal_t requestSignal;
protected:
  virtual void handleMessage(cMessage *msg);
};

class PoP : public cSimpleModule
{
private:
  simsignal_t requestSignal;
protected:
  virtual void handleMessage(cMessage *msg);
};

class User : public cSimpleModule
{
private:
  simsignal_t requestSignal;
protected:
  virtual void initialize();
};

Define_Module(Beyond);
Define_Module(Core);
Define_Module(PoP);
Define_Module(User);

void User::initialize()
{
  requestSignal = registerSignal("request"); // name assigned to signal ID
  // first request
  Request *req = new Request("test", 0);
  req->setSize(intuniform(1, 1<<31));
  EV << "Size: " << req->getSize() << endl;
  req->setBitLength(1);
  EV << "BitLength: " << req->getBitLength() << endl;
  send(req, "gate$o");
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
  Request *req = check_and_cast<Request*>(msg);
  unsigned int size = req->getSize();
  EV << "Received request for " << size << "b\n";
  emit(requestSignal, size);
  delete msg;
}
