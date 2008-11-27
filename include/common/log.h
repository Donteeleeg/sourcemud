/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>

#include "common/string.h"
#include "common/error.h"
#include "common/streams.h"
#include "common/imanager.h"
#include "common/strbuf.h"

enum LogClass {
	LOG_NOTICE,
	LOG_WARNING,
	LOG_ERROR,
	LOG_NETWORK,
	LOG_ADMIN
};

class SLogManager : public IManager
{
	public:
	int initialize();
	void shutdown();

	void print(LogClass klass, const std::string& msg);

	void reset();

	std::string get_path() const { return path; }

	protected:
	std::string path;
	FILE *file;
};

extern SLogManager LogManager;

// C++ logging
namespace Log
{
	class LogWrapper : public IStreamSink
	{
		public:
		LogWrapper (LogClass s_klass) : klass(s_klass) {}

		virtual void stream_put(const char* str, size_t len);
		virtual void stream_end();

		private:
		LogClass klass;
		StringBuffer msg;
	};

	// the output
	extern LogWrapper Error;
	extern LogWrapper Warning;
	extern LogWrapper Info;
	extern LogWrapper Network;
	extern LogWrapper Admin;
}

#endif
