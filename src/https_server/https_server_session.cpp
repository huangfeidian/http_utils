
#include "https_server_session.h"
#include <utility>
#include <vector>
#include <iostream>

namespace spiritsaway::http_utils {

	https_server_session::https_server_session(std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>>&& socket,
		std::shared_ptr<spdlog::logger> in_logger, std::uint64_t in_session_idx, http_session_manager<https_server_session>& session_mgr, const request_handler& handler)
		: m_socket(std::move(socket))
		, m_session_mgr(session_mgr)
		, m_request_handler(handler)
		, m_con_timer(m_socket->get_executor())
		, m_logger(in_logger)
		, m_session_idx(in_session_idx)
	{
	}

	void https_server_session::start()
	{
		m_logger->debug("session {} start", m_session_idx);
		do_handshake();
	}

	void https_server_session::stop()
	{
		m_logger->debug("session {} stop", m_session_idx);
		m_con_timer.cancel();
		asio_ec ignored_ec;
		m_socket->shutdown(ignored_ec);
		
	}
	void https_server_session::do_handshake()
	{
		auto self(shared_from_this());
		if (m_con_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds)) != 0)
		{
			m_session_mgr.stop(self);
			return;
		}
		m_con_timer.async_wait([self, this](const asio_ec& error)
			{
				if (error != asio::error::operation_aborted)
				{
					on_timeout("do handshake");
				}

			});
		m_socket->async_handshake(asio::ssl::stream_base::server, 
			[this, self](const asio_ec& error)
			{
				m_con_timer.cancel();
			if (!error)
			{
				do_read();
			}
			else
			{
				m_logger->error("session {} handle shake error {}", m_session_idx, error.message());
				m_session_mgr.stop(shared_from_this());
			}
			});
	}

	void https_server_session::do_read()
	{
		auto self(shared_from_this());

		if (m_con_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds)) != 0)
		{
			m_session_mgr.stop(shared_from_this());
			return;
		}
		m_con_timer.async_wait([self, this](const asio_ec& error)
			{
				if (error != asio::error::operation_aborted)
				{
					on_timeout("read_request");
				}

			});
		m_socket->async_read_some(asio::buffer(m_buffer),
			[this, self](asio_ec ec, std::size_t bytes_transferred)
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
					m_logger->error("https_server_session {} error {}", m_session_idx, ec.message());
					m_session_mgr.stop(shared_from_this());
				}
			});
	}

	void https_server_session::do_write()
	{
		auto self(shared_from_this());

		if (m_con_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds)) != 0)
		{
			m_session_mgr.stop(shared_from_this());
			return;
		}
		m_con_timer.async_wait([self, this](const asio_ec& error)
			{
				if (error != asio::error::operation_aborted)
				{
					on_timeout("write reply");
				}
			});
		m_reply_str = m_reply.to_string();
		asio::async_write(*m_socket, asio::buffer(m_reply_str),
			[this, self](asio_ec ec, std::size_t)
			{
				m_con_timer.cancel();
				if (!ec)
				{
					// Initiate graceful https_server_session closure.
					m_session_mgr.stop(shared_from_this());
					return;
				}

				if (ec != asio::error::operation_aborted)
				{
					m_logger->error("https_server_session {} error {}", m_session_idx, ec.message());
					m_session_mgr.stop(shared_from_this());
				}
			});
	}

	void https_server_session::on_reply(const reply& in_reply)
	{
		if (m_stopped)
		{
			return;
		}
		m_con_timer.cancel();
		m_reply = in_reply;
		do_write();

	}
	void https_server_session::on_timeout(const std::string& reason)
	{
		m_stopped = true;
		m_logger->warn("session {} timeout for {} ", m_session_idx, reason);
		m_session_mgr.stop(shared_from_this());
	}
	void https_server_session::handle_request()
	{
		
		m_con_timer.cancel();
		auto self = shared_from_this();
		m_request_parser.move_req(m_request);
		if (m_con_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds)) != 0)
		{
			m_session_mgr.stop(self);
			return;
		}
		m_con_timer.async_wait([self, this](const asio_ec& error)
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
