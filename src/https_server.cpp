
#include <https_server.h>
#include <magic_enum.hpp>
#include <spdlog/fmt/ostr.h>


using namespace spiritsaway::http_utils::common;
using namespace spiritsaway::http_utils::ssl_server;

// Start the asynchronous operation
void session::run()
{
	// We need to be executing within a strand to perform async operations
	// on the I/O objects in this session. Although not strictly necessary
	// for single-threaded contexts, this example code is written to be
	// thread-safe by default.
	net::dispatch(
		stream_.get_executor(),
		beast::bind_front_handler(
			&session::on_run,
			shared_from_this()));
}

void session::on_run()
{
	// Set the timeout.
		beast::get_lowest_layer(stream_).expires_after(
			std::chrono::seconds(expire_time));

		// Perform the SSL handshake
		stream_.async_handshake(
			ssl::stream_base::server,
			beast::bind_front_handler(
				&session::on_handshake,
				shared_from_this()));
}

void session::on_handshake(beast::error_code ec)
{
	if(ec)
		return fail(ec, error_pos::handshake);

	do_read();
}

void session::do_read()
{
	// Make the request empty before reading,
	// otherwise the operation behavior is undefined.
	req_ = {};

	// Set the timeout.
	beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(expire_time));

	// Read a request
	http::async_read(stream_, buffer_, req_,
		beast::bind_front_handler(
			&session::on_read,
			shared_from_this()));
}

void session::on_read(
	beast::error_code ec,
	std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	// This means they closed the connection
	if(ec == http::error::end_of_stream)
		return do_close();

	if(ec)
		return fail(ec, error_pos::read);

	// Send the response
	handle_request_precheck();
}

void session::on_write(
	bool close,
	beast::error_code ec,
	std::size_t bytes_transferred)
{
	boost::ignore_unused(bytes_transferred);

	if (ec)
		return fail(ec, error_pos::write);

	if (close)
	{
		// This means we should close the connection, usually because
		// the response indicated the "Connection: close" semantic.
		return do_close();
	}

	// We're done with the response so delete it
	res_ = nullptr;

	// Read another request
	do_read();
}

void session::do_close()
{
	// Set the timeout.
	beast::get_lowest_layer(stream_).expires_after(std::chrono::seconds(expire_time));

	// Perform the SSL shutdown
	stream_.async_shutdown(
		beast::bind_front_handler(
			&session::on_shutdown,
			shared_from_this()));
}

void session::on_shutdown(beast::error_code ec)
{
	if(ec)
		return fail(ec, error_pos::shutdown);

	// At this point the connection is closed gracefully
}


void session::fail(beast::error_code ec, error_pos where)
{
	// ssl::error::stream_truncated, also known as an SSL "short read",
	// indicates the peer closed the connection without performing the
	// required closing handshake (for example, Google does this to
	// improve performance). Generally this can be a security issue,
	// but if your communication protocol is self-terminated (as
	// it is with both HTTP and WebSocket) then you may simply
	// ignore the lack of close_notify.
	//
	// https://github.com/boostorg/beast/issues/38
	//
	// https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown
	//
	// When a short read would cut off the end of an HTTP message,
	// Beast returns the error beast::http::error::partial_message.
	// Therefore, if we see a short read here, it has occurred
	// after the message has been completed, so it is safe to ignore it.

	if(ec == net::ssl::error::stream_truncated)
	{
		return;
	}
	logger->info("fail in https server session ec is {} where is {}", ec.message(), magic_enum::enum_name(where));
}



void session::handle_request_precheck()
{
	// Make sure we can handle the method
	if( req_.method() != http::verb::get &&
		req_.method() != http::verb::head)
		return do_write(create_response::bad_request("Unknown HTTP-method", req_));

	// Request path must be absolute and not contain "..".
	if( req_.target().empty() ||
		req_.target()[0] != '/' ||
		req_.target().find("..") != beast::string_view::npos)
		return do_write(create_response::bad_request("Illegal request-target", req_));

	route_request();
	
}
void session::route_request()
{
	return do_write(create_response::not_found("route_request not implemented", req_));
}

