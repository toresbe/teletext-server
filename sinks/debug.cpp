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

#include "ttxdata.hpp"
#include "parity.hpp"
#include "encoder.hpp"
#include "sinks/sinks.hpp"

namespace logging = boost::log;

class TrivialPageDumper {
    public:
        TrivialPageDumper(ttxPageEntry_p const & page) {
            print_header(page->first);

            for (auto entry: page->second->lines) {
                printf("[%02d] ", entry.first);
                dump_line(entry.second);
            }
        }
    private:
        void const print_header(const ttxPageAddress & addr) {
            printf("[00] P%3s %34s\n", addr.to_str().c_str(), "Page dumper");
        }

        void const dump_line(ttxLine_p const & line) {
            uint8_t ch;

            for (int i = 0; i < sizeof(line->data); i++) {
                ch = line->data[i];
                if (isprint(ch)) std::cout << ch;
                else std::cout << '.';
            }

            std::cout << '\n';
        }
};

void DebugSink::start() {
    BOOST_LOG_TRIVIAL(info) << "Starting debug sink";
}
