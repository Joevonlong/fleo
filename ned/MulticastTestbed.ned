network MulticastTestbed
{
    parameters:
        int fanout = default(2);
    types:
    submodules:
        global: Global;
    	controller: Controller;
        beyond: Beyond;
        core: Core;
        pop[fanout]: PoP;
        user[fanout^2]: User;
    connections:
        beyond.out++ --> Cat5 --> core.in++;
        beyond.in++ <-- Cat5 <-- core.out++;
        for i=0..fanout-1, for j=(i*fanout)..(i+1)*fanout-1 {
            core.out++ --> Cat5 --> pop[j].in++;
            core.in++ <-- Cat5 <-- pop[j].out++;
        };
        for i=0..fanout^2-1, for j=(i*fanout)..(i+1)*fanout-1 {
            pop[i].out++ --> Cat5 --> user[j].in;
            pop[i].in++ <-- Cat5 <-- user[j].out;
        };
}
