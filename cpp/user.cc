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
    double requestBytes = par("requestSize").doubleValue();
    uint64_t bits;
    if (requestBytes < 0) {
        bits = getVideoBitSize(vID);
    }
    else {
        bits = requestBytes * 8;
    }
    Path path = getShortestPathDijkstra((Logic*)simulation.getModule(nearestCache), this);
    // assume shortest path only
    //endMsgs.insert(controller->userCallsThis(path, vID));
    controller->userCallsThis(path, bits);
    global->recordFlowSuccess(true); // pointless atm
    return;
}

void User::sendRequest()
{
    // new flow based...
    /**
     * TODO trigger connection from replica to replica/origin if content not in cache
     */
    int vID = getRandCustomVideoID();
    std::deque<Logic*> waypoints = ((Logic*)simulation.getModule(nearestCache))->getRequestWaypoints(vID, cacheTries); // note: doesn't include User itself
    // output for debugging
    EV << "waypoints: ";
    for (std::deque<Logic*>::iterator it = waypoints.begin(); it != waypoints.end(); ++it) {
        EV << (*it)->getFullPath() << " > ";
    }
    EV << endl;
    // end output
    // Add user node to head...
    Path waypointNodes;
    waypointNodes.push_back(topo.getNodeFor(this));
    // ... and convert the rest to nodes.
    for (std::deque<Logic*>::iterator logic_it = waypoints.begin(); logic_it != waypoints.end(); ++logic_it) {
        waypointNodes.push_back(topo.getNodeFor(*logic_it));
    }
    // reverse since data flow moves from server/replica to user
    std::reverse(waypointNodes.begin(), waypointNodes.end());
    // Then look for a possible Flow for each node-pair
    /**
     * trying a proper multiflow/waypoints check instead
     */
    PathList setMeUp = waypointsToAvailablePaths(waypointNodes, getBitRate(vID, 1), 1);
    if (setMeUp.size() == 0) {
        EV << "could not find bandwidth to serve request\n";
        global->recordFlowSuccess(false);
    }
    else {
        for (PathList::iterator setup_it = setMeUp.begin(); setup_it != setMeUp.end(); ++setup_it) {
            cMessage *vidComplete = new cMessage("video transfer complete"); // one self timer for each expiry (no need to link all flows together?)
            scheduleAt(simTime()+getVideoSeconds(vID), vidComplete);
            Flow* f = createFlow(*setup_it, getBitRate(vID, 1), 1);
            printPath(f->path);
            flowMap[vidComplete] = f;
        }
        // cache at replicas too
        for (std::deque<Logic*>::iterator it = waypoints.begin(); it != waypoints.end(); ++it) {
            (*it)->setCached(vID, true);
        }
        global->recordFlowSuccess(true);
    }
    return;
}

void User::handleMessage(cMessage *msg)
{
    if (msg == idleTimer) { // if idle timer is back
        sendRequest();
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

/*
void User::setupFlowTo(int destID) {
    double minDatarate = DBL_MAX; // bit/ss
    Logic *dest = check_and_cast<Logic*>(simulation.getModule(destID));
    // walk from this to dest, finding smallest available BW
    cTopology::Node *node = dest->topo.getNodeFor(this);
    if (node == NULL) {
        ev << "We (" << getFullPath() << ") are not included in the topology.\n";
    }
    else if (node->getNumPaths()==0) {
        ev << "No path to destination.\n";
    }
    else {
        while (node != dest->topo.getTargetNode()) {
            // BUG in this loop but not first iteration
            ev << "We are in " << node->getModule()->getFullPath() << endl;
            ev << node->getDistanceToTarget() << " hops to go\n";
            ev << "There are " << node->getNumPaths()
               << " equally good directions, taking the first one\n";
            cTopology::LinkOut *path = node->getPath(0);
            ev << "Taking gate " << path->getLocalGate()->getFullName()
               << " we arrive in " << path->getRemoteNode()->getModule()->getFullPath()
               << " on its gate " << path->getRemoteGate()->getFullName() << endl;

            
            if (path->getLocalGate()->getChannel()->isTransmissionChannel()) {
                EV << "rerqrewq\n";
                //double nextDatarate = ((cDatarateChannel*)path->getLocalGate()->getTransmissionChannel())->getDatarate();
                //minDatarate = std::min(minDatarate, nextDatarate);
            }
            else {
                EV << "sdasd\n";
            }
            node = path->getRemoteNode();
            
        }
        error("sdasd");
    }
    EV << "min rate is " << minDatarate << endl;
}
*/

void User::finish()
{
    //requestHistogram.record();
    //completionHistogram.record();
}

