/*
	pipeConnection.h
	mperron (2022)
*/

#ifndef KWS3_PIPE_CONNECTION_H
#define KWS3_PIPE_CONNECTION_H

#include "connection.h"

class PipeConnection : public Connection {
public:
	virtual ~PipeConnection(){
		tryWrite("done");
	}

	virtual int readFailure(int code) override;

	bool nextMsg(std::string &msg);
};

#endif
