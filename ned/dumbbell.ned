network Dumbbell
{
    parameters:
        int fanout;
    types:
    submodules:
        global: Global;
        beyond: Beyond;
        core: Core;
        pop: PoP;
        user[fanout]: User;
    connections:
        for i=0..fanout-1 {
            user[i].out --> OC3 --> pop.in++;
            user[i].in <-- OC3 <-- pop.out++;
        };
        pop.out++ --> OC12 --> core.in++;
        pop.in++ <-- OC12 <-- core.out++;
        core.out++ --> OC12 --> beyond.in++;
        core.in++ <-- OC12 <-- beyond.out++;
}
