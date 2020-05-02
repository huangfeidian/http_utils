#include <http_common.h>
#include <boost/beast/version.hpp>

using namespace spiritsaway::http_utils::common;

bool request_data::method_supported() const
{
	switch (method)
	{
	case http::verb::get:
	case http::verb::head:
	case http::verb::delete_:
	case http::verb::post:
	case http::verb::put:
	case http::verb::options:
		return true;
	default:
		return false;
	}
}

http::request<http::string_body> create_request::create(const request_data& origin_req)
{
	// Set up an HTTP GET request message
	http::request<http::string_body> dest_request;
	auto cur_method = origin_req.method;
	dest_request.version(origin_req.version == http_version::v1_0?10:11);
	dest_request.method(origin_req.method);
	dest_request.target(origin_req.target);
	dest_request.set(http::field::host, origin_req.host);
	dest_request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

	if (cur_method == http::verb::post || cur_method == http::verb::put || cur_method == http::verb::options)
	{
		dest_request.body() = origin_req.data;
	}
	return dest_request;
}

beast::string_view create_response::mime_type(beast::string_view path)
{
	using beast::iequals;
	auto const ext = [&path]
	{
		auto const pos = path.rfind(".");
		if(pos == beast::string_view::npos)
			return beast::string_view{};
		return path.substr(pos);
	}();
	if(iequals(ext, ".htm"))  return "text/html";
	if(iequals(ext, ".html")) return "text/html";
	if(iequals(ext, ".php"))  return "text/html";
	if(iequals(ext, ".css"))  return "text/css";
	if(iequals(ext, ".txt"))  return "text/plain";
	if(iequals(ext, ".js"))   return "application/javascript";
	if(iequals(ext, ".json")) return "application/json";
	if(iequals(ext, ".xml"))  return "application/xml";
	if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
	if(iequals(ext, ".flv"))  return "video/x-flv";
	if(iequals(ext, ".png"))  return "image/png";
	if(iequals(ext, ".jpe"))  return "image/jpeg";
	if(iequals(ext, ".jpeg")) return "image/jpeg";
	if(iequals(ext, ".jpg"))  return "image/jpeg";
	if(iequals(ext, ".gif"))  return "image/gif";
	if(iequals(ext, ".bmp"))  return "image/bmp";
	if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
	if(iequals(ext, ".tiff")) return "image/tiff";
	if(iequals(ext, ".tif"))  return "image/tiff";
	if(iequals(ext, ".svg"))  return "image/svg+xml";
	if(iequals(ext, ".svgz")) return "image/svg+xml";
	return "application/text";
}
http::response<http::string_body> create_response::bad_request(beast::string_view why, const http::request<http::string_body>& req)
{
	http::response<http::string_body> res{http::status::bad_request, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, "text/html");
	res.keep_alive(req.keep_alive());
	res.body() = std::string(why);
	res.prepare_payload();
	return res;
}

http::response<http::string_body> create_response::not_found(beast::string_view target, const http::request<http::string_body>& req)
{
	http::response<http::string_body> res{http::status::not_found, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, "text/html");
	res.keep_alive(req.keep_alive());
	res.body() = "The resource '" + std::string(target) + "' was not found.";
	res.prepare_payload();
	return res;
}

http::response<http::string_body> create_response::server_error(beast::string_view what, const http::request<http::string_body>& req)
{
	http::response<http::string_body> res{http::status::internal_server_error, req.version()};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::content_type, "text/html");
	res.keep_alive(req.keep_alive());
	res.body() = "An error occurred: '" + std::string(what) + "'";
	res.prepare_payload();
	return res;
}
std::string create_response::path_cat(beast::string_view base, beast::string_view path)
{
	if (base.empty())
		return std::string(path);
	std::string result(base);
#ifdef BOOST_MSVC
	char constexpr path_separator = '\\';
	if (result.back() == path_separator)
		result.resize(result.size() - 1);
	result.append(path.data(), path.size());
	for (auto& c : result)
		if (c == '/')
			c = path_separator;
#else
	char constexpr path_separator = '/';
	if (result.back() == path_separator)
		result.resize(result.size() - 1);
	result.append(path.data(), path.size());
#endif
	return result;
}