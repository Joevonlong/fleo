packet MyPacket extends cPacket
{
    int sourceID;
    int destinationID;
    int customID; // internal video ID, not yt/daum's
    int cacheTries;
    short state;
    uint64_t bitsPending;
}

