/*
	cmdConnection.cc
	mperron (2022)
*/

#include "cmdConnection.h"

#include <iostream>

#include <unistd.h>

#include "util.h"

using namespace std;

CmdConnection::DebugStats CmdConnection::s_debugStats = { 0 };

bool CmdConnection::receiveCmd(){
	if(!valid())
		return false;

	tryWrite("\n> ");
	prepareToRead();

	int rc;
	do {
		rc = tryRead();

		if(rc < 0)
			// Retry
			continue;

		if(rc == 0)
			// Complete
			return false;

	} while(m_sockstream.str().find('\n') == string::npos);

	string cmd = m_sockstream.str();
	{
		size_t p = cmd.find('\n');

		// Invalid command, something has gone wrong.
		if(p == string::npos)
			return false;

		// Clear the receive stream and reload it when the next line if there was more data.
		m_sockstream.str("");
		if(p + 1 < cmd.size())
			m_sockstream << cmd.substr(p + 1);

		cmd = cmd.substr(0, p);
	}

	++ s_debugStats.m_cmdReceived;

	// Parse command string. It should start with some verb.
	{
		stringstream sss(cmd), oss;
		string verb;

		sss >> verb;
		if(!sss){
			oss << "?" << endl;
		} else {
			if(verb == "quit"){
				// Close this bad boy out.
				oss << "goodbye" << endl;
				m_valid = false;

			} else if(verb == "echo"){
				// Echo the command back
				if(sss.str().size() > verb.size() + 1)
					oss << "-> " << sss.str().substr(verb.size() + 1) << endl;

			} else {
				// Display the verb if we didn't understand it.
				oss << "unknown verb: " << verb << endl;
			}
		}

		tryWrite(oss.str());
	}

	// 0 is generally a timeout.
	return true;
}

void CmdConnection::DumpDebugStats(stringstream &ss){
	ss << "+ CmdConnection DebugStats" << endl
		<< "| m_cmdReceived: " << s_debugStats.m_cmdReceived << endl
		<< "+" << endl;
}
