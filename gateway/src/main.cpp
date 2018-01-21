#include "main.h"

#include "lydhorn_handler.h"
#include "restbed/custom_logger.hpp"


void log_arg(FILE* stream, const char* format, va_list& arguments)
{
	char date_str[100];
	nowAsString(date_str, sizeof(date_str)/sizeof(date_str[0]));
	fprintf(stream, "%s: ", date_str);

	vfprintf(stream, format, arguments);
	fprintf(stream, "\n");
	fflush(stream);
}

void log(FILE* stream, const char* format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	
	log_arg(stream, format, arguments);

	va_end(arguments);
}

void nowAsString(char* buf, const size_t& buf_len)
{
	time_t now = time(0);
	struct tm tm = *gmtime(&now);
	::strftime(buf, buf_len-1, "%a, %d %b %Y %H:%M:%S %Z", &tm);
}

void closeConnection(const std::shared_ptr<restbed::Session> session, int response_status, const std::string& response_body)
{
	char date_str[100];
	nowAsString(date_str, sizeof(date_str)/sizeof(date_str[0]));
	session->close(response_status, response_body,
								 {{"Server", "Lydhorn Server"},
								  {"Date", date_str},
								  {"Content-Type", "text/plain; charset=utf-8"},
								  {"Content-Length", std::to_string(response_body.size())}});
}

void closeConnection(const std::shared_ptr<restbed::Session> session, int response_status, const restbed::Bytes& response_bytes)
{
	char date_str[100];
	nowAsString(date_str, sizeof(date_str)/sizeof(date_str[0]));
	session->close(response_status, response_bytes,
								 {{"Server", "Lydhorn Server"},
								  {"Date", date_str},
								  {"Content-Type", "text/plain; charset=utf-8"},
								  {"Content-Length", std::to_string(response_bytes.size())}});
}

int main(int /*argc*/, char** /*argv*/)
{
	auto lydhorn_resource = std::make_shared<restbed::Resource>();
	lydhorn_resource->set_path("activate");
	lydhorn_resource->set_method_handler("GET", lydhorn_handler);

#ifdef SECURE
	auto ssl_settings = std::make_shared<restbed::SSLSettings>();
	ssl_settings->set_port(REST_PORT);
	ssl_settings->set_http_disabled(true);
	ssl_settings->set_sslv2_enabled(false);
	ssl_settings->set_sslv3_enabled(false);
	ssl_settings->set_tlsv1_enabled(true);
	ssl_settings->set_tlsv11_enabled(true);
	ssl_settings->set_tlsv12_enabled(true);
	ssl_settings->set_private_key(restbed::Uri("file://gill-roxrud.dyndns.org.key"));
	ssl_settings->set_certificate_chain(restbed::Uri("file://fullchain.cer"));
#endif

	auto settings = std::make_shared<restbed::Settings>();
	settings->set_root("lydhorn");
	settings->set_connection_timeout(std::chrono::seconds(10));
	settings->set_worker_limit(WORKER_THREADS);
#ifdef SECURE
	settings->set_ssl_settings(ssl_settings);
#else
    settings->set_port(REST_PORT);
#endif
	settings->set_default_header("Connection", "close");

	restbed::Service service;
	service.publish(lydhorn_resource);
    service.set_logger(make_shared<CustomLogger>());

	service.start(settings);
	
	return EXIT_SUCCESS;
}
