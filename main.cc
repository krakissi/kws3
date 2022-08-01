#include <iostream>

#include <signal.h>
#include <unistd.h>

#include "server.h"

using namespace std;

int main(int argc, char **argv){
	Kws3 server;
	pid_t pid;

	if(!server.valid())
		return 1;

	// Let init handle the children.
	signal(SIGCHLD, SIG_IGN);

	if(!(pid = fork())){
		while(server.run());

		return 0;
	}

	cout << "Server started with pid: " << pid << endl;

	return 0;
}
