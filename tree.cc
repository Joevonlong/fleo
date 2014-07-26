#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"

class Beyond : public cSimpleModule
{
private:
  simsignal_t requestSignal;
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

class Core : public cSimpleModule
{
private:
  simsignal_t requestSignal;
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

class PoP : public cSimpleModule
{
private:
  simsignal_t requestSignal;
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

class User : public cSimpleModule
{
public:
  virtual ~User();
private:
  simsignal_t idleSignal;
  cMessage* idleTimer;
  virtual void idle();
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

Define_Module(Beyond);
Define_Module(Core);
Define_Module(PoP);
Define_Module(User);

User::~User()
{
  cancelAndDelete(idleTimer);
}

void User::initialize()
{
  idleSignal = registerSignal("idle"); // name assigned to signal ID
  idleTimer = new cMessage("idle timer");
  idle();
}

void User::idle()
{
  simtime_t idleTime = par("idleTime"); // changed via ini
  emit(idleSignal, idleTime);
  EV << getFullName() <<"idling for " << idleTime << "s\n";
  scheduleAt(simTime()+idleTime, idleTimer);
}

void User::handleMessage(cMessage *msg)
{
  // assume is always idleTimer for now.
  // send request
  Request *req = new Request("request", 0);
  //double requestSize = par("requestSize");
  req->setSize(par("requestSize"));
  //req->setSize(intuniform(1, 1<<31));
  EV << "Size: " << req->getSize() << endl;
  req->setBitLength(1);
  EV << "BitLength: " << req->getBitLength() << endl;
  send(req, "gate$o");
  // idle for a bit
  idle();
}

void PoP::initialize()
{
  requestSignal = registerSignal("request"); // name assigned to signal ID
}

void PoP::handleMessage(cMessage *msg)
{
  // logging
  Request *req = check_and_cast<Request*>(msg);
  unsigned int size = req->getSize();
  emit(requestSignal, size);
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

void Core::initialize()
{
  requestSignal = registerSignal("request"); // name assigned to signal ID
}

void Core::handleMessage(cMessage *msg)
{
  // logging
  Request *req = check_and_cast<Request*>(msg);
  unsigned int size = req->getSize();
  emit(requestSignal, size);
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

void Beyond::initialize()
{
  requestSignal = registerSignal("request"); // name assigned to signal ID
}

void Beyond::handleMessage(cMessage *msg)
{
  Request *req = check_and_cast<Request*>(msg);
  unsigned int size = req->getSize();
  EV << "Received request for " << size << "b\n";
  emit(requestSignal, size);
  delete msg;
}
