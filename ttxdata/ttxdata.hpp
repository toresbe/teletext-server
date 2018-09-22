#pragma once
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/iterator.hpp>
#include <boost/log/trivial.hpp>
#include <stdio.h>
#include <vector>
#include <string.h>
#include <iostream>
#include <stdexcept>
#include <map>
#include <fstream>
#include <assert.h>
#include <array>
#include <mutex>

typedef	std::array <uint8_t, 40> ttxLineData;

#define LINES_PER_FIELD 10
#define FIELDS_PER_BUFFER 1000
#define LINES_PER_BUFFER LINES_PER_FIELD * FIELDS_PER_BUFFER

class ttxLine {
    private:
    public:
        ttxLineData data;

        // TODO: Can this be changed to ttxLineData *
        const uint8_t * get_line() const;

        friend std::istream& operator >>(std::istream & sb, ttxLine &line);

        friend void operator >>(std::string & sb, ttxLine &line);

        uint8_t &operator[](unsigned int i);
};

typedef uint8_t                                         ttxLineNumber;
typedef std::shared_ptr<ttxLine>                        ttxLine_p;
typedef std::map<ttxLineNumber, ttxLine_p>              ttxPageLines;

class ttxPageAddress {
    private:
        int magazine;
        int page_number;

        void parse_address(const std::string & addr_str);
    public:
        friend std::ostream & operator<<(std::ostream &os, const ttxPageAddress& p);

        std::string to_str() const;

        const int get_page_number() const;

        const int get_magazine() const;

        const int to_int() const;

        bool operator< (const ttxPageAddress & addr) const;

        ttxPageAddress(const std::string & addr_str);
};

typedef std::shared_ptr<ttxPageAddress>                 ttxPageAddress_p;
typedef std::vector<ttxPageAddress_p>                   ttxPageAddress_vector;

class ttxPage {
    private:

    public:
        ttxPageLines lines;

        ttxLine_p get_line(ttxLineNumber line_num);

        ttxLine_p get_line(int index);

        ttxLine_p new_line(ttxLineNumber line_num);

        ttxLine_p &operator[](ttxLineNumber i);


};

typedef std::shared_ptr<ttxPage>                ttxPage_p;
typedef std::map<ttxPageAddress, ttxPage_p>		ttxPageEntry_map;
typedef std::pair<ttxPageAddress, ttxPage_p>	ttxPageEntry;
typedef std::shared_ptr<ttxPageEntry>           ttxPageEntry_p;
typedef std::pair<ttxLineNumber, ttxLine_p>     ttxNumberedLine;
typedef std::shared_ptr<ttxNumberedLine>        ttxNumberedLine_p;

class ttxDatastore {
    private:
        ttxDatastore();
        ttxPageEntry_map							page_list;
        ttxPageEntry_map::iterator					page_list_iterator;
        std::mutex									global_lock;

    public:
		typedef std::shared_ptr<ttxDatastore> pointer;
        static ttxDatastore * get_instance();
        ttxDatastore(const ttxDatastore&) = delete;
        ttxDatastore& operator=(const ttxDatastore&) = delete;
        void	update_page_line(const ttxPageAddress & addr,
                const ttxLineNumber & line_num,
                const ttxLineData & line_data);
        ttxPage_p	get_page(const ttxPageAddress & addr);

        void attach(const ttxPageEntry & new_page);

        ttxPageEntry get_next_page_entry();

        ttxPageEntry_p  operator*();
        ttxPageEntry_p  operator++(int);
};