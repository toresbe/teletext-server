#include "ttxdata/parity.hpp"
#include "ttxdata/ttxdata.hpp"
#include "ttxdata/encoder.hpp"
#include "sinks/shm_common.h"
#include "sinks.hpp"

ttx_shm_buffer_file * shm_buffer;

class ttxTardisCarousel {
    public:
        ttxTardisCarousel() {
            datastore = ttxDatastore::get_instance();
            current_line = 0;
        }

        void init(timecode_t timecode) {
            //current_page = datastore->get_next_page_entry();
            current_line = 0;
        }

        ttxPacket_p yield_packet() {
        }

        // Yield a field's worth of lines
        ttxPacket_p_list yield_field() {

        }

    private:
        ttxDatastore * datastore;
        int current_line;
        ttxPageEntry_p current_page;
};

void ShmSink::start() {
    // todo: read this from config file
    shm_buffer = open_shm_file((char *)"frikanalen");

    ttxDatastore * datastore = ttxDatastore::get_instance();
    // wait on semaphore that can be cleared by a timer,
    // a page change, or a quit request
    //
    // if we're quitting, quit
    //
    // get the back buffer address
    // for each remaining field in buffer
    // if there is, insert the next page
    //
    // flip buffers
    //while(1) {
    //    auto page = datastore->get_next_page_entry();
    //}
}
