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
        idleTime = par("initIdleTime");
        underflowTimer = new cMessage("underflow timer");
        idle();
        cacheTries = par("cacheTries");
    }
    else if (stage == 3) {
        if (cacheTries > 0){
            nearestCache = getNearestCacheID(getId());
        }
        else if (cacheTries == 0) {
            nearestCache = getNearestID(getId(), completeCacheIDs);
        }
        else {error("negative cacheTries value");}
        EV << "Nearest cache for " << getFullPath() << "(" << par("loc").stringValue() << ") is " << simulation.getModule(nearestCache)->getFullPath() << "(" << simulation.getModule(nearestCache)->getParentModule()->par("loc").stringValue() << ")." << endl;
    }
}

void User::idle() {
    idle(0);
}
void User::idle(simtime_t t) {
    // use initial idle time (avg 180s) before generating new one (avg 5s)
    idleTime = par("idleTime"); // changed via ini

    emit(idleSignal, idleTime);
    EV << getFullName() << " idling for " << idleTime << "s\n";
    scheduleAt(simTime()+idleTime+t, idleTimer);
    global->recordIdleTime(idleTime); // record at global module
    // clean up previous stream
    cancelEvent(underflowTimer);
}

void User::sendRequest()
{
    MyPacket *req = new MyPacket("Request");
    req->setBitLength(headerBitLength); // assume no transmission delay
    req->setSourceID(getId());
    req->setHops(0);
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
    // video length
    emit(videoLengthSignal, pkt->getVideoLength());
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
        global->recordUnderflow();
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
                global->recordStartupDelay(playBackStart - requestStartTime);
                global->recordHops(pkt->getHops());

                playbackTimeDownloaded += pkt->getVideoSegmentLength();
                pkt->setState(stateStream);
                if (pkt->getVideoSegmentsPending() > 0) {
                    scheduleAt(simTime()+global->getBufferMin(), pkt); // request next block in 15s
                    scheduleAt(simTime()+global->getBufferBlock(), underflowTimer); // underflow in 30s
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
                    scheduleAt(playBackStart+playbackTimeDownloaded-global->getBufferMin(), pkt);
                    scheduleAt(playBackStart+playbackTimeDownloaded, underflowTimer);
                }
                else if (pkt->getVideoSegmentsPending() == 0) {
                    scheduleAt(playBackStart+playbackTimeDownloaded, pkt);
                    cancelEvent(underflowTimer);
                }
                else {error("getVideoSegmentsPending < 0");}
            }
        } // end if stateTransfer
        else if (pkt->getState() == stateStream) {
            if (pkt->getVideoSegmentsPending() == 0) {
                // record playback completed
                delete pkt;
                idle();
                return;
            }
            else if (pkt->getVideoSegmentsPending() > 0) {
                pkt->setDestinationID(pkt->getSourceID());
                pkt->setSourceID(getId());
                pkt->setBitLength(headerBitLength);
                send(pkt, "out");
                // TODO teleport ack to cache to avoid false propagation delay?
                return;
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

