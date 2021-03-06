﻿#include <http_client.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/ostr.h>
#include <fstream>
using namespace spiritsaway::http_utils;
using namespace std;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

std::shared_ptr<spdlog::logger> create_logger(const std::string& name)
{
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	console_sink->set_level(spdlog::level::debug);
	std::string pattern = "[" + name + "] [%^%l%$] %v";
	console_sink->set_pattern(pattern);

	auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(name + ".log", true);
	file_sink->set_level(spdlog::level::trace);
	auto logger = std::make_shared<spdlog::logger>(name, spdlog::sinks_init_list{ console_sink, file_sink });
	logger->set_level(spdlog::level::trace);
	return logger;
}

int main()
{
	// The io_context is required for all I/O
	net::io_context ioc;

	// Launch the asynchronous operation
	common::request_data cur_request;
	cur_request.host = "127.0.0.1";
	cur_request.port = "8080";
	cur_request.target = "/";
	cur_request.version = common::http_version::v1_1;
	cur_request.method = http::verb::get;
	auto cur_logger = create_logger("client");
	auto result_lambda = [=](common::error_pos ec, const http::response<http::string_body>& response)
	{
		if (ec != common::error_pos::ok)
		{
			cur_logger->info("request error {}", magic_enum::enum_name(ec));
		}
		else
		{
			cur_logger->info("request suc result write to file");
			std::ofstream output("../data/client/result.txt");
			output << response << endl;
			output.close();
		}
	};
	auto cur_callback = std::make_shared<common::callback_t>(result_lambda);
	auto cur_session = std::make_shared<client::session>(ioc, cur_request, cur_logger, cur_callback, 10);
	//cur_callback = std::shared_ptr<common::callback_t>();
	cur_session->run();

	// Run the I/O service. The call will return when
	// the get operation is complete.
	ioc.run();

	return EXIT_SUCCESS;
}
