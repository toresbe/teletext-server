#ifndef __EDITSERVER_HPP
#define __EDITSERVER_HPP

#include <boost/asio.hpp>
#include "config.hpp"

class ttxEditConnection;
typedef	std::vector<std::string> TokenizedCommandLine;
typedef std::vector<ttxPageAddress>	UserWritePermissions;

class ttxEditCLI {
    public:
        ttxEditCLI(ttxEditConnection * connection);
        void got_line(std::string input_line);
        void got_connection();
    private:
        bool cmd_login(const TokenizedCommandLine & cmd_tokens);
        bool cmd_update_page(const TokenizedCommandLine & cmd_tokens);
        TokenizedCommandLine tokenize_string(std::string cmd);
        ttxEditConnection * _connection;
        //fixme: inelegant
        UserWritePermissions write_permissions;
        std::string username;
        bool is_authenticated_ = false;
};

class ttxEditConnection
: public boost::enable_shared_from_this<ttxEditConnection>
{
    public:
        typedef boost::shared_ptr<ttxEditConnection> pointer;
        ttxEditConnection(boost::asio::io_service& io_service);
        static pointer create(boost::asio::io_service& io_service);
        boost::asio::ip::tcp::socket& socket();
        void send_line(std::string str);
        void start();

    private:
        void handle_read(const boost::system::error_code& error, size_t /*bytes_transferred*/);
        void handle_write(const boost::system::error_code& error, size_t /*bytes_transferred*/);

        ttxEditCLI cli;
        boost::asio::ip::tcp::socket socket_;
        boost::asio::streambuf input_buf_;
        bool is_running_ = true;
};

int EditServerStart(int port_no);
#endif
