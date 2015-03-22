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
#include "ttxdata.hpp"

const uint8_t * ttxLine::get_line() const {
	return &data[0];
}

std::istream& operator >>(std::istream & sb, ttxLine &line) {
	sb.read((char *)&line.data[0], line.data.size());

	if (sb.gcount() != line.data.size()) {
		throw std::invalid_argument("Failed to read");
	}

	return sb;
}

void operator >>(std::string & sb, ttxLine &line) {
	auto len = (sb.length() > 40) ? 40 : sb.length();
	memcpy((char *)&line.data[0], sb.c_str(), sb.length() - 1);
}

uint8_t &ttxLine::operator[](unsigned int i) {
	if (i > data.size()) {
		throw std::out_of_range("Out of bounds read from ttxLine");
	}
	else {
		return data[i];
	}
}

void ttxPageAddress::parse_address(const std::string & addr_str) {
	// todo: this might be done more cleanly with boost qi?

	if (sscanf(addr_str.c_str(), "%1u%2x", &magazine, &page_number) != 2) {
		throw std::invalid_argument("'" + addr_str + "' is not a valid page address");
	}
}

std::ostream & operator<<(std::ostream &os, const ttxPageAddress& p) {
	return os << p.to_str();
}

std::string ttxPageAddress::to_str() const {
	return std::to_string(to_int());
}

const int ttxPageAddress::get_page_number() const {
	return page_number;
}

const int ttxPageAddress::get_magazine() const {
	return magazine;
}

const int ttxPageAddress::to_int() const {
	int pgnum;

	if (!magazine) pgnum = 800;
	else pgnum = (magazine * 100);

	pgnum += page_number;

	return pgnum;
}

bool ttxPageAddress::operator< (const ttxPageAddress & addr) const {
	return (to_int() < addr.to_int());
}

ttxPageAddress::ttxPageAddress(const std::string & addr_str) {
	parse_address(addr_str);
}

ttxLine_p ttxPage::get_line(ttxLineNumber line_num) {
	return ttxLine_p(lines.at(line_num));
}

ttxLine_p ttxPage::get_line(int index) {
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

ttxLine_p ttxPage::new_line(ttxLineNumber line_num) {
	if (lines.count(line_num)) delete lines[line_num].get();
	auto line = std::make_shared<ttxLine>();
	lines[line_num] = line;
	return line;
}

ttxLine_p & ttxPage::operator [](ttxLineNumber i) {
	if (i > sizeof(lines)) {
		throw std::out_of_range("Out of bands read from ttxPage");
	}
	else {
		return lines[i];
	}
}

ttxCarousel::ttxCarousel() {
	page_list_iterator = page_list.begin();
};

void ttxCarousel::attach(const ttxPageEntry & new_page) {
	BOOST_LOG_TRIVIAL(info) << "Attaching new page to carousel";
	page_list.insert(new_page);
}

ttxPageEntry ttxCarousel::get_next_page_entry() {
	global_lock.lock();
	assert(!page_list.empty());

	if (page_list_iterator == page_list.end()) {
		page_list_iterator = page_list.begin();
	}

	auto retval = *page_list_iterator;
	page_list_iterator++;
	global_lock.unlock();
	return retval;
}

ttxPage_p	ttxCarousel::get_page(const ttxPageAddress & addr) {
	global_lock.lock();

	try {
		auto x = page_list.at(addr);
		global_lock.unlock();
		return x;
	}
	catch (std::out_of_range e) {
		global_lock.unlock();
		throw(e);
	}
}

void	ttxCarousel::update_page_line(
						const ttxPageAddress & addr, 
						const ttxLineNumber & line_num, 
						const ttxLineData & line_data) {
	ttxPage_p page;
	ttxLine_p line;
	try {
		// Does this page exist?
		page = get_page(addr);
	}
	catch (std::out_of_range e) {
		// Nope, make it and fill in the line
		page = std::make_shared<ttxPage>();
		ttxPageAddress new_addr = addr;
		attach(ttxPageEntry(addr, page));
		line = page->new_line(line_num);
		line->data = line_data;
		return;
	}
	// Yes, the page exists.
	line = page->get_line(line_num);
	line->data = line_data;
	return;
}