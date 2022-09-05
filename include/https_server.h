#pragma once

#include <asio.hpp>
#include <string>
#include "https_server_session.h"

#include <asio/ssl.hpp>

namespace spiritsaway::http_utils
{

	/// The top-level class of the HTTP server.
	class https_server
	{
	public:
		https_server(const https_server &) = delete;
		https_server &operator=(const https_server &) = delete;

		/// Construct the server to listen on the specified TCP address and port, and
		/// serve up files from the given directory.
		explicit https_server(asio::io_context &io_context, asio::ssl::context& ssl_ctx, const std::string &address, const std::string &port, const request_handler &handler);

		/// Run the server's io_context loop.
		void run();

		void stop();

		std::size_t get_session_count();

	private:
		/// Perform an asynchronous accept operation.
		void do_accept();


		/// The io_context used to perform asynchronous operations.
		asio::io_context &m_ioc;

		asio::ssl::context& m_ssl_ctx;


		/// Acceptor used to listen for incoming https_server_sessions.
		asio::ip::tcp::acceptor m_acceptor;

		/// The https_server_session manager which owns all live https_server_sessions.
		http_session_manager<https_server_session> m_session_mgr;

		/// The handler for all incoming requests.
		const request_handler m_request_handler;

		const std::string m_address;
		const std::string m_port;
	};

} // namespace spiritsaway::https_server