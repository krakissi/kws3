/*
	config.h
	mperron (2022)

	The configuration model, which is used by both the server itself and the
	CLI which configures it.
*/

#ifndef KWS3_HTTP_MODEL_H
#define KWS3_HTTP_MODEL_H

#include <list>
#include <string>
#include <unordered_map>

struct SerialConfig {
	virtual void load(const std::string &str) = 0;
	virtual std::string save() const = 0;
	virtual std::string display() const = 0;
};

// A web site.
struct HttpSite : public SerialConfig {
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

	virtual void load(const std::string &str) override;
	virtual std::string save() const override;
	virtual std::string display() const override;
};

// A port receive and respond to HTTP requests.
struct HttpPort : public SerialConfig {
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

	virtual void load(const std::string &str) override;
	virtual std::string save() const override;
	virtual std::string display() const override;
};

struct Kws3Config {
	std::unordered_map<std::string, HttpSite*> m_sites;
	std::unordered_map<int, HttpPort*> m_ports;

	template<class T, class U>
	inline static void ClearMap(std::unordered_map<T, U> &m){
		for(auto it = m.begin(); it != m.end(); it = m.erase(it))
			delete it->second;
	}
};

#endif

