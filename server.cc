/*
	server.cc
	mperron (2022)
*/

#include "server.h"

#include <iostream>

#include <unistd.h>

#include "httpConnection.h"

using namespace std;

void Kws3::init(){
	// FIXME debug - create listeners via configuration instead
	m_http_listeners.push_back(new TcpListener(9005));
}

void Kws3::cleanup(){
	// Delete server listeners
	for(auto l : m_http_listeners)
		delete l;
}

bool Kws3::valid() const {
	if(!m_cmd_listener.valid())
		return false;

	for(auto l : m_http_listeners)
		if(!l->valid())
			return false;

	return true;
}

bool Kws3::run(){
	// Check for queued incoming connections.
	{
		bool acceptedConnection = false;

		// Command interface
		{
			// FIXME create a CmdConnection instead
			HttpConnection conn;

			if(m_cmd_listener.accept(conn) >= 0){
				acceptedConnection = true;

				// FIXME - create a CmdConnection instead, do command processing.

				// FIXME debug - receive and echo an HTTP request
				conn.receiveRequest();
				if(conn.valid())
					conn.echoRequest();
			}
		}

		// HTTP Servers
		for(auto l : m_http_listeners){
			HttpConnection conn;

			if(l->accept(conn) >= 0){
				acceptedConnection = true;

				// Read from socket, look for incoming HTTP request.
				conn.receiveRequest();

				// Echo back if we parsed a request.
				if(conn.valid())
					conn.echoRequest();
			}
		}

		// Delay a variable amount of time based on how active the server is,
		// to reduce CPU utilization during quiet periods.
		if(!acceptedConnection){
			if(m_acceptPassCount < 20000){
				++ m_acceptPassCount;

				// High or mid-frequency polling state.
				usleep( ((m_acceptPassCount < 5000) ? 100 : 5000) );
			} else {
				// 30ms - low-frequency polling state
				usleep(30000);
			}

			return true;
		}

		// FIXME debug
		cout << "[m_acceptPassCount=" << m_acceptPassCount << "]" << endl;

		// Reset the pass counter, so we will enter high-frequency polling mode
		m_acceptPassCount = 0;
	}


	// FIXME debug
	{
		stringstream ss;

		HttpConnection::DumpDebugStats(ss);
		cout << ss.str();
	}

	// TODO - add a shutdown mechanism which allows this to return false.
	return true;
}
