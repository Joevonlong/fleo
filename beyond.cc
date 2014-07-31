#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"
#include "reply_m.h"
#include "router.cc"

class Beyond : public Router
{
private:
  //cTopology topo;
  void topoHelper();
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

Define_Module(Beyond);

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


void Beyond::initialize()
{
  topoHelper();
  EV << "gatesize" << gateSize("gate") << endl;
  requestSignal = registerSignal("request"); // name assigned to signal ID
  //queue = new cPacketQueue("Packet Queue");
}

void Beyond::handleMessage(cMessage *msg)
{
  Reply* reply = NULL;
  // if request
  if (msg->getKind() == 123) {
    Request *req = check_and_cast<Request*>(msg);
    unsigned int size = req->getSize();
    int src = req->getSource();
    EV << "Received request from user " << src << " for " << size << "b\n";
    emit(requestSignal, size);
    delete msg;

    // construct reply
    reply = new Reply("reply", 321);
    reply->setBitLength(size);
    reply->setDestination(src);
  }
  // if delayed reply
  else { //(msg->getKind() == 321) {
    reply = check_and_cast<Reply*>(msg);
  }
  //else {EV << "bug\n";}
  // choose random output gate
  cGate *outGate = gate("gate$o", intuniform(0, gateSize("gate")-1));
  // if busy, delay till it is free but try on random channel again. so needs some redoing later.
  cChannel *downstream = outGate->getTransmissionChannel();
  if (downstream->isBusy()) {
    EV << "Busy. Scheduled for " << downstream->getTransmissionFinishTime() << "s\n";
    scheduleAt(downstream->getTransmissionFinishTime(), reply);
  }
  else {
    send(reply, outGate);
  }

  // TODO reply with requested size.
  // DONE 1. identify sender. need info in request
  // DONE 2. find route to sender (going down tree so routing needed)
  // 3. queueing messages
  // 3b. add proper queues (cQueue) instead of cycling messages
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
