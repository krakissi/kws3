/*
	connection.h
	mperron(2022)
*/

#ifndef KWS3_CONNECTION_H
#define KWS3_CONNECTION_H

#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

class Connection {
	void connection_cleanup();

protected:
	int m_fd;
	struct sockaddr_in m_addr_client;

	std::stringstream m_sockstream;

public:
	Connection() :
		m_fd(-1),
		m_addr_client({})
	{}

	~Connection(){
		connection_cleanup();
	}

	inline int fd(int val){ return(m_fd = val); }
	inline struct sockaddr_in& addr_client(){ return m_addr_client; }

	ssize_t tryRead();
	ssize_t tryWrite(const std::string &msg);

	virtual int readFailure(int code) { return 0; }
	virtual int readSuccess(ssize_t rc, const char *buf) { return 0; }
};

#endif
