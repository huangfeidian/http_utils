#include "http_reply_parser.h"
#include <iostream>
namespace spiritsaway::http_utils
{
	namespace
	{
		int on_chunk_header_cb(http_parser *parser)
		{
			return 0;
		}
		int on_chunk_complete_cb(http_parser *parser)
		{
			return 0;
		}
		int on_status_cb(http_parser *parser, const char *at, std::size_t length)
		{
			auto &t = *reinterpret_cast<http_reply_parser *>(parser->data);
			t.m_reply.status_code = parser->status_code;
			t.m_reply.status_detail = std::string(at, length);
			return 0;
		}
		int on_body_cb(http_parser *parser, const char *at, std::size_t length)
		{
			auto &t = *reinterpret_cast<http_reply_parser *>(parser->data);

			t.m_reply.content.append(at, length);
			return 0;
		}
		int on_header_field_cb(http_parser *parser, const char *at, std::size_t length)
		{
			auto &t = *reinterpret_cast<http_reply_parser *>(parser->data);
			header temp_header;
			temp_header.name = std::string(at, length);
			t.m_reply.headers.push_back(temp_header);
			return 0;
		}
		int on_header_value_cb(http_parser *parser, const char *at, std::size_t length)
		{
			auto &t = *reinterpret_cast<http_reply_parser *>(parser->data);

			t.m_reply.headers.back().value = std::string(at, length);
			return 0;
		}
		int on_header_complete_cb(http_parser *parser)
		{
			return 0;
		}
		int on_message_complete_cb(http_parser *parser)
		{
			auto &t = *reinterpret_cast<http_reply_parser *>(parser->data);
			t.m_reply_complete = true;
			return 0;
		}
	} // namespace
	http_reply_parser::http_reply_parser()
		: m_parser(), m_parser_settings()
	{
		http_parser_init(&m_parser, http_parser_type::HTTP_RESPONSE);
		m_parser.data = reinterpret_cast<void *>(this);
		m_parser_settings.on_body = on_body_cb;
		m_parser_settings.on_header_field = on_header_field_cb;
		m_parser_settings.on_header_value = on_header_value_cb;
		m_parser_settings.on_headers_complete = on_header_complete_cb;
		m_parser_settings.on_message_complete = on_message_complete_cb;
		m_parser_settings.on_chunk_header = on_chunk_header_cb;
		m_parser_settings.on_chunk_complete = on_chunk_complete_cb;
		m_parser_settings.on_status = on_status_cb;
	}
	http_reply_parser::result_type http_reply_parser::parse(const char *input, std::size_t len)
	{
		std::size_t nparsed = http_parser_execute(&m_parser, &m_parser_settings, input, len);
		if (m_parser.upgrade)
		{
			return http_reply_parser::result_type::bad;
		}
		if (nparsed != len)
		{
			std::cout << http_errno_name(http_errno(m_parser.http_errno)) << std::endl;
			return http_reply_parser::result_type::bad;
		}
		if (m_reply_complete)
		{
			return http_reply_parser::result_type::good;
		}
		else
		{
			return http_reply_parser::result_type::indeterminate;
		}
	}

} // namespace spiritsaway::http_utils
