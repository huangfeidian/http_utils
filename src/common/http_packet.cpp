#include "http_packet.h"
#include <sstream>
namespace spiritsaway::http_utils
{
	namespace status_strings
	{

		const std::string ok =
			"HTTP/1.0 200 OK\r\n";
		const std::string created =
			"HTTP/1.0 201 Created\r\n";
		const std::string accepted =
			"HTTP/1.0 202 Accepted\r\n";
		const std::string no_content =
			"HTTP/1.0 204 No Content\r\n";
		const std::string multiple_choices =
			"HTTP/1.0 300 Multiple Choices\r\n";
		const std::string moved_permanently =
			"HTTP/1.0 301 Moved Permanently\r\n";
		const std::string moved_temporarily =
			"HTTP/1.0 302 Moved Temporarily\r\n";
		const std::string not_modified =
			"HTTP/1.0 304 Not Modified\r\n";
		const std::string bad_request =
			"HTTP/1.0 400 Bad Request\r\n";
		const std::string unauthorized =
			"HTTP/1.0 401 Unauthorized\r\n";
		const std::string forbidden =
			"HTTP/1.0 403 Forbidden\r\n";
		const std::string not_found =
			"HTTP/1.0 404 Not Found\r\n";
		const std::string internal_server_error =
			"HTTP/1.0 500 Internal Server Error\r\n";
		const std::string not_implemented =
			"HTTP/1.0 501 Not Implemented\r\n";
		const std::string bad_gateway =
			"HTTP/1.0 502 Bad Gateway\r\n";
		const std::string service_unavailable =
			"HTTP/1.0 503 Service Unavailable\r\n";

		std::string to_string(reply::status_type status)
		{
			switch (status)
			{
			case reply::status_type::ok:
				return ok;
			case reply::status_type::created:
				return created;
			case reply::status_type::accepted:
				return accepted;
			case reply::status_type::no_content:
				return no_content;
			case reply::status_type::multiple_choices:
				return multiple_choices;
			case reply::status_type::moved_permanently:
				return moved_permanently;
			case reply::status_type::moved_temporarily:
				return moved_temporarily;
			case reply::status_type::not_modified:
				return not_modified;
			case reply::status_type::bad_request:
				return bad_request;
			case reply::status_type::unauthorized:
				return unauthorized;
			case reply::status_type::forbidden:
				return forbidden;
			case reply::status_type::not_found:
				return not_found;
			case reply::status_type::internal_server_error:
				return internal_server_error;
			case reply::status_type::not_implemented:
				return not_implemented;
			case reply::status_type::bad_gateway:
				return bad_gateway;
			case reply::status_type::service_unavailable:
				return service_unavailable;
			default:
				return internal_server_error;
			}
		}

	} // namespace status_strings

	namespace misc_strings
	{

		const char name_value_separator[] = {':', ' '};
		const char crlf[] = {'\r', '\n'};

	} // namespace misc_strings

	std::string reply::to_string()
	{
		std::vector<std::string> buffers;
		buffers.push_back(status_strings::to_string(reply::status_type(status_code)));
		for (std::size_t i = 0; i < headers.size(); ++i)
		{
			header &h = headers[i];
			buffers.push_back(h.name);
			buffers.push_back(misc_strings::name_value_separator);
			buffers.push_back(h.value);
			buffers.push_back(misc_strings::crlf);
		}
		buffers.push_back(misc_strings::crlf);
		buffers.push_back(content);
		std::size_t total_sz = 0;
		for (const auto& one_str : buffers)
		{
			total_sz += one_str.size();
		}
		std::string result;
		result.reserve(total_sz);
		for (const auto& one_str : buffers)
		{
			result += one_str;
		}
		return result;
	}

	namespace stock_replies
	{

