#include <omnetpp.h>
#include "flow.h"

/*struct path {
    std::vector<cDatarateChannel*> links;
    double bps;
};*/

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
        void endReq(cMessage* endMsg);
    protected:
        virtual int numInitStages() const;
        virtual void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();
    private:
        void rescheduleEnds();
        void shareBandwidth(std::set<Flow*> flows);
        std::map<Priority, std::list<Flow*> > flowsAtPriority;
        std::map<Priority, int> nreqsAtPriority;
        std::map<cMessage*, Flow*> endFlows;
        std::map<Flow*, cMessage*> flowEnds;
        std::map<cMessage*, RequestData*> endReqs;
        //std::map<FlowChannel*, std::vector<Flow*> > channelFlows;
        std::pair<bool, Path> waypointsAvailable(Path waypoints, uint64_t bps, Priority p);
};
