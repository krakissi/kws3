/*
	cmdConnection.cc
	mperron (2022)
*/

#include "cmdConnection.h"

#include <iostream>

#include <unistd.h>
#include <sys/mman.h>

#include "util.h"
#include "httpConnection.h"
#include "pipeConnection.h"

using namespace std;

size_t findFirstOf(string needles, const string &str){
	size_t p = str.size();

	for(auto c : needles){
		size_t n = str.find(c);

		if((n != string::npos) && (n < p))
			p = n;
	}

	if(p == str.size())
		return string::npos;

	return p;
}

CmdConnection::DebugStats *CmdConnection::s_debugStats = nullptr;

void CmdConnection::getCrumbs(stringstream &ss, const char sep){
	for(const auto &s : m_configPath){
		if(s.empty())
			break;

		ss << s << sep;
	}
}

bool CmdConnection::receiveMsg(){
	static Cfg::ObjList defaultObjList;

	if(!m_pipe)
		return false;

	string msg;

	if(((PipeConnection*) m_pipe->read())->nextMsg(msg)){
		if(msg == "done"){
			m_pendingMessages.push_front(ERR_REMOTE_CLOSED);
			m_pipe = nullptr;

			// Send bell to indicate a pending error message.
			tryWrite("\a");
		} else {
			stringstream mss(msg);
			string verb;

			mss >> verb;
			if(!!mss){
				if(verb == "check"){
					// Configuration errors that need to be addressed before apply.
					string errorMsg;
					int errors = 0;

					while(getline(mss, errorMsg)){
						errorMsg = chomp(errorMsg);

						if(!errorMsg.empty()){
							m_pendingMessages.push_front(errorMsg);
							++ errors;
						}
					}

					if(errors){
						m_pendingMessages.push_back("check: failed config validation.");
						++ s_debugStats->m_configCheckFailure;
					} else {
						m_pendingMessages.push_front("check: ok.");
						++ s_debugStats->m_configCheckSuccess;
					}
				} else if(verb == "apply"){
					// Result of config apply. We made it past the check phase,
					// so this should generally be OK.
					int status;

					mss >> status;
					if(!!mss){
						if(!status){
							// OK
							m_pendingMessages.push_front("apply: ok.");
							++ s_debugStats->m_configApplySuccess;
						} else {
							// Some failure.
							m_pendingMessages.push_front("apply: failed!");
							++ s_debugStats->m_configApplyFailure;
						}
					}
				} else if(verb == "http-port"){
					// Remove existing cached data.
					Cfg::Kws3::ClearMap(m_configCache.m_ports);

					string type;

					if(!!(mss >> type)){
						if(type == "item"){
							int port;

							if(!!(mss >> port)){
								// One port with all of its configuration.
								Cfg::HttpPort *p = m_configCache.m_ports[port] = new Cfg::HttpPort(port);
								string rest;

								if(!!getline(mss, rest)){
									p->load(rest);
									m_lastRcvdConfigObj = p;
								}
							}
						} else if (type == "list"){
							// List of configured ports.
							string line;
							getline(mss, line);

							defaultObjList.load(line);
							m_lastRcvdConfigObj = &defaultObjList;
						}
					}
				} else if(verb == "http-site"){
					// Remove existing cached data.
					Cfg::Kws3::ClearMap(m_configCache.m_sites);

					string type;

					if(!!(mss >> type)){
						if(type == "item"){
							string name;

							if(!!(mss >> name)){
								// One site with all of its configuration.
								Cfg::HttpSite *s = m_configCache.m_sites[name] = new Cfg::HttpSite(name);
								string rest;

								if(!!getline(mss, rest)){
									s->load(rest);
									m_lastRcvdConfigObj = s;
								}
							}
						} else if (type == "list"){
							// List of configured ports.
							string line;
							getline(mss, line);

							defaultObjList.load(line);
							m_lastRcvdConfigObj = &defaultObjList;
						}
					}
				}
			}
		}

		// Received a message, stop retrying.
		return false;
	}

	// True to try to read again.
	return true;
}

void CmdConnection::waitForMsg(){
	// A command was issued which should get a response, wait a while and
	// see if it does.
	for(int i = 0; i < 13; ++ i){
		// 12 * 250000 = 3000000 us = 3 seconds timeout
		// Wait 30ms the first time in case the server gets back quickly, then poll every 250ms.
		usleep((i == 0) ? 30000 : 250000);

		if(!receiveMsg()){
			m_expectingMsg = false;
			break;
		}
	}

	if(m_expectingMsg){
		m_pendingMessages.push_front(ERR_EXPECTED_MSG_NOT_RECEIVED);
	}

	m_expectingMsg = false;
}

