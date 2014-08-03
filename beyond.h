#include "router.h"

class Beyond : public Router
{
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

