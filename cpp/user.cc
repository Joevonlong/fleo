#include <stdio.h>
#include <string.h>
#include <algorithm>
#include "request_m.h"
#include "reply_m.h"
#include "mypacket_m.h"
#include "user.h"
#include "logic.h"
#include "parse.h"
#include "routing.h"

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
        controller = (Controller*)getParentModule()->getSubmodule("controller");
        requestingBits = 0;
        requestHistogram.setName("Request Size");
        requestHistogram.setRangeAutoUpper(0);
        requestHistogram.setNumCells(100);
    //  requestHistogram.setRange(0, UINT64_MAX);
        //
        completionVector.setName("Time to complete request");
        completionHistogram.setName("Completion Time");
        completionHistogram.setRangeAutoUpper(0);
        completionHistogram.setNumCells(100);
        //
        lagVector.setName("Total time minus ");
        idleTimer = new cMessage("idle timer");
        underflowTimer = new cMessage("underflow timer");
        idle(par("initIdleTime").doubleValue());
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
        global->recordUserD2C(getDistanceBetween(getId(), nearestCache));
        EV << "Nearest cache for " << getFullPath() << "(" << par("loc").stringValue() << ") is " << simulation.getModule(nearestCache)->getFullPath() << "(" << simulation.getModule(nearestCache)->getParentModule()->par("loc").stringValue() << ")." << endl;
    }
}

void User::idle() {
    idleTime = par("idleTime"); // changed via ini
    idle(idleTime);
}
void User::idle(simtime_t t) {
    EV << getFullName() << " idling for " << t << "s\n";
    scheduleAt(simTime()+t, idleTimer);
    // record stats
    emit(idleSignal, t);
    global->recordIdleTime(t);
    // clean up previous stream
    cancelEvent(underflowTimer);
}

//~ std::vector<int> User::findAvailablePathTo(int destID, double bpsWanted) { // returns moduleIDs
    //~ 
//~ }

void viewVideo(int customID, int cacheID) {
    // gets number of qualities
    // for i from 1 to quality
    // get bitrate
    // setup conn
    //   need to cancel lower priorities... query central DB of flows?
    //     getAvailablePaths needs to accept priority argument?
}

// MAYBE factor out into another method that takes ID as intake, then basically
// try to set up all the avaibale comms for such.

void User::sendRequestSPO() {
    // using controller; implementing bandwidth sharing based on TCP behaviour
    int vID = getRandCustomVideoID();
    if (true) { // do time-varying demand
        short windowSec = 3600;
        short numWindows = 1;
        short window = simTime().dbl()/windowSec;
        window %= numWindows;
        EV << "Window: " << window << endl;
        vID = (vID/numWindows)*numWindows+window;
        /*
        do {
            vID = getRandCustomVideoID();
        } while (vID%24 != hour);
        */
    }
    EV << "Requested video ID " << vID << endl;
    std::deque<Logic*> waypoints = ((Logic*)simulation.getModule(nearestCache))->getRequestWaypoints(vID, cacheTries); // note: doesn't include User itself
    // output for debugging
    EV << "waypoints: ";
    for (std::deque<Logic*>::iterator it = waypoints.begin(); it != waypoints.end(); ++it) {
        EV << (*it)->getFullPath() << " > ";
    }
    EV << endl;
    // end output
    // Start with user node...
    Path waypointNodes;
    waypointNodes.push_back(topo.getNodeFor(this));
    // ... and convert the rest to nodes.
    for (std::deque<Logic*>::iterator logic_it = waypoints.begin(); logic_it != waypoints.end(); ++logic_it) {
        waypointNodes.push_back(topo.getNodeFor(*logic_it));
    }
    // reverse since data flow moves from server/replica to user
    std::reverse(waypointNodes.begin(), waypointNodes.end());
    EV << "Path to user: ";
    printPath(waypointNodes);
    global->recordFlowSuccess(controller->requestVID(waypointNodes, vID));
    return;
    //
    // old SPO
    //
    double requestBytes = par("requestSize").doubleValue();
    uint64_t bits;
    if (requestBytes <= 0) {
        //int vID = getRandCustomVideoID();
        bits = getVideoBitSize(getRandCustomVideoID());
    }
    else {
        bits = requestBytes * 8;
    }
    global->recordRequestedLength(bits/800000); // magic bitrate
    Path path = getShortestPathDijkstra((Logic*)simulation.getModule(nearestCache), this);
    // assume shortest path only
    //endMsgs.insert(controller->userCallsThis(path, vID));
    //global->recordFlowSuccess(controller->userCallsThis(path, bits));
    return;
}

void User::handleMessage(cMessage *msg)
{
    if (msg == idleTimer) { // if idle timer is back
        sendRequestSPO();
        idle();
    }
    else if (msg->getArrivalGate()->getId() == gate("directInput")->getId()) {
        completionVector.record(simTime() - msg->getCreationTime());
        EV << "it worked" << endl;
        delete msg;
    }
    else if (flowMap.count(msg) == 1) {
        // flow has finished
        revokeFlow(flowMap[msg]);
        flowMap.erase(msg);
        delete msg;
        //idle(); // sure?
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
                //////////////
                if (pkt->getVideoSegmentLength() < 20) {
                    global->recordStartupDelayL20(playBackStart - requestStartTime);
                }
                else {
                    global->recordStartupDelay(playBackStart - requestStartTime);
                }
                //////////////
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
    //requestHistogram.record();
    //completionHistogram.record();
}

