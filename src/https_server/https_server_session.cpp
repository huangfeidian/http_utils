
#include "https_server_session.h"
#include <utility>
#include <vector>
#include <iostream>

namespace spiritsaway::http_utils {

	https_server_session::https_server_session(asio::ssl::stream<asio::ip::tcp::socket> socket, http_session_manager<https_server_session>& session_mgr, const request_handler& handler)
		: socket_(std::move(socket)),
		m_session_mgr(session_mgr),
		m_request_handler(handler),
		con_timer_(socket_.get_executor())
	{
		std::cout << "new https_server_session begin" << std::endl;
	}

	void https_server_session::start()
	{
		do_handshake();
	}

	void https_server_session::stop()
	{
		asio::error_code ignored_ec;
		socket_.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both,
			ignored_ec);
		m_session_mgr.stop(shared_from_this());
	}
	void https_server_session::do_handshake()
	{
		auto self(shared_from_this());
		socket_.async_handshake(asio::ssl::stream_base::server, 
			[this, self](const std::error_code& error)
			{
			if (!error)
			{
				do_read();
			}
			else
			{
				stop();
			}
			});
	}

	void https_server_session::do_read()
	{
		auto self(shared_from_this());

		if (con_timer_.expires_from_now(std::chrono::seconds(timeout_seconds_)) != 0)
		{
			m_session_mgr.stop(self);
			return;
		}

		socket_.async_read_some(asio::buffer(buffer_),
			[this, self](std::error_code ec, std::size_t bytes_transferred)
			{
				con_timer_.cancel();

				if (!ec)
				{
					auto result = m_request_parser.parse(buffer_.data(), bytes_transferred);

					if (result == http_request_parser::result_type::good)
					{
						handle_request();
					}
					else if (result == http_request_parser::result_type::bad)
					{
						reply_ = reply::stock_reply(reply::status_type::bad_request);
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

	void https_server_session::do_write()
	{
		auto self(shared_from_this());

		if (con_timer_.expires_from_now(std::chrono::seconds(timeout_seconds_)) != 0)
		{
			stop();
			return;
		}
		m_reply_str = reply_.to_string();
		asio::async_write(socket_, asio::buffer(m_reply_str),
			[this, self](std::error_code ec, std::size_t)
			{
				if (!ec)
				{
					// Initiate graceful https_server_session closure.
					stop();
				}

				if (ec != asio::error::operation_aborted)
				{
					m_session_mgr.stop(shared_from_this());
				}
			});
	}

	void https_server_session::on_reply(const reply& in_reply)
	{
		con_timer_.cancel();
		reply_ = in_reply;
		do_write();

	}
	void https_server_session::on_timeout()
	{
		m_session_mgr.stop(shared_from_this());
	}
	void https_server_session::handle_request()
	{
		auto self = shared_from_this();
		request_ = std::make_shared<request>();
		m_request_parser.move_req(*request_);
		if (con_timer_.expires_from_now(std::chrono::seconds(timeout_seconds_)) != 0)
		{
			m_session_mgr.stop(self);
			return;
		}
		auto weak_self = std::weak_ptr<https_server_session>(self);
		auto weak_request = std::weak_ptr<request>(request_);
		m_request_handler(weak_request, [weak_self](const reply& in_reply) {
			auto strong_self = weak_self.lock();
			if (strong_self)
			{
				strong_self->on_reply(in_reply);
			}
			});
	}
}
