/*
	pipeConnection.h
	mperron (2022)
*/

#ifndef KWS3_PIPE_CONNECTION_H
#define KWS3_PIPE_CONNECTION_H

#include "connection.h"

class PipeConnection : public Connection {
	bool m_write;
	time_t m_timeLastMsgRecv;

public:
	PipeConnection(int fd, bool isWritePipe) :
		m_write(isWritePipe),
		m_timeLastMsgRecv(time(NULL))
	{
		m_fd = fd;
	}

	virtual int readFailure(int code) override;

	bool nextMsg(std::string &msg);

	bool timeout();
};

#endif
