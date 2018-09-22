#include "config.hpp"
#include "ttxdata/ttxdata.hpp"
#include "ttxdata/encoder.hpp"
#include "persist.hpp"
#include <list>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include "sinks/sinks.cpp"
#include <boost/thread.hpp>
#include "editserver/editserver.hpp"
#include <boost/bind.hpp>
int main() {
    BOOST_LOG_TRIVIAL(info) << "Starting teletext server";
    config::read_file("ttx.cfg");

    ttxDatastore * datastore = ttxDatastore::get_instance();

    for (auto page_entry : ttxPersist::load_directory("pages")) {
        datastore->attach(page_entry);
    }

	ttxSink * sink = SinkFactory::get_sink(config::get_value<std::string>("sink"));
	boost::thread dump_thread(boost::bind(&ttxSink::start, sink));
	boost::thread edit_thread(EditServerStart, config::get_value<int>("edit_port"));

    edit_thread.join();
	dump_thread.join();
}
