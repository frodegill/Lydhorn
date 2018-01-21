#ifndef _MAIN_H_
#define _MAIN_H_

#include <memory>
#include <cstdlib>
#include <restbed>


#define LYDHORN_URI     "http://10.0.0.205/"

#define WORKER_THREADS (4)
#define REST_PORT      (1113)
#ifndef DBG
 #define SECURE
#endif


void log_arg(FILE* stream, const char* format, va_list& arguments);
void log(FILE* stream, const char* format, ...);

void nowAsString(char* buf, const size_t& buf_len);

void closeConnection(const std::shared_ptr<restbed::Session> session, int response_status, const std::string& response_body);
void closeConnection(const std::shared_ptr<restbed::Session> session, int response_status, const restbed::Bytes& response_bytes);

int main(int argc, char** argv);

#endif // _MAIN_H_
