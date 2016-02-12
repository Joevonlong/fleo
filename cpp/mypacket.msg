packet MyPacket
{
    int sourceID;
    int destinationID;
    short hops;
    int customID; // internal video ID, not yt/daum's
    long videoLength; // total
    long videoSegmentLength; // 30s or less
    short videoSegmentsPending; // ceil(length/30)
    long videoLengthPending; // remaining
    short cacheTries;
    short state;
    uint64_t bitsPending;
}
