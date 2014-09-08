#include "global.h"
#include "routing.h"
#include "parse.h"

Define_Module(Global);

simsignal_t idleSignal;
simsignal_t requestSignal;
int requestKind;
int replyKind;
std::map<std::string, std::string> locCaches;

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
    }
    else if (stage == 1) {
        loadCacheLocs();
    }
}

void Global::loadCacheLocs() {
    for (cModule::SubmoduleIterator i(getParentModule()); !i.end(); i++) {
        cModule *subModule = i();
        if (subModule->hasPar("hasCache")) {
            if (subModule->par("hasCache").boolValue() == true) {
                //locCaches[subModule->par("loc").stringValue()] = subModule->getFullPath();
            }
        }
    }
    EV << "locCaches.size() is " << locCaches.size() << endl;
    for (std::map<std::string, std::string>::iterator it=locCaches.begin(); it!=locCaches.end(); it++) {
        EV << it->first << " => " << it->second << endl;
    }
}

