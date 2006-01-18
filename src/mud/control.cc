/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>

#include <algorithm>

#include "control.h"
#include "log.h"
#include "player.h"
#include "server.h"
#include "account.h"
#include "zone.h"
#include "settings.h"

SControlManager ControlManager;

#define MAX_CTRL_ARGS 20

#define CHECK_ADMIN \
	if (!is_admin()) { \
		*this << "+NOACCESS Access Denied\n"; \
		return; \
	} \

ControlHandler::ControlHandler (int s_sock, bool s_admin_flag) : SocketUser(s_sock)
{
	in_buffer[0] = '\0';
	admin_flag = s_admin_flag;
}

void
ControlHandler::in_handle (char* buffer, size_t size)
{
	int len = strlen(in_buffer);

	if (len + size + 1 > CONTROL_BUFFER_SIZE) {
		*this << "+INTERNAL Input overflow";
		close(sock);
		sock = -1;
		return;
	}

	memcpy(in_buffer + len, buffer, size);
	in_buffer[len + size] = 0;

	process();
}

void
ControlHandler::out_ready (void)
{
	// FIXME: might lose output
	if (send(sock, out_buffer.c_str(), out_buffer.size(), MSG_DONTWAIT) == -1) {
		Log::Error << "send() failed: " << strerror(errno);
		close(sock);
		sock = -1;
	}

	out_buffer.clear();
}

char
ControlHandler::get_poll_flags (void)
{
	char flags = POLLSYS_READ;
	if (!out_buffer.empty())
		flags |= POLLSYS_WRITE;
	return flags;
}

void
ControlHandler::hangup (void)
{
	Log::Network << "Control client disconnected";
}

void
ControlHandler::stream_put (const char* str, size_t len)
{
	out_buffer.append(str, len);
}

void
ControlHandler::process (void)
{
	int len = strlen(in_buffer);

	// process lines
	char* brk = strchr(in_buffer, '\n');
	while (brk != NULL) {
		size_t blen = brk - in_buffer;

		// setup
		int argc = 0;
		char* argv[MAX_CTRL_ARGS];
		argv[0] = in_buffer;
		char* cptr = in_buffer;
		char* bptr = in_buffer;
		bool quote = false;

		// process
		while (true) {
			// escape?  handle
			if (*cptr == '\\') {
				++cptr;
				// eol?  grr
				if (*cptr == '\n')
					break;
				// newline?
				else if (*cptr == 'n')
					*(bptr++) = '\n';
				// normal escape
				else
					*(bptr++) = *cptr;
			// quoted?  handle specially
			} else if (quote && *cptr) {
				// end quote?
				if (*cptr == '}')
					quote = false;
				// normal char
				else
					*(bptr++) = *cptr;
			// non-quoted space?  end of arg
			} else if (!quote && *cptr == ' ') {
				*bptr = 0;
				++argc;
				// out of argv?
				if (argc == 20)
					break;
				// increment arg pointer
				argv[argc] = ++bptr;
				// skip multiple whitespace
				while (cptr[1] == ' ')
					++cptr;
				// end?  break now
				if (cptr[1] == '\n')
					break;
			// end of input?
			} else if (*cptr == '\n') {
				*bptr = 0;
				++argc;
				break;
			// non-quoted { ?  begin quote
			} else if (!quote && *cptr == '{') {
				quote = true;
			// normal char
			} else {
				*(bptr++) = *cptr;
			}

			// next
			++cptr;
		}

		// process
		handle(argc, argv);

		// cleanup
		memmove(in_buffer, in_buffer + blen + 1, len - (blen + 1));
		in_buffer[len - (blen + 1)] = 0;
		len = strlen(in_buffer);

		// loop
		brk = strchr(in_buffer, '\n');
	}
}

