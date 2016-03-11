struct Video {
    int id;
    simtime_t duration;
    std::vector<uint64_t> bitRates;
    std::vector<short> resolutions;
};

extern const uint64_t bitRate;
void loadVideoLengthFile();
//uint64_t getVideoSize();
unsigned long getMaxCustomVideoID();
int getRandCustomVideoID();
uint64_t getVideoSeconds(int customID);
std::vector<std::string> getVideoResolutions(int customID);
uint64_t getVideoBitSize(int customID);
uint64_t getBitRate(int customID, std::string resolution);
std::vector<uint64_t> getBitRates(int customID);
int getQualities(int customID); // number of quality levels
