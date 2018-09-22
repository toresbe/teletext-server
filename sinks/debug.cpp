#include <stdio.h>
#include <iostream>
#include <array>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <boost/tokenizer.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "ttxdata/ttxdata.hpp"
#include "ttxdata/parity.hpp"
#include "ttxdata/encoder.hpp"
#include "sinks/sinks.hpp"

namespace logging = boost::log;

class TrivialPageDumper {
    public:
        static void dump(ttxPageEntry * page) {
            print_header(page->first);

            for (auto entry: page->second->lines) {
                printf("[%02d] \x1b[37m", entry.first);
                dump_line(entry.second);
                printf("\x1b[0m");
            }
            printf("\x1b[30;0H");
        }
    private:
        static void const print_header(const ttxPageAddress & addr) {
            printf("\x1b[0;0H");
            printf("[00] P%3s %34s\n", addr.to_str().c_str(), "Page dumper");
        }

        static inline void set_color(const uint8_t & ch) {
                printf("\x1b[3%dm", ch);
        }

        static inline bool is_color(const uint8_t & ch) {
            if( (ch >= 0) && (ch <= 7) )
                return true;
            return false;
        }

        static void put_char(const uint8_t & ch) {
                if (isprint(ch)) std::cout << ch;
                else if (is_color(ch)) {
                    set_color(ch);
                }
                else std::cout << '.';
        }

        static void const dump_line(ttxLine_p const & line) {
            uint8_t ch;

            for (int i = 0; i < sizeof(line->data); i++) {
                put_char(line->data[i]);
            }

            std::cout << '\n';
        }
};

void DebugSink::start() {
    BOOST_LOG_TRIVIAL(info) << "Starting debug sink";
    ttxDatastore * datastore = ttxDatastore::get_instance();

    while(1) {
        auto page = datastore->get_next_page_entry();
        TrivialPageDumper::dump(&page);
    }
}
