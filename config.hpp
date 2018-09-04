#pragma once
#include <string>
#include "ttxdata.hpp"
#include <sstream>
#include <iostream>
#include <list>
#include <utility>
#include <fstream>
#include <map>

namespace config {
	typedef std::map<std::string, std::string> settings_list_t;

	template<typename T> T get_value(std::string key);
	std::vector<ttxPageAddress> get_user_perms(std::string username, std::string password);
	std::pair<std::string, std::string> tokenize_line(std::string line);
	void read_file(const std::string & filespec);
}
