#include <stdio.h>
#include <string.h>
#include <fstream>
#include <algorithm>
#include <omnetpp.h>
#include "parse.h"

const uint64_t bitRate = 800000;

std::vector<VideoMetadata*> allMeta;

unsigned long arraySize;
int viewsTotal;
std::vector<std::string> videoIDs;
uint64_t* lengths;
int* cumulativeViews;

void loadVideoLengthFile() {
    unsigned long views;
    std::ifstream freqFile; // default: input from file
    freqFile.open("datasets/freqs_yt_sci.txt");

    // parse first line (metadata)
    freqFile >> arraySize >> views >> viewsTotal; // views used as temp var here
    videoIDs = std::vector<std::string>(arraySize);
    lengths = new uint64_t[arraySize];
    cumulativeViews = new int[arraySize];
    allMeta.reserve(arraySize);

    // parse subsequent lines
    unsigned long i = 0;
    while (freqFile >> videoIDs[i] >> lengths[i] >> views) {
        VideoMetadata* vmd = new VideoMetadata;
        vmd->customID = i;
        vmd->name = videoIDs[i];
        vmd->duration = lengths[i];
        vmd->bitRates.push_back(bitRate); vmd->bitRates.push_back(bitRate); vmd->bitRates.push_back(bitRate*2);
        vmd->resolutions.push_back("360p"); vmd->resolutions.push_back("480p"); vmd->resolutions.push_back("720p");
        allMeta[i] = vmd;
        // keep rolling sum to avoid repeated subtractions in getVideoSize()
        if (i != 0) {
            cumulativeViews[i] = cumulativeViews[i-1] + views;
        }
        else {
            cumulativeViews[i] = views;
        }
        i++;
    }
    if (i > arraySize) {
        throw cRuntimeError("loadVideoLengthFile: More video entries than expected");
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
    // intuniform returns SIM_API int which is signed 32 bits. If total views
    // exceed 2.1 bil this would fail.
    // YT ent has 3.7 bil views. could try to hack in unsigned int or move to
    // different randomiser.
    int view = intuniform(1, viewsTotal); // crashes if unsigned
    int i;
    for (i = 0; cumulativeViews[i] <= view; ++i) {}
    return i;
}

unsigned long getMaxCustomVideoID() {
    return arraySize;
}

uint64_t getVideoSeconds(int customID) {
    return lengths[customID];
}

std::vector<std::string> getVideoResolutions(int customID) {
    return allMeta[customID]->resolutions;
}

/**
 * BitSize of all qualities.
 */
uint64_t getVideoBitSize(int customID) {
    uint64_t totalBitRate = 0;
    for (size_t i = 0; i < allMeta[customID]->bitRates.size(); ++i) {
        totalBitRate += allMeta[customID]->bitRates[i];
    }
    return allMeta[customID]->duration.dbl() * totalBitRate;
}

uint64_t getAdditionalVideoBitSize(int customID, std::string resolution) {
    return allMeta[customID]->duration.dbl() * getAdditionalBitRate(customID, resolution);
}

uint64_t getAdditionalBitRate(int customID, std::string resolution) {
    // should return additional bitrate due to given quality only
    // ie. does not include bitrate of lower qualities
    // to expand on in future...
    for (size_t i = 0; i < allMeta[customID]->resolutions.size(); ++i) {
        if (allMeta[customID]->resolutions[i] == resolution) {
            return allMeta[customID]->bitRates[i];
        }
    }
    EV << "getBitRate: unrecognised resolution: " << resolution << endl;
    throw cRuntimeError("");
    return 0;
}

/**
 * Each bitrate past the first is the additional, not cumulative,
 * bitrate nequired for the next quality.
 * Using magic numbers taken from YouTube for now...
 */
std::vector<uint64_t> getBitRates(int customID) {
    return allMeta[customID]->bitRates;
}

int getQualities(int customID) {
    // number of quality levels
    return allMeta[customID]->resolutions.size();
}
