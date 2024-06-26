﻿#pragma once

#include "http_packet.h"
#include <memory>
#include <iostream>
#include <istream>
#include <ostream>
#include <boost/asio.hpp>
#include <spdlog/logger.h>
#include "http_reply_parser.h"

namespace spiritsaway::http_utils
{
	namespace asio = boost::asio;
	using asio_ec = boost::system::error_code;
	class http_client : public std::enable_shared_from_this<http_client>
	{
	private:
		asio::ip::tcp::resolver m_resolver;
		asio::ip::tcp::socket m_socket;
		std::function<void(const std::string &, const reply &)> m_callback;
		const std::string m_req_str;
		const std::string m_server_url;
		const std::string m_server_port;
		std::string m_header_read_buffer;
		std::array<char, 4096> m_content_read_buffer;
		std::ostringstream reply_oss;
		// timeout timer
		asio::basic_waitable_timer<std::chrono::steady_clock> m_timer;
		const std::size_t m_timeout_seconds = 5;
		http_reply_parser m_rep_parser;
		std::shared_ptr<spdlog::logger> m_logger;
	public:
		http_client(asio::io_context &io_context, std::shared_ptr<spdlog::logger> in_logger, const std::string &server_url, const std::string &server_port, const request &req, std::function<void(const std::string &, const reply &)> callback, std::uint32_t timeout_second);
		void run();

	private:
		void handle_resolve(const asio_ec& err, asio::ip::tcp::resolver::iterator iterator);
		void handle_connect(const asio_ec &err);
		void handle_write_request(const asio_ec &err);
		void handle_read_content(const asio_ec &err, std::size_t n);
		void invoke_callback(const std::string &err);
		void on_timeout(const asio_ec &err);
	};
}