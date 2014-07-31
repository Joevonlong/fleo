#include <stdio.h>
#include <omnetpp.h>
#include "buffer.h"

Define_Module(Buffer);

void Buffer::initialize() {
    queue = new cPacketQueue();
    fromLogicID = gateBaseId("fromLogic");
    toLogicID = gateBaseId("toLogic");
    receiveID = gateBaseId("receive");
    transmitID = gateBaseId("transmit");
}

void Buffer::handleMessage(cMessage* msg) {
    //which gate did it come from?
    if (msg->getArrivalGateId() == receiveID) {
        EV << "receive id\n";
        send(msg, toLogicID);
    }
    else if (msg->getArrivalGateId() == fromLogicID) {
        EV << "fromlogid id\n";
        send(msg, transmitID);
    }
    else {
        EV << "error in buffer handlemsg\n";
    }
    return;
    //cChannel *upstream =
      //  gate("gate$o", 0)->
        //getTransmissionChannel();

}

//    if (upstream->isBusy()) {
//      EV << "Busy. Scheduled for " << upstream->getTransmissionFinishTime() << "s\n";
//      scheduleAt(upstream->getTransmissionFinishTime(), msg);
//    }
//    else {
//      // EV << "Upstream free. Forwarding now.\n";
//      send(msg, "gate$o", 0);
//    }

