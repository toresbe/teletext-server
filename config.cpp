#include <string>
#include <sstream>
#include <iostream>
#include <list>
#include <fstream>
#include <map>
#include "config.hpp"
#include "ttxdata/ttxdata.hpp"
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

namespace config {
	settings_list_t settings_list;
	template<typename T> T get_value(std::string key) {
		T value;
		auto x = settings_list.find(key);
		if (x == settings_list.end()) {
			BOOST_LOG_TRIVIAL(error) << "Could not find key in config file:" + key;
			throw std::invalid_argument("Invalid config key lookup!");
		}
		std::string str = x->second;
		std::istringstream(str) >> value;
		return value;
	}

	void stupid_linker_workaround(){
		get_value<int>("");
		get_value<std::string>("");
	}
	std::vector<ttxPageAddress> get_user_perms(std::string username, std::string password) {
		std::pair<std::string, std::string> user_entry;
		// may raise an invalid argument exception
		try {
			// user=password=100,102,...
			user_entry = tokenize_line(get_value<std::string>(username));
		}
		catch (std::invalid_argument) {
			throw std::invalid_argument("User does not exist!");
		}

		if (user_entry.first != password) {
			throw std::invalid_argument("Incorrect password!");
		}

		std::string page_list_string = user_entry.second;

		std::vector<ttxPageAddress> page_list;

		size_t next_comma = std::string::npos;

		do {
			page_list_string = page_list_string.substr(next_comma + 1);
			next_comma = page_list_string.find_first_of(',');
			page_list.push_back(page_list_string.substr(0, next_comma));
		} while (next_comma != std::string::npos);

		return page_list;
	}

	std::pair<std::string, std::string> tokenize_line(std::string line) {
		auto separator = line.find_first_of('=');
		std::string key = line.substr(0, separator);
		std::string value = line.substr(separator+1);
		return std::pair<std::string, std::string>(key, value);
	}

	void read_file(const std::string & filespec) {
		BOOST_LOG_TRIVIAL(info) << "Loading configuration file '" << filespec << "'";
		std::ifstream file(filespec, std::ios_base::in);
		assert(file.is_open());

		for (std::string line; std::getline(file, line);) {
			if (line[0] == '#') continue;
			settings_list.insert(tokenize_line(line));
		}

		for (auto item : settings_list) {
			BOOST_LOG_TRIVIAL(debug) << "Loaded setting key " << item.first << ": " << item.second;
		}
	}

}
