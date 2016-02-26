#pragma once
#include <omnetpp.h>
#include "global.h"
#include "Flow.h"
#include "path.h"
#include "ChannelTree.h"
#include "Stream.h"

struct RequestData {
    std::list<Flow*> subflows;
    simtime_t viewtime;
    simtime_t elapsed;
    simtime_t remaining;
    simtime_t last_updated;
};

// finds, assigns, tracks, and revokes flows
class Controller : public cSimpleModule {
    public:
        bool userCallsThis_FixedBw(Path waypoints, uint64_t bits, uint64_t bps);
        bool userCallsThis(Path path, uint64_t bits);
        bool requestVID(Path waypoints, int vID);
        void end(cMessage* endMsg);
        void endStream(cMessage* endMsg);
    protected:
        virtual int numInitStages() const;
        virtual void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();
    private:
        Global* g;
        void rescheduleEnds();
        void shareBandwidth(std::set<Flow*> flows);
        std::map<Priority, std::list<Flow*> > flowsAtPriority;
        std::map<Priority, int> nreqsAtPriority;
        std::map<cMessage*, Flow*> endFlows;
        std::map<Flow*, cMessage*> flowEnds;
        //
        std::map<Flow*, Stream*> SubflowStreams;
        void checkAndCache(cModule* mod, int vID);
        void deactivateSubflow(Flow* f);
        void setupSubflow(Flow* f, int vID);
        std::map<cMessage*, Stream*> endStreams;
        //std::map<FlowChannel*, std::vector<Flow*> > channelFlows;
        void MergePathIntoFlowChannels(FlowChannels* fcs, Path* path);
        std::pair<bool, Path> waypointsAvailable(Path waypoints, uint64_t bps, Priority p);
        std::pair<bool, FlowChannels> treeAvailable(Node *root, std::vector<Node*> leaves, uint64_t bps, Priority p);
};
