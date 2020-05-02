#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>


#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>


namespace spiritsaway::http_utils::common
{
	namespace beast = boost::beast;         // from <boost/beast.hpp>
	namespace http = beast::http;           // from <boost/beast/http.hpp>
	enum class error_pos
	{
		ok = 0,
		unsupported_method,
		tls_set,
		resolve,
		connect,
		handshake,
		read,
		write,
		timeout,
		shutdown,
		bind,
		listen,
		set_option,
		open,
		accept,
	};
	enum http_version
	{
		v1_0,
		v1_1
	};
	using callback_t = std::function<void(error_pos ec, const http::response<http::string_body>& response)>;
	struct request_data
	{
		std::string port;
		std::string host;
		std::string target;
		std::string data;
		http_version version;
		http::verb method;
		std::vector<std::pair<std::string, std::string>> headers;
		bool method_supported() const;
	};
	struct create_request
	{
		static http::request<http::string_body> create(const request_data& origin_req);
	};
	struct create_response
	{
		static beast::string_view mime_type(beast::string_view path);
		static http::response<http::string_body> bad_request(beast::string_view why, const http::request<http::string_body>& req);

		static http::response<http::string_body> not_found(beast::string_view target, const http::request<http::string_body>& req);

		static http::response<http::string_body> server_error(beast::string_view what, const http::request<http::string_body>& req);
		static std::string path_cat(beast::string_view base, beast::string_view path);
	};
}