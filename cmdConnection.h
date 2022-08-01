/*
	cmdConnection.h
	mperron (2022)
*/

#ifndef KWS3_CMD_CONNECTION_H
#define KWS3_CMD_CONNECTION_H

#include "connection.h"

class CmdConnection : public Connection {
public:
	static struct DebugStats {
		uint64_t
			m_cmdReceived,

			m_lastone;

	} *s_debugStats;


	CmdConnection()
	{
		m_valid = true;
		m_readAgainTimeout = 6000; /* 60 seconds (6000 * 10ms) */
	}

	bool receiveCmd();

	// Write debug stat data to the provided stream.
	static void DumpDebugStats(std::stringstream &ss);

	// Manage shared memory for the stat table.
	static void InitStats();
	static void UninitStats();
};

#endif
