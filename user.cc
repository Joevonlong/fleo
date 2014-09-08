#include <stdio.h>
#include <string.h>
#include "request_m.h"
#include "reply_m.h"
#include "user.h"
#include "parse.h"
#include "routing.h"
#include "global.h"

Define_Module(User);

const bool message_switching = true;
const uint64_t packetBitSize = 1000000; // 1Mb

User::~User()
{
  cancelAndDelete(idleTimer);
}

void User::initialize()
{
    requestingBits = 0;
    requestHistogram.setName("Request Size");
    requestHistogram.setRangeAutoUpper(0);
    requestHistogram.setNumCells(100);
//  requestHistogram.setRange(0, UINT64_MAX);
    idleTimer = new cMessage("idle timer");
    idle();
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
    Request *req = new Request("request", requestKind); // use user[] index as message kind
    req->setCustomID(getRandCustomVideoID());
    EV << "Sending request for Custom ID " << req->getCustomID() << endl;
    req->setSourceID(getId());
    req->setDestinationID(locCaches[par("loc")]);
    req->setBitLength(1); // request packet 1 bit long only
    send(req, "out");
}

void User::handleMessage(cMessage *msg)
{
  if (msg->isSelfMessage()) { // if idle timer is back
    //uint64_t size = getVideoSize();
    //emit(requestSignal, static_cast<double>(size));
    //requestHistogram.collect(size);
    //requestingBits = size;
    sendRequest();
  }
  else { // else received reply
    Reply* reply = check_and_cast<Reply*>(msg);
    uint64_t size = reply->getBitLength();
    delete reply;
    EV << getFullName() << " received reply of size " << size;
    requestingBits -= size;
    if (requestingBits != 0) {
      EV << ". Still waiting for " << requestingBits << endl;
      sendRequest();
    }
    else {
      EV << ". Request fulfilled.\n";
      idle();
    }
  }
}

void User::finish()
{
  requestHistogram.record();
}

