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
    freqFile.open("freqs_yt_sci.txt");

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
