/*
	cmdConnection.cc
	mperron (2022)
*/

#include "cmdConnection.h"

#include <unistd.h>

#include "util.h"

using namespace std;

CmdConnection::DebugStats CmdConnection::s_debugStats = { 0 };

bool CmdConnection::receiveCmd(){
	if(!valid()){
		++ s_debugStats.m_invalidConn;
		return false;
	}

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

	++ s_debugStats.m_cmdReceived;

	// Write out a response (echo)
	tryWrite(m_sockstream.str());

	// Clear the receive stream.
	m_sockstream.str("");

	// 0 is generally a timeout.
	return true;
}

void CmdConnection::DumpDebugStats(stringstream &ss){
	ss << "+ CmdConnection DebugStats" << endl
		<< "| m_cmdReceived: " << s_debugStats.m_cmdReceived << endl
		<< "|" << endl
		<< "| m_invalidConn: " << s_debugStats.m_invalidConn << endl
		<< "+" << endl;
}
