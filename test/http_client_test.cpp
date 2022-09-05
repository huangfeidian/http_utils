#include "http_client.h"
#include <iostream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/logger.h>
using namespace spiritsaway::http_utils;
using namespace std;

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
	asio::io_context cur_context;

	try
	{
		auto cur_lambda = [](const std::string& err, const reply& rep)
		{
			std::cout << "err is " << err << " status is " << rep.status_code << " content is " << rep.content.substr(rep.content.size() - 20) << std::endl;
		};
		request cur_req;
		cur_req.uri = "/";
		cur_req.method = "GET";
		cur_req.http_version_major = 1;
		cur_req.http_version_minor = 1;
		std::string address = "www.baidu.com";
		std::string port = "80";
		auto cur_client = std::make_shared<http_client>(cur_context, create_logger("http_client"), address, port, cur_req, cur_lambda, 5);

		// Run the server until stopped.
		cur_client->run();
		cur_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "exception: " << e.what() << "\n";
	}

	return 0;
}