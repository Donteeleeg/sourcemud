/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef BODY_H
#define BODY_H

#include "awestr.h"
#include "gcbase.h"
#include "gcvector.h"
#include "fileobj.h"
#include "server.h"

// Gender
class GenderType
{
	public:
	typedef enum {
		NONE = 0,
		FEMALE,
		MALE,
		COUNT
	} type_t;
	
	public:
	inline GenderType (int s_value) : value((type_t)s_value) {}
	inline GenderType (void) : value(NONE) {}

	inline const String& get_name(void) const { return names[value]; }

	inline const String& get_hisher (void) const { return hisher[value]; }
	inline const String& get_hishers (void) const { return hishers[value]; }
	inline const String& get_heshe (void) const { return heshe[value]; }
	inline const String& get_himher (void) const { return himher[value]; }
	inline const String& get_manwoman (void) const { return manwoman[value]; }
	inline const String& get_malefemale (void) const { return malefemale[value]; }

	inline type_t get_value (void) const { return value; }

	static GenderType lookup (StringArg name);

	inline bool operator == (const GenderType& gender) const { return gender.value == value; }
	inline bool operator != (const GenderType& gender) const { return gender.value != value; }

	private:
	type_t value;

	static String names[];
	static String hisher[];
	static String hishers[];
	static String heshe[];
	static String himher[];
	static String manwoman[];
	static String malefemale[];
};

// Equip slot types
class EquipLocation
{
	public:
	typedef enum {
		NONE = 0,
		HEAD,
		TORSO,
		ARMS,
		LEGS,
		HANDS,
		FEET,
		NECK,
		BODY,
		BACK,
		WAIST,
		COUNT
	} type_t;
	
	public:
	EquipLocation (int s_value) : value((type_t)s_value) {}
	EquipLocation (void) : value(NONE) {}

	bool valid (void) const { return value != NONE; }

	StringArg get_name(void) const { return names[value]; }

	type_t get_value (void) const { return value; }

	static EquipLocation lookup (StringArg name);

	inline bool operator == (const EquipLocation& dir) const { return dir.value == value; }
	inline bool operator != (const EquipLocation& dir) const { return dir.value != value; }

	private:
	type_t value;

	static String names[];
};

#endif