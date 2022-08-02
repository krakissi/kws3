/*
	httpResponse.cc
	mperron (2022)
*/

#include "httpResponse.h"

#include <sys/mman.h>

using namespace std;

HttpResponse::DebugStats *HttpResponse::s_debugStats = nullptr;

void HttpResponse::status(stringstream& ss) const {
	ss << m_version << " " << m_code << " " << m_msg << "\r\n";
}

void HttpResponse::send(){
	stringstream ss;

	// Stream in status line, e.g.: HTTP/1.1 200 OK
	status(ss);

	string body = m_body.str();

	// Headers
	ss << "Server: kws3\r\n";
	ss << "Content-Length: " << body.size() << "\r\n";

	if(body.size()){
		// FIXME debug - TODO replace with correct mime type
		ss << "Content-Type: text/plain; charset=utf-8\r\n";
	}

	// End of headers.
	ss << "\r\n";

	// Append body.
	if(body.size())
		ss << body;

	// Send message.
	m_conn->tryWrite(ss.str());
	++ s_debugStats->m_rspSent;
}


void HttpResponse::DumpDebugStats(stringstream &ss){
	ss << "+ HttpResponse DebugStats" << endl
		<< "| m_rspCreated: " << s_debugStats->m_rspCreated << endl
		<< "|    m_rspSent: " << s_debugStats->m_rspSent << endl
		<< "|" << endl
		<< "| m_rspCode200: " << s_debugStats->m_rspCode200 << endl
		<< "| m_rspCode400: " << s_debugStats->m_rspCode400 << endl
		<< "| m_rspCode501: " << s_debugStats->m_rspCode501 << endl
		<< "+" << endl;
}
KWS3_SHMEM_STAT_INIT   (HttpResponse)
KWS3_SHMEM_STAT_UNINIT (HttpResponse)
