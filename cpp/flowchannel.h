#pragma once
#include <omnetpp.h>
#include "flow.h"

class FlowChannel : public cDatarateChannel {
    public:
        bool assignDatarate(double bps);
        double getAvailableBW();
        double getAvailableBW(int priority);
        double getUsedBW();
        void setUsedBW(double bps);
        void addUsedBW(double bps);
        bool isFlowPossible(Flow flow);
    protected:
        bool isTransmissionChannel() const;
        simtime_t getTransmissionFinishTime() const;
        void processMessage(cMessage *msg, simtime_t t, result_t &result);
        std::vector<Flow> currentFlows;
    private:
        bool isDisabled;
        double delay;
        double ber;
        double per;
        simtime_t txfinishtime;
        std::set<int> getPrioritySet();
};
