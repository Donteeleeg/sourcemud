/*
 * ZMP Example Implementation Library
 * Modified for AweMUD usage
 * http://www.awemud.net/zmp/
 */
 
/* Copyright (C) 2004	AwesomePlay Productions, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *	* Redistributions of source code must retain the above copyright notice,
 *		this list of conditions and the following disclaimer.
 *	* Redistributions in binary form must reproduce the above copyright
 *		notice, this list of conditions and the following disclaimer in the
 *		documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include <vector>

#include <stdlib.h>
#include <arpa/telnet.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#include "mud/zmp.h"
#include "mud/telnet.h"
#include "common/log.h"
#include "scriptix/function.h"
#include "scriptix/array.h"

SZMPManager ZMPManager;

// built-in handlers
namespace {
	void handle_zmp_ping (TelnetHandler* telnet, size_t argc, char** argv);
	void handle_zmp_check (TelnetHandler* telnet, size_t argc, char** argv);
	void handle_zmp_support (TelnetHandler* telnet, size_t argc, char** argv);
	void handle_zmp_input (TelnetHandler* telnet, size_t argc, char** argv);
}

// return 0 if not valid, or non-0 if valid
namespace {
	bool
	check_zmp_chunk(size_t size, const char* data)
	{
		// size must be at least two bytes
		if (size < 2)
			return false;

		// first byte must be printable ASCII
		if (!isprint(*data))
			return false;

		// last byte must be NUL
		if (data[size - 1] != '\0')
			return false;

		// good enough for us
		return true;
	}
}

// new zmp packed command
ZMPPack::ZMPPack (const char* command)
{
	add(command);
}

// free memory for command
ZMPPack::~ZMPPack (void)
{
	for (ArgList::iterator i = args.begin(); i != args.end(); ++i)
		delete[] *i;
	args.resize(0);
}

// add a string
ZMPPack&
ZMPPack::add (const char* command)
{
	// NULL evil.  grug kill.
	assert(command != NULL);

	// need to know length
	size_t len = strlen(command);

	// make new string of same length (plus NUL byte)
	char* newcmd = new char[len + 1];

	// copy string
	strcpy(newcmd, command);

	// add to arg list
	args.push_back(newcmd);

	return *this;
}

// add an 'int'
ZMPPack&
ZMPPack::add (long i)
{
	char buffer[40];
	snprintf(buffer, sizeof(buffer), "%ld", i);
	return add(buffer);
}

// add a 'uint'
ZMPPack&
ZMPPack::add (ulong i)
{
	char buffer[40];
	snprintf(buffer, sizeof(buffer), "%lu", i);
	return add(buffer);
}

SZMPManager::SZMPManager (void) : commands()
{
}

SZMPManager::~SZMPManager (void)
{
}

// initialize ZMP commands
int
SZMPManager::initialize (void)
{
	if (add("zmp.ping", handle_zmp_ping))
		return -1;
	if (add("zmp.check", handle_zmp_check))
		return -1;
	if (add("zmp.support", handle_zmp_support))
		return -1;
	if (add("zmp.input", handle_zmp_input))
		return -1;
	return 0;
}

// shutdown
void
SZMPManager::shutdown (void)
{
	commands.resize(0);
}

// register a new command
int
SZMPManager::add (StringArg name, ZMPFunction func)
{
	// must have a name
	if (!name)
		return -1;

	// must have a func
	if (!func)
		return -1;

	// add command
	ZMPCommand command;
	command.name = name;
	command.function = func;
	command.sx_function = NULL;
	command.wild = name[name.size()-1] == '.'; // ends in a . then its a wild-card match
	commands.push_back(command);

	return 0;
}

// register a new command
int
SZMPManager::add (StringArg name, Scriptix::ScriptFunction func)
{
	// must have a name
	if (!name)
		return -1;

	// must have a func
	if (func.empty())
		return -1;

	// add command
	ZMPCommand command;
	command.name = name;
	command.function = NULL;
	command.sx_function = func;
	command.wild = name[name.size()-1] == '.'; // ends in a . then its a wild-card match
	commands.push_back(command);

	return 0;
}

// find the request function; return NULL if not found
ZMPCommand*
SZMPManager::lookup(const char* name)
{
	// search list - easy enough
	for (ZMPCommandList::iterator i = commands.begin(); i != commands.end(); ++i) {
		if (i->wild && !strncmp(i->name.c_str(), name, i->name.size()))
			return &(*i);
		if (!i->wild && i->name == name)
			return &(*i);
	}
	// not found
	return NULL;
}

// match a package pattern; non-zero on match
bool
SZMPManager::match(const char* pattern)
{
	int package = 0; // are we looking for a package?
	size_t plen = strlen(pattern);

	// pattern must have a lengh
	if (plen == 0)
		return false;

	// check if this is a package we're looking for
	if (pattern[plen - 1] == '.')
		package = 1; // yes, it is

	// search for match
	for (ZMPCommandList::iterator i = commands.begin(); i != commands.end(); ++i) {
		// package match?
		if (package && !strncmp(i->name.c_str(), pattern, plen))
			return true; // found match
		else if (!package && i->name == pattern)
			return true; // found match
		else if (i->wild && !strncmp(i->name.c_str(), pattern, i->name.size()))
			return true; // found match
	}

	// no match
	return false;
}

// handle an ZMP command - size is size of chunk, data is chunk
void
TelnetHandler::process_zmp(size_t size, char* data)
{
	const size_t argv_size = 20; // argv[] element size
	char* argv[argv_size]; // arg list
	size_t argc; // number of args
	char* cptr; // for searching
	ZMPCommand* command;
	
	// check the data chunk is valid
	if (!check_zmp_chunk(size, data))
		return;
	
	// add command to argv
	argv[0] = data;
	argc = 1;

	// find the command
	command = ZMPManager.lookup(argv[0]);
	if (command == NULL) {
		// command not found
		return;
	}
	
	cptr = data; // init searching
	
	// parse loop - keep going as long as we have room in argv
	while (argc < argv_size) {
		// find NUL
		while (*cptr != '\0')
			++cptr;
	
		// is this NUL the last byte?
		if ((size_t)(cptr - data) == size - 1)
			break;
	
		// an argument follows
		++cptr; // move past the NUL byte
		argv[argc++] = cptr; // get argument
	}
		
	// invoke the proper command handler
	if (command->function) {
		// C++ function
		command->function(this, argc, argv);
	} else {
		// Scriptix Function
		Scriptix::Array* args = new Scriptix::Array(argc, NULL);
		for (uint i = 0; i < argc; ++i)
			Scriptix::Array::append(args, Scriptix::Value(argv[i]));
		command->sx_function.run(this, args);
	}
}

// send an zmp command
void
TelnetHandler::send_zmp(size_t argc, const char** argv)
{
	// check for ZMP support
	if (!has_zmp())
		return;

	// must have at least one arg
	if (!argc)
		return;

	// telnet codes
	static char start_sb[3] = { IAC, SB, 93 }; // begin; 93 is ZMP code
	static char end_sb[2] = { IAC, SE }; // end request
	static char double_iac[2] = { IAC, IAC }; // for IAC escaping

	size_t i; // current argument index
	const char* start; // for handling argument chunks
	const char* cptr; // for searching

	// send request start
	add_output(start_sb, 3);
	
	// loop through argv[], which has argc elements
	for (i = 0; i < argc; ++i) {
		// to handle escaping, we will send this in chunks
	
		start = argv[i]; // string section we are working on now
	
		// loop finding any IAC bytes
		while ((cptr = strchr(start, IAC)) != NULL) {
			// send the bytes from start until cptr
			add_output(start, cptr - start);
			// send the double IAC bytes
			add_output(double_iac, 2);
			// the byte _following_ the IAC is the new start
			start = cptr + 1;
		}
	
		/* send the rest of the argument - send one extra byte past
			 the remainder length, so we get the NUL byte in the string,
			 which we need to send for the ZMP specification. */
		add_output(start, strlen(start) + 1);
	}

	// send request end
	add_output(end_sb, 2);
}

