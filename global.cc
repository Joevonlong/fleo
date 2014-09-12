#include "global.h"
#include "routing.h"
#include "parse.h"

Define_Module(Global);

simsignal_t idleSignal;
simsignal_t requestSignal;
int requestKind;
int replyKind;
std::map<std::string, int> locCaches;

int Global::numInitStages () const {return 2;}

void Global::initialize(int stage)
{
    if (stage == 0) {
        idleSignal = registerSignal("idle"); // name assigned to signal ID
        requestSignal = registerSignal("request"); // name assigned to signal ID
        requestKind = 123;
        replyKind = 321;
        topoSetup();
        loadVideoLengthFile();
        EV << static_cast<double>(UINT64_MAX) << endl;
        loadAllLocs();
    }
    else if (stage == 1) {
        printCacheLocs();
    }
}

// insert all locations as keys into map with value -1
void Global::loadAllLocs() {
    for (cModule::SubmoduleIterator i(getParentModule()); !i.end(); i++) {
        cModule *subModule = i();
        if (subModule->hasPar("loc")) {
            locCaches[subModule->par("loc").stringValue()] = -1;
        }
    }
}

void Global::printCacheLocs() {
    EV << "locCaches.size() is " << locCaches.size() << endl;
    for (std::map<std::string, int>::iterator it=locCaches.begin(); it!=locCaches.end(); it++) {
        EV << it->first << " => " << it->second << endl;
    }
}

