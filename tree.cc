#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"
#include "reply_m.h"

class Beyond : public cSimpleModule
{
private:
  simsignal_t requestSignal;
  //cTopology topo;
  void topoHelper();
  cPacketQueue *queue;
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

class Core : public cSimpleModule
{
private:
  simsignal_t requestSignal;
  cPacketQueue *queue;
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

class PoP : public cSimpleModule
{
private:
  simsignal_t requestSignal;
  cPacketQueue *queue;
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
  cPacketQueue *queue;
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

Define_Module(Beyond);
Define_Module(Core);
Define_Module(PoP);
Define_Module(User);

cTopology topo;
cGate* getNextGate(cSimpleModule* current, Reply* reply) {
  short userIndex = reply->getDestination();
//EV << "userindex " << userIndex << endl;
  char userPath[20];
  sprintf(userPath, "Tree.user[%d]", userIndex);
//EV << "userPath " << userPath << endl;
  cTopology::Node *userNode =
    topo.getNodeFor(simulation.getModuleByPath(userPath));
  //EV << "usernode " << userNode << endl;
  topo.calculateUnweightedSingleShortestPathsTo(userNode);
  cTopology::Node *currentNode = topo.getNodeFor(current);
  //EV << "currentnode " << currentNode << endl;
  cTopology::LinkOut *next = currentNode->getPath(0);
  //EV << "next " << next << endl;
  return next->getLocalGate();
}

bool selectFunction(cModule *mod, void *)
{
  // EV << "selfunc debug:" <<  mod->getFullName() << mod->getParentModule() << simulation.getSystemModule() << endl;
  return mod->getParentModule() == simulation.getSystemModule();
}

User::~User()
{
  cancelAndDelete(idleTimer);
  //delete queue;
}

void User::initialize()
{
  idleSignal = registerSignal("idle"); // name assigned to signal ID
  idleTimer = new cMessage("idle timer");
  idle();
//queue = new cPacketQueue("Packet Queue");
}

void User::idle()
{
  simtime_t idleTime = par("idleTime"); // changed via ini
  emit(idleSignal, idleTime);
  EV << getFullName() << " idling for " << idleTime << "s\n";
  scheduleAt(simTime()+idleTime, idleTimer);
}

void User::handleMessage(cMessage *msg)
{
  //if msg
  if (msg->isSelfMessage()) {
    // send request
    Request *req = new Request("request", 123); // use user[] index as message kind
    //double requestSize = par("requestSize");
    req->setSize(par("requestSize"));
    //req->setSize(intuniform(1, 1<<31));
    EV << "Sending request for " << req->getSize() << " bits\n";
    req->setSource(getIndex());
    req->setBitLength(1); // request packet 1 bit long only
    send(req, "gate$o");
    // idle for a bit
    idle();
  }
  else { // else received reply
    Reply* reply = check_and_cast<Reply*>(msg);
    EV << getFullName() << " received reply of size " << reply->getBitLength() << endl;
    delete reply;
  }
}

void PoP::initialize()
{
  requestSignal = registerSignal("request"); // name assigned to signal ID
  //queue = new cPacketQueue("Packet Queue");
}

void PoP::handleMessage(cMessage *msg)
{
  /*
  // logging
  Request *req = check_and_cast<Request*>(msg);
  unsigned int size = req->getSize();
  emit(requestSignal, size);
  // forward to first gate (which should be towards the beyond)
  cChannel *upstream =
    gate("gate$o", 0)->
    getTransmissionChannel();
  if (upstream->isBusy()) {
    EV << "Busy. Scheduled for " << upstream->getTransmissionFinishTime() << "s\n";
    scheduleAt(upstream->getTransmissionFinishTime(), msg);
  }
  else {
    // EV << "Upstream free. Forwarding now.\n";
    send(msg, "gate$o", 0);
  }
  */
  if (msg->getKind() == 123) {
    // logging
    Request *req = check_and_cast<Request*>(msg);
    unsigned int size = req->getSize();
    emit(requestSignal, size);
    // forward to first gate (which should be towards the beyond)
    cChannel *upstream =
      gate("gate$o", 0)->
      getTransmissionChannel();
    if (upstream->isBusy()) {
      EV << "Busy. Scheduled for " << upstream->getTransmissionFinishTime() << "s\n";
      scheduleAt(upstream->getTransmissionFinishTime(), msg);
    }
    else {
      // EV << "Upstream free. Forwarding now.\n";
      send(msg, "gate$o", 0);
    }
  }
  else if (msg->getKind() == 321) {
    Reply *reply = check_and_cast<Reply*>(msg);
    cGate* outGate = getNextGate(this, reply);
    send(reply, getNextGate(this, reply));
    EV << "got reply. sending out of " << outGate->getFullName() << endl;
  }
}

void Core::initialize()
{
  requestSignal = registerSignal("request"); // name assigned to signal ID
  //queue = new cPacketQueue("Packet Queue");
}

void Core::handleMessage(cMessage *msg)
{
  if (msg->getKind() == 123) {
    // logging
    Request *req = check_and_cast<Request*>(msg);
    unsigned int size = req->getSize();
    emit(requestSignal, size);
    // forward to first gate (which should be towards the beyond)
    cChannel *upstream =
      gate("gate$o", 0)->
      getTransmissionChannel();
    if (upstream->isBusy()) {
      EV << "Busy. Scheduled for " << upstream->getTransmissionFinishTime() << "s\n";
      scheduleAt(upstream->getTransmissionFinishTime(), msg);
    }
    else {
      // EV << "Upstream free. Forwarding now.\n";
      send(msg, "gate$o", 0);
    }
  }
  else if (msg->getKind() == 321) {
    Reply *reply = check_and_cast<Reply*>(msg);
    cGate* outGate = getNextGate(this, reply);
    send(reply, getNextGate(this, reply));
    EV << "got reply. sending out of " << outGate->getFullName() << endl;
  }
}

void Beyond::initialize()
{
  topoHelper();
  EV << "gatesize" << gateSize("gate") << endl;
  requestSignal = registerSignal("request"); // name assigned to signal ID
  //queue = new cPacketQueue("Packet Queue");
}

void Beyond::handleMessage(cMessage *msg)
{
  Request *req = check_and_cast<Request*>(msg);
  unsigned int size = req->getSize();
  int src = req->getSource();
  EV << "Received request from user " << src << " for " << size << "b\n";
  emit(requestSignal, size);

  // reply via random core
  Reply *reply = new Reply("reply", 321);
  reply->setBitLength(size);
  reply->setDestination(src);
  send(reply, "gate$o", intuniform(0, gateSize("gate")-1));

  // TODO reply with requested size.
  // DONE 1. identify sender. need info in request
  // 2. find route to sender (going down tree so routing needed)
  // 3. queueing messages
  // 3b. add proper queues (cQueue) instead of cycling messages
  delete msg;
}

void Beyond::topoHelper()
{
  topo.clear();
  // 4 methods:
  //topo.extractByModulePath(cStringTokenizer("*").asVector());
  topo.extractByNedTypeName(cStringTokenizer("Core PoP User").asVector());
  //topo.extractByProperty("display");
  //topo.extractFromNetwork(selectFunction, NULL);
  EV << topo.getNumNodes() << " nodes in routing topology\n";
  /*
  EV << "topo nodes: " << topo.getNumNodes() << endl;
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
    }
  */
}

