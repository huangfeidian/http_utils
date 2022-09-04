
#pragma once
#include <tuple>
#include "http_parser.h"
#include "http_packet.h"

namespace spiritsaway::http_utils
{
	/// Parser for incoming requests.
	class http_reply_parser
	{
	public:
		/// Construct ready to parse the request method.
		http_reply_parser();


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

	public:
		reply m_reply;
		bool m_reply_complete = false;

	private:
		http_parser_settings m_parser_settings;
		http_parser m_parser;
	};

}
