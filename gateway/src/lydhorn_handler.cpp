#include "lydhorn_handler.h"

#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Path.h"
#include "Poco/URI.h"


void lydhorn_handler(const std::shared_ptr<restbed::Session> session)
{
	const std::shared_ptr<const restbed::Request> request = session->get_request();
    int response_status;

    try
	{
		Poco::URI uri(LYDHORN_URI);
		std::string path(uri.getPathAndQuery());
		if (path.empty()) path = "/";

		Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
		
        Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
        session.sendRequest(request);

        Poco::Net::HTTPResponse response;
        session.receiveResponse(response);
        response_status = response.getStatus();
	}
	catch (Poco::Exception& exc)
	{
        response_status = restbed::INTERNAL_SERVER_ERROR;
	}
    
	closeConnection(session, response_status, "");
}
