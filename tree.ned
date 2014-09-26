simple Global {
    parameters:
        int bufferBlock = default(30s) @unit(s);
        int bufferMin = default(15s) @unit(s);
        @signal[completionTimeGlobal](type="simtime");
        @statistic[completion-time-global](title="global completion time"; source="completionTimeGlobal"; record=histogram,vector,mean,count; interpolationmode=none);
        @signal[effectiveBitRateGlobal](type="double");
        @statistic[effective-bitrate-global](title="global effective bit rate"; source="effectiveBitRateGlobal"; record=vector,mean,count; interpolationmode=none);
}

channel MyDatarateChannel extends ned.DatarateChannel
{
//    inherited:
//    @class(cDatarateChannel);
//    @signal[channelBusy](type=long);
//    @signal[messageSent](type=cMessage);
//    @signal[messageDiscarded](type=cMessage);
//    @statistic[busy](source=channelBusy;record=vector?;interpolationmode=sample-hold);
//    @statistic[utilization](source="timeavg(channelBusy)";record=last?);
//    @statistic[packets](source="constant1(messageSent)";record=count?;interpolationmode=none);
//    @statistic[packetBytes](source="packetBytes(messageSent)";record=sum?;unit=B;interpolationmode=none);
//    @statistic[packetsDiscarded](source="constant1(messageDiscarded)";record=count?;interpolationmode=none);
//    @statistic[throughput](source="sumPerDuration(packetBits(messageSent))";record=last?;unit=bps);
//    bool disabled = default(false);
//    double delay = default(0s) @unit(s); // propagation delay
//    double datarate = default(0bps) @unit(bps); // bits per second; 0=infinite
//    double ber = default(0); // bit error rate (BER)
//    double per = default(0); // packet error rate (PER)
}

channel OC192 extends MyDatarateChannel {
    datarate = 9.953Gbps;
    delay = 20ms;
}
channel OC48 extends MyDatarateChannel {
    datarate = 2.488Gbps;
    delay = 15ms;
}
channel OC12 extends MyDatarateChannel {
    datarate = 622.08Mbps;
    delay = 5ms;
}
channel OC3 extends MyDatarateChannel {
    datarate = 155.52Mbps;
    delay = 5ms;
}
channel Cat5 extends MyDatarateChannel {
    datarate = 100Mbps;
    delay = 5ms;
}
channel T3 extends MyDatarateChannel {
    datarate = 45Mbps;
    delay = 5ms;
}
channel Cat3 extends MyDatarateChannel {
    datarate = 10Mbps;
    delay = 5ms;
}

simple Buffer
{
    gates:
        inout logicIO;
        input receive;
        output transmit;
}

simple Cache {
    parameters:
        int size @unit(GiB) = default(0);
}

simple Logic
{
    parameters:
//        @signal[request](type="double");
  //      @statistic[request-size](title="request size"; source="request"; record=vector,histogram; interpolationmode=none);
    @class(Logic);
    gates:
        inout gate[];
}

simple BeyondLogic extends Logic
{
    parameters:
//        @signal[request](type="double");
  //      @statistic[request-size](title="request size"; source="request"; record=vector,histogram; interpolationmode=none);
        @display("i=block/server");
    @class(BeyondLogic);
}

simple InternalLogic extends Logic
{
    @class(InternalLogic);
}

simple CoreLogic extends InternalLogic
{
    parameters:
        @display("i=block/routing");
    @class(CoreLogic);
}

simple PoPLogic extends InternalLogic
{
    parameters:
        @display("i=block/switch");
    @class(PoPLogic);
}

module Router
{
    parameters:
        string name = default("");
        string loc = default("");
        int rn = default(-1);
        bool hasCache = default(false);
        int cacheRank = default(-1);
        bool completeCache = default(false);
    gates:
        input in[];
        output out[];
    submodules:
        buffer[sizeof(in)]: Buffer;
        cache: Cache;
    connections:
        for i=0..sizeof(in)-1 {
            buffer[i].transmit --> out++;
            buffer[i].receive <-- in++;
        }
}

module PoP extends Router
{
    parameters:
        @display("i=block/switch,blue");
    submodules:
        pop: Logic;
    connections:
        for i=0..sizeof(in)-1 {
            pop.gate++ <--> buffer[i].logicIO;
        }
}

module Core extends Router
{
    parameters:
        @display("i=block/routing,yellow");
    submodules:
        core: Logic;
    connections:
        for i=0..sizeof(in)-1 {
            core.gate++ <--> buffer[i].logicIO;
        }
}

module Beyond extends Router
{
    parameters:
        @display("i=block/server,red");
    submodules:
        beyond: Logic;
    connections:
        for i=0..sizeof(in)-1 {
            beyond.gate++ <--> buffer[i].logicIO;
        }
}

simple User
{
    parameters:
        string name = default("");
        string loc = default("");
        int rn = default(-1);
        volatile double idleTime @unit(s);   // delay before next request
        volatile double requestSize;
        int cacheTries = default(2); // try 2 caches before master
        @signal[idle](type="simtime");
        @statistic[idle](title="idle time"; source="idle"; record=vector,min,max,mean,count; interpolationmode=none);
        @signal[videoLength](type="long");
        @statistic[video-length](title="requested video length"; source="videoLength"; record=vector,mean,count,histogram; interpolationmode=none);
        @signal[request](type="long");
        @statistic[request-size](title="request size"; source="request"; record=vector,count; interpolationmode=none);
        @signal[completionTime](type="simtime");
        @statistic[completion-time](title="completion time"; source="completionTime"; record=vector?,mean,max,count; interpolationmode=none);
        @signal[effectiveBitRate](type="double");
        @statistic[effective-bitrate](title="Effective Bit Rate"; source=effectiveBitRate; record=vector?,mean,count,histogram; interpolationmode=none);
        @display("i=block/user");
    gates:
        input in;
        output out;
}

network Tree
{
    parameters:
        int fanout;
        bool coreRing = default(true);
    types:
    submodules:
        global: Global;
        beyond: Beyond;
        core[fanout]: Core;
        pop[fanout^2]: PoP;
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
            pop[i].out++ --> Cat3 --> user[j].in;
            pop[i].in++ <-- Cat3 <-- user[j].out;
        };
}

network Test
{
    submodules:
        tree: Tree;
}

