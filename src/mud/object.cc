/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>

#include "mud/entity.h"
#include "mud/object.h"
#include "common/awestr.h"
#include "common/error.h"
#include "mud/char.h"
#include "mud/room.h"
#include "mud/server.h"
#include "mud/body.h"
#include "mud/player.h"
#include "common/streams.h"
#include "mud/settings.h"
#include "mud/hooks.h"

String ContainerType::names[] = {
	"none",
	"in",
	"on",
	"under",
	"behind",
};

ContainerType
ContainerType::lookup (StringArg name)
{
	for (uint i = 0; i < COUNT; ++i)
		if (str_eq(name, names[i]))
			return i;
	return NONE;
}

// ----- ObjectBlueprint -----

SCRIPT_TYPE(ObjectBlueprint);
ObjectBlueprint::ObjectBlueprint () : Scriptix::Native(AweMUD_ObjectBlueprintType), parent(NULL)
{
	weight = 0;
	cost = 0;
}

EntityName
ObjectBlueprint::get_name () const
{
	if (set_flags.name || parent == NULL)
		return name;
	else
		return parent->get_name();
}

void
ObjectBlueprint::reset_name ()
{
	// clear
	name.set_name("an object");
	set_flags.name = false;

	// get parent value
	const ObjectBlueprint* data = get_parent();
	if (data != NULL) {
		name = data->get_name();
	}
}

void
ObjectBlueprint::reset_desc ()
{
	// clear
	desc = "object";
	set_flags.desc = false;

	// get parent value
	const ObjectBlueprint* data = get_parent();
	if (data != NULL)
		desc = data->get_desc();
}

void
ObjectBlueprint::reset_weight ()
{
	// clear
	weight = 0;
	set_flags.weight = false;

	// get parent value
	const ObjectBlueprint* data = get_parent();
	if (data != NULL)
		weight = data->get_weight();
}

void
ObjectBlueprint::reset_cost ()
{
	// clear
	cost = 0;
	set_flags.cost = false;

	// get parent value
	const ObjectBlueprint* data = get_parent();
	if (data != NULL)
		cost = data->get_cost();
}

void
ObjectBlueprint::reset_equip ()
{
	// clear
	equip = 0;
	set_flags.equip = false;

	// get parent value
	const ObjectBlueprint* data = get_parent();
	if (data != NULL)
		equip = data->get_equip();
}

void
ObjectBlueprint::reset_hidden ()
{
	// clear
	flags.hidden = false;
	set_flags.hidden = false;

	// get parent value
	const ObjectBlueprint* data = get_parent();
	if (data != NULL)
		flags.hidden = data->is_hidden();
}

void
ObjectBlueprint::reset_gettable ()
{
	// clear
	flags.gettable = true;
	set_flags.gettable = false;

	// get parent value
	const ObjectBlueprint* data = get_parent();
	if (data != NULL)
		flags.gettable = data->is_gettable();
}

void
ObjectBlueprint::reset_touchable ()
{
	// clear
	flags.touchable = true;
	set_flags.touchable = false;

	// get parent value
	const ObjectBlueprint* data = get_parent();
	if (data != NULL)
		flags.touchable = data->is_touchable();
}

void
ObjectBlueprint::reset_dropable ()
{
	// clear
	flags.dropable = true;
	set_flags.dropable = false;

	// get parent value
	const ObjectBlueprint* data = get_parent();
	if (data != NULL)
		flags.dropable = data->is_dropable();
}

void
ObjectBlueprint::reset_trashable ()
{
	// clear
	flags.trashable = true;
	set_flags.trashable = false;

	// get parent value
	const ObjectBlueprint* data = get_parent();
	if (data != NULL)
		flags.trashable = data->is_trashable();
}

void
ObjectBlueprint::reset_rotting ()
{
	// clear
	flags.rotting = true;
	set_flags.rotting = false;

	// get parent value
	const ObjectBlueprint* data = get_parent();
	if (data != NULL)
		flags.rotting = data->is_rotting();
}

