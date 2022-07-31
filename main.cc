#include <iostream>
#include <sstream>

#include "tcpListener.h"
#include "httpConnection.h"

using namespace std;

int main(int argc, char **argv){
	TcpListener listener(9003);

	if(!listener.valid())
		return 1;

	while(true){
		HttpConnection conn;

		// Wait for a connection.
		if(listener.accept(conn) < 0)
			continue;

		// Read from socket, look for incoming HTTP request.
		conn.receiveRequest();

		// Echo back if we parsed a request.
		if(conn.valid())
			conn.echoRequest();


		// FIXME debug
		{
			stringstream ss;

			HttpConnection::DumpDebugStats(ss);
			cout << ss.str();
		}
	}

	return 0;
}
