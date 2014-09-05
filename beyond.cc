#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"
#include "reply_m.h"
#include "beyond.h"
#include "routing.h"
#include "cache.h"
#include "parse.h"

Define_Module(BeyondLogic);
std::string beyondPath;

void BeyondLogic::initialize()
{
    beyondPath = getFullPath();
    // populate cache with all content
    getParentModule()->par("hasCache").setBoolValue(true);
    Cache* cache = (Cache*)(getParentModule()->getSubmodule("cache"));
    for (unsigned long maxID = getMaxCustomVideoID(); maxID != ULONG_MAX; maxID--) {
        cache->setCached(maxID, true);
    }
}

void BeyondLogic::handleMessage(cMessage *msg)
{
  Reply* reply = NULL;
  // if request
  if (msg->getKind() == requestKind) {
    Request *req = check_and_cast<Request*>(msg);

    //uint64_t size = req->getSize();
    const char* msgSrc = req->getSource();
    char* newDest = strdup(msgSrc); //new char[strlen(msgSrc)];
    strcpy(newDest, msgSrc);
    EV << "Received request from user " << msgSrc << " for item #" << req->getCustomID() << endl;
    //emit(requestSignal, (double)size);

    // construct reply
    reply = new Reply("reply", replyKind);
    EV << "reply size: " << checkCache(req->getCustomID()) << endl;
    reply->setBitLength(checkCache(req->getCustomID()));
    reply->setDestination(newDest);

    delete msg;
  }
  // choose gate using routing
  cGate* outGate = getNextGate(this, reply);
  send(reply, outGate);
  EV << "Sending reply out of " << outGate->getFullName() << endl;
  // choose random output gate
  //cGate *outGate = gate("gate$o", intuniform(0, gateSize("gate")-1));
  //send(reply, outGate);
}

