/*
   HttpConnection
   mperron (2022)
*/

#ifndef KWS3_HTTP_CONNECTION_H
#define KWS3_HTTP_CONNECTION_H

#include <sstream>
#include <unordered_map>

#include "connection.h"
#include "httpResponse.h"

class HttpConnection : public Connection {
	enum RequestMethod {
		HTTP_METHOD_UNSET,
		HTTP_METHOD_HEAD,
		HTTP_METHOD_GET,
		HTTP_METHOD_POST,
	} m_method;

	int m_emptycounter;
	size_t m_headersize;

	std::string m_uri, m_version;
	std::unordered_map<std::string, std::string> m_headers;

	std::string m_body;

	// Returns the size in bytes of the received HTTP header, if one could be
	// parsed from the buf. Otherwise returns 0.
	size_t sizeofHeaders();

	// Read headers from a string buffer. Return a response if one should be
	// sent immediately.
	HttpResponse* parseHeaders();

public:
	static struct DebugStats {
		uint64_t
			m_validRequest,

			m_invalidRequest,
			m_invalidRequestBadFirstLine,
			m_invalidRequestMethodNotImpl,
			m_invalidRequestIncomplete,
			m_invalidRequestHeaders,

			m_numMethodHead,
			m_numMethodGet,
			m_numMethodPost,

			m_lastone;

	} *s_debugStats;

	HttpConnection() :
		m_headersize(0),
		m_method(HTTP_METHOD_UNSET),
		m_version("HTTP/1.1")
	{}

	// Read from the socket and look for an incoming HTTP request.
	void receiveRequest();

	// Respond to incoming request with exactly what we have received so far.
	void echoRequest();

	// Static function prototypes for shared memory stat tables.
	#include "shmemStat.h"
};

#endif
