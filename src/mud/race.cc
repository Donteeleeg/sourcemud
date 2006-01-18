/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common/rand.h"
#include "mud/race.h"
#include "mud/settings.h"
#include "mud/fileobj.h"
#include "common/log.h"
#include "mud/char.h"
#include "mud/pdesc.h"

SRaceManager RaceManager;

SCRIPT_TYPE(PlayerRace);
Race::Race (StringArg s_name, Race *s_next) :
	Scriptix::Native(AweMUD_PlayerRaceType),
	name(s_name.c_str()),
	next(s_next) {}

int
Race::load (File::Reader& reader)
{
	// clear and/or defaults
	adj.clear();
	about = "AweMUD player race.";
	desc.clear();
	age_min = 0;
	age_max = 0;
	life_span = 0;
	body = "human";
	traits.clear();
	skin_type = "skin";
	height[GenderType::NONE] = 72;
	height[GenderType::FEMALE] = 65;
	height[GenderType::MALE] = 68;
	for (int i = 0; i < CharStatID::COUNT; ++i)
		stats[i] = 0;

	FO_READ_BEGIN
		FO_ATTR("name")
			FO_TYPE_ASSERT(STRING)
			name = node.get_data();
		FO_ATTR("adj")
			FO_TYPE_ASSERT(STRING)
			adj = node.get_data();
		FO_ATTR("body")
			FO_TYPE_ASSERT(STRING)
			body = node.get_data();
		FO_ATTR("desc")
			FO_TYPE_ASSERT(STRING)
			desc = node.get_data();
		FO_ATTR("about")
			FO_TYPE_ASSERT(STRING)
			about = node.get_data();
		FO_ATTR("skin_type")
			FO_TYPE_ASSERT(STRING)
			skin_type = node.get_data();
		FO_ATTR("min_age")
			FO_TYPE_ASSERT(INT)
			age_min = tolong(node.get_data());
		FO_ATTR("max_age")
			FO_TYPE_ASSERT(INT)
			age_max = tolong(node.get_data());
		FO_ATTR("lifespan")
			FO_TYPE_ASSERT(INT)
			life_span = tolong(node.get_data());
		FO_ATTR("neuter_height")
			FO_TYPE_ASSERT(INT)
			height[GenderType::NONE] = tolong(node.get_data());
		FO_ATTR("female_height")
			FO_TYPE_ASSERT(INT)
			height[GenderType::FEMALE] = tolong(node.get_data());
		FO_ATTR("male_height")
			FO_TYPE_ASSERT(INT)
			height[GenderType::MALE] = tolong(node.get_data());
		FO_KEYED("trait")
			FO_TYPE_ASSERT(STRING)
			CharacterTraitID trait = CharacterTraitID::lookup(node.get_key());
			if (!trait.valid())
				throw File::Error("Invalid trait");
			CharacterTraitValue value = CharacterTraitValue::lookup(node.get_data());
			if (!value.valid())
				throw File::Error("Invalid trait value");

			GCType::map<CharacterTraitID, GCType::set<CharacterTraitValue> >::iterator i = traits.find(trait);
			if (i == traits.end()) {
				GCType::set<CharacterTraitValue> values;
				values.insert(value);
				traits[trait] = values;
				i = traits.find(trait);
			} else {
				std::pair<GCType::set<CharacterTraitValue>::iterator, bool> ret = i->second.insert(value);
			}
		FO_KEYED("stat")
			CharStatID stat = CharStatID::lookup(node.get_name());
			if (stat) {
				stats[stat.get_value()] = tolong(node.get_data());
			}
	FO_READ_ERROR
		return -1;
	FO_READ_END

	// fixup adjective
	if (!adj)
		adj = name;

	// sanity check age/lifespan
	if ((age_min > age_max) || (age_min <= 0) || (age_max >= life_span) || (life_span <= 0)) {
		Log::Error << "Minimum/maximum ages and/or lifespan in race '" << name << "' are invalid";
		return -1;
	}

	// ok
	return 0;
}

int
Race::get_rand_age (void) const
{
	return age_min + get_random (age_max - age_min);
}

bool
Race::has_trait_value (CharacterTraitID trait, CharacterTraitValue value) const
{
	GCType::map<CharacterTraitID, GCType::set<CharacterTraitValue> >::const_iterator i = traits.find(trait);
	if (i == traits.end())
		return false;

	return i->second.find(value) != i->second.end();
}

const GCType::set<CharacterTraitValue>&
Race::get_trait_values (CharacterTraitID trait) const
{
	static const GCType::set<CharacterTraitValue> empty; // if we don't have the trait, we return a reference to this

	GCType::map<CharacterTraitID, GCType::set<CharacterTraitValue> >::const_iterator i = traits.find(trait);
	if (i == traits.end())
		return empty;

	return i->second;
}

Race *
SRaceManager::get (StringArg name)
{
	Race *race = head;
	while (race != NULL) {
		if (str_eq (race->get_name(), name))
			return race;
		race = race->get_next();
	}
	return NULL;
}

int
SRaceManager::initialize(void)
{
	require(CharacterTraitManager);

	Log::Info << "Loading player races";

	File::Reader reader;
	String path = SettingsManager.get_misc_path() + "/races";

	if (reader.open(path)) {
		Log::Error << "Failed to open " << path;
		return 1;
	}

	// load
	FO_READ_BEGIN
		FO_OBJECT("race")
			// load race
			Race *race = new Race (node.get_name(), head);
			if (race->load (reader))
				return -1;
			head = race;
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
SRaceManager::shutdown (void)
{
	head = NULL;
}