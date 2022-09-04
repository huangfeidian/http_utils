#pragma once

#include "http_packet.h"
#include <memory>
#include <iostream>
#include <istream>
#include <ostream>
#include <asio.hpp>
#include "http_reply_parser.h"
#include "asio/ssl.hpp"

namespace spiritsaway::http_utils
{
	class https_client : public std::enable_shared_from_this<https_client>
	{
	private:
		asio::ip::tcp::resolver m_resolver;
		std::function<void(const std::string&, const reply&)> m_callback;
		const std::string m_req_str;
		const std::string m_server_url;
		const std::string m_server_port;
		std::string m_header_read_buffer;
		std::array<char, 4096> m_content_read_buffer;
		// timeout timer
		asio::basic_waitable_timer<std::chrono::steady_clock> m_timer;
		const std::size_t m_timeout_seconds = 5;
		http_reply_parser m_rep_parser;
		asio::ssl::stream<asio::ip::tcp::socket> m_socket;

	public:
		https_client(asio::io_context& io_context, asio::ssl::context& ssl_context, const std::string& server_url, const std::string& server_port, const request& req, std::function<void(const std::string&, const reply&)> callback, std::uint32_t timeout_second);
		void run();

	private:
		void handle_resolve(const asio::error_code& error, asio::ip::tcp::resolver::results_type results);
		void handle_connect(const asio::error_code& err, asio::ip::tcp::resolver::results_type::endpoint_type);
		void handle_hanshake(const asio::error_code& err);
		void handle_write_request(const asio::error_code& err);
		void handle_read_content(const asio::error_code& err, std::size_t n);
		void invoke_callback(const std::string& err);
		void on_timeout(const asio::error_code& err);
		bool verify_certificate(bool preverified,
			asio::ssl::verify_context& ctx);
	};
}