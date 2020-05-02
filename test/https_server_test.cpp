#include <https_server.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/ostr.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using namespace spiritsaway::http_utils;
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>

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
	std::string address_str = "127.0.0.1";
	std::uint16_t port = 443;
	auto const address = net::ip::make_address(address_str);
	std::string const doc_root = "../data/server/";
	std::uint8_t const threads = 2;
	std::uint32_t expire_time = 10;
	// The io_context is required for all I/O
	net::io_context ioc{ threads };
	ssl::context ctx{ ssl::context::tlsv12 };
	ctx.use_certificate_chain_file("../data/keys/server.crt");
	ctx.use_private_key_file("../data/keys/server.key", boost::asio::ssl::context::pem);
	ctx.use_tmp_dh_file("../data/keys/dh512.pem");
	auto cur_logger = create_logger("server");
	// Create and launch a listening port
	auto cur_listener = std::make_shared<ssl_server::file_listener>(
		ioc,
		ctx,
		tcp::endpoint{ address, port },
		cur_logger,
		expire_time,
		doc_root);

	cur_listener->run();

	// Run the I/O service on the requested number of threads
	std::vector<std::thread> v;
	v.reserve(threads - 1);
	for (auto i = threads - 1; i > 0; --i)
		v.emplace_back(
			[&ioc]
	{
		ioc.run();
	});
	ioc.run();

	return EXIT_SUCCESS;
}