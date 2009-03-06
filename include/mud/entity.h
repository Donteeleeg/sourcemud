/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef ENTITY_H
#define ENTITY_H

#include "common/types.h"
#include "common/error.h"
#include "common/imanager.h"
#include "mud/event.h"
#include "mud/color.h"
#include "mud/tag.h"
#include "mud/server.h"
#include "mud/clock.h"
#include "mud/macro.h"
#include "mud/name.h"
#include "lua/object.h"

#define E_SUBTYPE(name,par) \
	public: \
	typedef par _parent_type; \
	inline static const void *get_setype() { return (const void *)name::get_setype; } \
	virtual bool check_etype (const void *type) const { return (type == name::get_setype()) || par::check_etype(type); }
#define E_TYPE(name) E_SUBTYPE(name,Entity)

// for the global entity list
typedef std::list<Entity*> EntityList; // NOTE: no gc_alloc, don't want GC to scan this
typedef std::set<TagID> TagList;
typedef std::multimap<TagID, Entity*> TagTable; // NOTE: also non-GC scanning
typedef std::vector<EventHandler*> EventList;

// --- Entity Definiton ---

// entity control
class Entity : public IMacroObject
{
public:
	Entity();

	// factory handling
	virtual const char* factory_type() const = 0;
	static Entity* create(const std::string& type);  // efactory.cc

	// Lua scripting support -- returns a userdata representing the
	// Entity in question.
	void pushLuaTable() const { return Lua::getObject((void*)this); }

	// name stuff
	virtual EntityName get_name() const = 0;

	// description
	virtual std::string get_desc() const = 0;

	// events
	inline const EventList& get_events() const { return events; }
	EventHandler* get_event(EventID event);

	// active
	inline bool is_active() const { return state == ACTIVE; }
	virtual void activate(); // subclasses should call Entity::activate() first, then do custom code
	virtual void deactivate(); // subclasses do custom code, then call Entity::deactivate last

	// destroy() will remove the entity from its parent using
	// get_owner()->owner_release(this).  over-ridable for any necessary
	// custom destroy behaviour; *must* call Entity::destroy()
	virtual void destroy();

	// tags
	bool has_tag(TagID tag) const;
	int add_tag(TagID tag);
	int remove_tag(TagID tag);
	inline const TagList& get_tags() const { return tags; }

	// output
	virtual void display_desc(const class StreamControl& stream) const;
	virtual const char* ncolor() const { return CNORMAL; }

	// save/load
	static Entity* load(const std::string& factory, File::Reader& reader);
	int load(File::Reader& reader);
	virtual int load_node(File::Reader& reader, File::Node& node);
	virtual int load_finish() = 0;
	void save(File::Writer& writer, const std::string& ns, const std::string& name);
	virtual void save_data(File::Writer& writer);
	virtual void save_hook(File::Writer& writer);

	// check name
	virtual bool name_match(const std::string& name) const;

	// event triggers
	virtual void handle_event(const Event& event);
	virtual void broadcast_event(const Event& event) = 0;

	// for parsing, pull a property based on a char
	virtual int macro_property(const class StreamControl& stream, const std::string& method, const MacroList& argv) const;
	virtual void macro_default(const class StreamControl& stream) const;

	// heartbeat
	virtual void heartbeat() = 0;

	// sorting
	bool operator< (const Entity& ent) const;

	// our custom type checking system
	inline static const void *get_setype()
	{ return (const void *)Entity::get_setype; }
	inline virtual bool check_etype(const void *type) const
	{ return (type == Entity::get_setype()); }

	// owner management
	// set_owner() should *only* be called from an owning object's add*()
	// routines when adding the entity to the owner's child list.  get_owner()
	// returns the current owner.  owner_release() should *only* be called
	// from a child entity on its owner during a set_owner() operation.
	// set_owner() automatically sets the active state to the new owner's
	// active state.  set_owner() must be over-ridden to assert() on
	// invalid owner types and to set an owner pointer - it *MUST* call
	// Entity::set_owner() first.  get_owner() can be over-ridden to use a
	// sub-classed return type.  owner_release() must be over-ridden.
	virtual void set_owner(Entity* owner);
	virtual Entity* get_owner() const = 0;
	virtual void owner_release(Entity* child) = 0;

	// various data
protected:
	TagList tags;
	EventList events;

	// FLOAT: object created, not yet inserted into active world tree
	// ACTIVE: object within active world tree
	// DEAD: object removed from active world tree
	enum State { FLOAT, ACTIVE, DEAD } state;

private:
	// linked list tracking all entities
	Entity* link_prev;
	Entity* link_next;

	// event handler
	int perform_event(EventHandler *ea, Entity* trigger, Entity* target);

protected:
	// protected destructor
	virtual ~Entity();
	friend class _MEntity;
};

// manage all entities
class _MEntity : public IManager
{
public:
	_MEntity();
	~_MEntity();

	// initialize the system
	virtual int initialize();

	// shutdown system
	virtual void shutdown();

	// run a single heartbeat loop
	virtual void heartbeat();

	// fetch by tag
	std::pair<TagTable::const_iterator, TagTable::const_iterator> tag_list(TagID tag) const;

	// count by tag
	size_t tag_count(TagID tag) const;

	// free all dead entities
	void collect();

private:
	Entity* all;
	Entity* dead;

	// next entity we will be running the heartbeat for.
	// we track this so that if both the current and next
	// entities end up being moved to the dead list, we
	// still iterate to the proper entity.
	Entity* next;

	// tag map: no GC
	TagTable tag_map;

	// Entities have to be able to manage list - ick
	// also need to manage tag_map - double ick
	friend class Entity;
};
extern _MEntity MEntity;

// --- CASTING/TYPE-CHECKING ---

template <class TYPE>
struct _e_cast {
	static inline TYPE* cast(Entity* base) {
		return (base && base->check_etype(TYPE::get_setype())) ? (TYPE*)(base) : NULL;
	}
	static inline const TYPE* cast(const Entity* base) {
		return (base && base->check_etype(TYPE::get_setype())) ? (TYPE*)(base) : NULL;
	}
};

#define E_CAST(ENT,TYPE) (_e_cast<TYPE>::cast((ENT)))
#define ENTITY(ENT) E_CAST(ENT,Entity)

#endif
