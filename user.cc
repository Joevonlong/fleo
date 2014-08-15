#include <stdio.h>
#include <string.h>
#include "request_m.h"
#include "reply_m.h"
#include "user.h"
#include "parse.h"

Define_Module(User);

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
  Request *req = new Request("request", 123); // use user[] index as message kind
  req->setSize(std::min(requestingBits, packetBitSize)); //req->setSize(par("requestSize"));
  req->setSource(getIndex());
  req->setBitLength(1); // request packet 1 bit long only
  send(req, "gate$o");
}

void User::handleMessage(cMessage *msg)
{
  if (msg->isSelfMessage()) { // if idle timer is back
    uint64_t size = getVideoSize();
    EV << "Starting request for " << size << " bits\n";
    //emit(requestSignal, static_cast<double>(size));
    requestHistogram.collect(size);
    requestingBits = size;
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

