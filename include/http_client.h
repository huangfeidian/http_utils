#pragma once

#include <spdlog/spdlog.h>

#include <magic_enum.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>


#include "http_common.h"

namespace spiritsaway::http_utils::client
{

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
using logger_t = std::shared_ptr<spdlog::logger>;
using request_data_t = spiritsaway::http_utils::common::request_data;
using spiritsaway::http_utils::common::callback_t;
using spiritsaway::http_utils::common::error_pos;

//------------------------------------------------------------------------------

class session : public std::enable_shared_from_this<session>
{
	tcp::resolver resolver_;
	beast::tcp_stream stream_;
	beast::flat_buffer buffer_; // (Must persist between reads)
	http::request<http::string_body> req_;

	http::response<http::string_body> res_;
	request_data_t origin_req;
	const std::uint32_t expire_time;
	std::weak_ptr<callback_t> callback;
	logger_t logger;


public:
	explicit session(
		net::io_context& ioc,
		request_data_t in_request_data,
		logger_t in_logger,
		std::weak_ptr<callback_t> in_callback,
		std::uint32_t in_expire_time);

	// Start the asynchronous operation
	virtual void run();
	

protected:
	void on_resolve(beast::error_code ec, tcp::resolver::results_type results);

	void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type);

	void on_write(beast::error_code ec, std::size_t bytes_transferred);

	void on_read(beast::error_code ec, std::size_t bytes_transferred);

	virtual void fail(beast::error_code ec, error_pos where);
	void invoke_callback(error_pos ec);
};
} // namespace spiritsaway::http_utils::client