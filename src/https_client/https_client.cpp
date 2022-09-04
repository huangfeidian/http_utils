#include "https_client.h"
#include <sstream>

namespace spiritsaway::http_utils
{
	https_client::https_client(asio::io_context& io_context, asio::ssl::context& ssl_context, const std::string& server_url, const std::string& server_port, const request& req, std::function<void(const std::string&, const reply&)> callback, std::uint32_t timeout_second)
		: m_socket(io_context, ssl_context), m_resolver(io_context), m_callback(callback)
		, m_req_str(req.to_string(server_url, server_port))
		, m_timer(io_context)
		, m_timeout_seconds(timeout_second)
		, m_server_url(server_url)
		, m_server_port(server_port)
	{
		//m_socket.set_verify_mode(asio::ssl::verify_peer);
		//m_socket.set_verify_callback([this](bool preverified,
		//	asio::ssl::verify_context& ctx)
		//	{
		//		return verify_certificate(preverified, ctx);
		//	});

	}
	bool https_client::verify_certificate(bool preverified,
		asio::ssl::verify_context& ctx)
	{
		char subject_name[256];
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		std::cout << "Verifying " << subject_name << "\n";

		return preverified;
	}
	void https_client::run()
	{
		auto self = shared_from_this();
		asio::ip::tcp::resolver::query query(m_server_url, m_server_port);
		m_resolver.async_resolve(query, [self, this](const asio::error_code& error, asio::ip::tcp::resolver::results_type results)
			{ handle_resolve(error, results); });
		m_timer.expires_from_now(std::chrono::seconds(m_timeout_seconds));
		m_timer.async_wait([self, this](const asio::error_code& error)
			{
				on_timeout(error);
			});
	}

	void https_client::handle_resolve(const asio::error_code& error, asio::ip::tcp::resolver::results_type results)
	{
		if (error)
		{

			invoke_callback(error.message());
			return;
		}
		auto self = shared_from_this();
		asio::async_connect(m_socket.lowest_layer(), results, [self, this](const asio::error_code& err, asio::ip::tcp::resolver::results_type::endpoint_type endpoint)
			{ handle_connect(err, endpoint); });
	}

	
	void https_client::handle_connect(const asio::error_code& err, asio::ip::tcp::resolver::results_type::endpoint_type)
	{
		if (err)
		{

			invoke_callback(err.message());
			return;
		}

		auto self = shared_from_this();
		m_socket.async_handshake(asio::ssl::stream_base::client, [self, this](const asio::error_code& err)
			{
				handle_hanshake(err);
			});
		
	}
	void https_client::handle_hanshake(const asio::error_code& err)
	{
		if (err)
		{

			invoke_callback(err.message());
			return;
		}
		auto self = shared_from_this();
		asio::async_write(m_socket, asio::buffer(m_req_str), [self, this](const asio::error_code& err, std::size_t write_sz)
			{ handle_write_request(err); });
	}
	void https_client::handle_write_request(const asio::error_code& err)
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

	void https_client::handle_read_content(const asio::error_code& err, std::size_t n)
	{
		if (err)
		{
			if (err == asio::error::eof)
			{
				invoke_callback("");
			}
			else
			{
				invoke_callback(err.message());
			}
			return;
		}
		std::string temp_content(m_content_read_buffer.data(), n);
		// std::cout << "read content: " << temp_content << std::endl;
		auto temp_parse_result = m_rep_parser.parse(m_content_read_buffer.data(), n);
		if (temp_parse_result == http_reply_parser::result_type::bad)
		{
			invoke_callback("invalid reply");
			return;
		}
		m_socket.async_read_some(asio::buffer(m_content_read_buffer.data(), m_content_read_buffer.size()), [self = shared_from_this(), this](const asio::error_code& err, std::size_t bytes_transferred)
		{
			handle_read_content(err, bytes_transferred);
		});

	}
	void https_client::invoke_callback(const std::string& err)
	{
		m_timer.cancel();
		m_callback(err, m_rep_parser.m_reply);
		asio::error_code ignore_ec;
		m_socket.lowest_layer().close(ignore_ec);

	}

	void https_client::on_timeout(const asio::error_code& err)
	{
		if (err != asio::error::operation_aborted)
		{
			invoke_callback("timeout");
		}
	}


}