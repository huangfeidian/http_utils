
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
		std::cout << "new http_server_session begin" << std::endl;
	}

	void http_server_session::start()
	{
		m_logger->debug("session {} start", m_session_idx);
		do_read();
	}

	void http_server_session::stop()
	{
		m_socket.close();
	}

	void http_server_session::do_read()
	{
		auto self(shared_from_this());

		if (m_con_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds)) != 0)
		{
			m_session_mgr.stop(self);
			return;
		}

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

		if (m_con_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds)) != 0)
		{
			m_session_mgr.stop(self);
			return;
		}
		m_reply_str = m_reply.to_string();
		asio::async_write(m_socket, asio::buffer(m_reply_str),
			[this, self](std::error_code ec, std::size_t)
			{
				if (!ec)
				{
					// Initiate graceful http_server_session closure.
					asio::error_code ignored_ec;
					m_socket.shutdown(asio::ip::tcp::socket::shutdown_both,
						ignored_ec);
				}

				if (ec != asio::error::operation_aborted)
				{
					m_session_mgr.stop(shared_from_this());
				}
			});
	}

	void http_server_session::on_reply(const reply& in_reply)
	{
		m_con_timer.cancel();
		m_reply = in_reply;
		do_write();

	}
	void http_server_session::on_timeout()
	{
		m_logger->warn("session {} timeout ", m_session_idx);
		m_session_mgr.stop(shared_from_this());
	}
	void http_server_session::handle_request()
	{
		auto self = shared_from_this();
		m_request = std::make_shared<request>();
		m_request_parser.move_req(*m_request);
		if (m_con_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds)) != 0)
		{
			m_session_mgr.stop(self);
			return;
		}
		auto weak_self = std::weak_ptr<http_server_session>(self);
		auto weak_request = std::weak_ptr<request>(m_request);
		m_request_handler(weak_request, [weak_self](const reply& in_reply) {
			auto strong_self = weak_self.lock();
			if (strong_self)
			{
				strong_self->on_reply(in_reply);
			}
			});
	}
}
