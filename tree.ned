simple Logic
{
    gates:
        inout gate[];
}

simple Buffer
{
    gates:
        inout logicIO;
        input receive;
        output transmit;
}

simple Router
{
    parameters:
        @signal[request](type="unsigned long");
        @statistic[request-size](title="request size"; source="request"; record=vector,stats; interpolationmode=none);
    gates:
        inout gate[];
}

simple Beyond extends Router
{
    parameters:
        @signal[request](type="unsigned long");
        @statistic[request-size](title="request size"; source="request"; record=vector,histogram; interpolationmode=none);
        @display("i=block/server");
    @class(Beyond);
}

simple InternalRouter extends Router
{
    @class(InternalRouter);
}

simple Core extends InternalRouter
{
    parameters:
        @display("i=block/routing");
    @class(Core);
}

simple PoP extends InternalRouter
{
    parameters:
        @display("i=block/switch");
    @class(PoP);
}

module PoPShell
{
    parameters:
        @display("i=block/switch");
    gates:
        input in[];
        output out[];
    submodules:
        pop: PoP;
        buffer[sizeof(in)]: Buffer;
    connections:
        for i=0..sizeof(in)-1 {
            pop.gate++ <--> buffer[i].logicIO;
            buffer[i].transmit --> out++;
            buffer[i].receive <-- in++;
        }
}

module BeyondShell
{
    parameters:
        @display("i=block/server");
    gates:
        input in[];
        output out[];
    submodules:
        beyond: Beyond;
        buffer[sizeof(in)]: Buffer;
    connections:
        for i=0..sizeof(in)-1 {
            beyond.gate++ <--> buffer[i].logicIO;
            buffer[i].transmit --> out++;
            buffer[i].receive <-- in++;
        }
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
        beyond: BeyondShell;
        core[fanout]: PoPShell {@display("i=block/routing");};
        pop[fanout^2]: PoPShell;
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
            pop[i].out++ --> Cat3 --> user[j].gate$i;
            pop[i].in++ <-- Cat3 <-- user[j].gate$o;
        };
}

