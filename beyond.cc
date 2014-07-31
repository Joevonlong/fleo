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

