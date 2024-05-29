
#pragma once

#include <array>
#include <memory>
#include <boost/asio.hpp>

#include "http_request_parser.h"
#include "http_session_manager.h"
#include <spdlog/logger.h>

namespace spiritsaway::http_utils
{
	namespace asio = boost::asio;
	using asio_ec = boost::system::error_code;
	/// Represents a single http_server_session from a client.
	class http_server_session
		: public std::enable_shared_from_this<http_server_session>
	{
	public:
		http_server_session(const http_server_session &) = delete;
		http_server_session &operator=(const http_server_session &) = delete;

		/// Construct a http_server_session with the given socket.
		explicit http_server_session(asio::ip::tcp::socket socket, std::shared_ptr<spdlog::logger> in_logger, std::uint64_t in_session_idx, http_session_manager<http_server_session>& session_mgr, const request_handler &handler);

		/// Start the first asynchronous operation for the http_server_session.
		void start();

		/// Stop all asynchronous operations associated with the http_server_session.
		void stop();

	private:
		/// Perform an asynchronous read operation.
		void do_read();

		/// Perform an asynchronous write operation.
		void on_reply(const reply &in_reply);
		void do_write();
		bool should_close() const;
		
		void handle_request();
		void on_timeout(const std::string& reason);

		/// Socket for the http_server_session.
		asio::ip::tcp::socket m_socket;

		std::shared_ptr<spdlog::logger> m_logger;
		/// The manager for this http_server_session.

		/// The handler used to process the incoming request.
		const request_handler m_request_handler;

		/// Buffer for incoming data.
		std::array<char, 8192> m_buffer;

		/// The incoming request.
		request m_request;

		/// The parser for the incoming request.
		http_request_parser m_request_parser;

		http_session_manager<http_server_session>& m_session_mgr;

		/// The reply to be sent back to the client.
		reply m_reply;

		std::string m_reply_str;

		bool m_stopped = false;

		// timeout timer
		asio::basic_waitable_timer<std::chrono::steady_clock> m_con_timer;
		const std::size_t m_timeout_seconds = 5;
		const std::uint64_t m_session_idx;
	};

	typedef std::shared_ptr<http_server_session> http_server_session_ptr;

}
