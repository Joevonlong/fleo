#include <omnetpp.h>
#include "buffer.h"

Define_Module(Buffer);

Buffer::~Buffer()
{
  cancelAndDelete(transmitDone);
  delete queue;
}

void Buffer::initialize() {
    fromLogicID = gateBaseId("logicIO$i");
    toLogicID = gateBaseId("logicIO$o");
    receiveID = gateBaseId("receive");
    transmitID = gateBaseId("transmit");
    transmitDone = new cMessage("transmitDone");
    queue = new cPacketQueue("queue");
}

void Buffer::handleMessage(cMessage* msg) {
    if (msg == transmitDone) {
        if (queue->front() != NULL) {
            send(queue->pop(), transmitID);
            scheduleAt(gate(transmitID)->getTransmissionChannel()->getTransmissionFinishTime(), transmitDone);
        }
    }
    else if (msg->getArrivalGateId() == receiveID) {
        send(msg, toLogicID);
    }
    else if (msg->getArrivalGateId() == fromLogicID) {
        cPacket* pkt = check_and_cast<cPacket*>(msg);
        cChannel* transmissionChannel =
            gate(transmitID)->
            getTransmissionChannel();
        // EV << transmissionChannel->info() << endl;
        if (transmissionChannel->isBusy()) {
            queue->insert(pkt);
        }
        else {
            send(msg, transmitID);
            scheduleAt(transmissionChannel->getTransmissionFinishTime(), transmitDone);
        }
    }
    else {
        throw std::invalid_argument("unhandled message");
    }
}

//    if (upstream->isBusy()) {
//      EV << "Busy. Scheduled for " << upstream->getTransmissionFinishTime() << "s\n";
//      scheduleAt(upstream->getTransmissionFinishTime(), msg);
//    }
//    else {
//      // EV << "Upstream free. Forwarding now.\n";
//      send(msg, "gate$o", 0);
//    }

