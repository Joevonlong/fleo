#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "request_m.h"
#include "reply_m.h"
#include "beyond.h"
#include "routing.h"

Define_Module(BeyondLogic);
std::string beyondPath;

void BeyondLogic::initialize()
{
  EV << "gatesize" << gateSize("gate") << endl;
  beyondPath = getFullPath();
}

void BeyondLogic::handleMessage(cMessage *msg)
{
  Reply* reply = NULL;
  // if request
  if (msg->getKind() == 123) {
    Request *req = check_and_cast<Request*>(msg);
    uint64_t size = req->getSize();
    const char* msgSrc = req->getSource();
    char* newDest = strdup(msgSrc); //new char[strlen(msgSrc)];
    strcpy(newDest, msgSrc);
    EV << "Received request from user " << msgSrc << " for " << size << "b\n";
    emit(requestSignal, (double)size);
    delete msg;

    // construct reply
    reply = new Reply("reply", 321);
    reply->setBitLength(size);
    reply->setDestination(newDest);
  }
  // choose gate using routing
  cGate* outGate = getNextGate(this, reply);
  send(reply, outGate);
  EV << "Sending reply out of " << outGate->getFullName() << endl;
  // choose random output gate
  //cGate *outGate = gate("gate$o", intuniform(0, gateSize("gate")-1));
  //send(reply, outGate);
}

