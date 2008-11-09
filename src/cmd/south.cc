/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */

#include "mud/creature.h"
#include "mud/room.h"
#include "mud/command.h"

/* BEGIN COMMAND
 *
 * name: south
 *
 * format: south (10)
 * format: s (10)
 *
 * END COMMAND */

void command_south (Creature* ch, String argv[]) {
	// must be in a room
	if (!ch->get_room()) {
		*ch << "You are not in a room.\n";
		return;
	}

	// get portal
	Portal* portal = ch->get_room()->get_portal_by_dir (PortalDir::SOUTH);
	if (!portal) {
		*ch << "You do not see an portal in that direction.\n";
		return;
	}

	// go
	ch->do_go(portal);
}