/*
	connection.cc
	mperron(2022)
*/

#include "connection.h"

#include <cstring>
#include <unistd.h>

using namespace std;

void Connection::connection_cleanup(){
	if(m_fd >= 0)
		close(m_fd);
}

ssize_t Connection::tryRead(){
	static char buf[1024];
	memset(buf, 0, sizeof(buf));

	ssize_t rc = read(m_fd, &buf, sizeof(buf) - 1);
	return (rc < 0) ? readFailure(errno) : readSuccess(rc, buf);
}

ssize_t Connection::tryWrite(const string &msg) const {
	return write(m_fd, msg.c_str(), msg.size());
}
