simple Logic
{
    gates:
        inout gate[];
}

simple Buffer
{
    gates:
        input fromLogic;
        output toLogic;
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
        inout gate[];
    submodules:
        pop: PoP;
    connections:
        for i=0..sizeof(gate)-1 {
            pop.gate++ <--> gate++;
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

network qtest
{
    types:
        channel Cat3 extends ned.DatarateChannel {
            datarate = 10kbps;
            delay = 5ms;
        }
    submodules:
        buff: Buffer;
        outside: User;
        logic: Beyond;
    connections allowunconnected:
        outside.gate$o --> Cat3 --> buff.receive;
        outside.gate$i <-- Cat3 <-- buff.transmit;
        logic.gate$o++ --> buff.fromLogic;
        logic.gate$i[0] <-- buff.toLogic;
}

network shelltest
{
    types:
        channel Cat3 extends ned.DatarateChannel {
            datarate = 10kbps;
            delay = 5ms;
        }
    submodules:
        buff: Buffer;
        outside: User;
        logic: Beyond;
    connections allowunconnected:
        outside.gate$o --> Cat3 --> buff.receive;
        outside.gate$i <-- Cat3 <-- buff.transmit;
        logic.gate$o++ --> buff.fromLogic;
        logic.gate$i[0] <-- buff.toLogic;
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
        core[fanout]: PoPShell;
        pop[fanout^2]: PoPShell;
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
