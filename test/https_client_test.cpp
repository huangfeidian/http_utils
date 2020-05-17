#include <https_client.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/ostr.h>
#include <fstream>
#include <boost/asio/ssl.hpp>

using namespace spiritsaway::http_utils;
using namespace std;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
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

///@brief Helper class that prints the current certificate's subject
///       name and the verification results.
template <typename Verifier>
class verbose_verification
{
public:
	verbose_verification(Verifier verifier)
		: verifier_(verifier)
	{}

	bool operator()(
		bool preverified,
		boost::asio::ssl::verify_context& ctx
		)
	{
		char subject_name[256];
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		bool verified = verifier_(preverified, ctx);
		std::cout << "Verifying: " << subject_name << "\n"
			"Verified: " << verified << std::endl;
		return verified;
	}
private:
	Verifier verifier_;
};

///@brief Auxiliary function to make verbose_verification objects.
template <typename Verifier>
verbose_verification<Verifier>
make_verbose_verification(Verifier verifier)
{
	return verbose_verification<Verifier>(verifier);
}

int main()
{
	// The io_context is required for all I/O
	net::io_context ioc;

	// Launch the asynchronous operation
	common::request_data cur_request;
	cur_request.host = "www.zhihu.com";
	//cur_request.host = "127.0.0.1";
	cur_request.port = "443";
	cur_request.target = "/";
	cur_request.version = common::http_version::v1_1;
	cur_request.method = http::verb::get;
	auto cur_logger = create_logger("client_ssl");
	auto result_lambda = [=](common::error_pos ec, const http::response<http::string_body>& response)
	{
		if (ec != common::error_pos::ok)
		{
			cur_logger->info("request error {}", magic_enum::enum_name(ec));
		}
		else
		{
			cur_logger->info("request suc result write to file");
			std::ofstream output("../data/client/result_ssl.txt");
			output << response << endl;
			output.close();
		}
	};
	ssl::context ctx{ ssl::context::tlsv12_client };
	ctx.set_default_verify_paths();
	//ctx.load_verify_file("../data/keys/server.crt");
	ctx.set_verify_mode(ssl::verify_peer);
	//ctx.set_verify_callback(
	//	[=](bool v, boost::asio::ssl::verify_context &ctx) -> bool
	//{
	//	if (v)
	//		return true;
	//	X509_STORE_CTX *sctx = ctx.native_handle();
	//	int error = X509_STORE_CTX_get_error(sctx);
	//	char name[256];
	//	X509* cert = X509_STORE_CTX_get_current_cert(sctx);
	//	X509_NAME_oneline(X509_get_subject_name(cert), name, 256);
	//	cur_logger->info("verify_callback.untrusted ca ({}) error = {} preverify {}", name, error, v);
	//	return v;
	//});
	ctx.set_verify_callback(make_verbose_verification(
		boost::asio::ssl::rfc2818_verification(cur_request.host)));;
	auto cur_callback = std::make_shared<common::callback_t>(result_lambda);
	auto cur_session = std::make_shared<ssl_client::session>(ioc, ctx, cur_request, cur_logger, cur_callback, 10);
	//cur_callback = std::shared_ptr<common::callback_t>();
	cur_session->run();

	// Run the I/O service. The call will return when
	// the get operation is complete.
	ioc.run();

	return EXIT_SUCCESS;
}
