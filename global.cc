#include "global.h"
#include "routing.h"
#include "parse.h"

Define_Module(Global);

const int defaultLoc = -1;

simsignal_t idleSignal;
simsignal_t requestSignal;
int requestKind;
int replyKind;
std::vector<int> cacheIDs;
std::vector<int> completeCacheIDs;
std::map<std::string, int> locCaches;

int Global::numInitStages () const {return 3;}

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
        //loadAllLocs();
    }
    else if (stage == 1) {
    }
    else if (stage == 2) {
        printCacheLocs();
        buildCacheVector();
    }
}

// insert all locations as keys into map with value -1
void Global::loadAllLocs() {
    for (cModule::SubmoduleIterator i(getParentModule()); !i.end(); i++) {
        cModule *subModule = i();
        if (subModule->hasPar("loc")) {
            locCaches[subModule->par("loc").stringValue()] = defaultLoc;
        }
    }
}

void Global::printCacheLocs() {
    EV << "locCaches.size() is " << locCaches.size() << endl;
    for (std::map<std::string, int>::iterator it=locCaches.begin();
        it!=locCaches.end(); it++) {
        EV << it->first << " => " << it->second << endl;
    }
}

void Global::buildCacheVector() {
    for (std::map<std::string, int>::iterator it=locCaches.begin(); it!=locCaches.end(); it++) {
        if (it->second != defaultLoc){
            cacheIDs.push_back(it->second);
        }
    }
}

