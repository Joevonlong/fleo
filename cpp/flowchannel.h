#include <omnetpp.h>

class FlowChannel : public cDatarateChannel {
    protected:
        bool isTransmissionChannel() const;
        simtime_t getTransmissionFinishTime() const;
        void processMessage(cMessage *msg, simtime_t t, result_t& result);
        void handleParameterChange (const char *parname);
    private:
        bool isDisabled;
        double delay;
        double datarate, used;
        double ber, per;
        simtime_t txfinishtime;
};
