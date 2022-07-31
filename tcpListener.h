/*
	tcpListener.h
	mperron (2022)
*/

#ifndef KWS3_TCP_LISTENER_H
#define KWS3_TCP_LISTENER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>


class TcpListener {
	int m_fd;
	struct sockaddr_in m_addr;

	int m_error;

	bool init();
	void cleanup();

public:
	TcpListener(int port) :
		m_fd(-1),
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

	inline bool valid() const { return (m_error == 0); }

	// Blocks until a new connection opens, returns file descriptor.
	int accept(struct sockaddr_in &addr_client);
};

#endif
