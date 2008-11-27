/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_MACRO_H
#define SOURCEMUD_MUD_MACRO_H

#include <map>
#include <vector>
#include "common/streams.h"

class MacroValue;

typedef std::map<std::string, MacroValue> MacroArgs;
typedef std::vector<MacroValue> MacroList;

class IMacroObject
{
	public:
	virtual ~IMacroObject () {}

	// return non-zero if the requested method/property does not exist
	virtual int macro_property (const class StreamControl& stream, std::string method, const MacroList& argv) const = 0;

	// stream a default desc/name/whatever
	virtual void macro_default (const class StreamControl& stream) const = 0;
};

class MacroValue
{
	public:
	enum Type { T_OBJECT, T_STRING, T_NULL };

	// constructors
	MacroValue () : type(T_NULL), object(NULL), string() {}
	MacroValue (const IMacroObject* s_object) : type(s_object == NULL ? T_NULL : T_OBJECT), object(s_object), string() {}
	MacroValue (std::string s_string) : type(T_STRING), object(NULL), string(s_string) {}

	// fetch the type of the mixed value
	Type get_type () const { return type; }
	bool is_object () const { return type == T_OBJECT; }
	bool is_string () const { return type == T_STRING; }
	bool is_null () const { return type == T_NULL; }

	// specific getters
	const IMacroObject* get_object () const { return this->object; }
	const std::string& get_string () const { return this->string; }

	// assign
	const IMacroObject* operator= (const IMacroObject* object) { type = (object == NULL ? T_NULL : T_OBJECT); return this->object = object; }
	const std::string& operator= (std::string string) { type = T_STRING; return this->string = string; }
	MacroValue& operator= (const MacroValue& base) { type = base.type; object = base.object; string = base.string; return *this; }

	private:
	Type type;
	const IMacroObject* object;
	std::string string;
};

namespace macro {
	const StreamControl& text (const StreamControl& stream, std::string format, const MacroArgs& argv);
}

// macro info
struct StreamMacro {
	StreamMacro(std::string s_text);
	StreamMacro(std::string s_text, std::string s_name, MacroValue s_value);
	StreamMacro(std::string s_text, std::string s_name1, MacroValue s_value1, std::string s_name2, MacroValue s_value2);
	StreamMacro(std::string s_text, std::string s_name1, MacroValue s_value1, std::string s_name2, MacroValue s_value2, std::string s_name3, MacroValue s_value3);

	StreamMacro& add (std::string s_name, MacroValue s_value);

	friend inline
	const StreamControl&
	operator << (const StreamControl& stream, const StreamMacro& smacro)
	{
		return macro::text(stream, smacro.text, smacro.argv);
	}

	private:
	std::string text;
	MacroArgs argv;
};

#endif
