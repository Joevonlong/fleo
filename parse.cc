#include <stdio.h>
#include <string.h>
#include <fstream>
#include <omnetpp.h>
#include "parse.h"

unsigned long arraySize;
int viewsTotal;
int64* lengths;
unsigned long* views;

void loadVideoLengthFile() {
    std::string line;
    unsigned long tmp;
    std::ifstream daumFood; // default: input from file
    daumFood.open("daum_food_freqs.txt");

//    getline(daumFood, line);
//    parse first line
    daumFood >> arraySize >> tmp >> viewsTotal;
//    EV << viewsTotal << '\n';
    lengths = new int64[arraySize];
    views = new unsigned long[arraySize];

    unsigned long i = 0;
    while (daumFood >> lengths[i] >> views[i]) {
        //EV << lengths[i] << '\t' << views[i] << endl;
        i++;
    }
    daumFood.close();
}

int64 getVideoSize() {
    int i = 0;
    int view = intuniform(1, viewsTotal);
    while (view >= 0) {
        view -= views[i];
        i++;
    }
    i--;
    if (lengths[i] > (int64)11529215046068) { // 2^63/800k
        return ((int64)1<<62)-1;
    }
    else {
        return 800*1000*lengths[i];
    }
}
