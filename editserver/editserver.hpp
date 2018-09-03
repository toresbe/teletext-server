#ifndef __EDITSERVER_HPP
#define __EDITSERVER_HPP

#include <boost/asio.hpp>
class ttxEditConnection;

class ttxEditCLI {
    public:
        ttxEditCLI(ttxEditConnection * connection);
        void got_line(std::string input_line);
        void got_connection();
    private:
        ttxEditConnection * _connection;
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
        bool is_authenticated_ = false;
};


int EditServerStart(int port_no);
#endif
