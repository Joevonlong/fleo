#include <omnetpp.h>

class FlowChannel : public cDatarateChannel {
    public:
        bool assignDatarate(double bps);
        double getUsedBW();
        void setUsedBW(double bps);
        void addUsedBW(double bps);
    protected:
        bool isTransmissionChannel() const;
        simtime_t getTransmissionFinishTime() const;
        void processMessage(cMessage *msg, simtime_t t, result_t &result);
        //void handleParameterChange (const char *parname);
    private:
        bool isDisabled;
        double delay;
        double datarate;
        double used; // only new member compared to cDatarateChannel
        double ber;
        double per;
        simtime_t txfinishtime;
};
