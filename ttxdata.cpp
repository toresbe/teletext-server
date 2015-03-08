#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/iterator.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <stdexcept>
#include <map>
#include <fstream>
#include <assert.h>
#include <array>

namespace fs = boost::filesystem;

class ttxLine {
    private:
    public:
        std::array <uint8_t, 40> data;

        const uint8_t * get_line() const {
            return &data[0];
        }

        friend std::istream& operator >>(std::istream & sb, ttxLine &line) {
            sb.read((char *)&line.data[0], line.data.size());

            if (sb.gcount() != line.data.size()) {
                throw std::invalid_argument("Failed to read");
            }

            return sb;
        }

        void friend operator >>(std::string & sb, ttxLine &line) {
            auto len = (sb.length() > 40) ? 40 : sb.length();
            memcpy((char *)&line.data[0], sb.c_str(), sb.length()-1);
        }

        uint8_t &operator[](int i) {
            if (i > data.size()) {
                throw std::out_of_range("Out of bands read from ttxLine");
            }
            else {
                return data[i];
            }
        }
};

typedef uint8_t                                         ttxLineNumber;
typedef std::shared_ptr<ttxLine>                        ttxLine_p;
typedef std::map<ttxLineNumber, ttxLine_p>              ttxPageLines;

class ttxPageAddress {
    private:
        int magazine;
        int page_number;

        void parse_address(const std::string & addr_str) {
            // todo: this might be done more cleanly with boost qi?

            if (sscanf(addr_str.c_str(), "%1u%2u", &magazine, &page_number) != 2) {
                throw std::invalid_argument("'" + addr_str + "' is not a valid page address");
            }
        }

    public:
        friend std::ostream & operator<<(std::ostream &os, const ttxPageAddress& p) {
            return os << p.to_str();
        }

        std::string to_str() const {
            return std::to_string(to_int());
        }

        const int get_page_number() const {
            return page_number;
        }

        const int get_magazine() const {
            return magazine;
        }

        const int to_int() const {
            int pgnum;

            if (!magazine) pgnum = 800;
            else pgnum = (magazine * 100);

            pgnum += page_number;

            return pgnum;
        }

        bool operator< (const ttxPageAddress & addr) const {
            return (to_int() < addr.to_int());
        }

        ttxPageAddress(const std::string & addr_str) {
            parse_address(addr_str);
        }
};

typedef std::shared_ptr<ttxPageAddress>                 ttxPageAddress_p;
typedef std::vector<ttxPageAddress_p>                   ttxPageAddress_vector;

class ttxPage {
    private:

    public:
        ttxPageLines lines;

        ttxLine_p get_line(ttxLineNumber line_num) {
            return ttxLine_p(lines.at(line_num));
        }

        ttxLine_p get_line(int index) {
            int num_lines = 0;
            ttxLine_p ptr;

            if (index > num_lines) {
                throw std::out_of_range("Attempt to access line out of range");
            }

            // Returning null simply means no line found here, try next.
            try {
                auto x = lines.at((ttxLineNumber)index);
                return x;
            }
            catch (std::out_of_range) {
                return NULL;
            }
        }

        ttxLine_p new_line(ttxLineNumber line_num) {
            if (lines.count(line_num)) delete lines[line_num].get();
            auto line = std::make_shared<ttxLine>();
            lines[line_num] = line;
            return line;
        }

        ttxLine_p &operator[](ttxLineNumber i) {
            if (i > sizeof(lines)) {
                throw std::out_of_range("Out of bands read from ttxPage");
            }
            else {
                return lines[i];
            }
        }


};

typedef std::shared_ptr<ttxPage>                ttxPage_p;
typedef std::map<ttxPageAddress_p, ttxPage_p>   ttxPageEntryList;
typedef std::pair<ttxPageAddress_p, ttxPage_p>  ttxPageEntry;
typedef std::shared_ptr<ttxPageEntry>           ttxPageEntry_p;
typedef std::pair<ttxLineNumber, ttxLine_p>     ttxNumberedLine;
typedef std::shared_ptr<ttxNumberedLine>        ttxNumberedLine_p;

class ttxFilePage : public ttxPage {
    // A ttxPage loaded from, and persisted to a file.

    private:
        uint8_t line_num;

