network Dumbbell
{
    parameters:
        int fanout;
    types:
    submodules:
        global: Global;
        controller: Controller;
        beyond: Beyond{name="beyond"; loc="here"; rn=0; hasCache=true; cacheRank=1; @display("i=block/routing,green");};
        core[2]: Core{name="core"; loc="here"; rn=1;};
        pop: PoP{name="pop"; loc="here"; rn=2;};
        user[fanout]: User{name="user"; loc="here"; rn=3;};
    connections:
        for i=0..fanout-1 {
            user[i].out --> OC3 --> pop.in++;
            user[i].in <-- OC3 <-- pop.out++;
        };
        pop.out++ --> T3 --> core[0].in++;
        pop.in++ <-- T3 <-- core[0].out++;
        pop.out++ --> OC12 --> core[1].in++;
        pop.in++ <-- OC12 <-- core[1].out++;
        core[0].out++ --> OC12 --> beyond.in++;
        core[0].in++ <-- OC12 <-- beyond.out++;
        core[1].out++ --> OC12 --> beyond.in++;
        core[1].in++ <-- OC12 <-- beyond.out++;
}
