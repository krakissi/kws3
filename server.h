/*
	server.h
	mperron (2022)
*/

#ifndef KWS3_SERVER_H
#define KWS3_SERVER_H

#include <list>
#include <unordered_map>

#include "tcpListener.h"
#include "pipeConnection.h"
#include "config.h"


class Kws3 {
	TcpListener m_cmd_listener;
	std::list<BiConn*> m_pipes;

	// This should probably be a map of port:listener*
	std::list<TcpListener*> m_http_listeners;

	Cfg::Kws3 m_config;

	int m_acceptPassCount;

	void init();
	void cleanup();

	std::string checkConfig();
	int applyConfig();

public:

	Kws3() :
		m_cmd_listener(9003),
		m_acceptPassCount(0)
	{
		init();
	}

	~Kws3(){
		cleanup();
	}

	bool valid() const;
	bool run();
};

#endif
