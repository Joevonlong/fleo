#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"
#include "reply_m.h"
#include "routing.h"
#include "internal.h"

Define_Module(Core);
Define_Module(PoP);

void InternalRouter::initialize()
{
  requestSignal = registerSignal("request"); // name assigned to signal ID
  //queue = new cPacketQueue("Packet Queue");
}

void InternalRouter::handleMessage(cMessage *msg)
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
//    cChannel *upstream =
//      gate("gate$o", 0)->
//      getTransmissionChannel();
//    if (upstream->isBusy()) {
//      EV << "Busy. Scheduled for " << upstream->getTransmissionFinishTime() << "s\n";
//      scheduleAt(upstream->getTransmissionFinishTime(), msg);
//    }
//    else {
//      // EV << "Upstream free. Forwarding now.\n";
    // forward to first gate (which should be towards the beyond)
      send(msg, "gate$o", 0);
//    }
  }
  else if (msg->getKind() == 321) {
    Reply *reply = check_and_cast<Reply*>(msg);
    cGate* outGate = getNextGate(this, reply);
    send(reply, outGate);
    EV << "got reply. sending out of " << outGate->getFullName() << endl;
  }
}
