#include <stdio.h>
#include <string.h>
#include <fstream>
#include <omnetpp.h>
#include "parse.h"

const uint64_t bitRate = 800000;

unsigned long arraySize;
int viewsTotal;
std::vector<std::string> videoIDs;
uint64_t* lengths;
int* cumulativeViews;

void loadVideoLengthFile() {
    unsigned long tmp;
    std::ifstream freqFile; // default: input from file
    freqFile.open("datasets/freqs_yt_sci.txt");

    // parse first line (metadata)
    freqFile >> arraySize >> tmp >> viewsTotal;
    videoIDs = std::vector<std::string>(arraySize);
    lengths = new uint64_t[arraySize];
    cumulativeViews = new int[arraySize];

    // parse subsequent lines
    unsigned long i = 0;
    while (freqFile >> videoIDs[i] >> lengths[i] >> tmp) {
        //EV << lengths[i] << '\t' << cumulativeViews[i] << endl;
        // keep rolling sum to avoid repeated subtractions in getVideoSize()
        if (i != 0) {
            cumulativeViews[i] = cumulativeViews[i-1] + tmp;
        }
        else {
            cumulativeViews[i] = tmp;
        }
        i++;
    }
    freqFile.close();
}

uint64_t getVideoSize() {
    int i = 0;
    // intuniform returns SIM_API int which is signed 32 bits. If total views
    // exceed 2.1 bil this would fail.
    // YT ent has 3.7 bil views. could try to hack in unsigned int or move to
    // different randomiser.
    int view = intuniform(1, viewsTotal); // crashes if unsigned
    while (view >= cumulativeViews[i]) {
        i++;
    }
    if (lengths[i] >= UINT64_MAX/bitRate) { // 2^64/800k
        return UINT64_MAX;
    }
    else {
        return lengths[i]*bitRate;
    }
}

int getRandCustomVideoID() {
    int i = 0;
    // intuniform returns SIM_API int which is signed 32 bits. If total views
    // exceed 2.1 bil this would fail.
    // YT ent has 3.7 bil views. could try to hack in unsigned int or move to
    // different randomiser.
    int view = intuniform(1, viewsTotal); // crashes if unsigned
    while (view >= cumulativeViews[i]) {
        i++;
    }
    return i;
}

unsigned long getMaxCustomVideoID() {
    return arraySize;
}

uint64_t getVideoSeconds(int customID) {
    return lengths[customID];
}

std::vector<std::string> getVideoResolutions(int customID) {
    std::vector<std::string> resolutions;
    resolutions.push_back("360p");
    resolutions.push_back("480p");
    resolutions.push_back("720p");
    return resolutions;
}

uint64_t getVideoBitSize(int customID) {
    return lengths[customID]*bitRate;
}

uint64_t getBitRate(int customID, std::string resolution) {
    // should return additional bitrate due to given quality only
    // ie. does not include bitrate of lower qualities
    // to expand on in future...
    if (resolution == "360p") {return bitRate;}
    if (resolution == "480p") {return bitRate*2;}
    if (resolution == "720p") {return bitRate*4;}
    EV << "getBitRate: unrecognised resolution: " << resolution << endl;
    throw cRuntimeError("");
    return 0;
}

std::vector<uint64_t> getBitRates(int customID) {
    // Each bitrate past the first is the additional, not cumulative,
    // bitrate nequired for the next quality.
    // Using magic numbers taken from YouTube for now...
    std::vector<uint64_t> bitRates;
    bitRates.push_back(800000); // ~360p
    bitRates.push_back(800000); // ~480p-360p
    bitRates.push_back(1600000); // ~720p-480p
    return bitRates;
}

int getQualities(int customID) {
    // number of quality levels
    return 3;
}
