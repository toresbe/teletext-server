#pragma once
#include <boost/filesystem.hpp>
#include "ttxdata.hpp"

namespace ttxPersist {
	ttxPage_p load_file(const boost::filesystem::path & path);

	ttxPageEntry_map load_directory(const boost::filesystem::path & dir);

	void save_pages();
};
