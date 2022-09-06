
#include "http_server_session.h"
#include <utility>
#include <vector>
#include "http_session_manager.h"
#include <iostream>

namespace spiritsaway::http_utils {

	http_server_session::http_server_session(asio::ip::tcp::socket socket, std::shared_ptr<spdlog::logger> in_logger, std::uint64_t in_session_idx, http_session_manager<http_server_session>& session_mgr, const request_handler& handler)
		: m_socket(std::move(socket))
		, m_session_mgr(session_mgr)
		, m_request_handler(handler)
		, m_logger(in_logger)
		, m_con_timer(m_socket.get_executor())
		, m_session_idx(in_session_idx)
	{
	}

	void http_server_session::start()
	{
		m_logger->debug("session {} start", m_session_idx);
		do_read();
	}

	void http_server_session::stop()
	{
		m_logger->debug("session {} stop", m_session_idx);
		asio::error_code ignored_ec;
		m_socket.shutdown(asio::ip::tcp::socket::shutdown_both,
			ignored_ec);
		
	}

	void http_server_session::do_read()
	{
		auto self(shared_from_this());

		if (m_con_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds)) != 0)
		{
			m_session_mgr.stop(self);
			return;
		}
		m_con_timer.async_wait([self, this](const asio::error_code& error)
			{
				if (error != asio::error::operation_aborted)
				{
					on_timeout("read_request");
				}
				
			});
		m_socket.async_read_some(asio::buffer(m_buffer),
			[this, self](std::error_code ec, std::size_t bytes_transferred)
			{
				m_con_timer.cancel();

				if (!ec)
				{
					auto result = m_request_parser.parse(m_buffer.data(), bytes_transferred);

					if (result == http_request_parser::result_type::good)
					{
						handle_request();
					}
					else if (result == http_request_parser::result_type::bad)
					{
						m_stopped = true;
						m_reply = reply::stock_reply(reply::status_type::bad_request);
						do_write();
					}
					else
					{
						do_read();
					}
				}
				else if (ec != asio::error::operation_aborted)
				{
					m_session_mgr.stop(shared_from_this());
				}
			});
	}

	void http_server_session::do_write()
	{
		auto self(shared_from_this());
		m_con_timer.cancel();
		if (m_con_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds)) != 0)
		{
			m_session_mgr.stop(self);
			return;
		}
		m_con_timer.async_wait([self, this](const asio::error_code& error)
			{
				if (error != asio::error::operation_aborted)
				{
					on_timeout("write reply");
				}
			});
		m_reply_str = m_reply.to_string();
		asio::async_write(m_socket, asio::buffer(m_reply_str),
			[this, self](std::error_code ec, std::size_t)
			{
				m_con_timer.cancel();
				if (!ec)
				{
					// Initiate graceful http_server_session closure.
					m_session_mgr.stop(shared_from_this());
					return;
				}

				if (ec != asio::error::operation_aborted)
				{
					m_session_mgr.stop(shared_from_this());
				}
			});
	}

	void http_server_session::on_reply(const reply& in_reply)
	{
		if (m_stopped)
		{
			return;
		}
		m_con_timer.cancel();
		m_reply = in_reply;
		do_write();

	}
	void http_server_session::on_timeout(const std::string& reason)
	{
		m_stopped = true;
		m_logger->warn("session {} timeout for {} ", m_session_idx, reason);
		m_session_mgr.stop(shared_from_this());
	}
	void http_server_session::handle_request()
	{
		m_con_timer.cancel();
		auto self = shared_from_this();
		m_request_parser.move_req(m_request);
		if (m_con_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds)) != 0)
		{
			m_session_mgr.stop(self);
			return;
		}
		m_con_timer.async_wait([self, this](const asio::error_code& error)
			{
				if (error != asio::error::operation_aborted)
				{
					on_timeout("handle request");
				}
			});


		m_request_handler(m_request, [self, this](const reply& in_reply) {
			on_reply(in_reply); 
			});
	}
}
