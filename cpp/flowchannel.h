#pragma once
#include <omnetpp.h>
#include "flow.h"

class FlowChannel : public cDatarateChannel {
    public:
        bool assignDatarate(double bps);
        double getAvailableBW();
        double getAvailableBW(Priority p);
        double getUsedBW();
        // supercede these...
        void setUsedBW(double bps);
        void addUsedBW(double bps);
        // with these...
        void addFlow(Flow* f);
        void removeFlow(Flow* f);
        //
        bool isFlowPossible(Flow flow);
    protected:
        virtual void initialize();
        bool isTransmissionChannel() const;
        simtime_t getTransmissionFinishTime() const;
        void processMessage(cMessage *msg, simtime_t t, result_t &result);
        // new to subclass:
        std::set<Flow*> currentFlows;
        std::map<Priority, int> priorityUsage; // not needed?
        std::map<Priority, double> bandwidthLeftAtPriority;
        std::set<Priority>getPrioritySet(); // not needed?
    private:
        bool isDisabled;
        double delay;
        double ber;
        double per;
        simtime_t txfinishtime;
};
