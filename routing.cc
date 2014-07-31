#include "routing.h"

cTopology topo;

cGate* getNextGate(cSimpleModule* current, Reply* reply) {
  short userIndex = reply->getDestination();
//EV << "userindex " << userIndex << endl;
  char userPath[20];
  sprintf(userPath, "Tree.user[%d]", userIndex);
//EV << "userPath " << userPath << endl;
  cTopology::Node *userNode =
    topo.getNodeFor(simulation.getModuleByPath(userPath));
  //EV << "usernode " << userNode << endl;
  topo.calculateUnweightedSingleShortestPathsTo(userNode);
  cTopology::Node *currentNode = topo.getNodeFor(current);
  //EV << "currentnode " << currentNode << endl;
  cTopology::LinkOut *next = currentNode->getPath(0);
  //EV << "next " << next << endl;
  return next->getLocalGate();
}

bool selectFunction(cModule *mod, void *)
{
  // EV << "selfunc debug:" <<  mod->getFullName() << mod->getParentModule() << simulation.getSystemModule() << endl;
  return mod->getParentModule() == simulation.getSystemModule();
}

void topoSetup()
{
  topo.clear();
  // 4 methods:
  //topo.extractByModulePath(cStringTokenizer("*").asVector());
  topo.extractByNedTypeName(cStringTokenizer("Core PoP User").asVector());
  //topo.extractByProperty("display");
  //topo.extractFromNetwork(selectFunction, NULL);
  EV << topo.getNumNodes() << " nodes in routing topology\n";
  /*
  EV << "topo nodes: " << topo.getNumNodes() << endl;
  for (int i=0; i<topo.getNumNodes(); i++)
    {
      cTopology::Node *node = topo.getNode(i);
      ev << "Node i=" << i << " is " << node->getModule()->getFullPath() << endl;
      ev << " It has " << node->getNumOutLinks() << " conns to other nodes\n";
      ev << " and " << node->getNumInLinks() << " conns from other nodes\n";

      ev << " Connections to other modules are:\n";
      for (int j=0; j<node->getNumOutLinks(); j++)
        {
          cTopology::Node *neighbour = node->getLinkOut(j)->getRemoteNode();
          cGate *gate = node->getLinkOut(j)->getLocalGate();
          ev << " " << neighbour->getModule()->getFullPath()
             << " through gate " << gate->getFullName() << endl;
        }
    }
  */
}
