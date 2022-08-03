/*
	cmdConnection.h
	mperron (2022)
*/

#ifndef KWS3_CMD_CONNECTION_H
#define KWS3_CMD_CONNECTION_H

#include "connection.h"

#include <list>

class CmdConnection : public Connection {
	BiConn *m_pipe;

	std::string m_lastCmd;
	std::list<std::string> m_pendingError;

	bool m_configMode;
	bool m_expectingMsg;

	void execCommand(const std::string &cmd);
	void execConfig(const std::string &cmd);

	bool receiveMsg();

	const std::string ERR_REMOTE_CLOSED = "remote closed signal pipe!";

public:
	static struct DebugStats {
		uint64_t
			m_cmdReceived,

			m_pipesActive,

			m_lastone;

	} *s_debugStats;


	CmdConnection() :
		m_pipe(nullptr),
		m_configMode(false),
		m_expectingMsg(false)
	{
		m_valid = true;
		m_readAgainTimeout = 6000; /* 60 seconds (6000 * 10ms) */
	}

	~CmdConnection(){
		delete m_pipe;
	}

	inline void setPipe(BiConn *bc){ m_pipe = bc; }

	bool receiveCmd();


	// Static function prototypes for shared memory stat tables.
	#include "shmemStat.h"
};

#endif
