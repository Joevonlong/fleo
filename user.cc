#include <stdio.h>
#include <string.h>
#include "request_m.h"
#include "reply_m.h"
#include "user.h"
#include "parse.h"

Define_Module(User);

User::~User()
{
  cancelAndDelete(idleTimer);
}

void User::initialize()
{
  idleSignal = registerSignal("idle"); // name assigned to signal ID
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

void User::handleMessage(cMessage *msg)
{
  if (msg->isSelfMessage()) { // if idle timer is back
    Request *req = new Request("request", 123); // use user[] index as message kind
    //req->setSize(par("requestSize"));
    req->setSize(getVideoSize());
    EV << "Sending request for " << req->getSize() << " bits\n";
    req->setSource(getIndex());
    req->setBitLength(1); // request packet 1 bit long only
    send(req, "gate$o");
    idle();
  }
  else { // else received reply
    Reply* reply = check_and_cast<Reply*>(msg);
    EV << getFullName() << " received reply of size " << reply->getBitLength() << endl;
    delete reply;
  }
}

