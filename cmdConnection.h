/*
	cmdConnection.h
	mperron (2022)
*/

#ifndef KWS3_CMD_CONNECTION_H
#define KWS3_CMD_CONNECTION_H

#include "connection.h"

class CmdConnection : public Connection {
	Connection *m_pipe;

	std::string m_lastCmd;

public:
	static struct DebugStats {
		uint64_t
			m_cmdReceived,

			m_pipesActive,

			m_lastone;

	} *s_debugStats;


	CmdConnection() :
		m_pipe(nullptr)
	{
		m_valid = true;
		m_readAgainTimeout = 6000; /* 60 seconds (6000 * 10ms) */
	}

	~CmdConnection(){
		delete m_pipe;
	}

	inline void setPipe(Connection *p){ m_pipe = p; }

	bool receiveCmd();
	void execCommand(const std::string cmd);

	// Write debug stat data to the provided stream.
	static void DumpDebugStats(std::stringstream &ss);

	// Manage shared memory for the stat table.
	static void InitStats();
	static void UninitStats();
};

#endif
