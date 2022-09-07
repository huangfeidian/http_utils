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
class echo_https_server : public https_server
{
public:
	using https_server::https_server;
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

	auto cur_logger = create_logger("https_server");
	try
	{
		std::string const doc_root = "../data/server/";
		std::uint8_t const threads = 2;
		std::uint32_t expire_time = 10;
		// The io_context is required for all I/O
		asio::io_context ioc;
		asio::ssl::context ctx{ asio::ssl::context::tlsv12 };
		ctx.use_certificate_chain_file("../data/keys/server.crt");
		ctx.use_private_key_file("../data/keys/server.key", asio::ssl::context::pem);
		ctx.use_tmp_dh_file("../data/keys/dh1024.pem");
		
		// Create and launch a listening port
		std::string address = "127.0.0.1";
		std::string port = "443";
		echo_https_server s(ioc, ctx, cur_logger, address, port);

		// Run the server until stopped.
		s.run();
		ioc.run();
	}
	catch (std::exception& e)
	{
		cur_logger->error("error {}", e.what());
	}
	

	return EXIT_SUCCESS;
}