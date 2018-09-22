#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/asio.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>
#include "ttxdata/ttxdata.hpp"
#include "ttxdata/encoder.hpp"
#include "config.hpp"
#include "sinks.hpp"
#include "ttxdata/carousel.cpp"
using boost::asio::ip::tcp;

class TcpSinkListener;
class TcpSinkClient;
class TcpSink;

class TcpSinkClient : public boost::enable_shared_from_this<TcpSinkClient> {
public:
	typedef boost::shared_ptr<TcpSinkClient> pointer;

	TcpSinkClient(boost::asio::io_service& io_service) : socket_(io_service) {};

	static pointer create(boost::asio::io_service& io_service)
	{
		BOOST_LOG_TRIVIAL(info) << "New client connection";
		return TcpSinkClient::pointer(new TcpSinkClient(io_service));
	}

	void start() {
		BOOST_LOG_TRIVIAL(info) << "Connection opened from " << socket_.remote_endpoint();

	}

	void send_field(ttxEncodedField_p field_p) {
		boost::asio::async_write(socket_, boost::asio::buffer(*field_p->data()),
			boost::bind(&TcpSinkClient::handle_write, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}

	void handle_write(const boost::system::error_code& error, size_t /*bytes_transferred*/) {
		BOOST_LOG_TRIVIAL(warning) << "Got no error handling write!";

		if (error)
		{
			is_dead = true;
			BOOST_LOG_TRIVIAL(warning) << "Got error handling write!";
		}
	}

	boost::asio::ip::tcp::socket& socket() {
		return socket_;
	}

	bool is_dead = false;
private:
	boost::asio::ip::tcp::socket socket_;
};

class TcpSinkListener {
public:
	TcpSinkListener(unsigned int port_no, boost::asio::io_service & io_service) :
		tcp_acceptor(io_service, tcp::endpoint(tcp::v4(), port_no)),
		frame_timer(io_service, boost::posix_time::milliseconds(20))
	{
		BOOST_LOG_TRIVIAL(info) << "TCP sink initialized, set to listen on port " << port_no;
		accept_connections();
	}

	void accept_connections()
	{
		BOOST_LOG_TRIVIAL(info) << "TCP sink accepting connections...";

		TcpSinkClient::pointer new_connection = TcpSinkClient::create(tcp_acceptor.get_io_service());

		tcp_acceptor.async_accept(new_connection->socket(),
			boost::bind(
				&TcpSinkListener::handle_accept,
				this,
				new_connection,
				boost::asio::placeholders::error
			)
		);

		frame_timer.async_wait(boost::bind(&TcpSinkListener::frame_timeout, this));
	}

	void frame_timeout() {
		ttxEncodedField_p field = carousel.get_next_field();

		audience.erase(
			std::remove_if(
				audience.begin(),
				audience.end(),
				[](const TcpSinkClient::pointer& c) { return c->is_dead; }),
			audience.end());

		for (auto listener : audience) {
			listener->send_field(field);
		}

		//frame_timer.expires_at(frame_timer.expires_at() + boost::posix_time::milliseconds(20));
		frame_timer.async_wait(boost::bind(&TcpSinkListener::frame_timeout, this));
	}

private:
	ttxCarousel carousel;
	typedef std::vector<TcpSinkClient::pointer> audience_t;
	audience_t audience;

	void handle_accept(TcpSinkClient::pointer new_connection, const boost::system::error_code& error)
	{
		if (!error)
		{
			new_connection->start();

			audience.push_back(new_connection);
		}

		accept_connections();
	}

	boost::asio::deadline_timer frame_timer;
	tcp::acceptor tcp_acceptor;
};

class TcpSink : public ttxSink {
	void start() {
		try
		{
			boost::asio::io_service io_service;
			BOOST_LOG_TRIVIAL(info) << "TCP dumper thread alive";
			int dump_port = config::get_value<int>("dump_port");
			TcpSinkListener server(dump_port, io_service);
			BOOST_LOG_TRIVIAL(info) << "TCP dumper thread still alive";

			io_service.run();
		}
		catch (std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
};
