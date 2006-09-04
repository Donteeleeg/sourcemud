/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include "mud/room.h"
#include "common/string.h"
#include "common/error.h"
#include "mud/color.h"
#include "mud/server.h"
#include "mud/player.h"
#include "common/streams.h"
#include "mud/zone.h"
#include "mud/hooks.h"
#include "scriptix/function.h"
#include "mud/efactory.h"

const String PortalDetail::names[] = {
	S("none"),
	S("in"),
	S("on"),
	S("over"),
	S("under"),
	S("across"),
	S("out"),
	S("up"),
	S("down"),
	S("through"),
};

const String PortalUsage::names[] = {
	S("walk"),
	S("climb"),
	S("crawl"),
};

const String PortalDir::names[] = {
	S("none"),
	S("north"),
	S("east"),
	S("south"),
	S("west"),
	S("northwest"),
	S("northeast"),
	S("southeast"),
	S("southwest"),
};
PortalDir::dir_t PortalDir::opposites[] = {
	PortalDir::NONE,
	PortalDir::SOUTH,
	PortalDir::WEST,
	PortalDir::NORTH,
	PortalDir::EAST,
	PortalDir::SOUTHEAST,
	PortalDir::SOUTHWEST,
	PortalDir::NORTHWEST,
	PortalDir::NORTHEAST,
};

// local tables
namespace {
	// When You go {the-portal}
	const String portal_go_table[PortalDetail::COUNT][PortalUsage::COUNT] = {
		{ S("You head to {$portal.d}."), S("You climb {$portal.d}."), S("You crawl to {$portal.d}.") },
		{ S("You go in {$portal.d}."), S("You climb in {$portal.d}."), S("You crawl in {$portal.d}.") },
		{ S("You get on {$portal.d}."), S("You climb on {$portal.d}."), S("You crawl on {$portal.d}.") },
		{ S("You head over {$portal.d}."), S("You climb over {$portal.d}."), S("You crawl over {$portal.d}.") },
		{ S("You go under {$portal.d}."), S("You climb beneath {$portal.d}."), S("You crawl under {$portal.d}.") },
		{ S("You head across {$portal.d}."), S("You climb across {$portal.d}."), S("You crawl across {$portal.d}.") },
		{ S("You go out {$portal.d}."), S("You climb out {$portal.d}."), S("You crawl out {$portal.d}.") },
		{ S("You go up {$portal.d}."), S("You climb up {$portal.d}."), S("You crawl up {$portal.d}.") },
		{ S("You go down {$portal.d}."), S("You climb down {$portal.d}."), S("You crawl down {$portal.d}.") },
		{ S("You head through {$portal.d}."), S("You climb through {$portal.d}."), S("You crawl through {$portal.d}.") },
	};
	// When {person} goes {the-portal}
	const String portal_leaves_table[PortalDetail::COUNT][PortalUsage::COUNT] = {
		{ S("{$actor.I} heads to {$portal.d}."), S("{$actor.I} climbs {$portal.d}."), S("{$actor.I} crawls to {$portal.d}.") },
		{ S("{$actor.I} goes in {$portal.d}."), S("{$actor.I} climbs in {$portal.d}."), S("{$actor.I} crawls in {$portal.d}.") },
		{ S("{$actor.I} gets on {$portal.d}."), S("{$actor.I} climbs on {$portal.d}."), S("{$actor.I} crawls on {$portal.d}.") },
		{ S("{$actor.I} heads over {$portal.d}."), S("{$actor.I} climbs over {$portal.d}."), S("{$actor.I} crawls over {$portal.d}.") },
		{ S("{$actor.I} goes under {$portal.d}."), S("{$actor.I} climbs beneath {$portal.d}."), S("{$actor.I} crawls under {$portal.d}.") },
		{ S("{$actor.I} heads across {$portal.d}."), S("{$actor.I} climbs across {$portal.d}."), S("{$actor.I} crawls across {$portal.d}.") },
		{ S("{$actor.I} goes out {$portal.d}."), S("{$actor.I} climbs out {$portal.d}."), S("{$actor.I} crawls out {$portal.d}.") },
		{ S("{$actor.I} goes up {$portal.d}."), S("{$actor.I} climbs up {$portal.d}."), S("{$actor.I} crawls up {$portal.d}.") },
		{ S("{$actor.I} goes down {$portal.d}."), S("{$actor.I} climbs down {$portal.d}."), S("{$actor.I} crawls down {$portal.d}.") },
		{ S("{$actor.I} heads through {$portal.d}."), S("{$actor.I} climbs through {$portal.d}."), S("{$actor.I} crawls through {$portal.d}.") },
	};
	// When {person} enters from {the-portal}
	const String portal_enters_table[PortalDetail::COUNT][PortalUsage::COUNT] = {
		{ S("{$actor.I} arrives from {$portal.d}."), S("{$actor.I} climbs in from {$portal.d}."), S("{$actor.I} crawls in from {$portal.d}.") },
		{ S("{$actor.I} comes out from {$portal.d}."), S("{$actor.I} climbs out from {$portal.d}."), S("{$actor.I} crawls out from {$portal.d}.") },
		{ S("{$actor.I} gets off {$portal.d}."), S("{$actor.I} climbs off {$portal.d}."), S("{$actor.I} crawls off {$portal.d}.") },
		{ S("{$actor.I} arrives from over {$portal.d}."), S("{$actor.I} climbs over from {$portal.d}."), S("{$actor.I} crawls over from {$portal.d}.") },
		{ S("{$actor.I} comes from under {$portal.d}."), S("{$actor.I} climbs from beneath {$portal.d}."), S("{$actor.I} crawls from under {$portal.d}.") },
		{ S("{$actor.I} arrives from across {$portal.d}."), S("{$actor.I} climbs from across {$portal.d}."), S("{$actor.I} crawls from across {$portal.d}.") },
		{ S("{$actor.I} comes in from {$portal.d}."), S("{$actor.I} climbs in from {$portal.d}."), S("{$actor.I} crawls in from {$portal.d}.") },
		{ S("{$actor.I} comes down from {$portal.d}."), S("{$actor.I} climbs down {$portal.d}."), S("{$actor.I} crawls down {$portal.d}.") },
		{ S("{$actor.I} comes up {$portal.d}."), S("{$actor.I} climbs up {$portal.d}."), S("{$actor.I} crawls up {$portal.d}.") },
		{ S("{$actor.I} comes through {$portal.d}."), S("{$actor.I} climbs through from {$portal.d}."), S("{$actor.I} crawls from through {$portal.d}.") },
	};
}

