packet Request
{
    int customID; // internal video ID, not yt/daum's
    //uint64_t size; // in bytes
    int sourceID;
    int destinationID; // used to track waypoints
}
