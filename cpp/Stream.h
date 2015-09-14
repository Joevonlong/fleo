/*
 * Stream.h
 *
 *  Created on: Sep 10, 2015
 *      Author: gang
 */

#ifndef STREAM_H_
#define STREAM_H_

#include <Download.h>

class Stream: public Download {
public:
    Stream();
    virtual ~Stream();
    const simtime_t& getViewtime();
    void setViewtime(const simtime_t& viewtime);
protected:
    simtime_t viewtime;
    simtime_t elapsed;
    simtime_t remaining;
    // data maintenance:
    void update();
};

#endif /* STREAM_H_ */
