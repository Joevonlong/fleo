extern const uint64_t bitRate;
void loadVideoLengthFile();
//uint64_t getVideoSize();
unsigned long getMaxCustomVideoID();
int getRandCustomVideoID();
uint64_t getVideoSeconds(int customID);
uint64_t getVideoBitSize(int customID);
uint64_t getBitRate(int customID, int quality);
int getQualities(int customID); // number of quality levels
