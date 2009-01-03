/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef SOURCEMUD_MUD_NPC_H
#define SOURCEMUD_MUD_NPC_H

#include "mud/creature.h"
#include "common/imanager.h"

// Npc blueprint
class
			NpcBP
{
public:
	NpcBP();

	// blueprint id
	inline std::string get_id() const { return id; }

	// npc
	inline const std::vector<std::string>& get_equip_list() const { return equip_list; }

	// load
	int load(File::Reader& reader);
	void save(File::Writer& writer);

	// parent blueprint
	virtual NpcBP* get_parent() const { return parent; }

	// name
	inline const EntityName& get_name() const { return name; }
	inline bool set_name(const std::string& s_name) { bool ret = name.set_name(s_name); set_flags.name = true; return ret; }
	void reset_name();

	inline const std::vector<std::string>& get_keywords() const { return keywords; }

	// description
	inline const std::string& get_desc() const { return desc; }
	inline void set_desc(const std::string& s_desc) { desc = s_desc; set_flags.desc = true; }
	void reset_desc();

	// stats
	inline int get_stat(CreatureStatID stat) const { return base_stats[stat]; }
	inline void set_stat(CreatureStatID stat, int value) { base_stats[stat] = value; }
	void reset_stats();

	// gender
	inline GenderType get_gender() const { return gender; }
	inline void set_gender(GenderType s_gender) { gender = s_gender; set_flags.gender = true; }
	void reset_gender();

	// combat
	inline uint get_combat_dodge() const { return combat.dodge; }
	inline uint get_combat_attack() const { return combat.attack; }
	inline uint get_combat_damage() const { return combat.damage; }
	inline void set_combat_dodge(uint value) { combat.dodge = value; set_flags.dodge = true; }
	inline void set_combat_attack(uint value) { combat.attack = value; set_flags.attack = true; }
	inline void set_combat_damage(uint value) { combat.damage = value; set_flags.damage = true; }
	void reset_combat_dodge();
	void reset_combat_attack();
	void reset_combat_damage();

	// refresh all data
	void refresh();

private:
	std::string id;
	EntityName name;
	std::string desc;
	std::vector<std::string> keywords;
	GenderType gender;
	CreatureStatArray base_stats;
	NpcBP* parent;
	std::vector<std::string> equip_list;

	struct CombatData {
		uint dodge;
		uint attack;
		uint damage;
	} combat;

	struct SetFlags {
		int	name: 1,
			desc: 1,
			gender: 1,
			dodge: 1,
			attack: 1,
			damage: 1,
			stats: 1;

		inline SetFlags() : name(false), desc(false),
				gender(false), dodge(false), attack(false),
				damage(false), stats(false) {}
	} set_flags;

	void set_parent(NpcBP* blueprint);
};

class Npc : public Creature
{
public:
	Npc();
	Npc(NpcBP* s_blueprint);

	virtual const char* factory_type() const { return "npc"; }

	// blueprints
	virtual NpcBP* get_blueprint() const { return blueprint; }
	void set_blueprint(NpcBP* s_blueprint);
	static Npc* load_blueprint(const std::string& name);

	// name info
	virtual EntityName get_name() const;

	virtual bool name_match(const std::string& name) const;

	// description
	virtual std::string get_desc() const;

	// gender
	virtual GenderType get_gender() const;

	// stats
	virtual int get_base_stat(CreatureStatID stat) const;

	// save and load
	virtual int load_node(File::Reader& reader, File::Node& node);
	virtual int load_finish();
	virtual void save_data(File::Writer& writer);
	virtual void save_hook(File::Writer& writer);

	// display
	virtual const char* ncolor() const { return CNPC; }

	// return ture if we derive from the named blueprint
	bool is_blueprint(const std::string& blueprint) const;

	// combat
	virtual uint get_combat_dodge() const;
	virtual uint get_combat_attack() const;
	virtual uint get_combat_damage() const;

	// movement information
	inline bool is_zone_locked() const { return flags.zonelock; }
	inline void set_zone_locked(bool value) { flags.zonelock = value; }
	inline bool is_room_tag_reversed() const { return flags.revroomtag; }
	inline void set_room_tag_reversed(bool value) { flags.revroomtag = value; }
	inline TagID get_room_tag() const { return room_tag; }
	inline void set_room_tag(TagID s_room_tag) { room_tag = s_room_tag; }
	bool can_use_portal(class Portal* portal) const;

	// dead
	void kill(Creature* killer);

	// heartbeat
	void heartbeat();

	// handle events
	virtual void handle_event(const Event& event);

	// display desc
	virtual void display_desc(const class StreamControl& stream);

protected:
	~Npc();

	void initialize();

	// data
private:
	TagID room_tag; // tag needed in a room to enter it
	NpcBP* blueprint;

	struct NPCFlags {
int zonelock:
		1, // can't leave the zone they're in
revroomtag:
		1; // reverse meaning of room tag
	} flags;

protected:
	E_SUBTYPE(Npc, Creature);
};

class _MNpcBP : public IManager
{
	typedef std::map<std::string, NpcBP*> BlueprintMap;

public:
	int initialize();
	void shutdown();

	NpcBP* lookup(const std::string& id);

private:
	BlueprintMap blueprints;
};

extern _MNpcBP MNpcBP;

#define NPC(ent) E_CAST((ent),Npc)

#endif
