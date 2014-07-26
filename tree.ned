simple Beyond
{
    parameters:
        @signal[request](type="unsigned long");
        @statistic[request-size](title="request size"; source="request"; record=vector,stats; interpolationmode=none);
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
        channel Transit extends ned.DatarateChannel {
            datarate = 1Mbps;
            delay = 100ms;
        }
        channel Distribution extends ned.DatarateChannel {
            datarate = 5Mbps;
            delay = 20ms;
        }
        channel Access extends ned.DatarateChannel {
            datarate = 10Mbps;
            delay = 5ms;
        }
    submodules:
        beyond: Beyond;
        core[2]: Core;
        pop[4]: PoP;
        user[8]: User;
    connections:
        // almost a simple 4-level binary tree
        beyond.gate++ <--> Transit <--> core[0].gate++;
        beyond.gate++ <--> Transit <--> core[1].gate++;
        core[0].gate++ <--> Distribution <--> core[1].gate++;
        core[0].gate++ <--> Distribution <--> pop[0].gate++;
        core[0].gate++ <--> Distribution <--> pop[1].gate++;
        core[1].gate++ <--> Distribution <--> pop[2].gate++;
        core[1].gate++ <--> Distribution <--> pop[3].gate++;
        pop[0].gate++ <--> Access <--> user[0].gate;
        pop[0].gate++ <--> Access <--> user[1].gate;
        pop[1].gate++ <--> Access <--> user[2].gate;
        pop[1].gate++ <--> Access <--> user[3].gate;
        pop[2].gate++ <--> Access <--> user[4].gate;
        pop[2].gate++ <--> Access <--> user[5].gate;
        pop[3].gate++ <--> Access <--> user[6].gate;
        pop[3].gate++ <--> Access <--> user[7].gate;
}