    public:
        ttxFilePage(const fs::path & path) {
            BOOST_LOG_TRIVIAL(info) << "Loading teletext page from file: '" + path.string() + "'";

            //fixme: case sensitive
            if (path.extension() == ".tti") { LoadTTI(path.string()); }
            else {
                std::ifstream infile;
                line_num = 1;

                infile.open(path.string().c_str(), std::ios::binary);

                assert(infile.is_open());

                while (infile.peek() != EOF) {
                    infile >> (*new_line(line_num++));
                }
            }
        };

        bool LoadTTI(const std::string & filename)
        {

            const std::string cmd[] = { "DS", "SP", "DE", "CT", "PN", "SC", "PS", "MS", "OL", "FL", "RD", "RE" };
            const int cmdCount = 12; // There are 12 possible commands, maybe DT and RT too on really old files
            unsigned int lineNumber = 1;
            int lines = 0;
            // Open the file
            std::ifstream filein;

            filein.open(filename.c_str(), std::ios::binary);

            if (!filein.is_open()) {
                std::string error = std::string("Error opening TTI file '") + filename + std::string("':\n") + std::string(strerror(errno));
                //throw std::exception(error.c_str());
            }


            for (std::string line; std::getline(filein, line, ',');)
            {
                // This shows the command code:
                //std::cout << line << std::endl;
                bool found = false;
                for (int i = 0; i < cmdCount && !found; i++)
                {
                    // std::cout << "matching " << line << std::endl;
                    if (!line.compare(cmd[i]))
                    {
                        found = true;
                        // std::cout << "Matched " << line << std::endl;
                        switch (i)
                        {
                            case 0: // "DS" - Destination inserter name
                                // DS,inserter
                                // std::cout << "DS not implemented\n";
                                std::getline(filein, line);
                                // std::cout << "DS read " << m_destination << std::endl;
                                break;
                            case 1: // "SP" - Source page file name
                                // SP is the path + name of the file from where is was loaded. Used also for Save.
                                // SP,c:\Minited\inserter\ONAIR\P100.tti
                                //std::cout << "SP not implemented\n";

                                std::getline(filein, line);
                                // std::getline(filein, m_sourcepage);
                                break;
                            case 2: // "DE" - Description
                                // DE,Read back page  20/11/07
                                //std::cout << "DE not implemented\n";
                                std::getline(filein, line);
                                break;
                            case 3: // "CT" - Cycle time (seconds)
                                // CT,8,T
                                // std::cout << "CT not implemented\n";
                                std::getline(filein, line, ',');
                                //m_cycletimeseconds = atoi(line.c_str());
                                std::getline(filein, line);
                                //m_cycletimetype = line[0] == 'T' ? 'T' : 'C';
                                // TODO: CT is not decoded correctly
                                break;
                            case 4: // "PN" - Page Number mppss
                                // PN,10000
                                std::getline(filein, line);
                                //pageNumber = std::strtol(line.c_str(), &ptr, 16);
                                // std::cout << "PN enters with m_PageNumber=" << m_PageNumber << " param=" << pageNumber << std::endl;
                                //if (p->m_PageNumber != FIRSTPAGE) // // Subsequent pages need new page instances
                                //{
                                // std::cout << "Created a new subpage" << std::endl;
                                //      TTXPage* newSubPage = new TTXPage();  // Create a new instance for the subpage
                                //      p->Setm_SubPage(newSubPage);            // Put in a link to it
                                //      p = newSubPage;                       // And jump to the next subpage ready to populate
                                //}
                                //p->SetPageNumber(pageNumber);

                                // std::cout << "PN =" << std::hex << m_PageNumber << "\n";
                                //if (m_PageNumber)
                                //    std::cout << "new page. TBA\n";
                                break;
                            case 5: // "SC" - Subcode
                                // SC,0000
                                std::getline(filein, line);
                                //subcode = std::strtol(line.c_str(), &ptr, 16);
                                //std::cout << "SC: Subcode=" << subcode << std::endl;;

                                //                                              p->SetSubCode(subcode);
                                break;
                            case 6: // "PS" - Page status flags
                                // PS,8000
                                std::getline(filein, line);
                                //m_pagestatus = std::strtol(line.c_str(), &ptr, 16);
                                // Don't copy the bits to the UI...
                                // because this may not be the root page.
                                break;
                            case 7: // "MS" - Mask
                                // MS,0
                                // std::cout << "MS not implemented\n";
                                std::getline(filein, line);
                                break;
                            case 8: // "OL" - Output line
                                // OL,9,A-Z INDEX     199NEWS HEADLINES  101
                                std::getline(filein, line, ',');
                                lineNumber = atoi(line.c_str());
                                std::getline(filein, line);

                                line >> (*new_line(lineNumber));

                                //if (lineNumber>24) break;
                                // std::cout << "reading " << lineNumber << std::endl;
                                //p->m_pLine[lineNumber] = new TTXLine(line);
                                // TODO: Change this implementation to use SetRow
                                // std::cout << lineNumber << ": OL partly implemented. " << line << std::endl;
                                lines++;
                                break;
                            case 9: // "FL"; - Fastext links
                                // FL,104,104,105,106,F,100
                                // std::cout << "FL not implemented\n";
                                for (int fli = 0; fli < 6; fli++)
                                {
                                    std::getline(filein, line, ',');
                                    //SetFastextLink(fli, std::strtol(line.c_str(), &ptr, 16));
                                }
                                break;
                            case 10: // "RD"; - not sure!
                                std::getline(filein, line);
                                break;
                            case 11: // "RE"; - Set page region code 0..f
                                std::getline(filein, line); // TODO: Implement this
                                //m_region = std::strtol(line.c_str(), &ptr, 16);
                                break;
                            default:
                                std::cout << "Command not understood " << line << std::endl;
                        } // switch
                    } // if matched command
                    // If the command was not found then skip the rest of the line
                } // seek command
                if (!found) std::getline(filein, line);
            }
            filein.close(); // Not sure that we need to close it
            //p->Setm_SubPage(NULL);
            //std::cout << "Finished reading TTI page. Line count=" << lines << std::endl;
            return true;
        }
};

