/*
	httpModel.h
	mperron (2022)

	Data structures for modeling HTTP site configuration.
*/

#ifndef KWS3_HTTP_MODEL_H
#define KWS3_HTTP_MODEL_H

#include <list>
#include <string>

// A web site.
struct HttpSite {
	// Enabled/disabled.
	bool m_state;

	// Config model name.
	std::string m_name;

	// Hostnames for this site.
	std::list<std::string> m_hosts;

	// Document root.
	std::string m_root;

	HttpSite(const std::string &name) :
		m_state(true),
		m_name(name)
	{}
};

// A port receive and respond to HTTP requests.
struct HttpPort {
	// Enabled/disabled.
	bool m_state;

	// The TCP port to listen on.
	int m_port;

	// Web sites that might be served on this port if the request Host header matches.
	std::list<std::string> m_sites;

	// The site to serve if the Host header was not present in the request, or
	// did not match any configured site.
	std::string m_siteDefault;

	HttpPort(int port) :
		m_state(true),
		m_port(port)
	{}
};

#endif
