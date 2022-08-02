/*
	httpResponse.cc
	mperron (2022)
*/

#include "httpResponse.h"

using namespace std;

void HttpResponse::status(stringstream& ss) const {
	ss << m_version << " " << m_code << " " << m_msg << "\r\n";
}

void HttpResponse::send(){
	stringstream ss;

	// HTTP/1.1 200 OK
	status(ss);

	string body = m_body.str();

	// Headers
	ss << "Server: kws3\r\n";
	ss << "Content-Length: " << body.size() << "\r\n";

	// FIXME debug - TODO replace with correct mime type
	ss << "Content-Type: text/plain; charset=utf-8\r\n";

	// End of Headers
	ss << "\r\n";

	if(body.size())
		ss << body;

	m_conn->tryWrite(ss.str());
}

