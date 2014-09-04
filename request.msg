packet Request extends cPacket
{
    int customID; // internal video ID, not yt/daum's
    uint64_t size; // in bytes
    string source;
    string destination;
}

