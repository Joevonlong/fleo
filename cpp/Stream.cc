/*
 * Stream.cc
 *
 *  Created on: Sep 10, 2015
 *      Author: gang
 */

#include <Stream.h>

Stream::Stream() {
    elapsed = 0;
}

Stream::~Stream() {
    // TODO Auto-generated destructor stub
}

const simtime_t& Stream::getViewtime() {
    if (!updated) {
        update();
    }
    return viewtime;
}

void Stream::setViewtime(const simtime_t& vt) {
    viewtime = vt;
    elapsed = 0;
    remaining = vt;
}

void Stream::update() {
    updated = true;
}
