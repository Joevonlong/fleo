#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"

class Beyond : public cSimpleModule
{
private:
  simsignal_t requestSignal;
  cTopology topo;
  void topoHelper();
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
  //if msg
  // assume is always idleTimer for now.
  // send request
  Request *req = new Request("request", getIndex()); // use user[] index as message kind
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
    // EV << "Busy. Scheduled for " << upstream->getTransmissionFinishTime() << "s\n";
    scheduleAt(upstream->getTransmissionFinishTime(), msg);
  }
  else {
    // EV << "Upstream free. Forwarding now.\n";
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
    // EV << "Busy. Scheduled for " << upstream->getTransmissionFinishTime() << "s\n";
    scheduleAt(upstream->getTransmissionFinishTime(), msg);
  }
  else {
    // EV << "Upstream free. Forwarding now.\n";
    send(msg, "gate$o", 0);
  }
}

void Beyond::initialize()
{
  EV << getFullPath() << endl;
  topoHelper();
  requestSignal = registerSignal("request"); // name assigned to signal ID
}

void Beyond::handleMessage(cMessage *msg)
{
  Request *req = check_and_cast<Request*>(msg);
  unsigned int size = req->getSize();
  EV << "Received request from user " << req->getKind() << " for " << size << "b\n"; // use kind to ID sender
  emit(requestSignal, size);

  // TODO reply with requested size.
  // DONE 1. identify sender. need info in request
  // 2. find route to sender (going down tree so routing needed)
  // 3. queueing messages
  // 4. add proper queues (cQueue) instead of cycling messages
  topoHelper();
  delete msg;
}

void Beyond::topoHelper()
{
  topo.clear();
  //topo.extractByModulePath(cStringTokenizer("*").asVector());
  //topo.extractByNedTypeName(cStringTokenizer("Beyond Core PoP User").asVector());
  topo.extractByProperty("display");
  //EV << "topo nodes: " << topo.getNumNodes() << endl;
for (int i=0; i<topo.getNumNodes(); i++)
{
  cTopology::Node *node = topo.getNode(i);
  ev << "Node i=" << i << " is " << node->getModule()->getFullPath() << endl;
  ev << " It has " << node->getNumOutLinks() << " conns to other nodes\n";
  ev << " and " << node->getNumInLinks() << " conns from other nodes\n";

  ev << " Connections to other modules are:\n";
  for (int j=0; j<node->getNumOutLinks(); j++)
  {
    cTopology::Node *neighbour = node->getLinkOut(j)->getRemoteNode();
    cGate *gate = node->getLinkOut(j)->getLocalGate();
    ev << " " << neighbour->getModule()->getFullPath()
       << " through gate " << gate->getFullName() << endl;
  }
}}
