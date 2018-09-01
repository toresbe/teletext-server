#ifdef _WIN32
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <thread>
#include <mutex>
#include <chrono>
#include <boost/algorithm/string.hpp>


#include "config.hpp"
#include "editserver.hpp"
#include "ttxdata.hpp"
#include "encoder.hpp"
#include "persist.hpp"

namespace editserver {
	size_t EditConnection::send_str(std::string str) {
		return send(ConnectionSocket, str.c_str(), (int)str.size(), 0);
	}



	char * EditConnection::newline_in_buffer() {
		return (char *)memchr(buf, '\n', DEFAULT_BUFLEN);
	}

	std::string EditConnection::get_string_from_buffer(char * newline) {
		*newline = '\0';
		std::string line;
		line.assign(buf);
		// Is there any data left in the buffer?
		if (newline <= buf_p) {
			// Move the leftovers to the start of the buffer
			memmove(buf, newline + 1, sizeof(buf) - line.size());
			// Move the water mark backwards, too
			buf_p -= line.size() + 1;
		}
		else {
			// No, we've eaten the whole buffer.
			buf_p = buf;
		}
		// Clear remaining buffer
		memset(buf_p, 0, line.size() + 1);
		return line;
	}

	std::string EditConnection::get_line() {
		char * newline;
		
		while (!(newline = newline_in_buffer()))
			get_data();

		return get_string_from_buffer(newline);
	}

	int EditConnection::get_data() {
		int maxlen = DEFAULT_BUFLEN - (buf_p - buf);
		if (!maxlen) return 0;
		int received = recv(ConnectionSocket, buf_p, maxlen, (int)0);

		if (received > 0) {
			buf_p += received;
		}
		else if (received == 0) {
			throw std::underflow_error("Connection reset by peer");
		}
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			throw std::underflow_error("Connection failed");
			closesocket(ConnectionSocket);
		}

		return (int)received;
	}

	TokenizedCommandLine EditConnection::tokenize_string(std::string cmd) {
		size_t next_sep = std::string::npos;
		std::vector<std::string> tokens;

		do {
			cmd = cmd.substr(next_sep + 1);
			next_sep = cmd.find_first_of(" ");
			tokens.push_back(cmd.substr(0, next_sep));
		} while (next_sep != std::string::npos);
		return tokens;
	}

	bool EditConnection::cmd_login(const TokenizedCommandLine & cmd_tokens) {
		try {
			write_permissions = config::get_user_perms(cmd_tokens[1], cmd_tokens[2]);
			username = cmd_tokens[1];
			BOOST_LOG_TRIVIAL(info) << "User \"" << username << "\" authenticated.";
		}
		catch (std::invalid_argument e) {
			BOOST_LOG_TRIVIAL(warning) << "Invalid user login attempt: " << e.what();
			return false;
		}

		send_str("Welcome!\n");

		return true;
	}

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
		TokenizedCommandLine cmd_tokens;
		bool is_authenticated = false;

		while (1) {
			try {
				cmd_tokens = tokenize_string(get_line());
			}
			catch (std::underflow_error) {
				BOOST_LOG_TRIVIAL(info) << "Client connection reset by peer";
				break;
			}
			auto cmd_arg0 = boost::to_upper_copy(cmd_tokens.at(0));

			if (cmd_arg0 == "LOGIN") {
				is_authenticated = cmd_login(cmd_tokens);
			}

			if (cmd_arg0 == "BYE") {
				send_str("Y'all come back now, y'hear?\n");
				break;
			}

			// protected instructions
			if (is_authenticated) {
				if (cmd_arg0 == "UPDATE") {
					cmd_update_page(cmd_tokens);
				}
			}
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

	void DumpProtocol::accept_connection(SOCKET & NewSocket) {
		// accept 
		connection_list_mutex.lock();

		// enter item
		connection_list.push_back(NewSocket);

		// release lock
		connection_list_mutex.unlock();

		BOOST_LOG_TRIVIAL(info) << "New dump target connected.";
	}

	void DumpProtocol::operator()(ttxCarousel * Carousel) {
		carousel = Carousel;
		int portnum = config::get_value<int>("dump_port");

		SOCKET ListenSocket = make_socket(portnum);
		SOCKET ClientSocket = INVALID_SOCKET;

		std::thread new_thread(&DumpProtocol::thread_main, this);

		while (1) {
			// Accept a client socket
			ClientSocket = accept(ListenSocket, NULL, NULL);
			if (ClientSocket == INVALID_SOCKET) {
				printf("accept failed with error: %d\n", WSAGetLastError());
				closesocket(ListenSocket);
			}
			else {
				accept_connection(ClientSocket);
				ClientSocket = INVALID_SOCKET;
			}
		}

		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

}

namespace editserver {
	SOCKET make_socket(int portnum) {
		std::string port = std::to_string(portnum);

		WSADATA wsaData;
		int iResult;

		SOCKET ListenSocket = INVALID_SOCKET;

		struct addrinfo *result = NULL;
		struct addrinfo hints;

		// Initialize Winsock
		iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0) {
			printf("WSAStartup failed with error: %d\n", iResult);
			return 1;
		}

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Resolve the server address and port
		iResult = getaddrinfo(NULL, port.c_str(), &hints, &result);
		if (iResult != 0) {
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			return 1;
		}

		// Create a SOCKET for connecting to server
		ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (ListenSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		// Setup the TCP listening socket
		iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(result);
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		freeaddrinfo(result);

		iResult = listen(ListenSocket, SOMAXCONN);
		if (iResult == SOCKET_ERROR) {
			printf("listen failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		return ListenSocket;
	}



	void start(void)
	{
		ttxCarousel carousel;
		for (auto page_entry : ttxPersist::load_directory("C:\\Users\\ToreSinding\\Documents\\projects\\teletext-server\\pages")) {
			carousel.attach(page_entry);
		}
		std::thread dump_thread{ &DumpProtocol::operator(), new DumpProtocol(), &carousel };
		std::thread edit_thread{ &EditProtocol::operator(), new EditProtocol(), &carousel };

		dump_thread.join();
	}

}
#endif
