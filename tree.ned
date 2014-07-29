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
        core[4]: Core;
        pop[16]: PoP;
        user[64]: User;
    connections:
        beyond.gate++ <--> OC48 <--> core[0].gate++;
        beyond.gate++ <--> OC48 <--> core[1].gate++;
        beyond.gate++ <--> OC48 <--> core[2].gate++;
        beyond.gate++ <--> OC48 <--> core[3].gate++;
        core[0].gate++ <--> OC12 <--> core[1].gate++;
        core[1].gate++ <--> OC12 <--> core[2].gate++;
        core[2].gate++ <--> OC12 <--> core[3].gate++;
        core[3].gate++ <--> OC12 <--> core[0].gate++;
        core[0].gate++ <--> OC3 <--> pop[0].gate++;
        core[0].gate++ <--> OC3 <--> pop[1].gate++;
        core[0].gate++ <--> OC3 <--> pop[2].gate++;
        core[0].gate++ <--> OC3 <--> pop[3].gate++;
        core[1].gate++ <--> OC3 <--> pop[4].gate++;
        core[1].gate++ <--> OC3 <--> pop[5].gate++;
        core[1].gate++ <--> OC3 <--> pop[6].gate++;
        core[1].gate++ <--> OC3 <--> pop[7].gate++;
        core[2].gate++ <--> OC3 <--> pop[8].gate++;
        core[2].gate++ <--> OC3 <--> pop[9].gate++;
        core[2].gate++ <--> OC3 <--> pop[10].gate++;
        core[2].gate++ <--> OC3 <--> pop[11].gate++;
        core[3].gate++ <--> OC3 <--> pop[12].gate++;
        core[3].gate++ <--> OC3 <--> pop[13].gate++;
        core[3].gate++ <--> OC3 <--> pop[14].gate++;
        core[3].gate++ <--> OC3 <--> pop[15].gate++;
        pop[0].gate++ <--> Cat3 <--> user[0].gate;
        pop[0].gate++ <--> Cat3 <--> user[1].gate;
        pop[0].gate++ <--> Cat3 <--> user[2].gate;
        pop[0].gate++ <--> Cat3 <--> user[3].gate;
        pop[1].gate++ <--> Cat3 <--> user[4].gate;
        pop[1].gate++ <--> Cat3 <--> user[5].gate;
        pop[1].gate++ <--> Cat3 <--> user[6].gate;
        pop[1].gate++ <--> Cat3 <--> user[7].gate;
        pop[2].gate++ <--> Cat3 <--> user[8].gate;
        pop[2].gate++ <--> Cat3 <--> user[9].gate;
        pop[2].gate++ <--> Cat3 <--> user[10].gate;
        pop[2].gate++ <--> Cat3 <--> user[11].gate;
        pop[3].gate++ <--> Cat3 <--> user[12].gate;
        pop[3].gate++ <--> Cat3 <--> user[13].gate;
        pop[3].gate++ <--> Cat3 <--> user[14].gate;
        pop[3].gate++ <--> Cat3 <--> user[15].gate;
        pop[4].gate++ <--> Cat3 <--> user[16].gate;
        pop[4].gate++ <--> Cat3 <--> user[17].gate;
        pop[4].gate++ <--> Cat3 <--> user[18].gate;
        pop[4].gate++ <--> Cat3 <--> user[19].gate;
        pop[5].gate++ <--> Cat3 <--> user[20].gate;
        pop[5].gate++ <--> Cat3 <--> user[21].gate;
        pop[5].gate++ <--> Cat3 <--> user[22].gate;
        pop[5].gate++ <--> Cat3 <--> user[23].gate;
        pop[6].gate++ <--> Cat3 <--> user[24].gate;
        pop[6].gate++ <--> Cat3 <--> user[25].gate;
        pop[6].gate++ <--> Cat3 <--> user[26].gate;
        pop[6].gate++ <--> Cat3 <--> user[27].gate;
        pop[7].gate++ <--> Cat3 <--> user[28].gate;
        pop[7].gate++ <--> Cat3 <--> user[29].gate;
        pop[7].gate++ <--> Cat3 <--> user[30].gate;
        pop[7].gate++ <--> Cat3 <--> user[31].gate;
        pop[8].gate++ <--> Cat3 <--> user[32].gate;
        pop[8].gate++ <--> Cat3 <--> user[33].gate;
        pop[8].gate++ <--> Cat3 <--> user[34].gate;
        pop[8].gate++ <--> Cat3 <--> user[35].gate;
        pop[9].gate++ <--> Cat3 <--> user[36].gate;
        pop[9].gate++ <--> Cat3 <--> user[37].gate;
        pop[9].gate++ <--> Cat3 <--> user[38].gate;
        pop[9].gate++ <--> Cat3 <--> user[39].gate;
        pop[10].gate++ <--> Cat3 <--> user[40].gate;
        pop[10].gate++ <--> Cat3 <--> user[41].gate;
        pop[10].gate++ <--> Cat3 <--> user[42].gate;
        pop[10].gate++ <--> Cat3 <--> user[43].gate;
        pop[11].gate++ <--> Cat3 <--> user[44].gate;
        pop[11].gate++ <--> Cat3 <--> user[45].gate;
        pop[11].gate++ <--> Cat3 <--> user[46].gate;
        pop[11].gate++ <--> Cat3 <--> user[47].gate;
        pop[12].gate++ <--> Cat3 <--> user[48].gate;
        pop[12].gate++ <--> Cat3 <--> user[49].gate;
        pop[12].gate++ <--> Cat3 <--> user[50].gate;
        pop[12].gate++ <--> Cat3 <--> user[51].gate;
        pop[13].gate++ <--> Cat3 <--> user[52].gate;
        pop[13].gate++ <--> Cat3 <--> user[53].gate;
        pop[13].gate++ <--> Cat3 <--> user[54].gate;
        pop[13].gate++ <--> Cat3 <--> user[55].gate;
        pop[14].gate++ <--> Cat3 <--> user[56].gate;
        pop[14].gate++ <--> Cat3 <--> user[57].gate;
        pop[14].gate++ <--> Cat3 <--> user[58].gate;
        pop[14].gate++ <--> Cat3 <--> user[59].gate;
        pop[15].gate++ <--> Cat3 <--> user[60].gate;
        pop[15].gate++ <--> Cat3 <--> user[61].gate;
        pop[15].gate++ <--> Cat3 <--> user[62].gate;
        pop[15].gate++ <--> Cat3 <--> user[63].gate;
}
