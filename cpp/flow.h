typedef std::vector<cTopology::Node*> Path;
typedef std::vector<Path> PathList;

struct Flow {
    Path path;
    // remember gates too?
    double bps;
    int priority;
    // MAYBE cMessage* completionTimer;
    // MAYBE uint64_t videoSize/videoLength;
    // MAYBE uint64_t transferred/completed;
    // MAYBE simtime_t lastTransferredUpdateTime; //(currentTime-lastupdateTime)*bitrate = additional transferred bits
    // not necessary? since higher qualities simulated using separate flows so each flow has a fixed bitrate (though aggregate varies)
};
