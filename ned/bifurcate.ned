network Bifurcate
{
    parameters:
        int fanout = 2;
        int depth = default(1); // number of layers of routers (excludes dummy; minimum 1)
        bool coreRing = default(false);
    types:
        channel coreLink extends Cat5 {}
        channel accessLink extends Cat5 {}
    submodules:
        global: Global;
        controller: Controller;
        beyond: Beyond {
            hasCache = true;
            completeCache = true;
            cache.capacity = -1B;
        };
        core[fanout^depth]: Core { // first is dud
            //loc = "user";
            //hasCache = default(false);
        };
        user[fanout^depth]: User {
            //loc = "user";
        };
    connections allowunconnected:
        beyond.out++ --> coreLink --> core[0].in++;
        beyond.in++ <-- coreLink <-- core[0].out++;
        core[0].out++ --> coreLink --> core[1].in++;
        core[0].in++ <-- coreLink <-- core[1].out++;
        for d=1..depth-1, for i=2^(d-1)..2^d-1 {
            core[i].out++ --> coreLink --> core[2*i].in++;
            core[i].in++ <-- coreLink <-- core[2*i].out++;
            core[i].out++ --> coreLink --> core[2*i+1].in++;
            core[i].in++ <-- coreLink <-- core[2*i+1].out++;
        }
        for i=2^(depth-1)..2^depth-1 {
            core[i].out++ --> coreLink --> user[2*i-2^depth].in;
            core[i].in++ <-- coreLink <-- user[2*i-2^depth].out;
            core[i].out++ --> coreLink --> user[2*i-2^depth+1].in;
            core[i].in++ <-- coreLink <-- user[2*i-2^depth+1].out;
        }
}
