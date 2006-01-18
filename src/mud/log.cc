/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include "log.h"
#include "error.h"
#include "awestr.h"
#include "server.h"
#include "settings.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

SLogManager LogManager;

namespace Log {
	LogWrapper Error(LOG_ERROR);
	LogWrapper Warning(LOG_WARNING);
	LogWrapper Info(LOG_NOTICE);
	LogWrapper Network(LOG_NETWORK);
	LogWrapper Admin(LOG_ADMIN);

	bool
	LogWrapper::stream_end (void)
	{
		LogManager.print(klass, msg.c_str());
		msg.clear();
		return false;
	}
}

int
SLogManager::initialize (void)
{
	path = SettingsManager.get_log_file();
	file = NULL;

	if (!path.empty()) {
		if ((file = fopen(path, "a")) == NULL) {
			Log::Error << "Unable to open log file " << path << ": " << strerror(errno);
			return 1;
		}
	}

	Log::Info << "<--- AweMUD V" VERSION " --->";

	if (!path.empty())
		Log::Info << "Logging to " << path;

	return 0;
}

void
SLogManager::shutdown (void)
{
	if (file != NULL)
		fclose(file);
}

void
SLogManager::print (LogClass klass, const char* msg)
{
	char tbuf[41];
	char log[2048];
	char* prefix;
	time_t t;
	struct tm local;
	time (&t);

	switch (klass) {
		case LOG_ERROR:
			prefix = "**ERROR**: ";
			break;
		case LOG_WARNING:
			prefix = "[warning]: ";
			break;
		case LOG_NETWORK:
			prefix = "[network]: ";
			break;
		case LOG_ADMIN:
			prefix = "[admin]: ";
			break;
		case LOG_NOTICE:
		default:
			prefix = "";
			break;
	}

	strftime (tbuf, sizeof(tbuf) - 1, "%Y-%m-%d %T", localtime_r (&t, &local));
	snprintf (log, sizeof(log), "%s - %s%s\n", tbuf, prefix, msg);

	if (file != NULL)
		fputs(log, file);
	else
		fputs(log, stderr);
}

void
SLogManager::reset (void)
{
	if (file != NULL) {
		fclose(file);
		file = fopen(path.c_str(), "a+");
		if (file == NULL)
			Log::Warning << "Failed to re-load SLogManager " << path << ": " << strerror(errno);
	}
}