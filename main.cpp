#include <stdio.h>
#include <iostream>
#include <array>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "ttxdata.cpp"
#include "parity.cpp"
#include "encoder.cpp"

namespace logging = boost::log;
namespace fs = boost::filesystem;

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
        void const print_header(ttxPageAddress_p const & addr) {
            printf("[00] P%3s %34s\n", addr->to_str().c_str(), "Page dumper");
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

int main() {
    logging::core::get()->set_filter
        (
         logging::trivial::severity >= logging::trivial::error
        );
    ttxStaticPageSet_p page_set = std::make_shared<ttxStaticPageSet>(boost::filesystem::path("pages"));
    ttxCarousel carousel;
    TeletextEncoder encoder;

    carousel.attach(page_set);

    int i=0;

    while(1) {
        for (auto packet: encoder.encode(carousel.get_next_page_entry())) {
            i = 0;
            for (auto ch: *packet) {
                if(i++ >= 3)
                    putchar(ch);
            }
        }
    }

    return 0;
}
