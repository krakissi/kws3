/*
   HttpConnection
   mperron (2022)
*/

#include "httpConnection.h"
#include "util.h"

#include <unistd.h>

using namespace std;

HttpConnection::DebugStats HttpConnection::s_debugStats = { 0 };

size_t HttpConnection::sizeofHeaders(){
	stringstream ss(m_sockstream.str());
	size_t headersize = 0;

	for(string line; getline(ss, line); ){
		headersize += line.size() + 1; // +1 for \n which was stripped

		if(line.empty() || (line[0] == '\r'))
			return (m_headersize = headersize);
	}

	return (m_headersize = 0);
}

void HttpConnection::parseHeaders(){
	stringstream ss(m_sockstream.str());
	string line;

	// Parse the first line, which indicates request method, URI, and protocol version.
	if(getline(ss, line)){
		chomp(line);
		stringstream sss(line);

		string method;

		sss >> method >> m_uri >> m_version;

		if(!sss){
			++ s_debugStats.m_invalidRequest;
			++ s_debugStats.m_invalidRequestBadFirstLine;
			m_valid = false;
			return;
		}

		if(method == "GET"){
			++ s_debugStats.m_numMethodGet;
			m_method = HTTP_METHOD_GET;
		} else if(method == "POST"){
			++ s_debugStats.m_numMethodPost;
			m_method = HTTP_METHOD_POST;
		}

		// Unknown request method!
		if(m_method == HTTP_METHOD_UNSET){
			++ s_debugStats.m_invalidRequest;
			++ s_debugStats.m_invalidRequestMethodNotImpl;

			// TODO Return method not implemented
			m_valid = false;
			return;
		}
	}

	while(getline(ss, line)){
		// End of the header section.
		if(line.empty() || (line[0] == '\r'))
			break;

		chomp(line);

		// Parse the rest of the headers.
	}

	++ s_debugStats.m_validRequest;
	m_valid = true;
}

int HttpConnection::readFailure(int code){
	switch(code){
		case EAGAIN:
			if(m_emptycounter < 500){
				// No data, so wait and increment counter.
				usleep(
					((++ m_emptycounter > 10) ?
						10000 : /* Wait 10ms if no data recently arrived. We might timeout. */
						100)    /* Wait 100us if data recently arrived, as more may quickly be coming. */
				);

				// Read will retry if return code is -1.
				return -1;
			}
			// Fall-through to default error case, breaking read loop.

		default:
			// Cause the read loop to break by returning 0.
			return 0;
	}
}

int HttpConnection::readSuccess(ssize_t rc, const char *buf){
	if(rc > 0)
		m_sockstream << buf;

	return rc;
}

void HttpConnection::receiveRequest(){
	m_emptycounter = 0;

	do {
		int rc = tryRead();

		if(rc < 0)
			// Retry
			continue;

		if(rc == 0)
			// Complete
			break;

	} while(!sizeofHeaders());

	// It's possible that we read all pending data to break and did not hit the
	// while condition, so try one more time to read the size of the headers.
	if(!m_headersize)
		sizeofHeaders();

	if(m_headersize){
		// Possibly a valid request, let's try to parse it.
		parseHeaders();
	} else {
		++ s_debugStats.m_invalidRequest;
		++ s_debugStats.m_invalidRequestIncomplete;
	}
}

void HttpConnection::echoRequest(){
	tryWrite(m_sockstream.str());
}

void HttpConnection::DumpDebugStats(stringstream &ss){
	ss << "+ HttpConnection DebugStats" << endl
		<< "|                m_validRequest: " << s_debugStats.m_validRequest << endl
		<< "|" << endl
		<< "|              m_invalidRequest: " << s_debugStats.m_invalidRequest << endl
		<< "|  m_invalidRequestBadFirstLine: " << s_debugStats.m_invalidRequestBadFirstLine << endl
		<< "| m_invalidRequestMethodNotImpl: " << s_debugStats.m_invalidRequestMethodNotImpl << endl
		<< "|    m_invalidRequestIncomplete: " << s_debugStats.m_invalidRequestIncomplete << endl
		<< "|" << endl
		<< "|                m_numMethodGet: " << s_debugStats.m_numMethodGet << endl
		<< "|               m_numMethodPost: " << s_debugStats.m_numMethodPost << endl
		<< "+" << endl;
}
