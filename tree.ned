simple Beyond
{
    parameters:
        @signal[request](type="unsigned long");
        @statistic[request-size](title="request size"; source="request"; record=vector,histogram; interpolationmode=none);
        @display("i=block/server");
    gates:
        inout gate[];
}

simple Core// extends Beyond
{
    parameters:
        @signal[request](type="unsigned long");
        @statistic[request-size](title="request size"; source="request"; record=vector,stats; interpolationmode=none);
        @display("i=block/routing");
    gates:
        inout gate[];
}

simple PoP// extends Core
{
    parameters:
        @signal[request](type="unsigned long");
        @statistic[request-size](title="request size"; source="request"; record=vector,stats; interpolationmode=none);
        @display("i=block/switch");
    gates:
        inout gate[];
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
        core[2]: Core;
        pop[4]: PoP;
        user[8]: User;
    connections:
        beyond.gate++ <--> OC48 <--> core[0].gate++;
        beyond.gate++ <--> OC48 <--> core[1].gate++;
        core[0].gate++ <--> OC3 <--> pop[0].gate++;
        core[0].gate++ <--> OC3 <--> pop[1].gate++;
        core[1].gate++ <--> OC3 <--> pop[2].gate++;
        core[1].gate++ <--> OC3 <--> pop[3].gate++;
        pop[0].gate++ <--> Cat3 <--> user[0].gate;
        pop[0].gate++ <--> Cat3 <--> user[1].gate;
        pop[1].gate++ <--> Cat3 <--> user[2].gate;
        pop[1].gate++ <--> Cat3 <--> user[3].gate;
        pop[2].gate++ <--> Cat3 <--> user[4].gate;
        pop[2].gate++ <--> Cat3 <--> user[5].gate;
        pop[3].gate++ <--> Cat3 <--> user[6].gate;
        pop[3].gate++ <--> Cat3 <--> user[7].gate;
}
