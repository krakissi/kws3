/*
   util.cc
   mperron (2022)
*/

#include "util.h"

using namespace std;

string chomp(const string &str){
	size_t p = str.find_first_of("\r\n");

	if(p != string::npos)
		return str.substr(0, p);

	return str;
}

string trim(const string &str){
	if(str.empty())
		return str;

	ssize_t a, b;

	for(a = 0; (a < str.size()) && isspace(str[a]); ++ a);
	for(b = str.size() - 1; (b >= 0) && isspace(str[b]); -- b);

	return (str.substr(a, b - a + 1));
}
