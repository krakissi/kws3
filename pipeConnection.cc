/*
	pipeConnection.cc
	mperron(2022)
*/

#include "pipeConnection.h"

#include <iostream>

using namespace std;

int PipeConnection::readFailure(int code){
	// Never retry reading.
	return 0;
}

bool PipeConnection::nextMsg(string &msg){
	if(tryRead() > 0){
		msg = m_sockstream.str();
		m_sockstream.str("");

		return true;
	}

	return false;
}

bool PipeConnection::timeout(){
	if(!m_write){
		static time_t now;

		time(&now);

		// Five minute timeout.
		if((now - m_timeLastMsgRecv) > 300)
			return true;
	}

	return false;
}
