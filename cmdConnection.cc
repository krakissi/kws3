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

					if(errors)
						m_pendingMessages.push_back("check: failed config validation.");
					else m_pendingMessages.push_front("check: ok.");
				} else if(verb == "apply"){
					// Result of config apply. We made it past the check phase,
					// so this should generally be OK.
					int status;

					mss >> status;
					if(!!mss){
						if(!status){
							// OK
							m_pendingMessages.push_front("apply: ok.");
						} else {
							// Some failure.
							m_pendingMessages.push_front("apply: failed!");
						}
					}
				} else if(verb == "http-port"){
					// Receive HTTP listener config information.

					// FIXME debug
					m_pendingMessages.push_front(msg.substr(msg.find("http-port") + string("http-port").size() + 1));
				} else if(verb == "http-site"){
					// Receive site config

					// FIXME debug
					m_pendingMessages.push_front(msg.substr(msg.find("http-site") + string("http-site").size() + 1));
				}
			}
		}

		// Received a message, stop retrying.
		return false;
	}

	// True to try to read again.
	return true;
}

bool CmdConnection::receiveCmd(){
	if(!valid())
		return false;

	if(m_pipe){
		// A command was issued which should get a response, wait a while and
		// see if it does.
		if(m_expectingMsg){
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

		// Commands that work at all levels don't send the crumbs.
		if((cmd != "top") && (cmd != "end") && (cmd != "apply") && (cmd != "check")){
			// For nested configuration elements, insert the path into the command.
			getCrumbs(css, ' ');
		}

		css << cmd;
		execConfig(css.str());
	} else execCommand(cmd);

	// True to continue processing commands.
	return true;
}

void CmdConnection::execCommand(const string &cmd){
	++ s_debugStats->m_cmdReceived;

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
				oss << "  try : stats" << endl;
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
		} else if(verb == "http-port"){
			int port;

			sss >> port;
			if(!sss){
				// Get all configured port numbers.
				((PipeConnection*) m_pipe->write())->tryWrite("get http-port");
				m_expectingMsg = true;
			} else {
				// Store breadcrumbs.
				m_configPath[configLevel++] = verb;
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
			string name;

			sss >> name;
			if(!sss){
				// Get all configured site numbers.
				((PipeConnection*) m_pipe->write())->tryWrite("get http-site");
				m_expectingMsg = true;
			} else {
				// Store breadcrumbs.
				m_configPath[configLevel++] = verb;
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
		<< "|   m_cmdReceived: " << s_debugStats->m_cmdReceived << endl
		<< "|" << endl
		<< "|   m_pipesActive: " << s_debugStats->m_pipesActive << endl
		<< "|  m_pipesTimeout: " << s_debugStats->m_pipesTimeout << endl
		<< "| m_pipesPingSent: " << s_debugStats->m_pipesPingSent << endl
		<< "| m_pipesPingRcvd: " << s_debugStats->m_pipesPingRcvd << endl
		<< "+" << endl;
}
KWS3_SHMEM_STAT_INIT   (CmdConnection)
KWS3_SHMEM_STAT_UNINIT (CmdConnection)
