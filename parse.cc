#include <stdio.h>
#include <string.h>
#include <fstream>
#include <omnetpp.h>
#include "parse.h"

const uint64_t bitRate = 800000;

unsigned long arraySize;
int viewsTotal;
uint64_t* lengths;
int* views;

void loadVideoLengthFile() {
    std::string line;
    unsigned long tmp;
    std::ifstream freqFile; // default: input from file
    freqFile.open("yt_sci_freqs.txt");

//    parse first line
    freqFile >> arraySize >> tmp >> viewsTotal;
    lengths = new uint64_t[arraySize];
    views = new int[arraySize];

    unsigned long i = 0;
    while (freqFile >> lengths[i] >> views[i]) {
        //EV << lengths[i] << '\t' << views[i] << endl;
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
    while (view >= 0) {
        view -= views[i];
        i++;
    }
    i--;
    if (lengths[i] >= UINT64_MAX/bitRate) { // 2^64/800k
        return UINT64_MAX;
    }
    else {
        return lengths[i]*bitRate;
    }
}
