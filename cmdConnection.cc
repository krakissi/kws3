/*
	cmdConnection.cc
	mperron (2022)
*/

#include "cmdConnection.h"

#include <iostream>

#include <unistd.h>
#include <sys/mman.h>

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

bool CmdConnection::receiveCmd(){
	if(!valid())
		return false;

	// Write out and pending error messages before the next command prompt.
	while(m_pendingError.size()){
		stringstream ess;

		ess << "err: " << m_pendingError.front() << endl;
		m_pendingError.pop_front();

		tryWrite(ess.str());
	}

	// Display prompt.
	if(m_sockstream.str().empty()){
		stringstream css;

		css << endl << (m_pipe ? "" : "(d/c)") << "> ";
		tryWrite(css.str());
	}

	prepareToRead();

	int rc;
	do {
		if(m_pipe){
			string msg;

			if(((PipeConnection*) m_pipe->read())->nextMsg(msg)){
				if(msg == "done"){
					m_pendingError.push_front("remote closed signal pipe!");
					m_pipe = nullptr;

					// Send bell to indicate a pending error message.
					tryWrite("\a");
				}
			}
		}

		rc = tryRead();

		if(rc < 0)
			// Retry
			continue;

		if(rc == 0)
			// Complete
			return false;

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

	execCommand(cmd);

	// 0 is generally a timeout.
	return true;
}

void CmdConnection::execCommand(const string cmd){
	++ s_debugStats->m_cmdReceived;

	// Parse command string. It should start with some verb.
	stringstream sss(cmd), oss;
	string verb;

	sss >> verb;
	if(!sss){
		if(m_pendingError.size() == 0)
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
				oss << "-> " << sss.str().substr(n) << endl;

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

void CmdConnection::DumpDebugStats(stringstream &ss){
	ss << "+ CmdConnection DebugStats" << endl
		<< "| m_cmdReceived: " << s_debugStats->m_cmdReceived << endl
		<< "|" << endl
		<< "| m_pipesActive: " << s_debugStats->m_pipesActive << endl
		<< "+" << endl;
}
KWS3_SHMEM_STAT_INIT   (CmdConnection)
KWS3_SHMEM_STAT_UNINIT (CmdConnection)
