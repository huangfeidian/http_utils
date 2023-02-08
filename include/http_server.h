#pragma once

#include <asio.hpp>
#include <string>
#include "http_server_session.h"
#include "http_session_manager.h"


namespace spiritsaway::http_utils
{

	/// The top-level class of the HTTP server.
	class http_server
	{
	public:
		http_server(const http_server &) = delete;
		http_server &operator=(const http_server &) = delete;

		/// Construct the server to listen on the specified TCP address and port, and
		/// serve up files from the given directory.
		explicit http_server(asio::io_context &io_context, std::shared_ptr<spdlog::logger> in_logger, const std::string &address, const std::string &port);

		/// Run the server's io_context loop.
		void run();

		void stop();

		std::size_t get_session_count();
		~http_server()
		{
		}
	protected:
		virtual void handle_request(const request& req, reply_handler rep_cb) = 0;
	private:
		/// Perform an asynchronous accept operation.
		void do_accept();


		/// The io_context used to perform asynchronous operations.
		asio::io_context &m_ioc;


		/// Acceptor used to listen for incoming http_server_sessions.
		asio::ip::tcp::acceptor m_acceptor;

		/// The http_server_session manager which owns all live http_server_sessions.
		http_session_manager<http_server_session> m_session_mgr;

		/// The handler for all incoming requests.

		const std::string m_address;
		const std::string m_port;
		std::atomic<std::uint64_t> m_session_counter = 0;
	protected:
		std::shared_ptr<spdlog::logger> m_logger;
	};

} // namespace spiritsaway::http_server
