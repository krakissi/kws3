/*
	server.cc
	mperron (2022)
*/

#include "server.h"

#include <iostream>

#include <unistd.h>
#include <fcntl.h>

#include "httpConnection.h"
#include "cmdConnection.h"

using namespace std;

void Kws3::init(){
	// FIXME debug - create listeners via configuration instead
	m_http_listeners.push_back(new TcpListener(9005));

	HttpConnection::InitStats();
	CmdConnection::InitStats();
}

void Kws3::cleanup(){
	// Delete server listeners
	for(auto l : m_http_listeners)
		delete l;

	// Delete any open pipe connections
	for(auto pc : m_pipes)
		delete pc;

	HttpConnection::UninitStats();
	CmdConnection::UninitStats();
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

		// Pipes from child tasks
		for(auto it = m_pipes.begin(); it != m_pipes.end(); ){
			BiConn *bc = *it;
			bool erase = false;
			string msg;

			if(((PipeConnection*) bc->read())->nextMsg(msg)){
				if(msg == "done"){
					// Child task is done and we can close this pipe.
					erase = true;
				} else if(msg == "shutdown"){
					bc->write()->tryWrite("done");
					return false;
				}
			}

			if(erase){
				-- CmdConnection::s_debugStats->m_pipesActive;
				it = m_pipes.erase(it);
				delete bc;
			} else {
				++ it;
			}
		}

		// Command interface
		{
			CmdConnection conn;

			if(m_cmd_listener.accept(conn) >= 0){
				int fd_east[2], fd_west[2];
				bool havePipes = false;
				pid_t pid;

				if(!pipe2(fd_east, (O_NONBLOCK | O_DIRECT))){
					if(!pipe2(fd_west, (O_NONBLOCK | O_DIRECT))){
						havePipes = true;
					} else {
						// Failed to open both pipes, so abandon plumbing altogether.
						close(fd_east[0]);
						close(fd_east[1]);
					}
				}

				if(!(pid = fork())){
					// Child task
					if(havePipes){
						BiConn *bc = new BiConn(
							new PipeConnection(fd_west[0], false),
							new PipeConnection(fd_east[1], true)
						);

						close(fd_east[0]);
						close(fd_west[1]);

						conn.setPipe(bc);
					}

					conn.tryWrite("hello\n");
					while(conn.receiveCmd());

					return false;
				} else if(pid > 0){
					// Parent task
					if(havePipes){
						BiConn *bc = new BiConn(
							new PipeConnection(fd_east[0], false),
							new PipeConnection(fd_west[1], true)
						);

						close(fd_west[0]);
						close(fd_east[1]);

						m_pipes.push_back(bc);
						++ CmdConnection::s_debugStats->m_pipesActive;
					}
				} else {
					if(havePipes){
						close(fd_east[0]);
						close(fd_east[1]);
						close(fd_west[0]);
						close(fd_west[1]);
					}
				}

				acceptedConnection = true;
			}
		}

		// HTTP Servers
		for(auto l : m_http_listeners){
			HttpConnection conn;

			if(l->accept(conn) >= 0){
				pid_t pid;

				if(!(pid = fork())){
					// Read from socket, look for incoming HTTP request.
					conn.receiveRequest();

					return false;
				}

				acceptedConnection = true;
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

		// Reset the pass counter, so we will enter high-frequency polling mode
		m_acceptPassCount = 0;
	}

	// TODO - add a shutdown mechanism which allows this to return false.
	return true;
}
