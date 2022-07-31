/*
   HttpConnection
   mperron (2022)
*/

#ifndef KWS3_HTTP_CONNECTION_H
#define KWS3_HTTP_CONNECTION_H

#include <iostream>
#include <sstream>
#include <unordered_map>

class HttpConnection {
	bool m_valid;

	enum RequestMethod {
		HTTP_METHOD_UNSET,
		HTTP_METHOD_GET,
		HTTP_METHOD_POST,
	} m_method;

	std::string m_uri, m_version;
	std::unordered_map<std::string, std::string> m_headers;

	std::string m_body;


	static struct DebugStats {
		uint64_t
			m_validRequest,

			m_invalidRequest,
			m_invalidRequestBadFirstLine,
			m_invalidRequestMethodNotImpl,

			m_numMethodGet,
			m_numMethodPost,

			m_lastone;

	} s_debugStats;

public:
	HttpConnection() :
		m_valid(false),
		m_method(HTTP_METHOD_UNSET)
	{}

	inline bool valid() const { return m_valid; }

	// Read headers from a string buffer.
	void parseHeaders(const std::string& buf);

	// Returns the size in bytes of the received HTTP header, if one could be
	// parsed from the buf. Otherwise returns 0.
	static size_t SizeofHeader(const std::string& buf);

	// Write debug stat data to the provided stream.
	static void DumpDebugStats(std::stringstream &ss);
};

#endif
