#include "logic.h"

class BeyondLogic : public Logic
{
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

