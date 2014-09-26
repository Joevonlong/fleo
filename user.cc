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

User::~User()
{
    cancelAndDelete(idleTimer);
    cancelAndDelete(underflowTimer);
}

int User::numInitStages() const {return 4;}

void User::initialize(int stage) {
    if (stage == 0) {
        global = (Global*)getParentModule()->getSubmodule("global");
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
        underflowTimer = new cMessage("underflow timer");
        idle();
        cacheTries = par("cacheTries");
    }
    else if (stage == 3) {
        nearestCache = getNearestCacheID(getId());
        //nearestCache = getNearestID(getId(), completeCacheIDs); // temp
        EV << "Nearest cache for " << getFullPath() << "(" << par("loc").stringValue() << ") is " << simulation.getModule(nearestCache)->getFullPath() << "(" << simulation.getModule(nearestCache)->getParentModule()->par("loc").stringValue() << ")." << endl;
    }
}

void User::idle() {
    idle(0);
}
void User::idle(simtime_t t) {
    simtime_t idleTime = par("idleTime"); // changed via ini
    emit(idleSignal, idleTime);
    EV << getFullName() << " idling for " << idleTime << "s\n";
    scheduleAt(simTime()+idleTime+t, idleTimer);
    // clean up previous stream
    cancelEvent(underflowTimer);
}

void User::sendRequest()
{
    MyPacket *req = new MyPacket("Request");
    req->setBitLength(headerBitLength); // assume no transmission delay
    req->setSourceID(getId());
    req->setDestinationID(nearestCache);
    req->setCustomID(getRandCustomVideoID());
    EV << "Sending request for Custom ID " << req->getCustomID() << endl;
    emit(requestSignal, req->getCustomID());
    req->setCacheTries(cacheTries);
    req->setState(stateStart);
    send(req, "out");
    requestStartTime = simTime();
    playingBack = false;
    playbackTimeDownloaded = 0;
}

void User::endRequest(MyPacket *pkt) {
    simtime_t completionTime = simTime() - pkt->getCreationTime();
    EV << "Transfer of item #" << pkt->getCustomID() << " complete. "
       << "Total time to serve request was " << completionTime << endl;
    // completion time
    completionHistogram.collect(completionTime);
    emit(completionTimeSignal, completionTime);
    global->recordCompletionTimeGlobal(completionTime);
    // video length
    emit(videoLengthSignal, pkt->getVideoLength());
    // effective bit rate
    emit(effBitRateSignal, (double)pkt->getVideoLength()*bitRate/completionTime);
    global->recordEffBitRateGlobal(
            (double)pkt->getVideoLength()*bitRate/completionTime);
    delete pkt;
}

//void User::startPlayback(simtime_t remaining) {
//    playingBack = true;
//    playBackStart = simTime();
//    // record playback delay
//    // ... = simTime() - requestStartTime;
//    scheduleAt(simTime()+remaining, underflowTimer);
//}

void User::handleMessage(cMessage *msg)
{
    if (msg == idleTimer) { // if idle timer is back
        //uint64_t size = getVideoSize();
        //requestHistogram.collect(size);
        //requestingBits = size;
        sendRequest();
    }
    else if (msg == underflowTimer) {
        // record underflow statistics
        error("underflow");
    }
    else { // else received reply
        MyPacket *pkt = check_and_cast<MyPacket*>(msg);
        EV << "Received " << pkt->getBitLength() << "b of data. "
           << pkt->getBitsPending() << "b more to go.\n";
        if (pkt->getState() == stateTransfer) {
            if (!playingBack) {
                playingBack = true;
                playBackStart = simTime();
                // record playback delay
                // ... = simTime() - requestStartTime;
                playbackTimeDownloaded += pkt->getVideoSegmentLength();
                pkt->setState(stateStream);
                if (pkt->getVideoSegmentsPending() > 0) {
                    scheduleAt(simTime()+15, pkt); // request next block in 15s
                    scheduleAt(simTime()+30, underflowTimer); // underflow in 30s
                    return;
                }
                else if (pkt->getVideoSegmentsPending() == 0) {
                    scheduleAt(simTime()+pkt->getVideoSegmentLength(), pkt);
                    cancelEvent(underflowTimer);
                    return;
                }
                else {error("getVideoSegmentsPending < 0");}
            }
            else { // currently playing back
                // add onto buffer
                playbackTimeDownloaded += pkt->getVideoSegmentLength();
                pkt->setState(stateStream);
                cancelEvent(underflowTimer);
                if (pkt->getVideoSegmentsPending() > 0) {
                    scheduleAt(playBackStart+playbackTimeDownloaded-15, pkt);
                    scheduleAt(playBackStart+playbackTimeDownloaded, underflowTimer);
                }
                else if (pkt->getVideoSegmentsPending() == 0) {
                    scheduleAt(playBackStart+playbackTimeDownloaded, pkt);
                }
                else {error("getVideoSegmentsPending < 0");}
            }
        } // end if stateTransfer
        else if (pkt->getState() == stateStream) {
            if (pkt->getVideoSegmentsPending() == 0) {
                // record playback completed
                delete pkt;
                idle();
            }
            else if (pkt->getVideoSegmentsPending() > 0) {
                pkt->setDestinationID(pkt->getSourceID());
                pkt->setSourceID(getId());
                pkt->setBitLength(headerBitLength);
                send(pkt, "out");
                // TODO teleport ack to cache to avoid false propagation delay?
            }
            else {error("getVideoSegmentsPending < 0");}
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

