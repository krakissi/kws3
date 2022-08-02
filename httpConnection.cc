/*
   HttpConnection
   mperron (2022)
*/

#include "httpConnection.h"

#include <unistd.h>
#include <sys/mman.h>

#include "util.h"

using namespace std;

HttpConnection::DebugStats *HttpConnection::s_debugStats = nullptr;

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
		line = chomp(line);
		stringstream sss(line);

		string method;

		sss >> method >> m_uri >> m_version;

		if(!sss){
			++ s_debugStats->m_invalidRequest;
			++ s_debugStats->m_invalidRequestBadFirstLine;
			m_valid = false;
			return;
		}

		if(method == "HEAD"){
			++ s_debugStats->m_numMethodHead;
			m_method = HTTP_METHOD_HEAD;
		} else if(method == "GET"){
			++ s_debugStats->m_numMethodGet;
			m_method = HTTP_METHOD_GET;
		} else if(method == "POST"){
			++ s_debugStats->m_numMethodPost;
			m_method = HTTP_METHOD_POST;
		}

		// Unknown request method!
		if(m_method == HTTP_METHOD_UNSET){
			++ s_debugStats->m_invalidRequest;
			++ s_debugStats->m_invalidRequestMethodNotImpl;

			// TODO Return method not implemented
			m_valid = false;
			return;
		}
	}

	while(getline(ss, line)){
		// End of the header section.
		if(line.empty() || (line[0] == '\r'))
			break;

		line = chomp(line);

		// Parse the rest of the headers.
		size_t p = line.find(':');

		if((p == 0) || (p == string::npos) || (p == line.size() - 1)){
			++ s_debugStats->m_invalidRequest;
			++ s_debugStats->m_invalidRequestHeaders;

			// TODO Return 4xx invalid request
			m_valid = false;
			return;
		}

		string k = trim(line.substr(0, p));
		string v = trim(line.substr(p + 1));

		if(k.empty() || v.empty()){
			++ s_debugStats->m_invalidRequest;
			++ s_debugStats->m_invalidRequestHeaders;

			// TODO Return 4xx invalid request
			m_valid = false;
			return;
		}

		// FIXME - error for repeated headers?
		m_headers[k] = v;
	}

	++ s_debugStats->m_validRequest;
	m_valid = true;
}

void HttpConnection::receiveRequest(){
	prepareToRead();

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
		++ s_debugStats->m_invalidRequest;
		++ s_debugStats->m_invalidRequestIncomplete;
	}
}

void HttpConnection::echoRequest(){
	stringstream ss;
	string msg = m_sockstream.str();

	ss << m_version << " 200 OK\r\n"
		<< "Connection: close\r\n"
		<< "Content-Type: text/plain; charset=utf-8\r\n"
		<< "Content-Length: " << msg.size() << "\r\n"
		<< "kws3-uri: " << m_uri << "\r\n"
		<< "\r\n";

	for(auto kv : m_headers)
		ss << "[" << kv.first << "=" << kv.second << "]\r\n";

	tryWrite(ss.str());
}

void HttpConnection::DumpDebugStats(stringstream &ss){
	ss << "+ HttpConnection DebugStats" << endl
		<< "|                m_validRequest: " << s_debugStats->m_validRequest << endl
		<< "|" << endl
		<< "|              m_invalidRequest: " << s_debugStats->m_invalidRequest << endl
		<< "|  m_invalidRequestBadFirstLine: " << s_debugStats->m_invalidRequestBadFirstLine << endl
		<< "| m_invalidRequestMethodNotImpl: " << s_debugStats->m_invalidRequestMethodNotImpl << endl
		<< "|    m_invalidRequestIncomplete: " << s_debugStats->m_invalidRequestIncomplete << endl
		<< "}       m_invalidRequestHeaders: " << s_debugStats->m_invalidRequestHeaders << endl
		<< "|" << endl
		<< "|               m_numMethodHead: " << s_debugStats->m_numMethodHead << endl
		<< "|                m_numMethodGet: " << s_debugStats->m_numMethodGet << endl
		<< "|               m_numMethodPost: " << s_debugStats->m_numMethodPost << endl
		<< "+" << endl;
}

void HttpConnection::InitStats(){
	s_debugStats = (DebugStats*) mmap(NULL, sizeof(DebugStats), (PROT_READ | PROT_WRITE), (MAP_SHARED | MAP_ANONYMOUS), -1, 0);
}

void HttpConnection::UninitStats(){
	if(s_debugStats)
		munmap(s_debugStats, sizeof(DebugStats));

	s_debugStats = nullptr;
}
