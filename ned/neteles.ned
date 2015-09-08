simple Global {
    parameters:
        int bufferBlock = default(14s) @unit(s);
        int bufferMin = default(7s) @unit(s);
}

simple Controller {
	parameters:
		int detourAttempts = default(1); // 1 means shortest (Dijkstra's) only
}

channel FlowChannel extends ned.DatarateChannel
{
    @class(FlowChannel);
    double used = default(0bps) @unit(bps); // used datarate
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

channel OC192 extends FlowChannel {
    datarate = 9.953Gbps;
    delay = 20ms;
}
channel OC48 extends FlowChannel {
    datarate = 2.488Gbps;
    delay = 15ms;
}
channel OC12 extends FlowChannel {
    datarate = 622.08Mbps;
    delay = default(3.8ms) @unit(s);
}
channel OC3 extends FlowChannel {
    datarate = 155.52Mbps;
    delay = 5ms;
}
channel Cat5 extends FlowChannel {
    datarate = 100Mbps;
    delay = default(3.8ms) @unit(s);
}
channel T3 extends FlowChannel {
    datarate = 45Mbps;
    delay = default(5ms) @unit(s);
}
channel Cat3 extends FlowChannel {
    datarate = 10Mbps;
    delay = default(1ms) @unit(s);
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
        double capacity @unit(B) = default(1.5TiB);
}

simple Logic
{
    parameters:
    @class(Logic);
    gates:
        inout gate[]; // to be phased out
        input receive[];
        output transmit[];
        inout testlink[];
}

simple BeyondLogic extends Logic
{
    parameters:
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
        //buffer[sizeof(in)]: Buffer;
        cache: Cache;
    connections:
        //for i=0..sizeof(in)-1 {
            // buffer[i].transmit --> out++;
            // buffer[i].receive <-- in++;
        //}
}

module PoP extends Router
{
    parameters:
        @display("i=block/switch,blue");
    submodules:
        pop: Logic;
    connections:
        for i=0..sizeof(in)-1 {
            pop.transmit++ --> out[i];
            pop.receive++ <-- in[i];
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
            core.transmit++ --> out[i];
            core.receive++ <-- in[i];
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
            beyond.transmit++ --> out[i];
            beyond.receive++ <-- in[i];
        }
}

simple User
{
    parameters:
        string name = default("");
        string loc = default("");
        int rn = default(-1);
        volatile double initIdleTime @unit(s);   // delay before first request
        volatile double idleTime @unit(s);   // delay before subsequent requests
        int cacheTries = default(2); // replicas to try before origin
        double requestSize = default(-1B) @unit(B); // negative bits: use tubes data
        @signal[idle](type="simtime");
        @signal[videoLength](type="long");
        @display("i=block/user");
    gates:
        inout testlink[];
        input in;
        output out;
        input directInput;
}
