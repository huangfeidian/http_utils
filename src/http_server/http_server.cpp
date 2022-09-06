
#include "http_server.h"
#include <utility>

namespace spiritsaway::http_utils
{

	http_server::http_server(asio::io_context& io_context, std::shared_ptr<spdlog::logger> in_logger, const std::string& address, const std::string& port)
		: m_ioc(io_context)
		, m_logger(in_logger)
		, m_acceptor(io_context)
		, m_session_mgr()
		, m_address(address)
		, m_port(port)
	{

	}

	void http_server::run()
	{
		// Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
		asio::ip::tcp::resolver resolver(m_ioc);
		asio::ip::tcp::endpoint endpoint =
			*resolver.resolve(m_address, m_port).begin();
		m_acceptor.open(endpoint.protocol());
		m_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
		m_acceptor.bind(endpoint);
		m_acceptor.listen();
		do_accept();
	}

	void http_server::do_accept()
	{
		m_acceptor.async_accept(
			[this](std::error_code ec, asio::ip::tcp::socket socket) {
				// Check whether the http_server was stopped by a signal before this
				// completion handler had a chance to run.
				if (!m_acceptor.is_open())
				{
					return;
				}

				if (!ec)
				{
					m_session_mgr.start(std::make_shared<http_server_session>(
						std::move(socket), m_logger, m_session_counter++, m_session_mgr, [this](const request& req, reply_handler rep_cb)
						{
							return handle_request(req, rep_cb);
						}));
				}

				do_accept();
			});
	}


	void http_server::stop()
	{
		m_acceptor.close();
		m_session_mgr.stop_all();
	}

	std::size_t http_server::get_session_count()
	{
		return m_session_mgr.get_session_count();
	}
} // namespace spiritsaway::http_http_server
