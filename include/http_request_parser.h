
#pragma once
#include <tuple>
#include "http_parser.h"
#include "http_packet.h"

namespace spiritsaway::http_utils
{
	/// Parser for incoming requests.
	class http_request_parser
	{
	public:
		/// Construct ready to parse the request method.
		http_request_parser();

		void move_req(request &dest);

		/// Result of parse.
		enum class result_type
		{
			good,
			bad,
			indeterminate
		};

		/// Parse some data. The enum return value is good when a complete request has
		/// been parsed, bad if the data is invalid, indeterminate when more data is
		/// required. The InputIterator return value indicates how much of the input
		/// has been consumed.
		///
		result_type parse(const char *input, std::size_t len);

	private:
	public:
		request m_req;
		bool m_req_complete = false;

	private:
		http_parser_settings m_parse_settings;
		http_parser m_parser;
	};

}
