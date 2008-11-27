/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "common/regex.h"

RegEx::RegEx (std::string pattern, bool nocase)
{
	memset(&regex, 0, sizeof(regex));
	regcomp(&regex, pattern.c_str(), REG_EXTENDED | (nocase ? REG_ICASE : 0));
}

RegEx::~RegEx ()
{
	regfree(&regex);
}

bool
RegEx::grep (std::string string)
{
	return !regexec(&regex, string.c_str(), 0, NULL, 0);
}

StringList
RegEx::match (std::string string)
{
	regmatch_t results[5];
	const int size = sizeof(results)/sizeof(results[0]);
	if(regexec(&regex, string.c_str(), size, results, 0))
		return StringList();

	StringList ret(size);
	for (int i = 0; i < size; ++i) {
		if (results[i].rm_so != -1)
			ret.push_back(std::string(string.c_str() + results[i].rm_so, results[i].rm_eo - results[i].rm_so));
		else
			ret.push_back(std::string());
	}
	return ret;
}
