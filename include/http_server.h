#pragma once


#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "http_common.h"

// Handles an HTTP server connection
namespace spiritsaway::http_utils::server
{
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using logger_t = std::shared_ptr<spdlog::logger>;
using spiritsaway::http_utils::common::create_response;
using spiritsaway::http_utils::common::error_pos;


// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.


//------------------------------------------------------------------------------


class session : public std::enable_shared_from_this<session>
{
protected:
	beast::tcp_stream stream_;
	beast::flat_buffer buffer_;
	http::request<http::string_body> req_;
	std::shared_ptr<void> res_;
	logger_t logger;
	const std::uint32_t expire_time;

public:
	// Take ownership of the socket
	explicit
	session(
		tcp::socket&& socket,
		logger_t in_logger,
		std::uint32_t in_expire_time)
		: stream_(std::move(socket))
		, logger(in_logger)
		, expire_time(in_expire_time)
	{
	}

	// Start the asynchronous operation
	void run();
protected:
	void on_run();


	void do_read();

	void on_read(beast::error_code ec, std::size_t bytes_transferred);
	template<bool isRequest, class Body, class Fields>
	void do_write(http::message<isRequest, Body, Fields>&& msg)
	{
		// The lifetime of the message has to extend
		// for the duration of the async operation so
		// we use a shared_ptr to manage it.
		auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));

		// Store a type-erased version of the shared
		// pointer in the class to keep it alive.
		res_ = sp;

		// Write the response
		beast::get_lowest_layer(stream_).expires_after(
			std::chrono::seconds(expire_time));

		http::async_write(
			stream_,
			*sp,
			beast::bind_front_handler
			(
				&session::on_write,
				shared_from_this(),
				sp->need_eof()
			)
		);
	}
	
	void on_write(bool close, beast::error_code ec, std::size_t bytes_transferred);
	

	void do_close();
	// Report a failure
	virtual void fail(beast::error_code ec, error_pos where);
	


	virtual bool check_request();
	
	virtual void route_request();

};

class file_session: public session
{
public:
	// Take ownership of the socket
	explicit
	file_session(
		tcp::socket&& socket,
		logger_t in_logger,
		std::uint32_t in_expire_time,
		std::shared_ptr<const std::string> in_doc_root
		)
		: session(std::move(socket), std::move(in_logger), in_expire_time)
		, doc_root_(std::move(in_doc_root))
	{
	}
protected:
	const std::shared_ptr<const std::string> doc_root_;

	void route_request() override;
	
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
protected:
	net::io_context& ioc_;
	tcp::acceptor acceptor_;
	logger_t logger;
	bool valid = false;
	const std::uint32_t expire_time;

public:
	listener(
		net::io_context& ioc,
		tcp::endpoint endpoint,
		logger_t in_logger,
		std::uint32_t expire_time
	);

	// Start accepting incoming connections
	bool run();
	
private:
	void do_accept();
	void on_accept(beast::error_code ec, tcp::socket socket);
protected:
	virtual std::shared_ptr<session> make_session(tcp::socket&& socket);
	virtual void fail(beast::error_code ec, error_pos where);
};

class file_listener: public listener
{
protected:
	const std::shared_ptr<const std::string> doc_root;
public:
	file_listener(
		net::io_context& ioc,
		tcp::endpoint endpoint,
		logger_t in_logger,
		std::uint32_t in_expire_time,
		std::string in_doc_root)
		: listener(ioc, endpoint, std::move(in_logger), in_expire_time)
		, doc_root(std::make_shared<std::string>(in_doc_root))
	{

	}
	std::shared_ptr<session> make_session(tcp::socket&& socket) override;
};
}

