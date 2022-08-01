/*
	pipeConnection.h
	mperron (2022)
*/

#ifndef KWS3_PIPE_CONNECTION_H
#define KWS3_PIPE_CONNECTION_H

#include "connection.h"

class PipeConnection : public Connection {
	bool m_write;

public:
	PipeConnection(int fd, bool isWritePipe) :
		m_write(isWritePipe)
	{
		m_fd = fd;
	}

	virtual int readFailure(int code) override;

	bool nextMsg(std::string &msg);
};

#endif