void
ObjectBlueprint::refresh ()
{
	if (!set_flags.name)
		reset_name();
	if (!set_flags.desc)
		reset_desc();
	if (!set_flags.weight)
		reset_weight();
	if (!set_flags.cost)
		reset_cost();
	if (!set_flags.equip)
		reset_equip();
	if (!set_flags.hidden)
		reset_hidden();
	if (!set_flags.gettable)
		reset_gettable();
	if (!set_flags.touchable)
		reset_touchable();
	if (!set_flags.dropable)
		reset_dropable();
	if (!set_flags.trashable)
		reset_trashable();
	if (!set_flags.rotting)
		reset_rotting();
}

void
ObjectBlueprint::save (File::Writer& writer)
{
	if (id)
		writer.attr("id", id);

	if (set_flags.name)
		writer.attr("name", name.get_name());

	if (set_flags.desc)
		writer.attr("desc", desc);

	for (StringList::const_iterator i = keywords.begin(); i != keywords.end(); ++i)
		writer.attr("keyword", *i);

	if (set_flags.equip)
		writer.attr("equip", equip.get_name());

	if (set_flags.cost)
		writer.attr("cost", cost);
	if (set_flags.weight)
		writer.attr("weight", weight);

	if (set_flags.hidden)
		writer.attr("roomlist", !is_hidden());
	if (set_flags.gettable)
		writer.attr("gettable", is_gettable());
	if (set_flags.touchable)
		writer.attr("touchable", is_touchable());
	if (set_flags.dropable)
		writer.attr("dropable", is_dropable());
	if (set_flags.trashable)
		writer.attr("trashable", is_trashable());
	if (set_flags.rotting)
		writer.attr("rotting", is_rotting());

	if (parent)
		writer.attr("parent", parent->get_id());

	for (ContainerList::const_iterator i = containers.begin (); i != containers.end (); i ++)
		writer.attr("container", i->get_name());

	for (ActionList::const_iterator i = actions.begin(); i != actions.end(); ++i) {
		writer.begin("action");
		writer.attr("id", i->first);
		writer.block("script", i->second.get_source());
		writer.end();
	}

	// script hook
	ScriptRestrictedWriter* swriter = new ScriptRestrictedWriter(&writer);
	Hooks::save_object_blueprint(this, swriter);
	swriter->release();
	swriter = NULL;
}

