#include <cstdint>
#include <cstdio>

namespace ttxParity {
        // Parity decoding and encoding support functions.
        //
        // Todo:
        //
        //  - Implement data integrity statistics (parity error detection)
        //  - Implement 24/18 Hamming encoder/decoder (not likely to be used)
        //
        // For description of Teletext parity schemes refer to ETS 300 706, pt. 8
        // 
        //
        uint8_t xor_bits(uint8_t in_byte, uint8_t mask) {
			// xor_bits: XOR a set of bits in a byte. eg with mask=0xFF, 
			// the function will return the XOR of every bit in the byte.
            int i, result = 1;

            for(i=7;i+1;i--) {
                if (mask & (1 << i)) {
                    result ^= (in_byte & (1 << i)) ? 1 : 0;
                }
            }

            return result;
        }

        void add_odd_parity(char *s, int len) {
			// add_odd_parity to a string of length len
            while(--len) {
                *s = *s | (xor_bits((*s)&0x7F, 0x7F) ? 0x80 : 0x00);
                s++;
            }
        }

        uint8_t deham_8_4(uint8_t in_byte) {
			// Obtain a nybble of data from a byte of Hamming 8/4 encoded data.
			// See spec, section 8.2 
            uint8_t error = 0, out = 0;
            error |= (xor_bits(in_byte, 0xA3)>0);
            error |= (xor_bits(in_byte, 0x8E)>0) << 2;
            error |= (xor_bits(in_byte, 0x3A)>0) << 3;
            error |= (xor_bits(in_byte, 0xFF)>0) << 4;
            if (error) printf("Decoding %02X errors: %02X\n", in_byte, error);

            out = (in_byte & 0x02) >> 1|
                (in_byte & 0x08) >> 2|
                (in_byte & 0x20) >> 3|
                (in_byte & 0x80) >> 4;

            return out;
        }

        int _get_bit(uint8_t byte, int bit) {
			// Get a single bit from a byte
            return (byte & (1 << bit)) >> bit;
        }

        uint8_t enham_4_8(uint8_t nybble) {
			// Hamming encode a nybble into a byte.
			// See spec, section 8.2
            int p[4]; // parity bits
            int d[4]; // data bits
            int i=0;
            uint8_t out_byte = 0;

            for(i = 0; i < 4; i++)
            {
                d[i] = _get_bit(nybble,i); // unpacking like this is lazy, but makes the code more readable
            }

            p[0] = 1 ^ d[0] ^ d[2] ^ d[3];
            p[1] = 1 ^ d[0] ^ d[1] ^ d[3];
            p[2] = 1 ^ d[0] ^ d[1] ^ d[2];
            p[3] = 1 ^ p[0] ^ d[0] ^ p[1] ^ d[1] ^ p[2] ^ d[2] ^ d[3];
            // see, very readable

            for(i = 0; i <= 3; i++) {
                out_byte |= p[i] << i*2;
                out_byte |= d[i] << ((i*2)+1);
            }

            return out_byte;
        }

        uint16_t enham_8_16(uint8_t in_byte) {
			// convenience function: Take a bytes, produce a Hamming 16/8 pair 
			// and return the encoded pair as a 16-bit word.
            uint16_t out;

            out = enham_4_8(in_byte & 0xF);
            out |= enham_4_8((in_byte & 0xF0) >> 4) << 8;

            return out;
        } 

        uint8_t deham_16_8(uint8_t lsb, uint8_t msb) {
			// convenience function: Take two bytes forming a Hamming 16/8 pair 
			// and return the decoded pair as a byte.
            uint8_t out = 0;

            out |= deham_8_4(lsb) ;
            out |= deham_8_4(msb) << 4;

            return out;
        } 
};
