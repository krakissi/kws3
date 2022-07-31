#include <iostream>
#include <sstream>

#include <unistd.h>
#include <cstring>

using namespace std;

#include "util.h"
#include "tcpListener.h"
#include "httpConnection.h"

int main(int argc, char **argv){
	TcpListener listener(9003);
	struct sockaddr_in socket_addr_client = {};

	// Wait for a connection and process it.
	while(true){
		HttpConnection conn;

		int sockfd_client = listener.accept(socket_addr_client);

		if(sockfd_client < 0)
			return 4;

		// Read and write to the socket.
		{
			stringstream ss;
			ssize_t count = 0;
			static char buf[1024];
			int emptycounter = 0;

			size_t headersize = 0;

			do {
				memset(buf, 0, sizeof(buf));

				ssize_t rc = read(sockfd_client, &buf, sizeof(buf) - 1);
				int code = errno;

				if(rc < 0){
					switch(code){
						case EAGAIN:
							if(emptycounter < 500){
								// No data, so wait and increment counter.
								usleep(
									((++ emptycounter > 10) ? 
										10000 : /* Wait 10ms if no data recently arrived. We might timeout. */
										100)    /* Wait 100us if data recently arrived, as more may quickly be coming. */
								);
								continue;
							}
							// Fall-through to default error case, breaking read loop.

						default:
							// Cause the read loop to break by setting this return code to zero.
							rc = 0;
					}
				} else {
					// Reset counter to reset the timeout window.
					emptycounter = 0;
				}

				if(rc == 0){
					break;
				}

				count += rc;
				ss << buf;
			} while(!(headersize = HttpConnection::SizeofHeader(ss.str())));

			// If we broke out earlier due to an error, we might not have set
			// headersize. Try one more time.
			if(!headersize)
				headersize = HttpConnection::SizeofHeader(ss.str());

			if(headersize){
				// Possibly a valid request, let's try to parse it.
				conn.parseHeaders(ss.str());
			}
			
			if(conn.valid()){
				// Get the rest of the data and store it in the body.
				// TODO

				// FIXME debug
				ss << "Your request was valid.\r\n";
			} else {
				// FIXME debug
				ss << "Your request was invalid.\r\n";

				// Eat all data on the socket.
				while(0 < read(sockfd_client, &buf, sizeof(buf) - 1));
			}

			// Send message to client.
			{
				string msg = ss.str();

				// FIXME - This may not send all data. Wrap in a function which checks bytes sent and retries.
				write(sockfd_client, msg.c_str(), msg.size());
			}
		}

		close(sockfd_client);

		// FIXME debug
		{
			stringstream ss;

			HttpConnection::DumpDebugStats(ss);
			cout << ss.str();
		}
	}

	return 0;
}
