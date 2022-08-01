#include "server.h"

using namespace std;

int main(int argc, char **argv){
	Kws3 server;

	if(!server.valid())
		return 1;

	while(server.run());

	return 0;
}
