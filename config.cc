/*
	config.cc
	mperron (2022)
*/

#include "config.h"

#include <sstream>

using namespace std;

void HttpSite::load(const string &str){
	stringstream ss(str);
	string kv;

	// Clear lists.
	m_hosts.clear();

	while(!!(ss >> kv)){
		size_t p = kv.find(':');

		if(p != string::npos){
			string k = kv.substr(0, p), v = kv.substr(p + 1);

			if(k == "state")
				m_state = stoi(v);
			else if(k == "root")
				m_root = v;
			else if(k == "host")
				m_hosts.push_back(v);
		}
	}
}

string HttpSite::save() const {
	stringstream ss;

	ss << m_name
		<< " state:" << m_state
		<< " root:" << m_root;

	for(string host : m_hosts)
		ss << " host:" << host;

	return ss.str();
}

string HttpSite::display() const {
	stringstream ss;

	ss << "site[" << m_name << "] state[" << (m_state ? "enabled" : "disabled") << "]" << endl
		<< "  root: " << m_root << endl;

	for(const string &s : m_hosts)
		ss << "  host: " << s << endl;

	return ss.str();
}

void HttpPort::load(const string &str){
	stringstream ss(str);
	string kv;

	// Clear lists.
	m_sites.clear();

	while(!!(ss >> kv)){
		size_t p = kv.find(':');

		if(p != string::npos){
			string k = kv.substr(0, p), v = kv.substr(p + 1);

			if(k == "state")
				m_state = stoi(v);
			else if(k == "siteDefault")
				m_siteDefault = v;
			else if(k == "site")
				m_sites.push_back(v);
		}
	}
}

string HttpPort::save() const {
	stringstream ss;

	ss << m_port
		<< " state:" << m_state
		<< " siteDefault:" << m_siteDefault;

	for(auto site : m_sites)
		ss << " site:" << site;

	return ss.str();
}

string HttpPort::display() const {
	stringstream ss;

	ss << "port[" << m_port << "] state[" << (m_state ? "enabled" : "disabled") << "]" << endl
		<< "  site-default: " << (m_siteDefault.empty() ? "(not set)" : m_siteDefault.c_str()) << endl;

	for(const string &s : m_sites)
		ss << "  site: " << s << endl;

	return ss.str();
}
