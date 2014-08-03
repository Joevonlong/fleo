#include <omnetpp.h>
#include "logic.h"

Define_Module(Logic);

void Logic::initialize()
{
    EV << "logic init\n";
  //requestSignal = registerSignal("request"); // name assigned to signal ID
  //queue = new cPacketQueue("Packet Queue");
}

void Logic::handleMessage(cMessage *msg)
{
    EV << "logic handle msg\n";
}

