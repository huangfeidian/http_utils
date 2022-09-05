#include <https_server.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/logger.h>

using namespace spiritsaway::http_utils;

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
int main(int argc, char* argv[])
{
	// Check command line arguments.
	//if (argc != 5)
	//{
	//	std::cerr <<
	//		"Usage: http-server-async <address> <port> <doc_root> <threads>\n" <<
	//		"Example:\n" <<
	//		"    http-server-async 0.0.0.0 8080 . 1\n";
	//	return EXIT_FAILURE;
	//}
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

	std::string const doc_root = "../data/server/";
	std::uint8_t const threads = 2;
	std::uint32_t expire_time = 10;
	// The io_context is required for all I/O
	asio::io_context ioc;
	asio::ssl::context ctx{ asio::ssl::context::tlsv12 };
	ctx.use_certificate_chain_file("../data/keys/server.crt");
	ctx.use_private_key_file("../data/keys/server.key", asio::ssl::context::pem);
	ctx.use_tmp_dh_file("../data/keys/dh512.pem");
	auto cur_logger = create_logger("server");
	// Create and launch a listening port
	std::string address = "127.0.0.1";
	std::string port = "443";
	https_server s(ioc, ctx, address, port, echo_handler_ins);

	// Run the server until stopped.
	s.run();
	ioc.run();

	return EXIT_SUCCESS;
}