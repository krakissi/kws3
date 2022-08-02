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
	HttpResponse(Connection *conn, const std::string &version, const int code, const std::string &msg) :
		m_conn(conn),
		m_code(code),
		m_msg(msg),
		m_version(version)
	{}

	inline std::stringstream& body(){ return m_body; }

	void status(std::stringstream &ss) const;
	void send();
};

class RspOk : public HttpResponse {
public:
	RspOk(Connection *conn, const std::string &version) :
		HttpResponse(conn, version, 200, "OK")
	{}
};

class RspBadRequest : public HttpResponse {
public:
	RspBadRequest(Connection *conn, const std::string &version) :
		HttpResponse(conn, version, 400, "Bad Request")
	{}
};

#endif
