#include "http_server.h"
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
		// Check command line arguments.
		//if (argc != 2)
		//{
		//  std::cerr << "Usage: http_server <port>\n";
		//  return 1;
		//}

		// Initialise the server.
		request_handler echo_handler_ins = [](std::weak_ptr< request> weak_req, reply_handler cb)
		{
			auto req_ptr = weak_req.lock();
			if (!req_ptr)
			{
				return;
			}
			auto& req = *req_ptr;
			reply rep;
			// Fill out the reply to be sent to the client.
			rep.status_code = 200;
			rep.content = "echo request uri: " + req.uri + " body: " + req.body;
			rep.headers.resize(2);
			rep.headers[0].name = "Content-Length";
			rep.headers[0].value = std::to_string(rep.content.size());
			rep.headers[1].name = "Content-Type";
			rep.headers[1].value = "text";
			cb(rep);
		};
		std::string address = "127.0.0.1";
		std::string port = "8080";
		http_server s(cur_context, create_logger("http_server"), address, port, echo_handler_ins);

		// Run the server until stopped.
		s.run();
		cur_context.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "exception: " << e.what() << "\n";
	}

	return 0;
}