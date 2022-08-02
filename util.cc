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

string trim(const string &str){
	if(str.empty())
		return str;

	ssize_t a, b;

	for(a = 0; (a < str.size()) && isspace(str[a]); ++ a);
	for(b = str.size() - 1; (b >= 0) && isspace(str[b]); -- b);

	return (str.substr(a, b - a + 1));
}
