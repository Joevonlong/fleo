#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"
#include "reply_m.h"
#include "beyond.h"

Define_Module(BeyondLogic);

void BeyondLogic::initialize()
{
  EV << "gatesize" << gateSize("gate") << endl;
}

void BeyondLogic::handleMessage(cMessage *msg)
{
  Reply* reply = NULL;
  // if request
  if (msg->getKind() == 123) {
    Request *req = check_and_cast<Request*>(msg);
    uint64_t size = req->getSize();
    int src = req->getSource();
    EV << "Received request from user " << src << " for " << size << "b\n";
    emit(requestSignal, (double)size);
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

