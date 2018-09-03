#include "config.hpp"
#include "editserver.hpp"
#include "ttxdata.hpp"
#include "encoder.hpp"
#include "persist.hpp"
#include <list>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include "sinks/sinks.cpp"
#include <boost/thread.hpp>
#include "editserver/editserver.cpp"
ttxCarousel carousel;

int main() {
    BOOST_LOG_TRIVIAL(info) << "Starting teletext server";
    config::read_file("ttx.cfg");

    ttxSink * sink = SinkFactory::get_sink(config::get_value<std::string>("sink")); 
    boost::thread edit_thread(EditServerStart, config::get_value<int>("edit_port"));

    sink->start();
    edit_thread.join();
}
