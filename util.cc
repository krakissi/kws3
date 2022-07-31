/*
   util.cc
   mperron (2022)
*/

#include "util.h"

using namespace std;

void chomp(string &str){
	size_t p = str.find('\r');

	if(p != string::npos){
		str[p] = 0;
	} else {
		p = str.find('\n');

		if(p != string::npos)
			str[p] = 0;
	}
}
