#pragma once
#include <cstdint>
#include <cstdio>

namespace ttxParity {
	uint8_t xor_bits(uint8_t in_byte, uint8_t mask);
	void add_odd_parity(char *s, int len);
	uint8_t deham_8_4(uint8_t in_byte);
	int _get_bit(uint8_t byte, int bit);
	uint8_t enham_4_8(uint8_t nybble);
	uint16_t enham_8_16(uint8_t in_byte);
	uint8_t deham_16_8(uint8_t lsb, uint8_t msb);
};