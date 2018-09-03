#ifndef __EDITSERVER_CPP 
#define __EDITSERVER_CPP

#include <iostream>
#include <string>
#include <boost/algorithm/string/trim.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

class ttxEditConnection
: public boost::enable_shared_from_this<ttxEditConnection>
{
    public:
        typedef boost::shared_ptr<ttxEditConnection> pointer;

        static pointer create(boost::asio::io_service& io_service)
        {
            BOOST_LOG_TRIVIAL(info) << "Instantiating new listener";
            return pointer(new ttxEditConnection(io_service));
        }

        tcp::socket& socket()
        {
            return socket_;
        }

        void start()
        {
            BOOST_LOG_TRIVIAL(info) << "Connection opened from " << socket_.remote_endpoint();
            message_ = "HELLO\n";

            boost::asio::async_write(socket_, boost::asio::buffer(message_),
                    boost::bind(&ttxEditConnection::handle_write, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));

            boost::asio::async_read_until(socket_, input_buf_, "\r\n",
                    boost::bind(
                        &ttxEditConnection::handle_read, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred)
                    );

        }

    private:
        ttxEditConnection(boost::asio::io_service& io_service)
            : socket_(io_service) {
        }

        void handle_read(const boost::system::error_code& error, size_t /*bytes_transferred*/) {
            if(error)
            {
                BOOST_LOG_TRIVIAL(warning) << "Got error handling read!";
            } else {
                std::string s( (std::istreambuf_iterator<char>(&input_buf_)), std::istreambuf_iterator<char>() );
                boost::algorithm::trim_right(s);

                BOOST_LOG_TRIVIAL(info) << "Got data from socket: " << s;

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

        void handle_write(const boost::system::error_code& error, size_t /*bytes_transferred*/) {
            if(error)
            {
                BOOST_LOG_TRIVIAL(warning) << "Got error handling write!";
            }
        }

        tcp::socket socket_;
        std::string message_;
        boost::asio::streambuf input_buf_;
        bool is_running_ = true;
};

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
        BOOST_LOG_TRIVIAL(info) << "Starting edit server";
        ttxEditServer server(port_no, io_service);
        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

#endif
