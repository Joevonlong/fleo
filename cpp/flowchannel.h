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
        void printBpsLeftAtPriority();
        //
        bool isFlowPossible(double bps, Priority p);
        bool isFlowPossible(Flow* f); // unused?
    protected:
        virtual void initialize();
        virtual void finish();
        bool isTransmissionChannel() const;
        simtime_t getTransmissionFinishTime() const;
        void processMessage(cMessage *msg, simtime_t t, result_t &result);
        // new to subclass:
        std::set<Flow*> currentFlows;
        std::map<Priority, double> bpsLeftAtPriority;
        void recordUtil(); // will self convert to fraction
        cOutVector utilVec; cDoubleHistogram utilHist;
        simtime_t prevRecAt; double prevBw; double cumBwT;
    private:
        bool isDisabled;
        double delay;
        double ber;
        double per;
        simtime_t txfinishtime;
};
