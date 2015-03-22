#include <boost/filesystem.hpp>
#include "ttxdata.hpp"

namespace fs = boost::filesystem;


namespace ttxPersist {
	ttxPage_p load_file(const fs::path & path) {
		BOOST_LOG_TRIVIAL(info) << "Loading teletext page from file: '" + path.string() + "'";
		ttxPage_p page = std::make_shared<ttxPage>();

		std::ifstream infile;
		int line_num = 1;

		infile.open(path.string().c_str(), std::ios::binary);

		assert(infile.is_open());

		while (infile.peek() != EOF) {
			infile >> (*(page->new_line)(line_num++));
		}

		return page;
	};

	ttxPageEntry_map load_directory(const fs::path & dir) {
		ttxPageEntry_map page_list;
		ttxPage_p page;

		BOOST_LOG_TRIVIAL(info) << "Loading teletext pages from directory: '" + dir.string() + "'";

		assert(fs::exists(dir));

		for (auto ent : fs::directory_iterator(dir)) {
			if (ent.path().extension() == ".ttx") {
				try {
					// Is the filename a valid page address?
					ttxPageAddress addr(ent.path().stem().string());
					try {
						page = load_file(ent);
					}
					catch (std::invalid_argument) {
						BOOST_LOG_TRIVIAL(warning) << "Skipping invalid file: " << ent.path().filename();
						continue;
					}

					page_list.emplace(addr, page);
				}
				catch (std::invalid_argument) {
					BOOST_LOG_TRIVIAL(warning) << "Skipping invalid filename: " << ent.path().filename();
					continue;
				}


			}
		}

		return page_list;
	}

	void save_pages() {

	}

};
