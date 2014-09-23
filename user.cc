#include <stdio.h>
#include <string.h>
#include "request_m.h"
#include "reply_m.h"
#include "mypacket_m.h"
#include "user.h"
#include "parse.h"
#include "routing.h"
#include "global.h"

Define_Module(User);

//const bool message_switching = true;
//const uint64_t packetBitSize = 1000000; // 1Mb
const short cacheTries = 2; // try 2 caches before master

User::~User()
{
    cancelAndDelete(idleTimer);
}

int User::numInitStages() const {return 4;}

void User::initialize(int stage) {
    if (stage == 0) {
        requestingBits = 0;
        requestHistogram.setName("Request Size");
        requestHistogram.setRangeAutoUpper(0);
        requestHistogram.setNumCells(100);
    //  requestHistogram.setRange(0, UINT64_MAX);
//        completionVector.setName("Time to complete request")
//        completionVector.
        //
        completionHistogram.setName("Completion Time");
        completionHistogram.setRangeAutoUpper(0);
        completionHistogram.setNumCells(100);
        //
        lagVector.setName("Total time minus ");
        idleTimer = new cMessage("idle timer");
        idle();
    }
    else if (stage == 3) {
        nearestCache = getNearestCacheID(getId());
        EV << "Nearest cache for " << getFullPath() << "(" << par("loc").stringValue() << ") is " << simulation.getModule(nearestCache)->getFullPath() << "(" << simulation.getModule(nearestCache)->getParentModule()->par("loc").stringValue() << ")." << endl;
    }
}

void User::idle()
{
    simtime_t idleTime = par("idleTime"); // changed via ini
    emit(idleSignal, idleTime);
    EV << getFullName() << " idling for " << idleTime << "s\n";
    scheduleAt(simTime()+idleTime, idleTimer);
}

void User::sendRequest()
{
    MyPacket *req = new MyPacket("Request");
    req->setBitLength(headerBitLength); // assume no transmission delay
    req->setSourceID(getId());
    req->setDestinationID(nearestCache);
    req->setCustomID(getRandCustomVideoID());
    EV << "Sending request for Custom ID " << req->getCustomID() << endl;
    req->setCacheTries(cacheTries);
    req->setState(stateStart);
    send(req, "out");
}

void User::handleMessage(cMessage *msg)
{
    if (msg == idleTimer) { // if idle timer is back
        //uint64_t size = getVideoSize();
        //emit(requestSignal, static_cast<double>(size));
        //requestHistogram.collect(size);
        //requestingBits = size;
        sendRequest();
    }
    else { // else received reply
        MyPacket *pkt = check_and_cast<MyPacket*>(msg);
        EV << "Received " << pkt->getBitLength() << "b of data. "
           << pkt->getBitsPending() << "b more to go.\n";
        if (pkt->getState() == stateEnd) {
            simtime_t completionTime = simTime() - pkt->getCreationTime();
            EV << "Transfer of item #" << pkt->getCustomID() << " complete. "
               << "Total time to serve request was " << completionTime << endl;
            completionHistogram.collect(completionTime);
            emit(videoLengthSignal, pkt->getVideoLength());
            emit(completionTimeSignal, completionTime);
            emit(effBitRateSignal, (double)pkt->getVideoLength()*800000/completionTime);
            delete pkt;
            idle();
        }
        else if (pkt->getState() == stateTransfer) {
            pkt->setState(stateAck);
            pkt->setDestinationID(pkt->getSourceID());
            pkt->setSourceID(getId());
            pkt->setBitLength(headerBitLength);
            send(pkt, "out");
            // TODO teleport ack to cache to avoid false propagation delay?
        }
        else {
            throw std::invalid_argument("invalid packet state");
        }
    }
}

void User::finish()
{
    requestHistogram.record();
    completionHistogram.record();
}

