#pragma once

typedef std::vector<cTopology::Node*> Path;
typedef std::vector<Path> PathList;
typedef int Priority;

struct Flow {
    Path path;
    // remember gates too?
    double bps;
    Priority priority; // larger number signifies higher priority eg. 5 overrides 3
    // MAYBE cMessage* completionTimer;
    // MAYBE uint64_t videoSize/videoLength;
    // MAYBE uint64_t transferred/completed;
    // MAYBE simtime_t lastTransferredUpdateTime; //(currentTime-lastupdateTime)*bitrate = additional transferred bits
    // not necessary? since higher qualities simulated using separate flows so each flow has a fixed bitrate (though aggregate varies)
};