		const char ok[] = "";
		const char created[] =
			"<html>"
			"<head><title>Created</title></head>"
			"<body><h1>201 Created</h1></body>"
			"</html>";
		const char accepted[] =
			"<html>"
			"<head><title>Accepted</title></head>"
			"<body><h1>202 Accepted</h1></body>"
			"</html>";
		const char no_content[] =
			"<html>"
			"<head><title>No Content</title></head>"
			"<body><h1>204 Content</h1></body>"
			"</html>";
		const char multiple_choices[] =
			"<html>"
			"<head><title>Multiple Choices</title></head>"
			"<body><h1>300 Multiple Choices</h1></body>"
			"</html>";
		const char moved_permanently[] =
			"<html>"
			"<head><title>Moved Permanently</title></head>"
			"<body><h1>301 Moved Permanently</h1></body>"
			"</html>";
		const char moved_temporarily[] =
			"<html>"
			"<head><title>Moved Temporarily</title></head>"
			"<body><h1>302 Moved Temporarily</h1></body>"
			"</html>";
		const char not_modified[] =
			"<html>"
			"<head><title>Not Modified</title></head>"
			"<body><h1>304 Not Modified</h1></body>"
			"</html>";
		const char bad_request[] =
			"<html>"
			"<head><title>Bad Request</title></head>"
			"<body><h1>400 Bad Request</h1></body>"
			"</html>";
		const char unauthorized[] =
			"<html>"
			"<head><title>Unauthorized</title></head>"
			"<body><h1>401 Unauthorized</h1></body>"
			"</html>";
		const char forbidden[] =
			"<html>"
			"<head><title>Forbidden</title></head>"
			"<body><h1>403 Forbidden</h1></body>"
			"</html>";
		const char not_found[] =
			"<html>"
			"<head><title>Not Found</title></head>"
			"<body><h1>404 Not Found</h1></body>"
			"</html>";
		const char internal_server_error[] =
			"<html>"
			"<head><title>Internal Server Error</title></head>"
			"<body><h1>500 Internal Server Error</h1></body>"
			"</html>";
		const char not_implemented[] =
			"<html>"
			"<head><title>Not Implemented</title></head>"
			"<body><h1>501 Not Implemented</h1></body>"
			"</html>";
		const char bad_gateway[] =
			"<html>"
			"<head><title>Bad Gateway</title></head>"
			"<body><h1>502 Bad Gateway</h1></body>"
			"</html>";
		const char service_unavailable[] =
			"<html>"
			"<head><title>Service Unavailable</title></head>"
			"<body><h1>503 Service Unavailable</h1></body>"
			"</html>";

		std::string to_string(reply::status_type status)
		{
			switch (status)
			{
			case reply::status_type::ok:
				return ok;
			case reply::status_type::created:
				return created;
			case reply::status_type::accepted:
				return accepted;
			case reply::status_type::no_content:
				return no_content;
			case reply::status_type::multiple_choices:
				return multiple_choices;
			case reply::status_type::moved_permanently:
				return moved_permanently;
			case reply::status_type::moved_temporarily:
				return moved_temporarily;
			case reply::status_type::not_modified:
				return not_modified;
			case reply::status_type::bad_request:
				return bad_request;
			case reply::status_type::unauthorized:
				return unauthorized;
			case reply::status_type::forbidden:
				return forbidden;
			case reply::status_type::not_found:
				return not_found;
			case reply::status_type::internal_server_error:
				return internal_server_error;
			case reply::status_type::not_implemented:
				return not_implemented;
			case reply::status_type::bad_gateway:
				return bad_gateway;
			case reply::status_type::service_unavailable:
				return service_unavailable;
			default:
				return internal_server_error;
			}
		}

	} // namespace stock_replies

	reply reply::stock_reply(reply::status_type status)
	{
		reply rep;
		rep.status_code = int(status);
		rep.content = stock_replies::to_string(status);
		rep.headers.resize(2);
		rep.headers[0].name = "Content-Length";
		rep.headers[0].value = std::to_string(rep.content.size());
		rep.headers[1].name = "Content-Type";
		rep.headers[1].value = "text/html";
		return rep;
	}

	std::string request::to_string(const std::string& server_url, const std::string& server_port) const
	{
		std::ostringstream request_stream;
		request_stream << method << " " << uri << " HTTP/" << http_version_major << "." << http_version_minor << "\r\n";
		request_stream << "Host: " << server_url <<"\r\n";
		request_stream << "Accept: */*\r\n";
		for (const auto &one_header : headers)
		{
			request_stream << one_header.name << ": " << one_header.value << "\r\n";
		}
		request_stream << "Connection: close\r\n";
		request_stream << "Content-Length: " << body.size() << "\r\n\r\n";
		request_stream << body;
		return request_stream.str();
	}
	std::string parse_uri(const std::string& full_path, std::string& server_url, std::string& server_port, std::string& resource_path)
	{
		std::string_view path_view(full_path);
		static const std::string http_prefix = "http://";
		static const std::string https_prefix = "https://";
		bool is_https = false;
		if(path_view.find(http_prefix) == 0)
		{
			path_view.remove_prefix(http_prefix.size());
		}
		else if(path_view.find(https_prefix) == 0)
		{
			path_view.remove_prefix(https_prefix.size());
			is_https = true;
		}
		auto resource_iter = path_view.find("/");
		if(resource_iter == std::string_view::npos)
		{
			resource_path = "/";
		}
		else
		{
			resource_path = path_view.substr(resource_iter);
			path_view = path_view.substr(0, resource_iter);
		}
		
		auto port_iter = path_view.find(":");
		if(port_iter == std::string_view::npos)
		{
			server_port = is_https? "443": "80";
			server_url = path_view;
		}
		else
		{
			server_port = path_view.substr(port_iter + 1);
			server_url = path_view.substr(0, port_iter);
		}
		return {};
	}
}