// add a zmp command (to insert mid-processing, basically for color - YUCJ)
void
TelnetHandler::add_zmp(size_t argc, const char** argv)
{
	// check for ZMP support
	if (!has_zmp())
		return;

	// must have at least one arg
	if (!argc)
		return;

	// telnet codes
	static char start_sb[3] = { IAC, SB, 93 }; // begin; 93 is ZMP code
	static char end_sb[2] = { IAC, SE }; // end request
	static char double_iac[2] = { IAC, IAC }; // for IAC escaping

	size_t i; // current argument index
	const char* start; // for handling argument chunks
	const char* cptr; // for searching

	// send request start
	add_to_chunk(start_sb, 3);
	
	// loop through argv[], which has argc elements
	for (i = 0; i < argc; ++i) {
		// to handle escaping, we will send this in chunks
	
		start = argv[i]; // string section we are working on now
	
		// loop finding any IAC bytes
		while ((cptr = strchr(start, IAC)) != NULL) {
			// send the bytes from start until cptr
			add_to_chunk(start, cptr - start);
			// send the double IAC bytes
			add_to_chunk(double_iac, 2);
			// the byte _following_ the IAC is the new start
			start = cptr + 1;
		}
	
		/* send the rest of the argument - send one extra byte past
			 the remainder length, so we get the NUL byte in the string,
			 which we need to send for the ZMP specification. */
		add_to_chunk(start, strlen(start) + 1);
	}

	// send request end
	add_to_chunk(end_sb, 2);
}

