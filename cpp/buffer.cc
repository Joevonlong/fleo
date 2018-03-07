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
        EV << "Transmit done.\n";
        if (queue->front() != NULL) {
            EV << "\tQueue not empty, sending next.\n";
            EV << ((cPacket*)queue->front())->getBitLength() << endl;//
            send(queue->pop(), transmitID);
            if (transmitDone->isScheduled()) error("Buffer::handleMessage A");
            scheduleAt(gate(transmitID)->getTransmissionChannel()
                ->getTransmissionFinishTime(), transmitDone);
        }
    }
    else if (msg->getArrivalGateId() == receiveID) {
        send(msg, toLogicID);
    }
    else if (msg->getArrivalGateId() == fromLogicID) {
        EV << "Outgoing message from logic.\n";
        cPacket* pkt = check_and_cast<cPacket*>(msg);
        cChannel* transmissionChannel =
            gate(transmitID)->
            getTransmissionChannel();
        if (transmissionChannel->isBusy()) {
            EV << "\tChannel is busy, placing into queue.\n";
            queue->insert(pkt);
        }
        else {
            EV << "\tChannel is free, transmitting now.\n";
            EV << ((cPacket*)msg)->getBitLength() << endl;//
            send(msg, transmitID);
            if (transmitDone->isScheduled()) {
                cancelEvent(transmitDone);
            }
            scheduleAt(transmissionChannel->getTransmissionFinishTime(),
                transmitDone);
        }
    }
    else {
        throw std::invalid_argument("unhandled message");
    }
}
