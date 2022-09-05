#include "http_client.h"
#include <sstream>

namespace spiritsaway::http_utils
{
	http_client::http_client(asio::io_context &io_context, std::shared_ptr<spdlog::logger> in_logger, const std::string &server_url, const std::string &server_port, const request &req, std::function<void(const std::string &, const reply &)> callback, std::uint32_t timeout_second)
		: m_socket(io_context), m_resolver(io_context), m_callback(callback)
		, m_req_str(req.to_string(server_url, server_port))
		, m_timer(io_context)
		, m_timeout_seconds(timeout_second)
		, m_server_url(server_url)
		, m_server_port(server_port)
		, m_logger(in_logger)
	{
		

	}

	void http_client::run()
	{
		auto self = shared_from_this();
		asio::ip::tcp::resolver::query query(m_server_url, m_server_port);
		m_resolver.async_resolve(query, [self, this](const asio::error_code& error, asio::ip::tcp::resolver::iterator iterator)
								{ handle_resolve(error, iterator); });
		m_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds));
		m_timer.async_wait([self, this](const asio::error_code& error)
		{
			on_timeout(error);
		});
	}

	void http_client::handle_resolve(const asio::error_code& error, asio::ip::tcp::resolver::iterator iterator)
	{
		if (error)
		{

			invoke_callback(error.message());
			return;
		}
		auto self = shared_from_this();
		m_socket.async_connect(*iterator, [self, this](const asio::error_code &err)
							{ handle_connect(err); });
	}

	void http_client::handle_connect(const asio::error_code &err)
	{
		if (err)
		{

			invoke_callback(err.message());
			return;
		}

		auto self = shared_from_this();
		asio::async_write(m_socket, asio::buffer(m_req_str), [self, this](const asio::error_code &err, std::size_t write_sz)
						  { handle_write_request(err); });
	}
	void http_client::handle_write_request(const asio::error_code &err)
	{
		if (err)
		{

			invoke_callback(err.message());
			return;
		}
		m_socket.async_read_some(asio::buffer(m_content_read_buffer.data(), m_content_read_buffer.size()), [self = shared_from_this(), this](const asio::error_code& err, std::size_t n)
		{
			handle_read_content(err, n);
		});
	}
	
	void http_client::handle_read_content(const asio::error_code &err, std::size_t n)
	{
		if(err)
		{
			if(err == asio::error::eof)
			{
				invoke_callback("");
			}
			else
			{
				invoke_callback(err.message());
			}
			return;
		}

		m_logger->trace("read content: {}", std::string_view(m_content_read_buffer.data(), n));
		auto temp_parse_result = m_rep_parser.parse(m_content_read_buffer.data(), n);
		if (temp_parse_result == http_reply_parser::result_type::bad)
		{
			invoke_callback("invalid reply");
			return;
		}
		m_socket.async_read_some(asio::buffer(m_content_read_buffer.data(), m_content_read_buffer.size()),  [self=shared_from_this(), this](const asio::error_code& err, std::size_t bytes_transferred)
		{
			handle_read_content(err, bytes_transferred);
		});

	}
	void http_client::invoke_callback(const std::string& err)
	{
		m_timer.cancel();
		m_callback(err, m_rep_parser.m_reply);
		m_socket.close();

	}
	
	void http_client::on_timeout(const asio::error_code& err)
	{
		if(err != asio::error::operation_aborted)
		{
			invoke_callback("timeout");
		}
	}


}