network OneToOne
{
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
            @display("i=block/routing,green");
        }
        user: User {
            name = "user";
            loc = "here";
            rn = 3;
        }
    connections allowunconnected:
        user.out --> FlowChannel {  delay = 1ms; datarate = 1Mbps; } --> beyond.in++;
        user.in <-- FlowChannel {  delay = 1ms; datarate = 1Mbps; } <-- beyond.out++;
}
