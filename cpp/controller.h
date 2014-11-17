#include <omnetpp.h>

class Controller : public cSimpleModule {
    protected:
        virtual int numInitStages() const;
        virtual void initialize(int stage);
        virtual void finish();
};
