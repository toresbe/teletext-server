#pragma once
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include "config.hpp"
#include <thread>
#include <mutex>
#include <vector>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")


namespace editserver {
	#define DEFAULT_BUFLEN 512
	SOCKET make_socket(int portnum);

	class DumpProtocol {
	private:
		ttxCarousel * carousel;
		std::list<SOCKET> connection_list;
		std::mutex connection_list_mutex;
		void DumpProtocol::thread_main();
	public:
		void accept_connection(SOCKET & NewSocket);
		void operator() (ttxCarousel * Carousel);
	};

	typedef std::vector<ttxPageAddress>	UserWritePermissions;

	class EditConnection {
		ttxCarousel * carousel;
		char buf[DEFAULT_BUFLEN];
		SOCKET ConnectionSocket = INVALID_SOCKET;
		char * buf_p;
		std::string username;

		bool cmd_login(const TokenizedCommandLine & cmd_tokens);
		bool cmd_update_page(const TokenizedCommandLine & cmd_tokens);


		UserWritePermissions write_permissions;
		size_t send_str(std::string str);
		char * newline_in_buffer();
		std::string get_string_from_buffer(char * newline);
		std::string get_line();
		int get_data();
		TokenizedCommandLine tokenize_string(std::string cmd);
		bool run_command(std::string cmd);
	public:
		void thread_main(SOCKET NewSocket, ttxCarousel * Carousel);
	};

	class EditProtocol {
	private:
		ttxCarousel * carousel;
		SOCKET ClientSocket = INVALID_SOCKET;

	public:
		int operator()(ttxCarousel * Carousel);
	};

	void start(void);
}