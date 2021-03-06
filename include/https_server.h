﻿#pragma once


#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
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
namespace spiritsaway::http_utils::ssl_server
{
	namespace beast = boost::beast;         // from <boost/beast.hpp>
	namespace http = beast::http;           // from <boost/beast/http.hpp>
	namespace net = boost::asio;            // from <boost/asio.hpp>
	namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
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
		beast::ssl_stream<beast::tcp_stream> stream_;
		beast::flat_buffer buffer_;
		http::request<http::string_body> req_;
		std::shared_ptr<http::response<http::string_body>> string_res_;
		std::shared_ptr<http::response<http::file_body>> file_res_;
		logger_t logger;
		const std::uint32_t expire_time;

	public:
		// Take ownership of the socket
		explicit
			session(
				tcp::socket&& socket,
				ssl::context& ctx,
				logger_t in_logger,
				std::uint32_t in_expire_time = 10)
			: stream_(std::move(socket), ctx)
			, logger(in_logger)
			, expire_time(in_expire_time)
		{
		}

		// Start the asynchronous operation
		void run();

		void on_run();

		void on_handshake(beast::error_code ec);

		void do_read();

		void on_read(
			beast::error_code ec,
			std::size_t bytes_transferred);

		void do_write(http::response<http::string_body>&& msg);
		void do_write(http::response<http::file_body>&& msg);

		virtual void on_write(beast::error_code ec, std::size_t bytes_transferred);

		virtual bool should_close() const;

		void do_close();

		void on_shutdown(beast::error_code ec);
		// Report a failure
		void fail(beast::error_code ec, error_pos where);



		virtual std::string check_request();
		virtual void route_request();

	};

	class file_session : public session
	{
	public:
		// Take ownership of the socket
		explicit
			file_session(
				tcp::socket&& socket,
				ssl::context& ctx,
				logger_t in_logger,
				std::uint32_t in_expire_time,
				std::shared_ptr<const std::string> in_doc_root)
			: session(std::move(socket), ctx, std::move(in_logger), in_expire_time)
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
		ssl::context& ctx_;
		tcp::acceptor acceptor_;
		logger_t logger;
		bool valid = false;
		const std::uint32_t expire_time;

	public:
		listener(
			net::io_context& ioc,
			ssl::context& ctx,
			tcp::endpoint endpoint,
			logger_t in_logger,
			std::uint32_t expire_time);

		// Start accepting incoming connections
		bool run();
	private:
		void do_accept();

		void on_accept(beast::error_code ec, tcp::socket socket);
	protected:
		virtual std::shared_ptr<session> make_session(tcp::socket&& socket);

		void fail(beast::error_code ec, error_pos where);


	};

	class file_listener : public listener
	{
	protected:
		const std::shared_ptr<const std::string> doc_root;
	public:
		file_listener(
			net::io_context& ioc,
			ssl::context& ctx,
			tcp::endpoint endpoint,
			logger_t in_logger,
			std::uint32_t in_expire_time,
			std::string in_doc_root)
			: listener(ioc, ctx, endpoint, std::move(in_logger), in_expire_time)
			, doc_root(std::make_shared<std::string>(in_doc_root))
		{

		}
		std::shared_ptr<session> make_session(tcp::socket&& socket) override;

	};
}