bool CmdConnection::receiveCmd(){
	if(!valid())
		return false;

	if(m_pipe){
		// A command was issued which should get a response, wait a while and
		// see if it does.
		if(m_expectingMsg)
			waitForMsg();

	} else {
		// Cannot configure if pipe to the main task it broken.
		if(m_configMode){
			clearCrumbs();
			m_configMode = false;
			m_pendingMessages.push_front(ERR_REMOTE_CLOSED);
		}
	}

	// Write out and pending error messages before the next command prompt.
	while(m_pendingMessages.size()){
		stringstream ess;

		ess << "-> " << m_pendingMessages.front() << endl;
		m_pendingMessages.pop_front();

		tryWrite(ess.str());
	}

	// Display prompt.
	if(m_sockstream.str().empty()){
		stringstream css;

		if(m_configMode){
			css << "/";
			getCrumbs(css, '/');
		}

		css << (m_pipe ? "" : "(d/c)") << (m_configMode ? "# " : "> ");
		tryWrite(css.str());
	}

	prepareToRead();

	int rc;
	do {
		// Check pipe from the main server task for an unexpected message.
		receiveMsg();

		rc = tryRead();

		if(rc < 0)
			// Retry
			continue;

		if(rc == 0){
			// 0 bytes read is generally a timeout.
			tryWrite("\a\ntimeout\n");
			return false;
		}

	} while(findFirstOf("\n;", m_sockstream.str()) == string::npos);

	string cmd = m_sockstream.str();
	{
		size_t p = findFirstOf("\n;", cmd);

		// Invalid command, something has gone wrong.
		if(p == string::npos)
			return false;

		// Clear the receive stream and reload it when the next line if there was more data.
		m_sockstream.str("");
		if(p + 1 < cmd.size())
			m_sockstream << cmd.substr(p + 1);

		cmd = cmd.substr(0, p);
	}

	if(m_pipe){
		// Keep the pipe to the main task alive.
		((PipeConnection*) m_pipe->write())->tryWrite("ping");
		++ s_debugStats->m_pipesPingSent;
	}

	// Execute either config or command statement.
	if(m_configMode){
		stringstream css;

		cmd = trim(cmd);

		if(cmd == "show"){
			// If the command is show, prepend it instead.
			css << cmd << " ";
			getCrumbs(css, ' ');
		} else {
			if(cmd == "up"){
				if(!m_configPath[0].empty()){
					// Move up a level in config.
					for(int i = 1; i <= m_configPath.size(); ++ i){
						if((i == m_configPath.size()) || (m_configPath[i].empty())){
							m_configPath[i - 1] = "";
							break;
						}
					}
				}

				cmd = "";
			}

			// Commands that work at all levels don't send the crumbs.
			if((cmd != "top") && (cmd != "end") && (cmd != "apply") && (cmd != "check")){
				// For nested configuration elements, insert the path into the command.
				getCrumbs(css, ' ');
			}

			css << cmd;
		}

		++ s_debugStats->m_configReceived;
		execConfig(trim(css.str()));
	} else {
		++ s_debugStats->m_cmdReceived;
		execCommand(trim(cmd));
	}

	// True to continue processing commands.
	return true;
}

void CmdConnection::execCommand(const string &cmd){
	// Parse command string. It should start with some verb.
	stringstream sss(cmd), oss;
	string verb;

	sss >> verb;
	if(!sss){
		if(m_pendingMessages.size() == 0)
			oss << "?" << endl;
	} else {
		if(verb == "!!"){
			// Repeat the last command.
			execCommand(m_lastCmd);
			return;
		} else if(verb == "quit"){
			if(m_pipe)
				m_pipe->write()->tryWrite("done");

			// Close this bad boy out.
			oss << "goodbye" << endl;
			m_valid = false;
		} else if(verb == "shutdown"){
			// Stop the entire server.
			if(m_pipe){
				oss << "sending shutdown signal: ";

				if(m_pipe->write()->tryWrite("shutdown") > 0){
					m_expectingMsg = true;
					oss << "goodnight";
				} else {
					oss << "failed";
				}

				oss << endl;
			} else {
				oss << "PipeConnection not available" << endl;
			}
		} else if(verb == "echo"){
			// Echo the command back
			size_t p = sss.str().find(verb), n;

			if(p != string::npos)
				n = (p + verb.size() + 1);

			if(sss.str().size() > n)
				m_pendingMessages.push_front(sss.str().substr(n));

		} else if(verb == "show"){
			string what;
			sss >> what;

			if(!sss){
				oss << "show what?" << endl;
				oss << "  try: stats" << endl;
			} else {
				if(what == "stats"){
					string table;
					sss >> table;

					if(!sss){
						oss << "show which stat table?" << endl;
						oss << "  try: CmdConnection HttpConnection HttpResponse" << endl;
					} else {
						if(table == "CmdConnection"){
							CmdConnection::DumpDebugStats(oss);
						} else if(table == "HttpConnection"){
							HttpConnection::DumpDebugStats(oss);
						} else if(table == "HttpResponse"){
							HttpResponse::DumpDebugStats(oss);
						}  else {
							oss << "show: unknown stat table \"" << table << "\"" << endl;
						}
					}
				} else {
					oss << "show: unknown item \"" << what << "\"" << endl;
				}
			}
		} else if(verb == "config"){
			// Enter config mode.
			m_configMode = true;
			++ s_debugStats->m_configStart;

			oss << "entering config mode (\"end\" to exit)..." << endl;
		} else {
			// Display the verb if we didn't understand it.
			oss << "unknown verb: " << verb << endl;
		}

		// Store this command to potentially repeat.
		m_lastCmd = cmd;
	}

	if(oss.str().size())
		tryWrite(oss.str());
}

