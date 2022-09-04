#include "http_request_parser.h"

namespace spiritsaway::http_utils
{
	namespace
	{
		int on_url_cb(http_parser *parser, const char *at, std::size_t length)
		{
			auto &t = *reinterpret_cast<http_request_parser *>(parser->data);
			t.m_req.uri.append(at, length);
			return 0;
		}
		int on_body_cb(http_parser *parser, const char *at, std::size_t length)
		{
			auto &t = *reinterpret_cast<http_request_parser *>(parser->data);

			t.m_req.body.append(at, length);
			return 0;
		}
		int on_header_field_cb(http_parser *parser, const char *at, std::size_t length)
		{
			auto &t = *reinterpret_cast<http_request_parser *>(parser->data);
			header temp_header;
			temp_header.name = std::string(at, length);
			t.m_req.headers.push_back(temp_header);
			return 0;
		}
		int on_header_value_cb(http_parser *parser, const char *at, std::size_t length)
		{
			auto &t = *reinterpret_cast<http_request_parser *>(parser->data);

			t.m_req.headers.back().value = std::string(at, length);
			return 0;
		}
		int on_header_complete_cb(http_parser *parser)
		{
			return 0;
		}
		int on_message_complete_cb(http_parser *parser)
		{
			auto &t = *reinterpret_cast<http_request_parser *>(parser->data);
			t.m_req_complete = true;
			return 0;
		}
	} // namespace
	http_request_parser::http_request_parser()
		: m_parser(), m_parse_settings()
	{
		http_parser_init(&m_parser, http_parser_type::HTTP_REQUEST);
		m_parser.data = reinterpret_cast<void *>(this);
		m_parse_settings.on_url = on_url_cb;
		m_parse_settings.on_body = on_body_cb;
		m_parse_settings.on_header_field = on_header_field_cb;
		m_parse_settings.on_header_value = on_header_value_cb;
		m_parse_settings.on_headers_complete = on_header_complete_cb;
		m_parse_settings.on_message_complete = on_message_complete_cb;
	}
	http_request_parser::result_type http_request_parser::parse(const char *input, std::size_t len)
	{
		std::size_t nparsed = http_parser_execute(&m_parser, &m_parse_settings, input, len);
		if (m_parser.upgrade)
		{
			return http_request_parser::result_type::bad;
		}
		if (nparsed != len)
		{
			return http_request_parser::result_type::bad;
		}
		if (m_req_complete)
		{
			return http_request_parser::result_type::good;
		}
		else
		{
			return http_request_parser::result_type::indeterminate;
		}
	}
	void http_request_parser::move_req(request &dest)
	{
		dest = std::move(m_req);
	}

} // namespace spiritsaway::http_server
