#include <https_client.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/logger.h>
#include <fstream>

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
		asio::ssl::verify_context& ctx
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
	request cur_req;
	cur_req.uri = "/";
	cur_req.method = "GET";
	cur_req.http_version_major = 1;
	cur_req.http_version_minor = 1;
	std::string address = "www.baidu.com";
	std::string port = "443";
	// The io_context is required for all I/O
	asio::io_context ioc;
	asio::ssl::context ctx{ asio::ssl::context::sslv23 };
	ctx.set_default_verify_paths();
	//ctx.load_verify_file("../data/keys/server.crt");
	ctx.set_verify_mode(asio::ssl::verify_peer);
	ctx.set_verify_callback(make_verbose_verification(
		asio::ssl::rfc2818_verification(address)));;

	//cur_request.host = "127.0.0.1";
	auto cur_lambda = [](const std::string& err, const reply& rep)
	{
		std::cout << "err is " << err << " status is " << rep.status_code << " content is " << rep.content << std::endl;
	};

	auto cur_client = std::make_shared<https_client>(ioc,ctx, address, port, cur_req, cur_lambda, 5);

	// Run the server until stopped.
	cur_client->run();
	ioc.run();

	return EXIT_SUCCESS;
}
