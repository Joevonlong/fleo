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
        user[10]: User{name="user"; loc="here"; rn=3;};
    connections allowunconnected:
//        for i=0..fanout-1 {
//            user[i].out --> FlowChannel {  delay = 1ms; datarate = 10Mbps; } --> pop.in++;
//            user[i].in <-- FlowChannel {  delay = 1ms; datarate = 10Mbps; } <-- pop.out++;
//        };
        user[0].out --> FlowChannel {  delay = 1ms; datarate = 10Mbps; } --> pop.in++;
        user[0].in <-- FlowChannel {  delay = 1ms; datarate = 10Mbps; } <-- pop.out++;
        user[1].out --> FlowChannel {  delay = 11ms; datarate = 10Mbps; } --> pop.in++;
        user[1].in <-- FlowChannel {  delay = 11ms; datarate = 10Mbps; } <-- pop.out++;
        user[2].out --> FlowChannel {  delay = 21ms; datarate = 10Mbps; } --> pop.in++;
        user[2].in <-- FlowChannel {  delay = 21ms; datarate = 10Mbps; } <-- pop.out++;
        user[3].out --> FlowChannel {  delay = 31ms; datarate = 10Mbps; } --> pop.in++;
        user[3].in <-- FlowChannel {  delay = 31ms; datarate = 10Mbps; } <-- pop.out++;
        user[4].out --> FlowChannel {  delay = 41ms; datarate = 10Mbps; } --> pop.in++;
        user[4].in <-- FlowChannel {  delay = 41ms; datarate = 10Mbps; } <-- pop.out++;
        user[5].out --> FlowChannel {  delay = 51ms; datarate = 10Mbps; } --> pop.in++;
        user[5].in <-- FlowChannel {  delay = 51ms; datarate = 10Mbps; } <-- pop.out++;
        user[6].out --> FlowChannel {  delay = 61ms; datarate = 10Mbps; } --> pop.in++;
        user[6].in <-- FlowChannel {  delay = 61ms; datarate = 10Mbps; } <-- pop.out++;
        user[7].out --> FlowChannel {  delay = 71ms; datarate = 10Mbps; } --> pop.in++;
        user[7].in <-- FlowChannel {  delay = 71ms; datarate = 10Mbps; } <-- pop.out++;
        user[8].out --> FlowChannel {  delay = 81ms; datarate = 10Mbps; } --> pop.in++;
        user[8].in <-- FlowChannel {  delay = 81ms; datarate = 10Mbps; } <-- pop.out++;
        user[9].out --> FlowChannel {  delay = 91ms; datarate = 10Mbps; } --> pop.in++;
        user[9].in <-- FlowChannel {  delay = 91ms; datarate = 10Mbps; } <-- pop.out++;
        pop.out++ --> FlowChannel {  delay = 29ms; datarate = 1Mbps; } --> beyond.in++;
        pop.in++ <-- FlowChannel {  delay = 29ms; datarate = 1Mbps; } <-- beyond.out++;
}
