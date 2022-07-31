/*
	tcpListener.h
	mperron (2022)
*/

#ifndef KWS3_TCP_LISTENER_H
#define KWS3_TCP_LISTENER_H

#include "connection.h"

class TcpListener : public Connection {
	struct sockaddr_in m_addr;

	int m_error;

	bool init();
	void cleanup();

public:
	TcpListener(int port) :
		m_addr({
			.sin_family = AF_INET,
			.sin_port = htons(port),
			.sin_addr = { .s_addr = htonl(INADDR_ANY) }
		}),
		m_error(0)
	{
		// TODO - note failure in some way, log m_error, peg stats, etc.
		init();
	}

	~TcpListener(){
		cleanup();
	}

	// True if the socket was actually opened and can be used.
	inline bool valid() const { return (m_error == 0); }

	// Blocks until a new connection opens, returns file descriptor.
	int accept(Connection &conn) const;
};

#endif