typedef std::shared_ptr<ttxFilePage>                    ttxFilePage_p;

typedef int TTX_PAGESET_PRIORITY;

class ttxPageSet {
    public:
        virtual ttxPage_p const                         get_page(ttxPageAddress_p const & addr) = 0;
        virtual ttxPageEntry_p const                    get_page_entry(ttxPageAddress_p const & addr) = 0;
        virtual ttxPageAddress_vector  const            get_page_addresses(ttxPageAddress_vector & list) = 0;

        TTX_PAGESET_PRIORITY                            get_priority() { return 0; }
};

typedef std::shared_ptr<ttxPageSet>     ttxPageSet_p;

class ttxStaticPageSet : public ttxPageSet {
    private:
        ttxPageEntryList                                page_list;

        void enter_page(ttxPageAddress_p addr, ttxPage_p page) { page_list.emplace(addr, page); };
    public:
        ttxStaticPageSet(const fs::path & dir) {
            BOOST_LOG_TRIVIAL(info) << "Loading teletext pages from directory: '" + dir.string() + "'";

            assert(fs::exists(dir));

            ttxPageAddress_p addr;
            ttxFilePage_p page;

            for (auto ent : boost::make_iterator_range(fs::directory_iterator(dir), {})) {
                if (ent.path().extension() == ".ttx") {
                    try {
                        // Is the filename a valid page address?
                        addr = std::make_shared<ttxPageAddress>(ent.path().stem().string());
                    }
                    catch (std::invalid_argument) {
                        BOOST_LOG_TRIVIAL(warning) << "Skipping invalid filename: " << ent.path().filename();
                        continue;
                    }

                    try {
                        page = std::make_shared<ttxFilePage>(ent);
                    }
                    catch (std::invalid_argument) {
                        BOOST_LOG_TRIVIAL(warning) << "Skipping invalid file: " << ent.path().filename();
                        continue;
                    }


                    enter_page(addr, page);
                }
            }
        };

        ttxPage_p const get_page(ttxPageAddress_p const & address) {
            return page_list.at(address);
        }

        ttxPageEntry_p const get_page_entry(ttxPageAddress_p const & addr) {
            return std::make_shared<ttxPageEntry>(addr, get_page(addr));
        }

        ttxPageAddress_vector  const                            get_page_addresses(ttxPageAddress_vector & list) {
            list.clear();
            for (auto x : page_list) list.push_back(x.first);
            return list;
        }
};

typedef std::shared_ptr<ttxStaticPageSet>               ttxStaticPageSet_p;

