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
	// Example configuration.
	{
		Cfg::HttpSite *s = new Cfg::HttpSite("default");
		Cfg::HttpPort *p = new Cfg::HttpPort(9004);

		m_config.m_sites[s->m_name] = s;
		m_config.m_ports[p->m_port] = p;

		s->m_root = ".";
		p->m_siteDefault = s->m_name;
	}
	applyConfig();

	HttpConnection::InitStats();
	CmdConnection::InitStats();
	HttpResponse::InitStats();
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
	HttpResponse::UninitStats();
}

string Kws3::checkConfig(){
	stringstream ss;

	for(auto kv : m_config.m_ports){
		const string prefix = (string("http-port ") + to_string(kv.first) + ": ");
		Cfg::HttpPort *p = kv.second;

		if((kv.first > 0xffff) || (kv.first <= 0))
			ss << prefix << "invalid port number" << endl;

		if(p->m_state){
			// Sites can be unconfigured if the port is not actually enabled.
			if(p->m_siteDefault.empty() && (p->m_sites.size() == 0))
				ss << prefix << "enabled, but no sites configured" << endl;
			else {
				bool anySiteEnabled = false;

				if(!p->m_siteDefault.empty()){
					Cfg::HttpSite *s = nullptr;

					try {
						s = m_config.m_sites.at(p->m_siteDefault);
					} catch(...){}

					if(s && s->m_state)
						anySiteEnabled = true;
				}

				for(string site : p->m_sites){
					Cfg::HttpSite *s = nullptr;

					try {
						s = m_config.m_sites.at(site);
					} catch(...){}

					if(s && s->m_state)
						anySiteEnabled = true;
				}

				if(!anySiteEnabled)
					ss << prefix << "enabled, but all sites are disabled" << endl;
			}
		}
	}

	return ss.str();
}

int Kws3::applyConfig(){
	// Clear existing listeners.
	for(auto it = m_http_listeners.begin(); it != m_http_listeners.end(); it = m_http_listeners.erase(it))
		delete *it;

	for(auto kv : m_config.m_ports){
		// TODO - how does TcpListener know about site configuration?
		if(kv.second->m_state)
			m_http_listeners.push_back(new TcpListener(kv.first, kv.second->m_local));
	}

	// TODO apply all config

	return 0;
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

			PipeConnection *readPipe = (PipeConnection*) bc->read();

			if(readPipe->nextMsg(msg)){
				if(msg == "ping"){
					++ CmdConnection::s_debugStats->m_pipesPingRcvd;
				} else if(msg == "done"){
					// Child task is done and we can close this pipe.
					erase = true;
				} else if(msg == "shutdown"){
					bc->write()->tryWrite("done");
					return false;
				} else {
					stringstream mss(msg);
					string verb;

					mss >> verb;
					if(!!mss){
						if(verb == "check"){
							string errors = checkConfig();
							stringstream ss;

							ss << "check" << endl << errors;
							bc->write()->tryWrite(ss.str());
						} else if(verb == "apply"){
							string errors = checkConfig();
							stringstream ss;

							if(errors.size()){
								// Report errors.
								ss << "check" << endl << errors;
							} else {
								// Apply configuration changes.
								ss << "apply " << applyConfig();
							}

							bc->write()->tryWrite(ss.str());
						} else if(verb == "get"){
							string what;

							mss >> what;
							if(!!mss){
								stringstream cfgss;

								if(what == "http-port"){
									// HttpPort objects
									cfgss << "http-port ";

									int port;
									mss >> port;
									if(!mss){
										cfgss << "list ";

										// No port number specified, return a list of listening ports.
										for(auto kv : m_config.m_ports)
											cfgss << kv.first << " ";
									} else {
										Cfg::HttpPort *p;

										try {
											p = m_config.m_ports.at(port);
										} catch(...){
											m_config.m_ports[port] = p = new Cfg::HttpPort(port);
										}

										cfgss << "item " << p->save();
									}
								} else if(what == "http-site"){
									// HttpSite objects
									cfgss << "http-site ";

									string name;
									mss >> name;
									if(!mss){
										cfgss << "list ";

										// No site name specified, return a list of site names.
										for(auto kv : m_config.m_sites)
											cfgss << kv.first << " ";
									} else {
										Cfg::HttpSite *s;

										try {
											s = m_config.m_sites.at(name);
										} catch(...){
											m_config.m_sites[name] = s = new Cfg::HttpSite(name);
										}

										cfgss << "item " << s->save();
									}
								}

								if(cfgss.str().size())
									bc->write()->tryWrite(cfgss.str());
							}
						}
					}
				}
			}

			if(readPipe->timeout()){
				++ CmdConnection::s_debugStats->m_pipesTimeout;
				erase = true;
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
