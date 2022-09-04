#include "http_client.h"
#include <iostream>
using namespace spiritsaway::http_utils;
using namespace std;



int main()
{
	asio::io_context cur_context;

	try
	{
		auto cur_lambda = [](const std::string& err, const reply& rep)
		{
			std::cout << "err is " << err << " status is " << rep.status_code << " content is " << rep.content << std::endl;
		};
		request cur_req;
		cur_req.uri = "/";
		cur_req.method = "GET";
		cur_req.http_version_major = 1;
		cur_req.http_version_minor = 1;
		std::string address = "192.168.1.1";
		std::string port = "80";
		auto cur_client = std::make_shared<http_client>(cur_context, address, port, cur_req, cur_lambda, 5);

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