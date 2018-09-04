#include <iostream>
#include <string>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "editserver/editserver.hpp"

using boost::asio::ip::tcp;

ttxEditCLI::ttxEditCLI(ttxEditConnection * connection) {
    _connection = connection;
}

void ttxEditCLI::got_line(std::string input_line) {
    TokenizedCommandLine cmd_tokens;
    try {
        cmd_tokens = tokenize_string(input_line);
    }
    catch (std::underflow_error) {
        BOOST_LOG_TRIVIAL(info) << "Client connection reset by peer";
        return;
    }
    auto cmd_arg0 = boost::to_upper_copy(cmd_tokens.at(0));

    if (cmd_arg0 == "LOGIN") {
        is_authenticated_ = cmd_login(cmd_tokens);
    }

    if (cmd_arg0 == "BYE") {
        _connection->send_line("Y'all come back now, y'hear?\n");
        return;
    }

    // protected instructions
    if (is_authenticated_) {
        if (cmd_arg0 == "UPDATE") {
            //cmd_update_page(cmd_tokens);
        }
    }
}

void ttxEditCLI::got_connection() {
    _connection->send_line("Teletext server edit protocol 1.0, please login");
}

tcp::socket& ttxEditConnection::socket()
{
    return socket_;
}

TokenizedCommandLine ttxEditCLI::tokenize_string(std::string cmd) {
    size_t next_sep = std::string::npos;
    std::vector<std::string> tokens;

    do {
        cmd = cmd.substr(next_sep + 1);
        next_sep = cmd.find_first_of(" ");
        tokens.push_back(cmd.substr(0, next_sep));
    } while (next_sep != std::string::npos);
    return tokens;
}

bool ttxEditCLI::cmd_login(const TokenizedCommandLine & cmd_tokens) {
    try {
        write_permissions = config::get_user_perms(cmd_tokens[1], cmd_tokens[2]);
        username = cmd_tokens[1];
        BOOST_LOG_TRIVIAL(info) << "User \"" << username << "\" authenticated.";
    }
    catch (std::invalid_argument e) {
        BOOST_LOG_TRIVIAL(warning) << "Invalid user login attempt: " << e.what();
        return false;
    }

    _connection->send_line("Welcome!");

    return true;
}

void ttxEditConnection::send_line(std::string str) {
    boost::asio::async_write(socket_, boost::asio::buffer(str + "\r\n"),
            boost::bind(&ttxEditConnection::handle_write, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
}

void ttxEditConnection::start()
{
    BOOST_LOG_TRIVIAL(info) << "Connection opened from " << socket_.remote_endpoint();

    boost::asio::async_read_until(socket_, input_buf_, "\r\n",
            boost::bind(
                &ttxEditConnection::handle_read, shared_from_this(),
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
            );

}

ttxEditConnection::ttxEditConnection(boost::asio::io_service& io_service)
    : socket_(io_service), cli(this) {
    }

void ttxEditConnection::handle_read(const boost::system::error_code& error, size_t /*bytes_transferred*/) {
    if(error)
    {
        BOOST_LOG_TRIVIAL(warning) << "Got error handling read!";
    } else {
        std::string s( (std::istreambuf_iterator<char>(&input_buf_)), std::istreambuf_iterator<char>() );
        boost::algorithm::trim_right(s);

        BOOST_LOG_TRIVIAL(info) << "Got data from socket: " << s;
        cli.got_line(s);

        if(is_running_) {
            boost::asio::async_read_until(socket_, input_buf_, "\r\n",
                    boost::bind(
                        &ttxEditConnection::handle_read, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred)
                    );
        }
    }
}

void ttxEditConnection::handle_write(const boost::system::error_code& error, size_t /*bytes_transferred*/) {
    if(error)
    {
        BOOST_LOG_TRIVIAL(warning) << "Got error handling write!";
    }
}


class ttxEditServer
{
    public:
        ttxEditServer(unsigned int port_no, boost::asio::io_service& io_service)
            : tcp_acceptor(io_service, tcp::endpoint(tcp::v4(), port_no))
        {
            BOOST_LOG_TRIVIAL(info) << "Listening on port " << port_no;
            start_listening();
        }

    private:
        void start_listening()
        {
            ttxEditConnection::pointer new_connection = ttxEditConnection::create(tcp_acceptor.get_io_service());

            tcp_acceptor.async_accept(new_connection->socket(),
                    boost::bind(
                        &ttxEditServer::handle_accept,
                        this,
                        new_connection,
                        boost::asio::placeholders::error
                        )
                    );
        }

        void handle_accept(ttxEditConnection::pointer new_connection, const boost::system::error_code& error)
        {
            if (!error)
            {
                new_connection->start();
            }

            start_listening();
        }

        tcp::acceptor tcp_acceptor;
};

int EditServerStart(int port_no)
{
    try
    {
        boost::asio::io_service io_service;
        BOOST_LOG_TRIVIAL(info) << "Edit server thread alive";
        ttxEditServer server(port_no, io_service);
        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

ttxEditConnection::pointer ttxEditConnection::create(boost::asio::io_service& io_service)
{
    BOOST_LOG_TRIVIAL(info) << "Instantiating new listener";
    return ttxEditConnection::pointer(new ttxEditConnection(io_service));
}
