packet MyPacket extends cPacket
{
    int sourceID;
    int destinationID;
    int customID; // internal video ID, not yt/daum's
    short cacheTries;
    short state;
    uint64_t bitsPending;
}

