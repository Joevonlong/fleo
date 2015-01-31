#pragma once
#include <omnetpp.h>
#include "flow.h"

class FlowChannel : public cDatarateChannel {
    public:
        double getAvailableBps();
        double getAvailableBps(Priority p);
        double getUsedBps();
        // supercede these...
        void setUsedBps(double bps);
        void addUsedBps(double bps);
        // with these...
        void addFlow(Flow* f);
        void removeFlow(Flow* f);
        //
        bool isFlowPossible(Flow* f);
    protected:
        virtual void initialize();
        bool isTransmissionChannel() const;
        simtime_t getTransmissionFinishTime() const;
        void processMessage(cMessage *msg, simtime_t t, result_t &result);
        // new to subclass:
        std::set<Flow*> currentFlows;
        std::map<Priority, double> bpsLeftAtPriority;
    private:
        bool isDisabled;
        double delay;
        double ber;
        double per;
        simtime_t txfinishtime;
};
