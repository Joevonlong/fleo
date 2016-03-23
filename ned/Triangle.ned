network Triangle
{
    parameters:
        int fanout = default(2);
    types:
    submodules:
        global: Global;
        controller: Controller;
        beyond: Beyond {
            name = "beyond";
            loc = "here";
            rn = 0;
            hasCache = true;
            completeCache = true;
            cache.capacity = -1B;
            cacheRank = 1;
        }
        core: Core {
            name = "core";
            loc = "here";
            rn = 1;
        }
        pop: PoP {
            name = "pop";
            loc = "here";
            rn = 1;
        }
        user: User{name="user"; loc="here"; rn=1;};
    connections allowunconnected:
        beyond.out++ --> Cat3 --> core.in++;
        beyond.in++ <-- Cat3 <-- core.out++;
        core.out++ --> Cat3 --> pop.in++;
        core.in++ <-- Cat3 <-- pop.out++;
        pop.out++ --> Cat3 --> beyond.in++;
        pop.in++ <-- Cat3 <-- beyond.out++;

        pop.out++ --> T3 --> user.in;
        pop.in++ <-- T3 <-- user.out;
}
