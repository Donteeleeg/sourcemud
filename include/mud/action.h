/*
 * AweMUD NG - Next Generation AwesomePlay MUD
 * Copyright (C) 2000-2005  AwesomePlay Productions, Inc.
 * See the file COPYING for license details
 * http://www.awemud.net
 */

#ifndef ACTION_H
#define ACTION_H

#include "types.h"
#include "gcbase.h"

class Character;
class StreamControl;

// A pending action.  All actions should derive from this action.
// Must provide implementations of:
//  uint get_rounds() const
//      - return total rounds of action in seconds
//  void describe(StreamControl&) const
//      - describe the action
//  int start(void)
//      - initialize the action
//        return 1 to abort
//  void finish(void)
//      - finish the action
// May also provide implemenations of:
//  int update(uint round)
//      - perform parts of the action, return non-zero to abort
//  int cancel()
//      - called when the user cancels the actions
//        return 1 to abort the cancellation (ouch, head hurts)
class IAction : public GC
{
	public:
	inline IAction (Character* s_actor) : actor(s_actor) {}
	inline virtual ~IAction (void) {}

	// returns the character performing the action
	inline Character* get_actor (void) const { return actor; }

	// called when the user wishes to cancel; return 1 to abort the cancel
	inline virtual int cancel (void) { return 1; }

	// return the total number of rounds the action takes, in seconds
	virtual uint get_rounds (void) const = 0;

	// display a description of the action
	// should be sentence fragment in the current tense:
	// i.e.  "attacking the rabbit" or "reading the sign"
	virtual void describe (const StreamControl& stream) const = 0;

	// called to initialize action; return 1 to abort action
	virtual int start (void) = 0;

	// called at the last round, when the action is complete
	virtual void finish (void) = 0;

	// called once per round; return 1 to abort the action
	virtual int update (uint rounds) { return 0; }

	private:
	Character* actor;
};

// An instant pending action.
//  Just over-ride perform() with the action code.
class IInstantAction : public IAction
{
	public:
	inline IInstantAction (Character* s_actor) : IAction(s_actor) {}

	// always zero rounds
	inline virtual uint get_rounds (void) const { return 0; }

	// no description necessary
	virtual void describe (const StreamControl& stream) const {}

	// on start we just perform the action and "abort"
	inline virtual int start (void) { perform(); return 1; }

	// finish never gets called
	virtual void finish (void) {}

	// update never gets called
	virtual int update (uint rounds) { return 1; }

	// over-ride this to do actual work
	virtual void perform (void) = 0;
};

#endif