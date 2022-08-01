/*
	server.h
	mperron (2022)
*/

#ifndef KWS3_SERVER_H
#define KWS3_SERVER_H

#include <list>

#include "tcpListener.h"


class Kws3 {
	TcpListener m_cmd_listener;

	// This should probably be a map of port:listener*
	std::list<TcpListener*> m_http_listeners;

	int m_acceptPassCount;

	void init();
	void cleanup();

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
