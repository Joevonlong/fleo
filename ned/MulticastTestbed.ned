network MulticastTestbed
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
        pop0: PoP {
            name = "pop0";
            loc = "there";
            rn = 2;
            hasCache = true;
        }
        pop1: PoP {
            name = "pop1";
            loc = "somewhere";
            rn = 2;
            hasCache = true;
        }
        user0: User{name="user"; loc="there"; rn=3;};
        user1: User{name="user"; loc="there"; rn=3;};
        user2: User{name="user"; loc="somewhere"; rn=3;};
        user3: User{name="user"; loc="somewhere"; rn=3;};
    connections allowunconnected:
        beyond.out++ --> Cat5 --> core.in++;
        beyond.in++ <-- Cat5 <-- core.out++;

        core.out++ --> Cat5 --> pop0.in++;
        core.in++ <-- Cat5 <-- pop0.out++;
        core.out++ --> Cat5 --> pop1.in++;
        core.in++ <-- Cat5 <-- pop1.out++;

        pop0.out++ --> Cat5 --> user0.in;
        pop0.in++ <-- Cat5 <-- user0.out;
        pop0.out++ --> Cat5 --> user1.in;
        pop0.in++ <-- Cat5 <-- user1.out;
        pop1.out++ --> Cat5 --> user2.in;
        pop1.in++ <-- Cat5 <-- user2.out;
        pop1.out++ --> Cat5 --> user3.in;
        pop1.in++ <-- Cat5 <-- user3.out;
}
