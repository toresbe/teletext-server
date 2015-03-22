#include "config.hpp"
#include "editserver.hpp"
#include "ttxdata.hpp"
#include "encoder.hpp"
#include "persist.hpp"
#include <list>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
ttxCarousel carousel;


int main() {
	BOOST_LOG_TRIVIAL(info) << "Starting teletext server";
	config::read_file("C:\\Users\\ToreSinding\\Documents\\projects\\teletext-server\\ttx.cfg");
	editserver::start();
	
	int i = 0;


	/*
	while (1) {
		for (auto packet : ttxEncode::encode_page_entry(carousel.get_next_page_entry())) {
			i = 0;
			for (auto ch : *packet) {
				if (i++ >= 3);
					//putchar(ch);
			}
		}
	}
	*/
	system("pause");
}

#ifdef LOL
#include <stdio.h>
#include <iostream>
#include <array>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include <boost/tokenizer.hpp>

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

int maine() {
    logging::core::get()->set_filter
        (
         logging::trivial::severity >= logging::trivial::error
        );


    return 0;
}

#endif