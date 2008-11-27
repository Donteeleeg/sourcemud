/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#ifndef PLAYER_H
#define PLAYER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <vector>
#include <map>
#include <algorithm>

#include "common/string.h"
#include "common/imanager.h"
#include "common/time.h"
#include "mud/creature.h"
#include "mud/color.h"
#include "mud/form.h"
#include "mud/gametime.h"
#include "mud/skill.h"
#include "mud/pconn.h"
#include "net/socket.h"

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif // HAVE_LIBZ

// player name length requirements
#define PLAYER_NAME_MIN_LEN 3
#define PLAYER_NAME_MAX_LEN 15

class Player : public Creature
{
	public:
	// create and initialize
	Player (class Account* s_account, std::string s_id);

	virtual std::string factory_type () const { return S("player"); }

	// player's unique ID (this is identical to their name)
	inline std::string get_id () const { return name.get_text(); }

	// name information (special for players)
	inline virtual EntityName get_name () const { return name; }

	// description information
	virtual inline std::string get_desc () const { return std::string(); }
	virtual inline void set_desc (std::string s_desc) {}

	// gender
	virtual inline GenderType get_gender () const { return form.gender; }
	inline void set_gender (GenderType s_gender) { form.gender = s_gender; }

	// save and load
	virtual void save_data (File::Writer& writer);
	virtual void save_hook (File::Writer& writer);
	void save ();

	int load_node (File::Reader& reader, File::Node& node);

	// output color
	inline std::string ncolor () const { return S(CPLAYER); }

	// account
	inline class Account* get_account () const { return account; }

	// connected?
	//   currently in use by someone
	inline bool is_connected () const { return conn != NULL; }

	// time
	time_t get_time_created () const { return time_created; }
	time_t get_time_lastlogin () const { return time_lastlogin; }
	uint32 get_total_playtime () const { return total_playtime; }

	// session management
	int start_session ();
	void end_session ();

	// birthday/age
	uint get_age () const;
	inline GameTime get_birthday () const { return birthday; }
	inline void set_birthday (const GameTime& gt) { birthday = gt; }

	// misc
	void kill (Creature* killer);
	void heartbeat ();

	// manage account active counts
	virtual void activate ();
	virtual void deactivate ();

	// race
	inline class Race* get_race () const { return race; }
	inline class Race* set_race (class Race* race_in) { return race = race_in; }

	// character traits
	FormColor get_eye_color () const { return form.eye_color; }
	FormColor get_hair_color () const { return form.hair_color; }
	FormColor get_skin_color () const { return form.skin_color; }
	FormHairStyle get_hair_style () const { return form.hair_style; }
	FormBuild get_build () const { return form.build; }
	FormHeight get_height () const { return form.height; }

	void set_eye_color (FormColor value) { form.eye_color = value; }
	void set_hair_color (FormColor value) { form.hair_color = value; }
	void set_skin_color (FormColor value) { form.skin_color = value; }
	void set_hair_style (FormHairStyle value) { form.hair_style = value; }
	void set_build (FormBuild value) { form.build = value; }
	void set_height (FormHeight value) { form.height = value; }

	// combat
	virtual uint get_combat_dodge () const;
	virtual uint get_combat_attack () const;
	virtual uint get_combat_damage () const;

	// class/exp
	inline uint get_exp () const { return experience; }
	void grant_exp (uint amount);

	// stats
	virtual inline int get_base_stat (CreatureStatID stat) const { return base_stats[stat.get_value()]; }
	inline void set_base_stat (CreatureStatID stat, int value) { base_stats[stat.get_value()] = value; }

	// recalcuate everything
	virtual void recalc_stats ();
	virtual void recalc ();

	// display info
	void display_inventory ();
	virtual void display_desc (const class StreamControl& stream) const;
	void display_skills ();

	// I/O
	virtual void stream_put (const char* data, size_t len = 0);
	void show_prompt ();
	void process_command (std::string cmd);
	void connect (IPlayerConnection* conn);
	void disconnect ();
	IPlayerConnection* get_conn() const { return conn; }
	void toggle_echo (bool value);
	void set_indent (uint level);
	uint get_width ();
	void clear_scr ();

	// parsing
	virtual int macro_property (const class StreamControl& stream, std::string method, const MacroList& argv) const;

	// player-only actions
	void do_tell (Player* who, std::string what);
	void do_reply (std::string what);

	protected:
	EntityName name;
	IPlayerConnection* conn;
	Account* account;
	std::string last_command;
	std::string last_tell;
	struct PDesc {
		GenderType gender;
		FormColor eye_color;
		FormColor hair_color;
		FormColor skin_color;
		FormHairStyle hair_style;
		FormBuild build;
		FormHeight height;
	} form;
	CreatureStatArray base_stats;
	class Race *race;
	uint32 experience;
	time_t time_created;
	time_t time_lastlogin;
	uint32 total_playtime;
	GameTime birthday;
	SkillSet skills;
	struct NetworkInfo {
		uint last_rt; // last reported round-time
		uint last_max_rt; // last reported max round-time
		int last_hp; // last reported hp 
		int last_max_hp; // last reported hp 
		uint timeout_ticks; // remaining ticks until timeout
	} ninfo;

#ifdef HAVE_LIBZ
	// compression
	bool begin_mccp ();
	void end_mccp ();
#endif // HAVE_LIBZ

	E_SUBTYPE(Player,Creature);

	friend class SPlayerManager; // obvious

	protected:
	~Player ();
};

// manage all players
class SPlayerManager : public IManager
{
	public:
	// list of *connected* players - do *not* use GC
	typedef std::list<Player*> PlayerList;

	// initialize the player manager
	virtual int initialize ();

	// shutdown the player manager
	virtual void shutdown ();

	// true if 'name' is a valid player name
	bool valid_name (std::string name);

	// return the path a player's file is at
	std::string path (std::string name);

	// return the logged-in player with the given name
	Player* get (std::string name);

	// load a player - from disk
	Player* load (class Account* account, std::string name);

	// DESTROY a player permanently (with backup)
	int destroy (std::string name);

	// does a valid player of this name exist?
	bool exists (std::string name);

	// count of connected players
	size_t count ();

	// list all connected players
	void list (const StreamControl& stream);

	// save all players
	void save ();

	// EEEEW - return list of all players - yuck
	inline const PlayerList& get_player_list () { return player_list; }

	private:
	PlayerList player_list;

	// yuck - let Player class manage their own membership
	friend class Player;
};
extern SPlayerManager PlayerManager;

#define PLAYER(ent) E_CAST((ent),Player)

extern std::string get_stat_level (uint value);
extern std::string get_stat_color (uint value);

#endif
