/*
	config.cc
	mperron (2022)
*/

#define LABEL_WIDTH 14

#include "config.h"

#include <sstream>
#include <iomanip>

using namespace std;

void Cfg::ObjList::load(const string &str){
	stringstream ss(str);
	string item;

	m_items.clear();

	while(!!(ss >> item))
		m_items.push_back(item);
}

string Cfg::ObjList::save() const {
	stringstream ss;

	for(const auto &item : m_items)
		ss << item << " ";

	return ss.str();
}

string Cfg::ObjList::display() const {
	stringstream ss;

	ss << save() << endl;

	return ss.str();
}


void Cfg::HttpSite::load(const string &str){
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

string Cfg::HttpSite::save() const {
	stringstream ss;

	ss << m_name
		<< " state:" << m_state
		<< " root:" << m_root;

	for(string host : m_hosts)
		ss << " host:" << host;

	return ss.str();
}

string Cfg::HttpSite::display() const {
	stringstream ss;

	ss << "http-site " << m_name << endl
		<< setw(LABEL_WIDTH) << "state: " << (m_state ? "enabled" : "disabled") << endl
		<< setw(LABEL_WIDTH) << "root: " << (m_root.empty() ? "(not set)" : m_root.c_str()) << endl;

	for(const string &s : m_hosts)
		ss << setw(LABEL_WIDTH) << "  host: " << s << endl;

	return ss.str();
}

void Cfg::HttpPort::load(const string &str){
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
			else if(k == "local")
				m_local = stoi(v);
		}
	}
}

string Cfg::HttpPort::save() const {
	stringstream ss;

	ss << m_port
		<< " state:" << m_state
		<< " siteDefault:" << m_siteDefault
		<< " local:" << m_local;

	for(auto site : m_sites)
		ss << " site:" << site;

	return ss.str();
}

string Cfg::HttpPort::display() const {
	stringstream ss;

	ss << "http-port " << m_port << endl
		<< setw(LABEL_WIDTH) << "state: " << (m_state ? "enabled" : "disabled") << endl
		<< setw(LABEL_WIDTH) << "local: " << (m_local ? "yes" : "no") << endl
		<< setw(LABEL_WIDTH) << "site-default: " << (m_siteDefault.empty() ? "(not set)" : m_siteDefault.c_str()) << endl;

	for(const string &s : m_sites)
		ss << setw(LABEL_WIDTH) << "site: " << s << endl;

	return ss.str();
}
