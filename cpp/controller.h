#include <omnetpp.h>
#include "flow.h"

struct path {
    std::vector<cDatarateChannel*> links;
    double bps;
};

enum FlowPriority{low, medium, high};

// finds, assigns, tracks, and revokes flows
class Controller : public cSimpleModule {
    public:
        Flow* createFlow(Path path, uint64_t bps, Priority p);
        Flow* createFlow(Flow* f);
        bool revokeFlow(Flow* f);
    protected:
        virtual int numInitStages() const;
        virtual void initialize(int stage);
        virtual void finish();
    private:
        //std::map<cDatarateChannel*, FlowPriority> flows;
        std::map<cMessage*, Flow*> flows;
        std::map<FlowChannel*, std::vector<Flow*> > channelFlows;
};
