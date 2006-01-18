/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef ZONE_H
#define ZONE_H

#include "entity.h"
#include "gcvector.h"
#include "server.h"
#include "imanager.h"

// announce flags
enum AnnounceFlags {
	ANFL_NONE = (0),
	ANFL_OUTDOORS = (1 << 0),
	ANFL_INDOORS = (1 << 1),
};

class SZoneManager;
class Zone;

class Spawn 
{
	protected:
	TagID tag;
	StringList blueprints;
	StringList rooms;
	uint min;
	uint delay;
	uint dcount;

	public:
	Spawn () : tag(), blueprints(), rooms(), min(1), delay(0), dcount(0) {}

	bool check (const class Zone* zone) const;
	void spawn (class Zone* zone) const;
	bool heartbeat ();

	int load (File::Reader& reader);
	void save (File::Writer& writer) const;
};

class Zone : public Entity
{
	public:
	Zone ();

	// zone ID
	String get_id () const { return id; }
	void set_id (StringArg new_id) { id = new_id; }

	// name information
	virtual EntityName get_name () const { return name; }
	virtual void set_name (StringArg s_name) { name.set_name(s_name); }

	// description information
	virtual String get_desc () const { return desc; }
	virtual void set_desc (StringArg s_desc) { desc = s_desc; }

	// find rooms
	class Room* get_room (StringArg name) const;
	class Room* get_room_at (size_t index) const;
	size_t get_room_count () const;

	// manage rooms
	void add_room (class Room*);

	// load/save
	using Entity::load;
	virtual int load_node(File::Reader& reader, File::Node& node);
	virtual int load_finish () { return 0; }
	int load (StringArg path);
	virtual void save (File::Writer& writer);
	virtual void save_hook (class ScriptRestrictedWriter* writer);
	void save ();

	// announce to all rooms
	void announce (StringArg text, AnnounceFlags type = ANFL_NONE) const;

	// update zone
	virtual void heartbeat ();

	// (de)activate children rooms
	virtual void activate ();
	virtual void deactivate ();

	// owner manager - see entity.h - we have no owner
	virtual Entity* get_owner () const { return NULL; }
	virtual void set_owner (Entity* owner);
	virtual void owner_release (Entity* child);

	virtual void destroy ();

	protected:
	String id;
	EntityName name;
	String desc;

	typedef GCType::vector<class Room*> RoomList;
	RoomList rooms;

	typedef GCType::vector<Spawn> SpawnList;
	SpawnList spawns;

	E_TYPE(Zone)

	friend class SZoneManager;
};

class SZoneManager : public IManager
{
	public:
	virtual int initialize ();
	virtual void shutdown ();
	virtual void save ();

	// lookup entries
	Zone* get_zone (StringArg);
	Zone* get_zone_at (size_t index);
	class Room* get_room (StringArg);

	// send an announcement to all the rooms in all the zones
	void announce (StringArg, AnnounceFlags = ANFL_NONE);

	// add a new zone
	void add_zone (Zone*);

	// show all rooms
	void list_rooms (const class StreamControl& stream);

	private:
	typedef GCType::vector<Zone*> ZoneList;
	ZoneList zones;

	friend void Zone::destroy ();
};
extern SZoneManager ZoneManager;

#define ZONE(ent) E_CAST(ent,Zone)

#endif