#pragma once
//#include "flowchannel.h"
#include <omnetpp.h>

typedef std::vector<cTopology::Node*> Path;
typedef std::vector<Path> PathList;
typedef int Priority;

class Flow {
public:
    Flow();
    virtual ~Flow();
    bool isActive() const;
    void setActive(bool active);
    const Path& getPath() const;
    void setPath(const Path& path);
    const std::vector<cChannel*>& getChannels();
    const simtime_t& getLag();
    uint64_t getBps() const;
    void setBps(uint64_t bps);
    uint64_t getBpsMin() const;
    void setBpsMin(uint64_t bpsMin);
    uint64_t getBitsLeft() const;
    void setBitsLeft(uint64_t bitsLeft);
    Priority getPriority() const;
    void setPriority(Priority priority);
protected:
    bool active; // whether flow is actually moving
    Path path;
    std::vector<cChannel*> channels;
    simtime_t lag; // round-trip-time / latency / lag / delay
    uint64_t bps;
    uint64_t bpsMin;
    uint64_t bits_left;
    Priority priority; // larger number signifies higher priority eg. 5 overrides 3
    // MAYBE cMessage* completionTimer;
    // MAYBE uint64_t transferred/completed;
    // data maintenance:
    bool updated;
    simtime_t last_updated;
    void update();
    void updateChannels();
    void updateLag();
};
