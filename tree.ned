module Router
{
    parameters:
        @signal[request](type="unsigned long");
        @statistic[request-size](title="request size"; source="request"; record=vector,stats; interpolationmode=none);
    gates:
        inout gate[];
    submodules:
        //cpu : CPU;
        queue[sizeof(gate)]: Queue;
    connections allowunconnected:
        for i=0..sizeof(gate)-1 {
            gate[i] <--> queue[i].external;
        }
}

//simple CPU

simple Queue
{
    gates:
        inout internal;
        inout external;
}

module Beyond extends Router
{
    parameters:
        @signal[request](type="unsigned long");
        @statistic[request-size](title="request size"; source="request"; record=vector,histogram; interpolationmode=none);
        @display("i=block/server");
    @class(Beyond);
}

module InternalRouter extends Router
{
    @class(InternalRouter);
}

module Core extends InternalRouter
{
    parameters:
        @display("i=block/routing");
    @class(Core);
}

module PoP extends InternalRouter
{
    parameters:
        @display("i=block/switch");
    @class(PoP);
}

simple User
{
    parameters:
        volatile double idleTime @unit(s);   // delay before next request
        volatile double requestSize;
        @signal[idle](type="simtime");
        @statistic[idle](title="idle time"; source="idle"; record=vector,histogram; interpolationmode=none);
        @display("i=block/user");
    gates:
        inout gate;
}

network Tree
{
    parameters:
        int fanout = default(2);
        bool coreRing = default(true);
    types:
        channel OC192 extends ned.DatarateChannel {
            datarate = 9.92Gbps;
            delay = 20ms;
        }
        channel OC48 extends ned.DatarateChannel {
            datarate = 2.488Gbps;
            delay = 15ms;
        }
        channel OC12 extends ned.DatarateChannel {
            datarate = 622Mbps;
            delay = 10ms;
        }
        channel OC3 extends ned.DatarateChannel {
            datarate = 155Mbps;
            delay = 5ms;
        }
        channel Cat5 extends ned.DatarateChannel {
            datarate = 100Mbps;
            delay = 5ms;
        }
        channel T3 extends ned.DatarateChannel {
            datarate = 45Mbps;
            delay = 5ms;
        }
        channel Cat3 extends ned.DatarateChannel {
            datarate = 10Mbps;
            delay = 5ms;
        }
    submodules:
        beyond: Beyond;
        core[fanout]: Core;
        pop[fanout^2]: PoP;
        user[fanout^3]: User;
    connections:
        for i=0..fanout-1 {
            beyond.gate++ <--> OC48 <--> core[i].gate++;
        };
       for i=0..fanout-2 {
           core[i].gate++ <--> OC12 <--> core[i+1].gate++ if coreRing==true;
       }
       core[fanout-1].gate++ <--> OC12 <--> core[0].gate++ if coreRing==true;
        for i=0..fanout-1, for j=(i*fanout)..(i+1)*fanout-1 {
            core[i].gate++ <--> OC3 <--> pop[j].gate++;
        };
        for i=0..fanout^2-1, for j=(i*fanout)..(i+1)*fanout-1 {
            pop[i].gate++ <--> Cat3 <--> user[j].gate;
        };
}
