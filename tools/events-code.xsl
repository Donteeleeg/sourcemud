<?xml version="1.0"?>
<xsl:stylesheet version="1.1" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text"/>
  <xsl:template match="/events">

<!-- header -->
<xsl:text><![CDATA[
// Generated by events-code.xsl
// DO NOT EDIT

#include "mud/eventids.h"
#include "mud/event.h"
#include "mud/room.h"
#include "mud/exit.h"
#include "mud/object.h"
#include "mud/zone.h"
#include "scriptix/function.h"

namespace Events {
]]></xsl:text>

<!-- event ids -->
<xsl:for-each select="event">
	<xsl:text>EventID ON_</xsl:text><xsl:value-of select="translate(name, 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/><xsl:text>;
</xsl:text>

	<xsl:text>void send_</xsl:text><xsl:value-of select="name" /><xsl:text> (Room* room, Entity* actor</xsl:text>
	<xsl:for-each select="arg">
		<xsl:text>, </xsl:text>

		<xsl:choose>
			<xsl:when test="type='String'">
				<xsl:text>StringArg </xsl:text>
			</xsl:when>
			<xsl:when test="type='Integer'">
				<xsl:text>long </xsl:text>
			</xsl:when>
			<xsl:when test="type='Boolean'">
				<xsl:text>bool </xsl:text>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="type" />
				<xsl:text>* </xsl:text>
			</xsl:otherwise>
		</xsl:choose>
		<xsl:value-of select="name" />
	</xsl:for-each>
	<xsl:text>) { EventManager.send(ON_</xsl:text><xsl:value-of select="translate(name, 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')" /><xsl:text>, room, actor</xsl:text>
	<xsl:for-each select="arg">
		<xsl:text>, </xsl:text>
		<xsl:value-of select="name" />
	</xsl:for-each>
	<xsl:text>); }
</xsl:text>
</xsl:for-each>

<!-- initializer -->
<xsl:text><![CDATA[
void SEventManager::initialize_ids (void) {
]]></xsl:text>
<xsl:for-each select="event">
	Events::ON_<xsl:value-of select="translate(name, 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/><xsl:text> = EventID::create("</xsl:text><xsl:value-of select="name"/><xsl:text>");
</xsl:text>
</xsl:for-each>

<!-- footer -->
<xsl:text><![CDATA[
} // initialize()

Scriptix::ScriptFunction
SEventManager::compile (EventID id, StringArg source, StringArg filename, unsigned long fileline)
{
]]></xsl:text>

<xsl:for-each select="event">
<xsl:text>	if(id == Events::ON_</xsl:text><xsl:value-of select="translate(name, 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')" /><xsl:text>)
		return Scriptix::ScriptFunction::compile("event </xsl:text><xsl:value-of select="name"/><xsl:text>", source, "self,event,actor,room</xsl:text>
		<xsl:for-each select="arg">
			<xsl:text>, </xsl:text>
			<xsl:value-of select="name" />
		</xsl:for-each>
		<xsl:text>", filename, fileline);
</xsl:text>
</xsl:for-each>

<xsl:text><![CDATA[
	return Scriptix::ScriptFunction::compile(String("event ") + EventID::nameof(id), source, "self,event,room,actor,data1,data2,data3,data4,data5", filename, fileline);
} // compile_event()

} // namespace Events
]]></xsl:text>

  </xsl:template>
</xsl:stylesheet>