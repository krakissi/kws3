#include <iostream>

#include <signal.h>
#include <unistd.h>

#include "server.h"

using namespace std;

int main(int argc, char **argv){
	Kws3 server;
	pid_t pid;

	if(!server.valid()){
		cout << "Failed to bind ports, likely already in use." << endl;
		return 1;
	}

	// Let init handle the children.
	signal(SIGCHLD, SIG_IGN);

	if(!(pid = fork())){
		while(server.run());

		return 0;
	}

	if(pid > 0)
		cout << "Server started with pid: " << pid << endl;
	else {
		cout << "Failed to start server." << endl;
		return 2;
	}

	return 0;
}
