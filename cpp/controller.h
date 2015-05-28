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
        std::vector<path> findPathsBetween(cModule *src, cModule *dst);
        double findHighestBps(cModule *src, cModule *dst);
        bool reserveFlow(std::vector<cDatarateChannel*> path);
    protected:
        virtual int numInitStages() const;
        virtual void initialize(int stage);
        virtual void finish();
    private:
        //std::map<cDatarateChannel*, FlowPriority> flows;
        std::map<cMessage*, Flow*> flows;
};