void CmdConnection::execConfig(const std::string &cmd){
	stringstream sss(cmd), oss;
	string verb;

	int configLevel = 0;

	sss >> verb;
	if(!sss){
		return;
	} else {
		if(verb == "end"){
			clearCrumbs();
			m_configMode = false;
			oss << "exiting config mode." << endl;
		} else if(verb == "top"){
			clearCrumbs();
		} else if((verb == "check") || (verb == "apply")){
			((PipeConnection*) m_pipe->write())->tryWrite(verb);
			m_expectingMsg = true;
		} else if(verb == "show"){
			string what;

			sss >> what;
			if(!sss){
				oss << "show: what?" << endl;
			} else {
				Cfg::SerialConfig *obj = nullptr;

				m_lastRcvdConfigObj = nullptr;
				m_expectingMsg = true;

				stringstream pss;
				pss << "get " << what;

				// Was a specific item specified?
				{
					string which;

					sss >> which;
					if(!!sss)
						pss << " " << which;
				}

				// Request and wait for data.
				((PipeConnection*) m_pipe->write())->tryWrite(pss.str());
				waitForMsg();

				// m_lastRcvdConfigObj will be set by waitForMsg() if a response comes from the server.
				if(m_lastRcvdConfigObj)
					oss << m_lastRcvdConfigObj->display();
			}
		} else if(verb == "http-port"){
			m_configPath[configLevel++] = verb;

			int port;

			sss >> port;
			if(!!sss){
				// Store breadcrumbs.
				m_configPath[configLevel++] = to_string(port);

				// Get config for this port.
				{
					stringstream pss;

					pss << "get http-port " << port;
					((PipeConnection*) m_pipe->write())->tryWrite(pss.str());
					m_expectingMsg = true;
				}
			}
		} else if(verb == "http-site"){
			m_configPath[configLevel++] = verb;

			string name;

			sss >> name;
			if(!!sss){
				// Store breadcrumbs.
				m_configPath[configLevel++] = name;

				// Get config for this site.
				{
					stringstream pss;

					pss << "get http-site " << name;
					((PipeConnection*) m_pipe->write())->tryWrite(pss.str());
					m_expectingMsg = true;
				}
			}
		} else {
			m_pendingMessages.push_front("unrecognized config");
		}
	}

	if(oss.str().size())
		tryWrite(oss.str());
}

void CmdConnection::DumpDebugStats(stringstream &ss){
	ss << "+ CmdConnection DebugStats" << endl
		<< "|        m_cmdReceived: " << s_debugStats->m_cmdReceived << endl
		<< "|" << endl
		<< "|        m_pipesActive: " << s_debugStats->m_pipesActive << endl
		<< "|       m_pipesTimeout: " << s_debugStats->m_pipesTimeout << endl
		<< "|      m_pipesPingSent: " << s_debugStats->m_pipesPingSent << endl
		<< "|      m_pipesPingRcvd: " << s_debugStats->m_pipesPingRcvd << endl
		<< "|" << endl
		<< "|        m_configStart: " << s_debugStats->m_configStart << endl
		<< "|     m_configReceived: " << s_debugStats->m_configReceived << endl
		<< "| m_configCheckSuccess: " << s_debugStats->m_configCheckSuccess << endl
		<< "| m_configCheckFailure: " << s_debugStats->m_configCheckFailure << endl
		<< "| m_configApplySuccess: " << s_debugStats->m_configApplySuccess << endl
		<< "| m_configApplyFailure: " << s_debugStats->m_configApplyFailure << endl
		<< "+" << endl;
}
KWS3_SHMEM_STAT_INIT   (CmdConnection)
KWS3_SHMEM_STAT_UNINIT (CmdConnection)
