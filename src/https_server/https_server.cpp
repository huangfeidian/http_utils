
#include "https_server.h"
#include <utility>

namespace spiritsaway::http_utils
{

	https_server::https_server(asio::io_context& io_context, asio::ssl::context& in_ssl_ctx, std::shared_ptr<spdlog::logger> in_logger, const std::string& address, const std::string& port)
		: m_ioc(io_context)
		, m_ssl_ctx(in_ssl_ctx)
		, m_acceptor(io_context)
		, m_session_mgr()
		, m_address(address)
		, m_port(port)
		, m_logger(in_logger)
	{
	}

	void https_server::run()
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

	void https_server::do_accept()
	{
		m_acceptor.async_accept(
			[this](std::error_code ec, asio::ip::tcp::socket socket)
			{
				// Check whether the https_server was stopped by a signal before this
				// completion handler had a chance to run.
				if (!m_acceptor.is_open())
				{
					return;
				}

				if (!ec)
				{
					m_session_mgr.start(std::make_shared<https_server_session>(
						std::make_unique<asio::ssl::stream<asio::ip::tcp::socket>>(
							std::move(socket), m_ssl_ctx), m_logger, m_session_counter++,
						m_session_mgr, [this](const request& req, reply_handler rep_cb)
						{
							return handle_request(req, rep_cb);
						}));
				}

				do_accept();
			});
	}

	void https_server::stop()
	{
		m_acceptor.close();
		m_session_mgr.stop_all();
	}

	std::size_t https_server::get_session_count()
	{
		return m_session_mgr.get_session_count();
	}
} // namespace spiritsaway::http_https_server
