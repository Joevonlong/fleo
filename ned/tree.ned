network Tree
{
    parameters:
        int fanout;
        bool coreRing = default(true);
    types:
    submodules:
        global: Global;
        beyond: Beyond;
        core[fanout]: Core;
        pop[fanout^2]: PoP;
        user[fanout^3]: User;
    connections:
        for i=0..fanout-1 {
            beyond.out++ --> OC48 --> core[i].in++;
            beyond.in++ <-- OC48 <-- core[i].out++;
        };
        for i=0..fanout-2 {
            core[i].out++ --> OC12 --> core[i+1].in++ if coreRing==true;
            core[i].in++ <-- OC12 <-- core[i+1].out++ if coreRing==true;
        }
        core[fanout-1].out++ --> OC12 --> core[0].in++ if coreRing==true;
        core[fanout-1].in++ <-- OC12 <-- core[0].out++ if coreRing==true;
        for i=0..fanout-1, for j=(i*fanout)..(i+1)*fanout-1 {
            core[i].out++ --> OC3 --> pop[j].in++;
            core[i].in++ <-- OC3 <-- pop[j].out++;
        };
        for i=0..fanout^2-1, for j=(i*fanout)..(i+1)*fanout-1 {
            pop[i].out++ --> Cat3 --> user[j].in;
            pop[i].in++ <-- Cat3 <-- user[j].out;
        };
}

network Test
{
    submodules:
        tree: Tree;
}

