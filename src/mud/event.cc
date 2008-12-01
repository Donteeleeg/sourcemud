/*
 * Source MUD
 * Copyright (C) 2000-2005  Sean Middleditch
 * See the file COPYING for license details
 * http://www.sourcemud.org
 */


#include <stdlib.h>
#include <stdio.h>

#include "mud/entity.h"
#include "common/string.h"
#include "mud/server.h"
#include "mud/room.h"

_MEvent MEvent;

std::string EventID::names[] = {
	S("None"),
	S("Look"),
	S("LeaveRoom"),
	S("EnterRoom"),
	S("LeaveZone"),
	S("EnterZone"),
	S("TouchItem"),
	S("GraspItem"),
	S("ReleaseItem"),
	S("GetItem"),
	S("PutItem"),
	S("DropItem"),
	S("PickupItem"),
};

EventID EventID::lookup(const std::string& name) {
  for (size_t i = 0; i < COUNT; ++i)
    if (name == names[i])
      return EventID(i);
  return EventID();
}

EventHandler::EventHandler () : event(), script() {}

int
EventHandler::load (File::Reader& reader) {
	FO_READ_BEGIN
		FO_ATTR("event", "id")
			event = EventID::lookup(node.get_string());
			if (!event.valid()) 
				Log::Warning << node << ": Unknown event '" << node.get_string() << "'";
		FO_ATTR("event", "script")
			script = node.get_string();
	FO_READ_ERROR
		return -1;
	FO_READ_END

	return 0;
}

void
EventHandler::save (File::Writer& writer) const {
	writer.attr(S("event"), S("id"), event.get_name());
	if (!script.empty())
		writer.block(S("event"), S("script"), script);
}

void
Entity::handle_event (const Event& event)
{
	for (EventList::const_iterator i = events.begin (); i != events.end (); ++ i) {
		if (event.get_id() == (*i)->get_event()) {
			//(*i)->get_func().run(len, argv);
		}
	}
}

int
_MEvent::initialize ()
{
	return 0;
}

void
_MEvent::shutdown ()
{
	events.clear();
}

void
_MEvent::send (EventID id, Entity* recipient, Entity* aux1, Entity* aux2, Entity* aux3)
{
	assert (id.valid());
	assert (recipient != NULL);

	// create event
	Event event;
	event.id = id;
	event.recipient = recipient;
	event.aux1 = aux1;
	event.aux2 = aux2;
	event.aux3 = aux3;

	// just queue it, doesn't need to happen now
	events.push_back(event);
}

void
_MEvent::broadcast (EventID id, Entity* recipient, Entity* aux1, Entity* aux2, Entity* aux3)
{
	assert (id.valid());
	assert (recipient != NULL);

	// create event
	Event event;
	event.id = id;
	event.recipient = recipient;
	event.aux1 = aux1;
	event.aux2 = aux2;
	event.aux3 = aux3;

	// ask entity to broadcast it
	recipient->broadcast_event(event);
}

void
_MEvent::resend (const Event& event, Entity* recipient)
{
	Event new_event = event;
	new_event.recipient = recipient;
	events.push_back(new_event);
}

void
_MEvent::process ()
{
	// remember how many events we started with
	// this way, events which trigger more events
	// won't cause an infinite loop, because we'll
	// only process the initial batch
	size_t processing = events.size();

	// as long as we have events...
	while (processing > 0 && !events.empty()) {
		// get event
		Event event = events.front();
		events.pop_front();
		--processing;

		// DEBUG:
		/*
		Log::Info << "Event[" << event.get_name() <<
			"] P:" << event.get_id().name() <<
			" R:" << (event.get_room() ? event.get_room()->get_id().c_str() : "n/a") <<
			" A:" << (event.get_actor() ? event.get_actor()->get_name().get_name().c_str() : "n/a") <<
			" D:" << (event.get_data(0) ? event.get_data(0).get_type()->get_name().name().c_str() : "n/a") <<
			" D:" << (event.get_data(1) ? event.get_data(1).get_type()->get_name().name().c_str() : "n/a") <<
			" D:" << (event.get_data(2) ? event.get_data(2).get_type()->get_name().name().c_str() : "n/a") <<
			" D:" << (event.get_data(3) ? event.get_data(3).get_type()->get_name().name().c_str() : "n/a") <<
			" D:" << (event.get_data(4) ? event.get_data(4).get_type()->get_name().name().c_str() : "n/a");
		*/

		// send event
		event.get_recipient()->handle_event(event);
	}
}

int _MEvent::compile(EventID id, const std::string& source, const std::string& filename, unsigned long fileline)
{
	return 0;
}

namespace Events {
	void sendLook(Room* room, Creature* actor, Entity* target)
	{
		/*EventManager.send(EventID::Look,room,actor,target,aux);*/
	}

	void sendLeaveRoom(Room* room, Creature* actor, Portal* aux, Room* arg_dest)
	{
		/*EventManager.send(EventID::LeaveRoom,room,actor,target,aux,arg_dest);*/
	}

	void sendEnterRoom(Room* room, Creature* actor, Portal* aux, Room* arg_from)
	{
		/*EventManager.send(EventID::EnterRoom,room,actor,target,aux,arg_from);*/
	}

	void sendLeaveZone(Room* room, Creature* actor, Zone* arg_dest)
	{
		/*EventManager.send(EventID::LeaveZone,room,actor,target,aux,arg_dest);*/
	}

	void sendEnterZone(Room* room, Creature* actor, Zone* arg_from)
	{
		/*EventManager.send(EventID::EnterZone,room,actor,target,aux,arg_from);*/
	}

	void sendTouchItem(Room* room, Creature* actor, Object* target)
	{
		/*EventManager.send(EventID::TouchItem,room,actor,target,aux);*/
	}

	void sendGraspItem(Room* room, Creature* actor, Object* target)
	{
		/*EventManager.send(EventID::GraspItem,room,actor,target,aux);*/
	}

	void sendReleaseItem(Room* room, Creature* actor, Object* target)
	{
		/*EventManager.send(EventID::ReleaseItem,room,actor,target,aux);*/
	}

	void sendGetItem(Room* room, Creature* actor, Object* target, Object* aux, const std::string& arg_)
	{
		/*EventManager.send(EventID::GetItem,room,actor,target,aux,arg_);*/
	}

	void sendPutItem(Room* room, Creature* actor, Object* target, Object* aux, const std::string& arg_)
	{
		/*EventManager.send(EventID::PutItem,room,actor,target,aux,arg_);*/
	}

	void sendDropItem(Room* room, Creature* actor, Object* target)
	{
		/*EventManager.send(EventID::DropItem,room,actor,target,aux);*/
	}

	void sendPickupItem(Room* room, Creature* actor, Object* target)
	{
		/*EventManager.send(EventID::PickupItem,room,actor,target,aux);*/
	}

} // namespace Events