// deal with ZMP support/no-support
void
TelnetHandler::zmp_support (const char* pkg, bool value)
{
	// color.define?
	if (str_eq(pkg, "color.define")) {
		io_flags.zmp_color = value;

		// init if true
		if (value) {
			char buf[10];
			const char* argv[4] = {"color.define", buf, NULL, NULL};
			for (int i = 1; i < NUM_CTYPES; ++i) {
				snprintf(buf, sizeof(buf), "%d", i);
				argv[2] = color_type_names[i];
				argv[3] = color_type_rgb[i];
				send_zmp(4, argv);
			}
		}
	}

	// net.awemud?
	else if (str_eq(pkg, "net.awemud.")) {
		io_flags.zmp_net_awemud = value;

		// init if true
		if (value) {
			// send net.awemud.name if we're on
			ZMPPack name("net.awemud.name");
			name.add("AweMUD NG");
			name.send(this);

			// make health status bar
			ZMPPack health_bar("net.awemud.status.create");
			health_bar.add("hp");
			health_bar.add("Health");
			health_bar.add("fraction");
			health_bar.send(this);

			// make round status bar
			ZMPPack round_bar("net.awemud.status.create");
			round_bar.add("rt");
			round_bar.add("Round");
			round_bar.add("count");
			round_bar.send(this);
		}
	}
}

// built-in handlers
namespace {
	// handle a zmp.ping command
	void
	handle_zmp_ping (TelnetHandler* telnet, size_t argc, char** argv)
	{
		// generate response
		char buffer[40];
		time_t t;
		time(&t);
		strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", gmtime(&t));
		buffer[sizeof(buffer) - 1] = 0;
		const char* response[2];
		response[0] = "zmp.time";
		response[1] = buffer;
		telnet->send_zmp(2, response);
	}

	// handle a zmp.check command
	void
	handle_zmp_check (TelnetHandler* telnet, size_t argc, char** argv)
	{
		// valid args?
		if (argc != 2)
			return;

		// have we the argument?
		if (ZMPManager.match(argv[1])) {
			argv[0] = "zmp.support";
			telnet->send_zmp(2, argv);
		// nope
		} else {
			argv[0] = "zmp.no-support";
			telnet->send_zmp(2, argv);
		}
	}

	// handle a zmp.support command
	void
	handle_zmp_support (TelnetHandler* telnet, size_t argc, char** argv)
	{
		// valid args?
		if (argc != 2)
			return;

		// tell the user about it
		telnet->zmp_support(argv[1], true);
	}

	// handle a zmp.no-support command
	void
	handle_zmp_nosupport (TelnetHandler* telnet, size_t argc, char** argv)
	{
		// valid args?
		if (argc != 2)
			return;

		// tell the user about it
		telnet->zmp_support(argv[1], false);
	}

	// handle a zmp.input command
	void
	handle_zmp_input (TelnetHandler* telnet, size_t argc, char** argv)
	{
		// valid args
		if (argc != 2)
			return;

		// process input
		telnet->process_command(argv[1]);
	}
}