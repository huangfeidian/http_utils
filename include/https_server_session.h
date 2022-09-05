
#pragma once

#include <array>
#include <memory>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include "http_request_parser.h"
#include "http_session_manager.h"

namespace spiritsaway::http_utils
{
	/// Represents a single https_server_session from a client.
	class https_server_session
		: public std::enable_shared_from_this<https_server_session>
	{
	public:
		https_server_session(const https_server_session &) = delete;
		https_server_session &operator=(const https_server_session &) = delete;

		/// Construct a https_server_session with the given socket.
		explicit https_server_session(asio::ssl::stream<asio::ip::tcp::socket> socket, http_session_manager<https_server_session>& session_mgr, const request_handler &handler);

		/// Start the first asynchronous operation for the https_server_session.
		void start();

		/// Stop all asynchronous operations associated with the https_server_session.
		void stop();

	private:
		void do_handshake();
		/// Perform an asynchronous read operation.
		void do_read();

		/// Perform an asynchronous write operation.
		void on_reply(const reply &in_reply);
		void do_write();
		bool should_close() const;
		
		void handle_request();
		void on_timeout();

		/// Socket for the https_server_session.
		asio::ssl::stream<asio::ip::tcp::socket> socket_;

		/// The manager for this https_server_session.

		/// The handler used to process the incoming request.
		const request_handler m_request_handler;

		/// Buffer for incoming data.
		std::array<char, 8192> buffer_;

		/// The incoming request.
		std::shared_ptr<request> request_;

		/// The parser for the incoming request.
		http_request_parser m_request_parser;

		http_session_manager<https_server_session>& m_session_mgr;

		/// The reply to be sent back to the client.
		reply reply_;

		std::string m_reply_str;

		// timeout timer
		asio::basic_waitable_timer<std::chrono::steady_clock> con_timer_;
		const std::size_t timeout_seconds_ = 5;
	};

	typedef std::shared_ptr<https_server_session> https_server_session_ptr;

}
