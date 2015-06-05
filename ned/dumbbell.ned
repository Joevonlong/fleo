network Dumbbell
{
    parameters:
        int fanout;
    types:
    submodules:
        global: Global;
        controller: Controller;
        beyond: Beyond{name="beyond"; loc="here"; rn=0; hasCache=true; cacheRank=1; @display("i=block/routing,green");};
        pop: PoP{name="pop"; loc="here"; rn=2;};
        user[fanout]: User{name="user"; loc="here"; rn=3;};
    connections:
        for i=0..fanout-1 {
            user[i].out --> Cat3 --> pop.in++;
            user[i].in <-- Cat3 <-- pop.out++;
        };
        pop.out++ --> Cat3 --> beyond.in++;
        pop.in++ <-- Cat3 <-- beyond.out++;
}
