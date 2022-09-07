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
	std::string pattern = "[%H:%M:%S.%e %z] [" + name + "] [%^%l%$] %v";
	console_sink->set_pattern(pattern);

	auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(name + ".log", true);
	file_sink->set_level(spdlog::level::trace);
	auto logger = std::make_shared<spdlog::logger>(name, spdlog::sinks_init_list{ console_sink, file_sink });
	logger->set_level(spdlog::level::trace);
	return logger;
}

class echo_http_server: public http_server
{
public:
	using http_server::http_server;
protected:
	void handle_request(const request& req, reply_handler rep_cb) override
	{
		reply rep;
		// Fill out the reply to be sent to the client.
		rep.status_code = 200;
		rep.content = "echo request uri: " + req.uri + " body: " + req.body;
		rep.add_header("Content-Type", "text");
		rep_cb(rep);
	}
};
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

		std::string address = "127.0.0.1";
		std::string port = "8080";
		echo_http_server s(cur_context, create_logger("http_server"), address, port);

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