/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#include "command.h"
#include "player.h"
#include "help.h"

namespace {
	template <typename TYPE>
	void
	add_format (Command* command, const char* format, void(*func)(TYPE*, char**), int priority)
	{
		CommandFormat* cformat = new CommandFormat(command, priority);
		cformat->set_callback(func);
		cformat->build (format);
		command->add_format (cformat);
	}
}

#define COMMAND(name,usage,func,access,klass) \
	{ \
		extern void command_ ## func (klass*, char**); \
		void (*fptr)(klass*, char**) = command_ ## func; \
		Command* command = new Command(name,usage,access);
#define FORMAT(priority, format) add_format(command, (format), fptr, (priority));
#define END_COMM add(command); }
		
int
SCommandManager::initialize (void)
{
	// access IDs
	AccessID ACCESS_ALL;
	AccessID ACCESS_GM = AccessID::create("gm");
	AccessID ACCESS_BUILDER = AccessID::create("builder");
	AccessID ACCESS_ADMIN = AccessID::create("admin");

	// movement commands
	COMMAND("north",
			"north\n",
			north,
			ACCESS_ALL,
			Character)
		FORMAT(10, "north")
	END_COMM
	COMMAND("south",
			"south\n",
			south,
			ACCESS_ALL,
			Character)
		FORMAT(10, "south")
	END_COMM
	COMMAND("east",
			"east\n",
			east,
			ACCESS_ALL,
			Character)
		FORMAT(10, "east")
	END_COMM
	COMMAND("west",
			"west\n",
			west,
			ACCESS_ALL,
			Character)
		FORMAT(10, "west")
	END_COMM
	
	COMMAND("northeast",
			"ne\n"
			"northeast\n",
			northeast,
			ACCESS_ALL,
			Character)
		FORMAT(12, "ne")
		FORMAT(12, "northeast")
	END_COMM
	COMMAND("southeast",
			"se\n"
			"southeast\n",
			southeast,
			ACCESS_ALL,
			Character)
		FORMAT(12, "se")
		FORMAT(12, "southeast")
	END_COMM
	COMMAND("southwest",
			"sw\n"
			"southwest\n",
			southwest,
			ACCESS_ALL,
			Character)
		FORMAT(12, "sw")
		FORMAT(12, "southwest")
	END_COMM
	COMMAND("northwest",
			"nw\n"
			"northwest\n",
			northwest,
			ACCESS_ALL,
			Character)
		FORMAT(12, "nw")
		FORMAT(12, "northwest")
	END_COMM

	COMMAND("go",
			"go <exit>\n",
			go,
			ACCESS_ALL,
			Character)
		FORMAT(10, "go :0*")
	END_COMM
	COMMAND("climb",
			"climb <exit>\n",
			climb,
			ACCESS_ALL,
			Character)
		FORMAT(10, "climb :0*")
	END_COMM
	COMMAND("crawl",
			"crawl <exit>\n",
			crawl,
			ACCESS_ALL,
			Character)
		FORMAT(10, "crawl :0*")
	END_COMM

	// looking
	COMMAND("look",
			"look\n"
			"look [at|in|on|under|behind] <object|person>\n",
			look,
			ACCESS_ALL,
			Character)
		FORMAT(15, "look")
		FORMAT(15, "look :0(in,on,under,behind,at)? :1*")
	END_COMM

	// communication
	COMMAND("say",
			"say <text>\n",
			say,
			ACCESS_ALL,
			Character)
		FORMAT(50, "say :0*")
	END_COMM
	COMMAND("sing",
			"sing <line> [; <line2> ... ; <line-n>]\n"
			"  ex: sing once upon a time; in a land far away; there lived three grand knights; and dragons to slay!\n",
			sing,
			ACCESS_ALL,
			Character)
		FORMAT(50, "sing :0*")
	END_COMM
	COMMAND("emote",
			"emote <action>\n",
			emote,
			ACCESS_ALL,
			Character)
		FORMAT(50, "emote :0*")
	END_COMM

	// position
	COMMAND("stand",
			"stand\n",
			stand,
			ACCESS_ALL,
			Character)
		FORMAT(50, "stand")
	END_COMM
	COMMAND("sit",
			"sit\n",
			sit,
			ACCESS_ALL,
			Character)
		FORMAT(50, "sit")
	END_COMM
	COMMAND("lay",
			"lay\n",
			lay,
			ACCESS_ALL,
			Character)
		FORMAT(50, "lay")
	END_COMM
	COMMAND("kneel",
			"kneel\n",
			kneel,
			ACCESS_ALL,
			Character)
		FORMAT(50, "kneel")
	END_COMM

	// interact
	COMMAND("open",
			"open <door>\n",
			open,
			ACCESS_ALL,
			Character)
		FORMAT(50, "open :0*")
	END_COMM
	COMMAND("close",
			"close <door>\n",
			close,
			ACCESS_ALL,
			Character)
		FORMAT(50, "close :0*")
	END_COMM
	COMMAND("lock",
			"lock <door>\n",
			lock,
			ACCESS_ALL,
			Character)
		FORMAT(50, "lock :0*")
	END_COMM
	COMMAND("unlock",
			"unlock <door>\n",
			unlock,
			ACCESS_ALL,
			Character)
		FORMAT(50, "unlock :0*")
	END_COMM
	COMMAND("get",
			"get <object> [from <location>]\n"
			"get <amount> coins\n",
			get,
			ACCESS_ALL,
			Character)
		FORMAT(50, "get :0* from :1(in,on,under,behind)? :2*")
		FORMAT(50, "get :3%? coins")
		FORMAT(50, "get :0*")
	END_COMM
	COMMAND("put",
			"put <object> [in|on|under|behind] <location>\n",
			put,
			ACCESS_ALL,
			Character)
		FORMAT(50, "put :0* :1(in,on,under,behind) :2*")
	END_COMM
	COMMAND("give",
			"give <amount> coins [to] <player>\n",
			give,
			ACCESS_ALL,
			Character)
		FORMAT(50, "give :0% coins to? :1*")
	END_COMM
	COMMAND("drop",
			"drop <object>\n"
			"drop <amount> coins\n",
			drop,
			ACCESS_ALL,
			Character)
		FORMAT(50, "drop :1% coins")
		FORMAT(50, "drop :0*")
	END_COMM
	COMMAND("remove",
			"remove <object>\n",
			remove,
			ACCESS_ALL,
			Character)
		FORMAT(50, "remove :0*")
	END_COMM
	COMMAND("wear",
			"wear <object>\n",
			wear,
			ACCESS_ALL,
			Character)
		FORMAT(50, "wear :0*")
	END_COMM
	COMMAND("equip",
			"equip <object>\n",
			equip,
			ACCESS_ALL,
			Character)
		FORMAT(50, "equip :0*")
	END_COMM
	COMMAND("raise",
			"raise <object>\n",
			raise,
			ACCESS_ALL,
			Character)
		FORMAT(50, "raise :0*")
	END_COMM
	COMMAND("eat",
			"eat <object>\n",
			eat,
			ACCESS_ALL,
			Character)
		FORMAT(50, "eat :0*")
	END_COMM
	COMMAND("drink",
			"drink <object>\n",
			drink,
			ACCESS_ALL,
			Character)
		FORMAT(50, "drink :0*")
	END_COMM
	COMMAND("touch",
			"touch <object>\n",
			touch,
			ACCESS_ALL,
			Character)
		FORMAT(50, "touch :0*")
	END_COMM
	COMMAND("read",
			"read <object>\n",
			read,
			ACCESS_ALL,
			Character)
		FORMAT(50, "read :0*")
	END_COMM
	COMMAND("kick",
			"kick <object>\n"
			"kick <door>\n",
			kick,
			ACCESS_ALL,
			Character)
		FORMAT(50, "kick :0*")
	END_COMM
	COMMAND("swap",
			"swap\n",
			swap,
			ACCESS_ALL,
			Character)
		FORMAT(50, "swap")
	END_COMM
	COMMAND("attack",
			"attack <target>\n",
			attack,
			ACCESS_ALL,
			Character)
		FORMAT(50, ":0attack :1*")
	END_COMM
	COMMAND("kill",
			"kill <target>\n",
			attack,
			ACCESS_ALL,
			Character)
		FORMAT(50, ":0kill :1*")
	END_COMM

	// player commands
	COMMAND("tell",
			"tell <player> <message>\n",
			tell,
			ACCESS_ALL,
			Player)
		FORMAT(50, "tell :0% :1*")
	END_COMM
	COMMAND("reply",
			"reply <message>\n",
			reply,
			ACCESS_ALL,
			Player)
		FORMAT(50, "reply :0*")
	END_COMM
	COMMAND("inventory",
			"inventory\n",
			inventory,
			ACCESS_ALL,
			Player)
		FORMAT(50, "inventory")
	END_COMM
	COMMAND("skills",
			"skills\n",
			skills,
			ACCESS_ALL,
			Player)
		FORMAT(50, "skills")
	END_COMM
	COMMAND("affects",
			"affects\n",
			affects,
			ACCESS_ALL,
			Character)
		FORMAT(50, "affects")
	END_COMM
	COMMAND("prompt",
			"prompt reset\n"
			"prompt <prompt>\n",
			prompt,
			ACCESS_ALL,
			Player)
		FORMAT(50, "prompt reset")
		FORMAT(50, "prompt :0*")
	END_COMM
	COMMAND("set color",
			"set color <type> <color>\n",
			setcolor,
			ACCESS_ALL,
			Player)
		FORMAT(50, "set color :0% :1%")
	END_COMM
	COMMAND("stop",
			"stop\n",
			stop,
			ACCESS_ALL,
			Character)
		FORMAT(50, "stop");
	END_COMM

	// server/info commands
	COMMAND("server",
			"server\n",
			server,
			ACCESS_ALL,
			Player)
		FORMAT(50, "server")
	END_COMM
	COMMAND("commands",
			"commands\n",
			commands,
			ACCESS_ALL,
			Player)
		FORMAT(50, "commands")
	END_COMM
	COMMAND("time",
			"time\n"
			"date\n",
			time,
			ACCESS_ALL,
			Player)
		FORMAT(50, "time")
		FORMAT(50, "date")
	END_COMM
	COMMAND("who",
			"who\n",
			who,
			ACCESS_ALL,
			Player)
		FORMAT(50, "who")
	END_COMM
	COMMAND("help",
			"help <topic>\n",
			help,
			ACCESS_ALL,
			Player)
		FORMAT(50, "help :0?*")
	END_COMM
	COMMAND("man",
			"man <command>\n",
			man,
			ACCESS_ALL,
			Player)
		FORMAT(50, "man :0?*")
	END_COMM
	COMMAND("abuse",
			"abuse <player> <problem>\n",
			abuse,
			ACCESS_ALL,
			Player)
		FORMAT(50, "abuse :0% :1*")
	END_COMM
	COMMAND("bug",
			"bug <problem>\n",
			bug,
			ACCESS_ALL,
			Player)
		FORMAT(50, "bug :0*")
	END_COMM

	// builder commands
	COMMAND("create",
			"create [npc|object] [<template>]\n"
			"create room <name> [<zone>]\n"
			"create zone <name>\n"
			"crate exit [<name/dir> [to <target>]]\n",
			create,
			ACCESS_BUILDER,
			Player)
		FORMAT(80, "create :0npc :1%?")
		FORMAT(80, "create :0object :1%?")
		FORMAT(80, "create :0room :1% :2%?")
		FORMAT(80, "create :0zone :1%")
		FORMAT(80, "create :0exit :1%?")
		FORMAT(80, "create :0exit :1% to :2%")
	END_COMM
	COMMAND("edit",
			"edit [<type>] <entity\n",
			edit,
			ACCESS_BUILDER,
			Player)
		FORMAT(80, "edit :0*")
	END_COMM
	COMMAND("destroy",
			"destroy [<type>] <entity>\n",
			destroy,
			ACCESS_BUILDER,
			Player)
		FORMAT(80, "destroy :0*")
	END_COMM
	COMMAND("exitlist",
			"exitlist [<room>]\n",
			exitlist,
			ACCESS_BUILDER,
			Player)
		FORMAT(80, "exitlist :0%?")
	END_COMM
	COMMAND("bvision",
			"bvision [on|off]\n",
			bvision,
			ACCESS_BUILDER,
			Player)
		FORMAT(80, "bvision :0%?")
	END_COMM

	// GM commands
	COMMAND("gm teleport room",
			"gm teleport room <room>\n",
			gm_teleport_room,
			ACCESS_GM,
			Player)
		FORMAT(80, "gm teleport room :0%")
	END_COMM
	COMMAND("gm teleport player",
			"gm teleport player <player>\n",
			gm_teleport_player,
			ACCESS_GM,
			Player)
		FORMAT(80, "gm teleport player :0%")
	END_COMM
	COMMAND("gm summon",
			"gm summon <player>\n",
			gm_summon_player,
			ACCESS_GM,
			Player)
		FORMAT(80, "gm summon :0%")
	END_COMM
	COMMAND("gm announce",
			"gm announce <text>\n",
			gm_announce,
			ACCESS_GM,
			Player)
		FORMAT(80, "gm announce :0*")
	END_COMM
	COMMAND("gm boot",
			"gm boot <player>\n",
			gm_boot,
			ACCESS_GM,
			Player)
		FORMAT(80, "gm boot :0%")
	END_COMM

	// admin commands
	COMMAND("admin shutdown",
			"admin shutdown\n",
			admin_shutdown,
			ACCESS_ADMIN,
			Player)
		FORMAT(80, "admin shutdown")
	END_COMM
	COMMAND("admin grant",
			"admin grant <player> <level>\n",
			admin_grant,
			ACCESS_ADMIN,
			Player)
		FORMAT(80, "admin grant :0% :1%")
	END_COMM
	COMMAND("admin revoke",
			"admin revoke <player> <level>\n",
			admin_revoke,
			ACCESS_ADMIN,
			Player)
		FORMAT(80, "admin revoke :0% :1%")
	END_COMM
	COMMAND("admin blockip",
			"admin blockip <address mask>\n",
			admin_blockip,
			ACCESS_ADMIN,
			Player)
		FORMAT(80, "admin blockip :0%")
	END_COMM
	COMMAND("admin chpass",
			"admin chpass <player>\n",
			admin_chpass,
			ACCESS_ADMIN,
			Player)
		FORMAT(80, "admin chpass :0%")
	END_COMM

	return 0;
}

void
SCommandManager::shutdown (void)
{
	commands.resize(0);
}