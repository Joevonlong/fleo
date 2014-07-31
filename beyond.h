#include "router.cc"

class Beyond : public Router
{
private:
//  void topoHelper();
protected:
  virtual void initialize();
  virtual void handleMessage(cMessage *msg);
};

