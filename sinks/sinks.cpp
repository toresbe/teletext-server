#include "sinks/sinks.hpp"
#include "sinks/tcpdumper.cpp"
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <iostream>

class SinkFactory {
    public:
    static ttxSink * get_sink(std::string requested_sink) {
        ttxSink * selected_sink;

		if (requested_sink == "debug") {
			selected_sink = new DebugSink();
#ifndef _WIN32
		}
		else if (requested_sink == "shm") {
			selected_sink = new ShmSink();
#endif
		} else if (requested_sink == "tcpdumper") {
				selected_sink = new TcpSink();
        } else {
            selected_sink = nullptr;
            BOOST_LOG_TRIVIAL(error) << "Configured sink is not implemented!";
            throw "sink requested by config file is not implemented!";
        }

        return selected_sink;
    }
};