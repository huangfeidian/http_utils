
#pragma once

#include <array>
#include <memory>
#include <asio.hpp>

#include "http_request_parser.h"

namespace spiritsaway::http_utils
{
	class http_server_session_manager;
	/// Represents a single http_server_session from a client.
	class http_server_session
		: public std::enable_shared_from_this<http_server_session>
	{
	public:
		http_server_session(const http_server_session &) = delete;
		http_server_session &operator=(const http_server_session &) = delete;

		/// Construct a http_server_session with the given socket.
		explicit http_server_session(asio::ip::tcp::socket socket, http_server_session_manager& session_mgr, const request_handler &handler);

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
		void on_timeout();

		/// Socket for the http_server_session.
		asio::ip::tcp::socket socket_;

		/// The manager for this http_server_session.

		/// The handler used to process the incoming request.
		const request_handler m_request_handler;

		/// Buffer for incoming data.
		std::array<char, 8192> buffer_;

		/// The incoming request.
		std::shared_ptr<request> request_;

		/// The parser for the incoming request.
		http_request_parser m_request_parser;

		http_server_session_manager& m_session_mgr;

		/// The reply to be sent back to the client.
		reply reply_;

		std::string m_reply_str;

		// timeout timer
		asio::basic_waitable_timer<std::chrono::steady_clock> con_timer_;
		const std::size_t timeout_seconds_ = 5;
	};

	typedef std::shared_ptr<http_server_session> http_server_session_ptr;

}
