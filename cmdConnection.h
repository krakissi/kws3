/*
	cmdConnection.h
	mperron (2022)
*/

#ifndef KWS3_CMD_CONNECTION_H
#define KWS3_CMD_CONNECTION_H

#include "connection.h"

#include <list>
#include <vector>

#include "config.h"


class CmdConnection : public Connection {
	BiConn *m_pipe;

	std::vector<std::string> m_configPath;
	std::string m_lastCmd;
	std::list<std::string> m_pendingMessages;

	Cfg::Kws3 m_configCache;
	Cfg::SerialConfig *m_lastRcvdConfigObj;

	bool m_configMode;
	bool m_expectingMsg;

	inline void clearCrumbs() { for(auto &s : m_configPath) s = ""; }
	inline void getCrumbs(std::stringstream &ss, const char sep);

	void execCommand(const std::string &cmd);
	void execConfig(const std::string &cmd);

	void waitForMsg();
	bool receiveMsg();

	const std::string
		ERR_REMOTE_CLOSED = "remote closed signal pipe!",
		ERR_EXPECTED_MSG_NOT_RECEIVED = "timed out waiting for remote response";

public:
	static struct DebugStats {
		uint64_t
			m_cmdReceived,

			m_pipesActive,
			m_pipesTimeout,
			m_pipesPingSent,
			m_pipesPingRcvd,

			m_configStart,
			m_configReceived,
			m_configCheckSuccess,
			m_configCheckFailure,
			m_configApplySuccess,
			m_configApplyFailure,

			m_lastone;

	} *s_debugStats;


	CmdConnection() :
		m_pipe(nullptr),
		m_configPath(10 /* Hopefully no more than 10 nested levels of config */),
		m_lastRcvdConfigObj(nullptr),
		m_configMode(false),
		m_expectingMsg(false)
	{
		m_valid = true;
		m_readAgainTimeout = 18000; /* 180 seconds (18000 * 10ms) */
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