void file_session::route_request()
{
	// Build the path to the requested file

	std::string path = create_response::path_cat(*doc_root_, req_.target());
	if(req_.target().back() == '/')
		path.append("index.html");

	// Attempt to open the file
	beast::error_code ec;
	http::file_body::value_type body;
	body.open(path.c_str(), beast::file_mode::scan, ec);

	// Handle the case where the file doesn't exist
	if(ec == beast::errc::no_such_file_or_directory)
		return do_write(create_response::not_found(req_.target(), req_));

	// Handle an unknown error
	if(ec)
		return do_write(create_response::server_error(ec.message(), req_));

	// Cache the size since we need it after the move
	auto const size = body.size();

	// Respond to HEAD request
	if(req_.method() == http::verb::head)
	{
		http::response<http::empty_body> res{http::status::ok, req_.version()};
		res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
		res.set(http::field::content_type, create_response::mime_type(path));
		res.content_length(size);
		res.keep_alive(req_.keep_alive());
		return do_write(std::move(res));
	}

	// Respond to GET request
	http::response<http::file_body> res{
		std::piecewise_construct,
		std::make_tuple(std::move(body)),
		std::make_tuple(http::status::ok, req_.version())};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, create_response::mime_type(path));
	res.content_length(size);
	res.keep_alive(req_.keep_alive());
	return do_write(std::move(res));
}

listener::listener(
	net::io_context& ioc,
	ssl::context& ctx,
	tcp::endpoint endpoint,
	logger_t in_logger,
	std::uint32_t in_expire_time)
	: ioc_(ioc)
	, ctx_(ctx)
	, acceptor_(ioc)
	, logger(std::move(in_logger))
	, expire_time(in_expire_time)
{
	beast::error_code ec;

	// Open the acceptor
	acceptor_.open(endpoint.protocol(), ec);
	if(ec)
	{
		fail(ec, error_pos::open);
		return;
	}

	// Allow address reuse
	acceptor_.set_option(net::socket_base::reuse_address(true), ec);
	if(ec)
	{
		fail(ec, error_pos::set_option);
		return;
	}

	// Bind to the server address
	acceptor_.bind(endpoint, ec);
	if(ec)
	{
		fail(ec, error_pos::bind);
		return;
	}

	// Start listening for connections
	acceptor_.listen(
		net::socket_base::max_listen_connections, ec);
	if(ec)
	{
		fail(ec, error_pos::listen);
		return;
	}
	valid = true;
}

// Start accepting incoming connections
bool listener::run()
{
	if(!valid)
	{
		return false;
	}
	do_accept();
	return true;
}

void listener::do_accept()
{
	// The new connection gets its own strand
	acceptor_.async_accept(
		net::make_strand(ioc_),
		beast::bind_front_handler(
			&listener::on_accept,
			shared_from_this()));
}

void listener::on_accept(beast::error_code ec, tcp::socket socket)
{
	if(ec)
	{
		fail(ec, error_pos::accept);
	}
	else
	{
		// Create the session and run it
		auto cur_session = make_session(std::move(socket));
		cur_session->run();
	}

	// Accept another connection
	do_accept();
}
void listener::fail(beast::error_code ec, error_pos where)
{
	logger->info("fail in https server listend ec is {} what is {}", ec.message(), magic_enum::enum_name(where));
}

std::shared_ptr<session> listener::make_session(tcp::socket&& socket)
{
	return std::make_shared<session>(std::move(socket), ctx_, logger,  expire_time);
}

std::shared_ptr<session> file_listener::make_session(tcp::socket&& socket)
{
	return std::make_shared<file_session>(std::move(socket), ctx_, logger, expire_time, doc_root);
}