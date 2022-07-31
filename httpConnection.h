/*
   HttpConnection
   mperron (2022)
*/

#ifndef KWS3_HTTP_CONNECTION_H
#define KWS3_HTTP_CONNECTION_H

#include <iostream>
#include <sstream>
#include <unordered_map>

using namespace std;

class HttpConnection {
	bool m_valid;

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
	enum RequestMethod {
		HTTP_METHOD_UNSET,
		HTTP_METHOD_GET,
		HTTP_METHOD_POST,
	} m_method;

	string m_uri, m_version;
	unordered_map<string, string> m_headers;

	string m_body;


	HttpConnection() :
		m_valid(false),
		m_method(HTTP_METHOD_UNSET)
	{}

	inline bool valid() const { return m_valid; }

	// Read headers from a string buffer
	void parseHeaders(const string& buf);


	// Static functions
	static size_t SizeofHeader(const string& buf);

	// Debug
	static void DumpDebugStats(stringstream &ss);
};

#endif
