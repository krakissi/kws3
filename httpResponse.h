/*
	httpResponse.h
	mperron (2022)
*/

#ifndef KWS3_HTTP_RESPONSE_H
#define KWS3_HTTP_RESPONSE_H

#include <sstream>

#include "connection.h"

class HttpResponse {
protected:
	Connection *m_conn;

	int m_code;
	std::string m_msg, m_version;

	std::stringstream m_body;

public:
	static struct DebugStats {
		uint64_t
			m_rspCreated,
			m_rspSent,

			m_rspCode200,
			m_rspCode400,
			m_rspCode501,

			m_lastone;

	} *s_debugStats;

	HttpResponse(Connection *conn, const std::string &version, const int code, const std::string &msg) :
		m_conn(conn),
		m_code(code),
		m_msg(msg),
		m_version(version)
	{
		++ s_debugStats->m_rspCreated;
	}

	inline std::stringstream& body(){ return m_body; }

	void status(std::stringstream &ss) const;
	void send();

	// Static function prototypes for shared memory stat tables.
	#include "shmemStat.h"
};


// 200 OK
class RspOk : public HttpResponse {
public:
	RspOk(Connection *conn, const std::string &version) :
		HttpResponse(conn, version, 200, "OK")
	{
		++ s_debugStats->m_rspCode200;
	}
};

// 400 Bad Request
class RspBadRequest : public HttpResponse {
public:
	RspBadRequest(Connection *conn, const std::string &version) :
		HttpResponse(conn, version, 400, "Bad Request")
	{
		++ s_debugStats->m_rspCode400;
	}
};

// 501 Not Implemented
class RspNotImplemented : public HttpResponse {
public:
	RspNotImplemented(Connection *conn, const std::string &version) :
		HttpResponse(conn, version, 501, "Not Implemented")
	{
		++ s_debugStats->m_rspCode501;
	}
};

#endif
