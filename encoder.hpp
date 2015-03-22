#pragma once
#include <array>
#include <list>
#include <memory>
#include "ttxdata.hpp"
#include "parity.hpp"

typedef std::array<uint8_t, 45>     ttxPacket;
typedef std::shared_ptr<ttxPacket>  ttxPacket_p;
typedef std::list<ttxPacket_p>      ttxPacket_p_list;

namespace ttxEncode {
	ttxPacket_p_list encode_page_entry(const ttxPageEntry & page_entry);
};
