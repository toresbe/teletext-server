typedef std::array<uint8_t, 45>     ttxPacket;
typedef std::shared_ptr<ttxPacket>  ttxPacket_p;
typedef std::list<ttxPacket_p>      ttxPacket_p_list;

class TeletextEncoder {
    private:
        //TeletextParity parity;

        static uint8_t position_bits(uint8_t data, int bits, int offset) {
            unsigned int mask;
            mask = (1<<bits)-1;
            return (data & mask) << offset;
        }

        // Populate the page address bytes of a packet.
        // (The address is the last two digits of a page number).
        // See spec: 9.3.1.1
        static void write_page_number(ttxPacket_p packet, ttxPageAddress_p page_address){
            uint8_t *p = (uint8_t *)packet.get();
            uint16_t tmp;

            tmp = TeletextParity::enham_8_16(page_address->get_page_number());

            p[5] = tmp & 0xFF;
            p[6] = (tmp & 0xFF00) >> 8;
        }

        // Write packet address, consisting of 
        // magazine 0-7, first digit of page number (0 is presented as 8, for some reason)
        // packet address, 0-31
        // See spec: 7.2.2
        static void write_packet_address(ttxPacket_p packet, const ttxPageAddress_p & address, int packet_number) {
            uint8_t *p = (uint8_t *)packet.get();
            uint16_t tmp;
            uint8_t packet_address;
            uint8_t magazine = address->get_magazine();

            packet_address = position_bits(magazine,3,0);
            packet_address |= position_bits(packet_number, 5, 3);

            tmp = TeletextParity::enham_8_16(packet_address);

            p[3] = tmp & 0xFF;
            p[4] = (tmp & 0xFF00) >> 8;
        }

        // Write and encode the header text.
        // Bugs: The text is hard-coded. 
        // See spec: 9.3.1
        static void write_packet_header_text(ttxPacket_p & packet){
            char *p = (char *)packet.get();
            time_t now;
            char RTC[8];

            now = time(NULL);
            strftime(RTC, 9, "%T", localtime(&now));
            sprintf(p+13, "\x06%.25s %.8s", "FRIKANALEN\x04TEKST-TV\x06", RTC);
            TeletextParity::add_odd_parity(p+13, 32);
        }

        // Write clock lead-in and framing code.
        // See spec: 7.1.1
        static void write_lead_in(ttxPacket_p packet) {
            uint8_t *p = (uint8_t *)packet.get();
            p[0] = 0x55;
            p[1] = 0x55;
            p[2] = 0x27;
        }

        // Write control bits of page header.
        // See spec: 9.3.1.3
        static void write_control_bits(ttxPacket_p packet) {
            uint8_t *p = (uint8_t *)packet.get();

            p[10] = TeletextParity::enham_4_8(0x0);
            p[11] = TeletextParity::enham_4_8(0x0);
            p[12] = TeletextParity::enham_4_8(0x0);
        }

        ttxPacket_p yield_page_header(ttxPageEntry_p page_entry) {
            ttxPacket_p packet;
            uint8_t *packet_p;

            packet = std::make_shared<ttxPacket>();
            packet_p = (uint8_t *)packet.get();

            write_lead_in(packet);
            write_packet_address(packet, page_entry->first, 0);
            write_page_number(packet, page_entry->first);
            write_packet_header_text(packet);
            write_control_bits(packet);

            packet_p[7] = TeletextParity::enham_4_8(0x0); //
            packet_p[8] = TeletextParity::enham_4_8(0x0); // Generate "valid zero" subpage codes
            packet_p[9] = TeletextParity::enham_4_8(0x0); //

            return packet;
        }

        const ttxPacket_p yield_text_line(const ttxPageEntry_p & page_entry, int packet_number) {
            ttxPacket_p packet = std::make_shared<ttxPacket>();
            uint8_t *packet_p = (uint8_t *)packet.get();

            write_lead_in(packet);
            write_packet_address(packet, page_entry->first, packet_number);
            write_page_number(packet, page_entry->first);

            //std::cout << page_entry->second->lines[1] << "\n";
            // Write the actual text of the page
            memcpy(packet_p+5,(page_entry->second->lines[packet_number].get()),40);
            TeletextParity::add_odd_parity((char *)packet_p+5, 40);

            return packet;
        }
    public:
        
        ttxPacket_p_list encode(const ttxPageEntry_p & page_entry) {
            ttxPacket_p_list packet_list;
            int line_number = 1;

            packet_list.push_back(yield_page_header(page_entry));
            for (auto line: page_entry->second->lines) {
                packet_list.push_back(yield_text_line(page_entry, line_number++));
            }

            return packet_list;
        }

};
