#ifndef __SINKS_HPP
#define __SINKS_HPP

class ttxSink {
    public:
    virtual void start() = 0;
};

class DebugSink: public ttxSink {
    void start();
};

#ifndef _WIN32
class ShmSink: public ttxSink {
    void start();
};
#endif

#endif