void
ControlHandler::handle (int argc, char **argv)
{
	// server version
	if (str_eq(argv[0], "version")) {
		*this << "-" << VERSION << "\n+OK\n";
	// server build
	} else if (str_eq(argv[0], "build")) {
		*this << "-" << __DATE__ << " " << __TIME__ << "\n+OK\n";
	// account count
	} else if (str_eq(argv[0], "pcount")) {
		*this << "-" << PlayerManager.count() << "\n+OK\n";
	// quit
	} else if (str_eq(argv[0], "quit")) {
		*this << "+OK Farewell\n";
		close(sock);
		sock = -1;
	// change password
	} else if (str_eq(argv[0], "chpass")) {
		if (argc != 3) {
			*this << "+BADPARAM chpass <account> <pass>\n";
			return;
		}

		CHECK_ADMIN

		// check account exists
		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		// set password
		account->set_passphrase(argv[2]);

		Log::Info << "Password of account '" << argv[1] << "' changed over control interface";
		*this << "+OK Password changed\n";
	// new account
	} else if (str_eq(argv[0], "newaccount")) {
		// check args
		if (argc != 2) {
			*this << "+BADPARAM newaccount <account>\n";
			return;
		}

		CHECK_ADMIN

		// check accountname
		if (!AccountManager.valid_name(argv[1])) {
			*this << "+INVALID Invalid characters in account name or name too short or long\n";
			return;
		}

		// account exists?
		if (AccountManager.get(argv[1]) != NULL) {
			*this << "+DUPLICATE Account already exists\n";
			return;
		}

		// make the account
		if (AccountManager.create(argv[1]) == NULL) {
			*this << "+INTERNAL Failed to create new account\n";
			return;
		}
		Log::Info << "New account '" << argv[1] << "' created over control interface";

		*this << "+OK\n";
	// change name
	} else if (str_eq(argv[0], "chname")) {
		if (argc < 3) {
			*this << "+BADPARAM chname <account> <name>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		account->set_name(argv[2]);

		Log::Info << "Real name of account '" << account->get_id() << "' changed over control interface";
		*this << "+OK Name changed\n";
	// change email
	} else if (str_eq(argv[0], "chmail")) {
		if (argc != 3) {
			*this << "+BADPARAM chmail <account> <mail address>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		account->set_email(argv[2]);

		Log::Info << "E-mail address of account '" << account->get_name() << "' changed over control interface";
		*this << "+OK Mail address changed\n";
	// disable an account
	} else if (str_eq(argv[0], "disable")) {
		if (argc < 2) {
			*this << "+BADPARAM disable <account>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		account->set_disabled(true);
		account->save();

		Log::Info << "Account '" << account->get_id() << "' disabled over control interface";
		*this << "+OK Account disabled\n";
	// enable an account
	} else if (str_eq(argv[0], "enable")) {
		if (argc < 2) {
			*this << "+BADPARAM enable <account>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		account->set_disabled(false);
		account->save();

		Log::Info << "Account '" << account->get_id() << "' enabled over control interface";
		*this << "+OK Account enabled\n";
	// set max chars for an account
	} else if (str_eq(argv[0], "setmaxchars")) {
		if (argc < 3) {
			*this << "+BADPARAM setmaxchars <account> <amount>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		int amount = tolong(argv[2]);
		if (amount < 0) {
			*this << "+BADPARAM <amount> must be zero or greater\n";
			return;
		}

		account->set_max_chars(amount);
		account->save();

		Log::Info << "Account '" << account->get_id() << "' has max chars set to " << amount << " over control interface";
		*this << "+OK Account updated\n";
	// set max active for an account
	} else if (str_eq(argv[0], "setmaxactive")) {
		if (argc < 3) {
			*this << "+BADPARAM setmaxactive <account> <amount>\n";
			return;
		}

		CHECK_ADMIN

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		int amount = tolong(argv[2]);
		if (amount < 0) {
			*this << "+BADPARAM <amount> must be zero or greater\n";
			return;
		}

		account->set_max_active(amount);
		account->save();

		Log::Info << "Account '" << account->get_id() << "' has max active set to " << amount << " over control interface";
		*this << "+OK Account updated\n";
	// show account info
	} else if (str_eq(argv[0], "showaccount")) {
		if (argc < 2) {
			*this << "+BADPARAM showaccount <account>\n";
			return;
		}

		Account* account = AccountManager.get(argv[1]);
		if (account == NULL) {
			*this << "+NOTFOUND Account does not exist\n";
			return;
		}

		*this << "-ID=" << account->get_id() << "\n";
		*this << "-NAME=" << account->get_name() << "\n";
		*this << "-EMAIL=" << account->get_email() << "\n";
		*this << "-MAXCHARS=" << account->get_max_chars() << "\n";
		*this << "-MAXACTIVE=" << account->get_max_active() << "\n";
		*this << "-DISABLED=" << (account->is_disabled() ? "YES" : "NO") << "\n";
		*this << "-CHARACTERS=" << implode(account->get_char_list(), ',') << "\n";
		*this << "+OK\n";
	// shutdown server
	} else if (str_eq(argv[0], "shutdown")) {
		CHECK_ADMIN

		AweMUD::shutdown();
		*this << "+OK Shutting down\n";
	// announce
	} else if (str_eq(argv[0], "announce")) {
		if (argc < 2) {
			*this << "+BADPARAM announce <text>\n";
			return;
		}

		CHECK_ADMIN

		ZoneManager.announce(argv[1]);

		*this << "+OK\n";
	// connection list
	} else if (str_eq(argv[0], "connections")) {
		const IPConnList::ConnList& conn_list = NetworkManager.connections.get_conn_list();

		for (IPConnList::ConnList::const_iterator i = conn_list.begin(); i != conn_list.end(); ++i)
			*this << '-' << Network::get_addr_name(i->addr) << '\t' << i->conns << '\n';

		*this << "+OK\n";
	// unknown command
	} else {
		*this << "+BADCOMMAND " << argv[0] << "\n";
	}
}

int
SControlManager::initialize (void)
{
	struct passwd* pwd;

	StringList users = explode(SettingsManager.get_control_users(), ',');
	StringList admins = explode(SettingsManager.get_control_admins(), ',');

	for (StringList::iterator i = users.begin(); i != users.end(); ++i) {
		pwd = getpwnam(*i);
		if (pwd != NULL) {
			Log::Info << "User '" << pwd->pw_name << "' is a control user";
			user_list.push_back(pwd->pw_uid);
		} else {
			Log::Error << "Unknown user '" << *i << "' given as control user";
		}
	}

	for (StringList::iterator i = admins.begin(); i != admins.end(); ++i) {
		pwd = getpwnam(*i);
		if (pwd != NULL) {
			Log::Info << "User '" << pwd->pw_name << "' is a control admin";
			admin_list.push_back(pwd->pw_uid);
		} else {
			Log::Error << "Unknown user '" << *i << "' given as control admin";
		}
	}

	return 0;
}

void
SControlManager::shutdown (void)
{
}

bool
SControlManager::has_user_access (uid_t uid) const
{
	return std::find(user_list.begin(), user_list.end(), uid) != user_list.end();
}

bool
SControlManager::has_admin_access (uid_t uid) const
{
	return std::find(admin_list.begin(), admin_list.end(), uid) != admin_list.end();
}