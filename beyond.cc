#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"
#include "reply_m.h"
#include "routing.h"
#include "beyond.h"

Define_Module(Beyond);

void Beyond::initialize()
{
  topoSetup();
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
  send(reply, outGate);
}

