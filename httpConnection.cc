/*
   HttpConnection
   mperron (2022)
*/

#include "httpConnection.h"
#include "util.h"

HttpConnection::DebugStats HttpConnection::s_debugStats = { 0 };

void HttpConnection::parseHeaders(const string &buf){
	stringstream ss(buf);
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

size_t HttpConnection::SizeofHeader(const string& buf){
	stringstream ss(buf);
	string line;

	size_t headersize = 0;

	while(getline(ss, line)){
		headersize += line.size() + 1; // +1 for \n which was stripped

		if(line.empty() || (line[0] == '\r'))
			return headersize;
	}

	return 0;
}

void HttpConnection::DumpDebugStats(stringstream &ss){
	ss << "+ HttpConnection DebugStats" << endl
		<< "|                m_validRequest: " << s_debugStats.m_validRequest << endl
		<< "|              m_invalidRequest: " << s_debugStats.m_invalidRequest << endl
		<< "|  m_invalidRequestBadFirstLine: " << s_debugStats.m_invalidRequestBadFirstLine << endl
		<< "| m_invalidRequestMethodNotImpl: " << s_debugStats.m_invalidRequestMethodNotImpl << endl
		<< "|                m_numMethodGet: " << s_debugStats.m_numMethodGet << endl
		<< "|               m_numMethodPost: " << s_debugStats.m_numMethodPost << endl
		<< "+" << endl;
}
