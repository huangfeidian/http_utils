
#include <https_client.h>
#include <spdlog/fmt/ostr.h>

using namespace spiritsaway::http_utils::ssl_client;
using namespace spiritsaway::http_utils::common;
session::session(
		net::io_context& ioc,
		ssl::context& ctx,
		request_data_t in_request_data,
		logger_t in_logger,
		std::weak_ptr<callback_t> in_callback,
		std::uint32_t in_expire_time)
		: resolver_(ioc)
		, stream_(ioc, ctx)
		, logger(std::move(in_logger))
		, origin_req(in_request_data)
		, req_(create_request::create(in_request_data))
		, callback(in_callback)
		, expire_time(in_expire_time)


{

}

void session::run()
{
	// Set SNI Hostname (many hosts need this to handshake successfully)
	if(! SSL_set_tlsext_host_name(stream_.native_handle(), origin_req.host.data()))
	{
		beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
		return fail(ec, error_pos::tls_set);
	}

	if(!origin_req.method_supported())
	{
		return invoke_callback(error_pos::unsupported_method);
	}

	// Look up the domain name
	resolver_.async_resolve(
		origin_req.host,
		origin_req.port,
		beast::bind_front_handler(
			&session::on_resolve,
			shared_from_this()));
}
void session::on_resolve(
	beast::error_code ec,
	tcp::resolver::results_type results)
{
	if (ec)
		return fail(ec, error_pos::resolve);

	// Set a timeout on the operation
	beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(expire_time));

	// Make the connection on the IP address we get from a lookup
	beast::get_lowest_layer(stream_).async_connect(
		results,
		beast::bind_front_handler(
			&session::on_connect,
			shared_from_this()));
}

void session::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
{
	if(ec)
		return fail(ec, error_pos::connect);

	// Perform the SSL handshake
	stream_.async_handshake(
		ssl::stream_base::client,
		beast::bind_front_handler(
			&session::on_handshake,
			shared_from_this()));

}
void session::on_handshake(beast::error_code ec)
{
	if(ec)
		return fail(ec, error_pos::handshake);

	// Set a timeout on the operation
	beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(expire_time));

	// Send the HTTP request to the remote host
	http::async_write(stream_, req_,
		beast::bind_front_handler(
			&session::on_write,
			shared_from_this()));
}

void session::on_write(
	beast::error_code ec,
	std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec)
		return fail(ec, error_pos::write);

	// Receive the HTTP response
	http::async_read(stream_, buffer_, res_,
						beast::bind_front_handler(
							&session::on_read,
							shared_from_this()));
}

void session::on_read(
	beast::error_code ec,
	std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec)
		return fail(ec, error_pos::read);

	// Write the message to standard out
	invoke_callback(error_pos::ok);

	// Set a timeout on the operation
	beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(expire_time));

	// Gracefully close the stream
	stream_.async_shutdown(
		beast::bind_front_handler(
			&session::on_shutdown,
			shared_from_this()));
}


void session::on_shutdown(beast::error_code ec)
{
	if(ec == net::error::eof)
	{
		// Rationale:
		// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
		ec = {};
	}
	if(ec)
		return fail(ec, error_pos::shutdown);

	// If we get here then the connection is closed gracefully
}
void session::fail(beast::error_code ec, error_pos where)
{
	logger->info("https client report error {} where {} when request {} method {}", ec.message(), magic_enum::enum_name(where), origin_req.target, beast::http::to_string(origin_req.method));
	invoke_callback(where);
}
void session::invoke_callback(error_pos ec)
{
	auto cur_callback_ptr = callback.lock();
	callback.reset();
	if(!cur_callback_ptr)
	{
		return;
	}
	cur_callback_ptr->operator()(ec, res_);
}