/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_HTTP_H
#define SOURCEMUD_MUD_HTTP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <map>

#include "common/types.h"
#include "common/strbuf.h"
#include "common/streams.h"
#include "common/imanager.h"
#include "mud/account.h"
#include "mud/macro.h"
#include "net/socket.h"

class HTTPSession
{
	public:
	HTTPSession(Account* s_account);

	std::string get_id() const { return id; }
	Account* get_account() const { return account; }

	std::string get_var(std::string id) const;
	void set_var(std::string id, std::string value);

	void update_timestamp();
	bool check_timestamp();

	void clear();

	private:
	std::string id;
	time_t timestamp;
	Account* account;
	std::map<std::string,std::string> vars;
};

class HTTPHandler : public SocketConnection, public IStreamSink, public IMacroObject
{
	public:
	HTTPHandler(int s_sock, const SockStorage& s_netaddr);

	// output
	virtual void stream_put(const char*, size_t len);
	virtual IStreamSink* get_stream() { return this; }

	// processing
	void process();
	void execute();

	// hard-coded pages
	void page_index();
	void page_login();
	void page_logout();
	void page_account();

	// error
	void http_error (int error, std::string msg);

	// get post data
	std::string get_post (std::string name) const;
	std::string get_request (std::string name) const;

	// get user account
	HTTPSession* get_session() const { return session; }
	Account* get_account() const { return session ? session->get_account() : NULL; }

	// low-level IO
	void disconnect();
	virtual void sock_input (char* buffer, size_t size);
	virtual void sock_hangup();
	virtual void sock_flush();

	// macro values
	int macro_property (const StreamControl& stream, std::string method, const MacroList& argv) const;
	void macro_default (const StreamControl& stream) const;

	protected:
	~HTTPHandler() {}

	protected:
	// parse urlencoded data (GET/POST)
	void parse_request_data (std::map<std::string,std::string>& map, const char* input) const;


	SockStorage addr;

	// HTTP parsing
	StringBuffer line;
	std::string url;
	std::string path;
	enum { NONE, URLENCODED } posttype;
	enum { GET, POST } reqtype;
	enum { REQ, HEADER, BODY, DONE, ERROR } state;
	size_t content_length;
	time_t timeout;
	std::map<std::string, std::string> headers;

	// request data
	std::map<std::string, std::string> get;
	std::map<std::string, std::string> post;

	// the session
	HTTPSession* session;
};

class SHTTPManager : public IManager
{
	public:
	virtual int initialize();
	virtual void shutdown();

	std::string get_template (std::string id);

	HTTPSession* create_session (Account* account);
	void destroy_session (HTTPSession* session);
	HTTPSession* get_session (std::string id);

	void check_timeouts();

	private:
	typedef std::map<std::string, std::string> TemplateMap;
	TemplateMap templates;

	typedef std::map<std::string, HTTPSession*> SessionMap;
	SessionMap sessions;
};
extern SHTTPManager HTTPManager;

struct StreamHTTPEscape {
	inline
	explicit StreamHTTPEscape(std::string s_text) : text(s_text) {}

	friend const class StreamControl& operator << (const class StreamControl& stream, const StreamHTTPEscape& esc);

	std::string text;
};
typedef StreamHTTPEscape StreamXMLEscape;

#endif
