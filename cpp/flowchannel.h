#pragma once
#include <omnetpp.h>
#include "Flow.h"

class FlowChannel : public cDatarateChannel {
    public:
        uint64_t getAvailableBps();
        uint64_t getAvailableBps(Priority p);
        uint64_t getUsedBps();
        // supercede these...
        void setUsedBps(uint64_t bps);
        void addUsedBps(int64_t bps);
        // with these...
        void addFlow(Flow* f);
        void addFlowV2(Flow* f);
        void shareBW(std::map<Flow*, cMessage*> flowEnds);
        void shareBWexcept(std::map<Flow*, cMessage*> flowEnds, Flow* except);
        void spreadUpdates();
        std::set<Flow*> getFlows();
        void removeFlow(Flow* f);
        void removeFlowV2(Flow* f);
        void printBpsLeftAtPriority();
        //
        bool isFlowPossible(uint64_t bps, Priority p);
        bool isFlowPossible(Flow* f); // unused?
        Flow* getLowestPriorityFlow();
    protected:
        virtual void initialize();
        virtual void finish();
        bool isTransmissionChannel() const;
        simtime_t getTransmissionFinishTime() const;
        void processMessage(cMessage *msg, simtime_t t, result_t &result);
        // new to subclass:
        std::set<Flow*> currentFlows;
        std::map<Priority, uint64_t> bpsLeftAtPriority;
        void shareBwEqual(Flow* except);
        void shareBwRttInverse(Flow* except);
        void shareBwRtt2Inverse(Flow* except);
        // statistics
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