PortalDir
PortalDir::lookup (String name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (names[i] == name)
			return i;
	return NONE;
}

PortalUsage
PortalUsage::lookup (String name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (names[i] == name)
			return i;
	return WALK;
}

PortalDetail
PortalDetail::lookup (String name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (names[i] == name)
			return i;
	return NONE;
}

SCRIPT_TYPE(Portal);
Portal::Portal() : Entity(AweMUD_PortalType), name(),
	target(), dir(), usage(), detail(), parent_room(NULL),
	flags(), on_use()
{}

EntityName
Portal::get_name () const
{
	// default name w/ direction
	if (name.empty())
		return EntityName(EntityArticleClass::UNIQUE, dir.get_name());
	else
		return name;
}

void
Portal::add_keyword (String keyword)
{
	keywords.push_back(keyword);
}

Room *
Portal::get_target_room () const
{
	if (target)
		return ZoneManager.get_room (target);
	else
		return NULL;
}

Portal *
Portal::get_target_portal () const
{
	Room *r = ZoneManager.get_room (target);
	if (r == NULL)
		return NULL;

	Portal *e = r->get_portal_by_dir (dir.get_opposite());

	return e;
}

bool
Portal::is_valid () const
{
	return target && ZoneManager.get_room (target);
}

void
Portal::save (File::Writer& writer)
{
	if (!name.empty())
		writer.attr(S("portal"), S("name"), name.get_name());

	if (!desc.empty())
		writer.attr(S("portal"), S("desc"), desc);
	
	Entity::save (writer);

	for (StringList::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr(S("portal"), S("keyword"), *i);

	if (dir.valid())
		writer.attr(S("portal"), S("dir"), dir.get_name());
	if (usage != PortalUsage::WALK)
		writer.attr(S("portal"), S("usage"), usage.get_name());
	if (detail != PortalDetail::NONE)
		writer.attr(S("portal"), S("detail"), detail.get_name());
	if (is_hidden ())
		writer.attr(S("portal"), S("hidden"), true);
	if (is_door ()) {
		writer.attr(S("portal"), S("door"), true);
		if (is_closed ())
			writer.attr(S("portal"), S("closed"), true);
		if (is_locked ())
			writer.attr(S("portal"), S("locked"), true);
		if (!is_synced ())
			writer.attr(S("portal"), S("nosync"), true);
	}
	if (is_nolook())
		writer.attr(S("portal"), S("nolook"), true);
	if (is_disabled())
		writer.attr(S("portal"), S("disabled"), true);

	if (!target.empty())
		writer.attr(S("portal"), S("target"), target);

	if (text.enters)
		writer.attr(S("portal"), S("enters"), text.enters);
	if (text.leaves)
		writer.attr(S("portal"), S("leaves"), text.leaves);
	if (text.go)
		writer.attr(S("portal"), S("go"), text.go);

	if (!on_use.empty())
		writer.block (S("portal"), S("on_use"), on_use.get_source());
}

void
Portal::save_hook (ScriptRestrictedWriter* writer)
{
	Entity::save_hook(writer);
	Hooks::save_portal(this, writer);
}

int
Portal::load_node (File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_ATTR("portal", "name")
			set_name(node.get_data());
		FO_ATTR("portal", "keyword")
			keywords.push_back(node.get_data());
		FO_ATTR("portal", "desc")
			set_desc(node.get_data());
		FO_ATTR("portal", "usage")
			usage = PortalUsage::lookup(node.get_data());
		FO_ATTR("portal", "dir")
			dir = PortalDir::lookup(node.get_data());
		FO_ATTR("portal", "direction") // duplicate of above - should we keep this?
			dir = PortalDir::lookup(node.get_data());
		FO_ATTR("portal", "detail")
			detail = PortalDetail::lookup(node.get_data());
		FO_ATTR("portal", "hidden")
			FO_TYPE_ASSERT(BOOL);
			set_hidden(str_is_true(node.get_data()));
		FO_ATTR("portal", "door")
			FO_TYPE_ASSERT(BOOL);
			set_door(str_is_true(node.get_data()));
		FO_ATTR("portal", "closed")
			FO_TYPE_ASSERT(BOOL);
			set_closed(str_is_true(node.get_data()));
		FO_ATTR("portal", "locked")
			FO_TYPE_ASSERT(BOOL);
			set_locked(str_is_true(node.get_data()));
		FO_ATTR("portal", "nosync")
			FO_TYPE_ASSERT(BOOL);
			set_synced(str_is_true(node.get_data()));
		FO_ATTR("portal", "nolook")
			FO_TYPE_ASSERT(BOOL);
			set_nolook(str_is_true(node.get_data()));
		FO_ATTR("portal", "disabled")
			FO_TYPE_ASSERT(BOOL);
			set_disabled(str_is_true(node.get_data()));
		FO_ATTR("portal", "target")
			target = node.get_data();
		FO_ATTR("portal", "enters")
			text.enters = node.get_data();
		FO_ATTR("portal", "leaves")
			text.leaves = node.get_data();
		FO_ATTR("portal", "go")
			text.go = node.get_data();
		FO_ATTR("portal", "on_use")
			on_use = Scriptix::ScriptFunctionSource::compile(S("on_use"), node.get_data(), S("portal,user"), reader.get_filename(), node.get_line());
		FO_PARENT(Entity)
	FO_NODE_END
}

int
Portal::load_finish ()
{
	return 0;
}

void
Portal::open () {
	flags.closed = false;;

	if (is_synced ()) {
		Portal *other = get_target_portal ();
		if (other) {
			other->flags.closed = false;;
			*other->get_room() << StreamName(other, DEFINITE, true) << " is opened from the other side.\n";
		}
	}
}

void
Portal::close () {
	flags.closed = true;;

	if (is_synced ()) {
		Portal *other = get_target_portal ();
		if (other) {
			other->flags.closed = true;;
			*other->get_room() << StreamName(other, DEFINITE, true) << " is closed from the other side.\n";
		}
	}
}

void
Portal::unlock () {
	flags.locked = false;;

	if (is_synced ()) {
		Portal *other = get_target_portal ();
		if (other) {
			other->flags.locked = false;;
			*other->get_room() << "A click eminates from " << StreamName(other) << ".\n";
		}
	}
}

void
Portal::lock () {
	flags.locked = true;;

	if (is_synced ()) {
		Portal *other = get_target_portal ();
		if (other) {
			other->flags.locked = true;;
			*other->get_room() << "A click eminates from " << StreamName(other) << ".\n";
		}
	}
}

void
Portal::heartbeat ()
{
}

void
Portal::set_owner (Entity* owner)
{
	assert(ROOM(owner));
	Entity::set_owner(owner);
	parent_room = (Room*)owner;
}

Entity*
Portal::get_owner () const
{
	return parent_room;
}

void
Portal::owner_release (Entity* child)
{
	// we have no children
	assert(false);
}

String
Portal::get_go () const
{
	// customized?
	if (text.go)
		return text.go;

	// use table
	return portal_go_table[detail.get_value()][usage.get_value()];
}

String
Portal::get_leaves () const
{
	// customized?
	if (text.leaves)
		return text.leaves;

	// use table
	return portal_leaves_table[detail.get_value()][usage.get_value()];
}

String
Portal::get_enters () const
{
	// customized?
	if (text.enters)
		return text.enters;

	// use table
	return portal_enters_table[detail.get_value()][usage.get_value()];
}

bool
Portal::operator< (const Portal& portal) const
{
	// empty names always first
	if (name.empty() && !portal.name.empty())
		return true;
	else if (!name.empty() && portal.name.empty())
		return false;
		
	// sort by direction
	if (dir.get_value() < portal.dir.get_value())
		return true;
	else if (dir.get_value() > portal.dir.get_value())
		return false;

	// then name
	return get_name() < portal.get_name();
}

bool
Portal::name_match (String match) const
{
	if (name.matches(match))
		return true;

	// try keywords
	for (StringList::const_iterator i = keywords.begin(); i != keywords.end(); i ++)
		if (phrase_match (*i, match))
			return true;

	// no match
	return false;
}

void
Portal::set_use (String script)
{
	if (script) {
		on_use = Scriptix::ScriptFunctionSource::compile(S("on_use"), script, S("portal,user"), S("portal db"), 1);
	} else {
		on_use.clear();
	}
}