int
ObjectBlueprint::load (File::Reader& reader)
{
	FO_READ_BEGIN
		FO_ATTR("id")
			id = node.get_data();
		FO_ATTR("name")
			set_name(node.get_data());
		FO_ATTR("keyword")
			keywords.push_back(node.get_data());
		FO_ATTR("desc")
			set_desc(node.get_data());
		FO_ATTR("weight")
			FO_TYPE_ASSERT(INT);
			set_weight(tolong(node.get_data()));
		FO_ATTR("cost")
			FO_TYPE_ASSERT(INT);
			set_cost(tolong(node.get_data()));
		FO_ATTR("equip")
			set_equip(EquipLocation::lookup(node.get_data()));
		FO_ATTR("gettable")
			FO_TYPE_ASSERT(BOOL);
			set_gettable(str_is_true(node.get_data()));
		FO_ATTR("touchable")
			FO_TYPE_ASSERT(BOOL);
			set_touchable(str_is_true(node.get_data()));
		FO_ATTR("roomlist")
			FO_TYPE_ASSERT(BOOL);
			set_hidden(!str_is_true(node.get_data()));
		FO_ATTR("dropable")
			FO_TYPE_ASSERT(BOOL);
			set_dropable(str_is_true(node.get_data()));
		FO_ATTR("trashable")
			FO_TYPE_ASSERT(BOOL);
			set_trashable(str_is_true(node.get_data()));
		FO_ATTR("rotting")
			FO_TYPE_ASSERT(BOOL);
			set_rotting(str_is_true(node.get_data()));
		FO_ATTR("container")
			ContainerType type = ContainerType::lookup(node.get_data());
			if (type.valid())
				containers.insert(type);
		FO_OBJECT("action")
			String id;
			Scriptix::ScriptFunctionSource script;
			FO_READ_BEGIN
				FO_ATTR("id")
					id = node.get_data();
				FO_ATTR("script")
					script = Scriptix::ScriptFunctionSource::compile("action", node.get_data(), "self,user,data", reader.get_filename(), node.get_line());
					if(!script)
						throw File::Error("Failed to compile action script");
			FO_READ_ERROR
				return -1;
			FO_READ_END
			if (id.empty())
				throw File::Error("Action has no ID");
			actions[id] = script;
		FO_ATTR("parent")
			ObjectBlueprint* blueprint = ObjectBlueprintManager.lookup(node.get_data());
			if (blueprint)
				set_parent(blueprint);
			else
				Log::Warning << "Undefined parent object blueprint '" << node.get_data() << "' at " << reader.get_filename() << ':' << node.get_line();
		FO_CUSTOM
			if (node.get_datatype() == File::TYPE_INT)
				set_property(node.get_name(), tolong(node.get_data()));
			else if (node.get_datatype() == File::TYPE_STRING)
				set_property(node.get_name(), node.get_data());
			else if (node.get_datatype() == File::TYPE_BOOL)
				set_property(node.get_name(), str_is_true(node.get_data()));
			else {
				Log::Error << "Invalid data type for script attribute at " << reader.get_filename() << ':' << node.get_line();
				return -1;
			}
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
ObjectBlueprint::set_parent (ObjectBlueprint* blueprint)
{
	parent = blueprint;
	refresh();
}

Scriptix::Value
ObjectBlueprint::get_undefined_property (Scriptix::Atom id) const
{
	if (parent == NULL)
		return Scriptix::Nil;
	return parent->get_property(id);
}

bool
ObjectBlueprint::set_container_exist (ContainerType type, bool exist)
{
	// find container
	ContainerList::iterator i = containers.find(type);
	if (i != containers.end()) {
		if (!exist) {
			containers.erase (i);
			// parent(s) might still have it...
			return has_container(type);
		}
		// has container
		return true;
	}
	// add if doesn't exist
	if (exist) {
		containers.insert(type);
		return true;
	}
	// parent(s) might have it
	return has_container(type);
}

bool
ObjectBlueprint::has_container (ContainerType type) const
{
	const ObjectBlueprint* blueprint = this;
	while (blueprint) {
		if (blueprint->get_containers().find(type) != blueprint->get_containers().end())
			return true;
		blueprint = blueprint->get_parent();
	}

	return false;
}

// ----- Object -----

SCRIPT_TYPE(Object);
Object::Object () : Entity (AweMUD_ObjectType)
{
	blueprint = NULL;
	owner = NULL;
	calc_weight = 0;
	trash_timer = 0;
}

Object::Object (ObjectBlueprint* s_blueprint) : Entity(AweMUD_ObjectType)
{
	blueprint = NULL;
	owner = NULL;
	calc_weight = 0;
	set_blueprint(s_blueprint);
	trash_timer = 0;
}

Object::~Object ()
{
}

void
Object::save (File::Writer& writer)
{
	// save blueprint
	if (get_blueprint()) {
		// real blueprint
		if (get_blueprint()->get_id()) {
			writer.attr("blueprint", get_blueprint()->get_id());
		// anonymous blueprint
		} else {
			writer.begin("blueprint");
			get_blueprint()->save(writer);
			writer.end();
		}
	}

	// parent data
	Entity::save(writer);

	if (location.valid())
		writer.attr("location", location.get_name());

	for (EList<Object>::const_iterator e = children.begin (); e != children.end(); ++e) {
		writer.begin("object");
		(*e)->save (writer);
		writer.end();
	}
}

void
Object::save_hook (ScriptRestrictedWriter* writer)
{
	Entity::save_hook(writer);
	Hooks::save_object(this, writer);
}

int
Object::load_finish ()
{
	recalc_weight();
	
	if (blueprint == NULL) {
		Log::Error << "object has no blueprint";
		return -1;
	}

	return 0;
}

int
Object::load_node(File::Reader& reader, File::Node& node)
{
	FO_NODE_BEGIN
		FO_OBJECT("blueprint")
			// creates a new anonymous blueprint
			ObjectBlueprint* blueprint = new ObjectBlueprint();
			if (blueprint->load(reader))
				throw File::Error("failed to load anonymous blueprint");
			set_blueprint(blueprint);
		FO_ATTR("blueprint")
			// sets a real blueprint
			ObjectBlueprint* blueprint = NULL;
			if ((blueprint = ObjectBlueprintManager.lookup(node.get_data())) == NULL)
				Log::Error << "Could not find object blueprint '" << node.get_data() << "'";
			else
				set_blueprint(blueprint);
		FO_ATTR("location")
			location = ContainerType::lookup(node.get_data());
		FO_OBJECT("object")
			Object* obj = new Object ();
			if (obj->load (reader))
				throw File::Error("failed to load object");
			if (!obj->location.valid())
				throw File::Error("child object has no valid location");
			obj->set_owner (this);
			children.add (obj);
		FO_PARENT(Entity)
	FO_NODE_END
}

void
Object::set_owner (Entity* s_owner)
{
	// type check
	assert(OBJECT(s_owner) || ROOM(s_owner) || CHARACTER(s_owner));

	// set owner
	Entity::set_owner(s_owner);
	owner = s_owner;
}

void
Object::owner_release (Entity* child)
{
	// we only own objects
	Object* obj = OBJECT(child);
	assert(obj != NULL);

	// find it
	EList<Object>::iterator e = std::find(children.begin(), children.end(), obj);
	if (e != children.end()) {
		obj->location = ContainerType::NONE;
		children.erase(e);
		return;
	}
}

void
Object::heartbeat()
{
	// see if we can trash the object
	if (is_trashable()) {
		// must be laying in a room
		Room* room = ROOM(get_owner());
		if (room != NULL) {
			// rotting?
			if (is_rotting() && trash_timer >= OBJECT_ROT_TICKS) {
				// destroy it
				*room << StreamName(this, INDEFINITE, true) << " rots away.\n";
				destroy();

			// not rotting - normal trash
			} else if (trash_timer >= OBJECT_TRASH_TICKS) {
				// room must not have any players in it
				if (room->count_players() == 0) {
					// destroy it
					destroy();
				}
			} else {
				++trash_timer;
			}
		}
	}

	// call update hook
	Hooks::object_heartbeat(this);
}

void
Object::activate ()
{
	Entity::activate();

	for (EList<Object>::iterator e = children.begin (); e != children.end(); ++e)
		(*e)->activate();
}

void
Object::deactivate ()
{
	for (EList<Object>::iterator e = children.begin (); e != children.end(); ++e)
		(*e)->deactivate();

	Entity::deactivate();
}

Scriptix::ScriptFunction
Object::get_action (StringArg name)
{
	ObjectBlueprint* blueprint = get_blueprint();
	while (blueprint) {
		ObjectBlueprint::ActionList::const_iterator i = blueprint->get_actions().find(name);
		if (i != blueprint->get_actions().end())
			return i->second;
		blueprint = blueprint->get_parent();
	}
	return NULL;
}

ObjectActionCode
Object::do_action (StringArg name, Entity* user, Scriptix::Value data)
{
	// get script
	Scriptix::ScriptFunction func = get_action(name);

	// no script?  return default
	if (func.empty())
		return OBJECT_ACTION_OK_NORMAL;

	// run script
	int res = func.run(this, user, data);

	// sanity check return value
	if (res != OBJECT_ACTION_OK_NORMAL && res != OBJECT_ACTION_OK_QUIET && res != OBJECT_ACTION_CANCEL_NORMAL && res != OBJECT_ACTION_CANCEL_QUIET) {
		Log::Error << "Object action '" << name << "' on " << get_name().get_text() << " returned invalid object action code";
		return OBJECT_ACTION_CANCEL_NORMAL;
	}

	// return script code
	return (ObjectActionCode)res;
}

bool
Object::has_container (ContainerType type) const
{
	assert(blueprint);
	return blueprint->has_container(type);
}

bool
Object::add_object (Object *object, ContainerType type)
{
	assert (type.valid());
	assert (object != NULL);

	// has contianer?
	if (!has_container(type))
		return false;

	// release and add
	object->set_owner(this);
	object->location = type;
	children.add(object);

	// recalc our weight, and parent's weight
	recalc_weight();
	if (OBJECT(owner))
		((Object*)owner)->recalc_weight();

	// ok add
	return true;
}

void
Object::show_contents (Player *player, ContainerType type) const
{
	*player << "You see ";
	
	Object* last = NULL;
	int displayed = 0;

	// show objects
	for (EList<Object>::const_iterator i = children.begin (); i != children.end(); ++i) {
		// not right container?
		if ((*i)->location != type)
			continue;
		// had a previous item?
		if (last != NULL) {
			// first item?
			if (displayed)
				*player << ", ";
			*player << StreamName(last, INDEFINITE, false);
			++displayed;
		}
		last = *i;
	}
	// one more item?
	if (last != NULL) {
		if (displayed > 1)
			*player << ", and ";
		else if (displayed == 1)
			*player << " and ";
		*player << StreamName(last, INDEFINITE, false);
		++displayed;
	}

	// no items?
	if (!displayed)
		*player << "nothing";

	// finish up
	*player << " " << type.get_name() << " " << StreamName(*this, DEFINITE, false) << ".\n";
}

Object *
Object::find_object (const char *name, uint index, ContainerType type, uint *matches) const
{
	assert (name != NULL);
	assert (index != 0);

	// clear matches
	if (matches)
		*matches = 0;
	
	for (EList<Object>::const_iterator i = children.begin (); i != children.end(); ++i) {
		// right container type
		if (!type.valid() || (*i)->location == type) {
			// check name
			if ((*i)->name_match (name)) {
				if (matches)
					++ *matches;
				if ((-- index) == 0)
					return OBJECT((*i));
			}
		}
	}

	// not found
	return NULL;
}

// recalc weight of object
void
Object::recalc_weight ()
{
	calc_weight = 0;

	// add up weight of objects
	for (EList<Object>::const_iterator i = children.begin(); i != children.end(); ++i)
		if ((*i)->location == ContainerType::IN || (*i)->location == ContainerType::ON)
			calc_weight += (*i)->get_weight();
}

// find parent room
Room*
Object::get_room () const
{
	Entity* owner = get_owner();
	while (owner != NULL && !ROOM(owner))
		owner = owner->get_owner();
	return ROOM(owner);
}

// find parent owner
Character* 
Object::get_holder () const
{
	Entity* owner = get_owner();
	while (owner != NULL && !CHARACTER(owner))
		owner = owner->get_owner();
	return CHARACTER(owner);
}

// get object name information
EntityName
Object::get_name () const
{
	assert(blueprint != NULL);
	if (name.empty())
		return blueprint->get_name();
	else
		return name;
}

// get object description
String
Object::get_desc () const
{
	assert(blueprint != NULL);
	return blueprint->get_desc();
}

// get object properties
uint
Object::get_cost () const
{
	assert(blueprint != NULL);
	return blueprint->get_cost();
}
uint
Object::get_real_weight () const
{
	assert(blueprint != NULL);
	return blueprint->get_weight();
}
EquipLocation
Object::get_equip () const
{
	assert(blueprint != NULL);
	return blueprint->get_equip();
}

// get flags
bool
Object::is_hidden () const
{
	assert(blueprint != NULL);
	return blueprint->is_hidden();
}
bool
Object::is_trashable () const
{
	assert(blueprint != NULL);
	return blueprint->is_trashable();
}
bool
Object::is_gettable () const
{
	assert(blueprint != NULL);
	return blueprint->is_gettable();
}
bool
Object::is_dropable () const
{
	assert(blueprint != NULL);
	return blueprint->is_dropable();
}
bool
Object::is_touchable () const
{
	assert(blueprint != NULL);
	return blueprint->is_touchable();
}
bool
Object::is_rotting () const
{
	assert(blueprint != NULL);
	return blueprint->is_rotting();
}

// get parsable member values
int
Object::parse_property (const StreamControl& stream, StringArg comm, const ParseArgs& argv) const
{
	// COST
	if (!strcmp(comm, "cost")) {
		stream << get_cost();
		return 0;
	}
	// WEIGHT
	if (!strcmp(comm, "weight")) {
		stream << get_weight();
		return 0;
	}
	// try the entity
	return Entity::parse_property(stream, comm, argv);
}

void
Object::set_blueprint (ObjectBlueprint* s_blueprint)
{
	blueprint = s_blueprint;
}

// load object from a blueprint
Object*
Object::load_blueprint (StringArg name)
{
	ObjectBlueprint* blueprint = ObjectBlueprintManager.lookup(name);
	if (!blueprint)
		return NULL;
	
	return new Object(blueprint);
}

bool
Object::is_blueprint (StringArg name) const
{
	ObjectBlueprint* blueprint = get_blueprint();

	while (blueprint != NULL) {
		if (str_eq(blueprint->get_id(), name))
			return true;

		blueprint = blueprint->get_parent();
	}

	return false;
}

bool
Object::name_match (StringArg name) const
{
	if (phrase_match (get_name().get_text(), name))
		return true;

	// blueprint keywords
	ObjectBlueprint* blueprint = get_blueprint();
	while (blueprint != NULL) {
		for (StringList::const_iterator i = blueprint->get_keywords().begin(); i != blueprint->get_keywords().end(); i ++)
			if (phrase_match (*i, name))
				return true;

		blueprint = blueprint->get_parent();
	}

	// no match
	return false;
}

Scriptix::Value
Object::get_undefined_property (Scriptix::Atom id) const
{
	const ObjectBlueprint* data = get_blueprint();
	if (data == NULL)
		return Scriptix::Nil;
	return data->get_property(id);
}

// Object Blueprint Manager

SObjectBlueprintManager ObjectBlueprintManager;

int
SObjectBlueprintManager::initialize ()
{
	// requirements
	if (require(ScriptBindings) != 0)
		return 1;
	if (require(EventManager) != 0)
		return 1;


	Log::Info << "Loading object blueprints";

	String path = SettingsManager.get_blueprint_path();
	
	dirent* d_ent;
	DIR* dir = opendir(path.c_str());
	if (!dir) {
		Log::Error << "Failed to open blueprint folder '" << path << "': " << strerror(errno);
		return -1;
	}
	while ((d_ent = readdir(dir)) != NULL) {
		// match file name
		size_t len = strlen(d_ent->d_name);
		if (len >= 6 && d_ent->d_name[0] != '.' && str_eq(".objs", &d_ent->d_name[len - 5])) {
			// load from file
			File::Reader reader;
			if (reader.open(path + "/" + d_ent->d_name))
				return -1;
			FO_READ_BEGIN
				FO_OBJECT("object_blueprint")
					ObjectBlueprint* blueprint = new ObjectBlueprint();
					if (blueprint->load(reader)) {
						Log::Warning << "Failed to load blueprint in " << reader.get_filename() << " at " << node.get_line();
						return -1;
					}

					if (!blueprint->get_id()) {
						Log::Warning << "Blueprint has no ID in " << reader.get_filename() << " at " << node.get_line();
						return -1;
					}

					blueprints[blueprint->get_id()] = blueprint;
			FO_READ_ERROR
				return -1;
			FO_READ_END
		}
	}
	closedir(dir);

	return 0;
}

void
SObjectBlueprintManager::shutdown ()
{
}

ObjectBlueprint*
SObjectBlueprintManager::lookup (StringArg id)
{
	BlueprintMap::iterator iter = blueprints.find(id);
	if (iter == blueprints.end())
		return NULL;
	else
		return iter->second;
}