class ttxPageSetFeeder {
    private:
        ttxPageSet_p                    pageset;
        ttxPageAddress_vector           pagelist;
        unsigned int                    pagelist_index;
        ttxPageEntry_p                  cur_page;
        bool                            is_done;

    public:
        ttxPageSet_p const              get_pageset() {
            return pageset;
        }

        ttxPageEntry_p                  get_next_page_entry() {
            if (is_done) {
                throw std::out_of_range("Attempt to get page from empty feeder");
            }

            if (pagelist_index + 1 >= pagelist.size())
                is_done = true;

            return pageset->get_page_entry(pagelist.at(pagelist_index++));
        }

        ttxPageEntry_p const    get_current_page() {
            return cur_page;
        }

        ttxPageSetFeeder(ttxPageSet_p new_pageset) {
            assert(new_pageset);
            pageset = new_pageset;
            pageset->get_page_addresses(pagelist);
            assert(!pagelist.empty());
            pagelist_index = 0;
            is_done = false;
        }

        bool all_out_of_pages() {
            return is_done;
        }
};

typedef std::shared_ptr<ttxPageSetFeeder>               ttxPageSetFeeder_p;

class ttxCarousel {
    private:
        unsigned int                                    pageset_list_index;
        std::vector<ttxPageSet_p>               pageset_list;
        std::list<ttxPageSetFeeder_p>   pageset_queue;

        ttxPageSet_p                    max_pri_pageset() {
            ttxPageSet_p max = pageset_list.front();

            for (auto x : pageset_list) {
                if (x->get_priority() > max->get_priority())  {
                    max = x;
                }
            }

            return max;
        }
        TTX_PAGESET_PRIORITY    max_pri(){
            return max_pri_pageset()->get_priority();
        }
        TTX_PAGESET_PRIORITY    cur_pri() {
            return cur_pageset()->get_priority();
        }

        ttxPageSet_p                    cur_pageset() {
            return pageset_queue.front()->get_pageset();
        }

        ttxPageSetFeeder_p              cur_pageset_feeder() {
            return pageset_queue.front();
        }

        ttxPageSet_p                    next_pageset() {
            // advance to the next pageset in our list of pagesets.
            assert(!pageset_list.empty());

            if (pageset_list_index >= pageset_list.size()) {
                // we're at the end of the list; rewind
                pageset_list_index = 0;
            }

            return pageset_list.at(pageset_list_index++);
        }

        ttxPageSetFeeder_p              get_pageset_feeder() {
            // Is there any higher-priority pageset?
            if (max_pri() > cur_pri()) {
                // Yes; put a feeder for that at the front of the queue.
                pageset_queue.push_front(std::make_shared<ttxPageSetFeeder>(max_pri_pageset()));
            }

            // Is the current pageset feeder at the end?
            if (cur_pageset_feeder()->all_out_of_pages()) {
                // Yes. Do we have any lower-priority queued pagesets to return to?
                if (pageset_queue.size() > 1) {
                    // yeah, so we just go down the queue.
                    pageset_queue.pop_front();
                    return get_pageset_feeder();
                }
                else {
                    // No, we pick the next one in the list.
                    pageset_queue.clear();
                    pageset_queue.push_front(std::make_shared<ttxPageSetFeeder>(next_pageset()));
                }
            }

            // All clear.
            return pageset_queue.front();
        }

    public:
        ttxCarousel() {
            pageset_list_index = 0;
        };

        void                    attach(ttxPageSet_p new_pageset) {
            BOOST_LOG_TRIVIAL(info) << "Attaching new pageset to carousel";
            pageset_list.push_back(new_pageset);
            pageset_queue.push_back(std::make_shared<ttxPageSetFeeder>(new_pageset));
        }

        ttxPageEntry_p  get_next_page_entry() {
            assert(!pageset_list.empty());
            return get_pageset_feeder()->get_next_page_entry();
        }

        ttxPageEntry_p  operator*();
        ttxPageEntry_p  operator++(int);
};

typedef std::shared_ptr<ttxCarousel>                    ttxCarousel_p;
typedef	std::array<uint8_t, 45>							ttxEncodedLine;
typedef std::shared_ptr<ttxEncodedLine>                 ttxEncodedLine_p;

class ttxPageEncoder {
    private:
        ttxCarousel                                             carousel;

    public:
        ttxPageEncoder(ttxCarousel slice_carousel);
        ttxEncodedLine_p yield_line();
};
