#include "ttxdata.hpp"
#include "ttxdata/encoder.hpp"

class ttxCarousel {
public:
	ttxCarousel() {
		datastore = ttxDatastore::get_instance();
		auto l = ttxEncode::encode_page_entry(datastore->get_next_page_entry());
		current_page_packet_vector = { std::begin(l), std::end(l) };
	}

	// Get the next packet, retreiving a new line from datastore if necessary
	ttxPacket_p get_next_line() {
		if (current_page_packet_idx == current_page_packet_vector.size()) {
			current_page_packet_idx = 0;
			auto l = ttxEncode::encode_page_entry(datastore->get_next_page_entry());
			current_page_packet_vector = { std::begin(l), std::end(l) };
		}

		return current_page_packet_vector.at(current_page_packet_idx++);
	}

	ttxEncodedField_p get_next_field() {
		auto field_p = std::make_shared<ttxEncodedField>();

		for (int i = 0; i <= field_p->size() - 1; i++) {
			(*field_p)[i] = *get_next_line();
		}

		return field_p;
	}

	ttxDatastore * datastore;
	int current_page_packet_idx = 0;
	std::vector<ttxPacket_p> current_page_packet_vector;
};