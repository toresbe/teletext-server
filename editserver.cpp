#ifdef LMAO

#include "config.hpp"
#include "editserver.hpp"
#include "ttxdata.hpp"
#include "encoder.hpp"
#include "persist.hpp"

namespace editserver {

	bool EditConnection::cmd_update_page(const TokenizedCommandLine & cmd_tokens) {
		ttxLineNumber	line_num;
		ttxPage_p		page_p;
		ttxLineData		line_data;
		int				line_byte_counter = 0;

		try {
			ttxPageAddress addr(cmd_tokens.at(1));

			line_num = std::stoi(cmd_tokens.at(2), 0, 16);

			for (auto &&line_byte : line_data)
				line_byte = std::stoi(cmd_tokens.at(3).substr(line_byte_counter++ * 2, 2), 0, 16);

			carousel->update_page_line(addr, line_num, line_data);

			BOOST_LOG_TRIVIAL(info) << "Updating line " << addr << (unsigned int)line_num;
			return true;

		}
		catch (std::exception e) {
			BOOST_LOG_TRIVIAL(warning) << "Error updating page: " << e.what();
			send_str(e.what());
			return false;
		}
	}

	void EditConnection::thread_main(SOCKET NewSocket, ttxCarousel * Carousel) {
		carousel = Carousel;
		ConnectionSocket = NewSocket;
		buf_p = buf;

		BOOST_LOG_TRIVIAL(info) << "Client connection thread active";
		int iResult;
		send_str("Teletext server edit protocol 1.0, please login\n");

		while (1) {
		}


		iResult = shutdown(ConnectionSocket, SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectionSocket);
		}
		closesocket(ConnectionSocket);
	}


	int EditProtocol::operator()(ttxCarousel * Carousel) {
		carousel = Carousel;
		int portnum = config::get_value<int>("edit_port");

		SOCKET ListenSocket = make_socket(portnum);

		while (1) {
			// Accept a client socket
			ClientSocket = accept(ListenSocket, NULL, NULL);
			if (ClientSocket == INVALID_SOCKET) {
				printf("accept failed with error: %d\n", WSAGetLastError());
				closesocket(ListenSocket);
			}
			else {
				std::thread new_thread(&EditConnection::thread_main, new EditConnection(), ClientSocket, carousel);
				new_thread.detach();
			}
		}

		closesocket(ListenSocket);
		WSACleanup();
		return 0;
	}
}

namespace editserver {
	void DumpProtocol::thread_main() {
		int iSendResult = 0;

		std::list<SOCKET> bring_out_yer_dead;
		while (1) {
			for (auto packet : ttxEncode::encode_page_entry(carousel->get_next_page_entry())) {
				connection_list_mutex.lock();

				for (auto outSocket : connection_list) {
					auto outPacket = packet.get();
					iSendResult = send(outSocket, (char *)outPacket->data() + 3, outPacket->size() - 3, 0);
					if (iSendResult == SOCKET_ERROR) {
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(outSocket);
						BOOST_LOG_TRIVIAL(info) << "Dump target disconnected.";
						bring_out_yer_dead.push_front(outSocket);
					}
				}

				for (auto dead : bring_out_yer_dead) connection_list.remove(dead);
				bring_out_yer_dead.clear();

				connection_list_mutex.unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(3));
			}
		}

	}
#endif
