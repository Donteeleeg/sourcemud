<?xml version="1.0"?>
<xsl:stylesheet version="1.1" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text"/>
  <xsl:template match="/events">

<!-- header -->
<xsl:text><![CDATA[
// Generated by events-header.xsl
// DO NOT EDIT

#include "mud/event.h"
#include "mud/room.h"
#include "mud/creature.h"
#include "mud/object.h"
#include "mud/zone.h"

std::string EventID::names[] = {
  S("None"),
]]></xsl:text>
<xsl:for-each select="event">
  <xsl:text>S("</xsl:text><xsl:value-of select="@name" /><xsl:text>"),</xsl:text>
</xsl:for-each>
<xsl:text><![CDATA[
};

EventID
EventID::lookup (std::string name) {
  for (size_t i = 0; i < COUNT; ++i)
    if (name == names[i])
      return EventID(i);
  return EventID();
}

namespace Events {
  // hack
  namespace {
    Entity* actor = NULL;
    Entity* target = NULL;
    Entity* aux = NULL;
  }
]]></xsl:text>

<!-- event ids -->
<xsl:for-each select="event">
  <xsl:text>void send</xsl:text><xsl:value-of select="@name" />
  <xsl:text>(Room* room</xsl:text>
  <xsl:if test="actor"><xsl:text>, </xsl:text><xsl:value-of select="actor/@type" />* actor</xsl:if>
  <xsl:if test="target"><xsl:text>, </xsl:text><xsl:value-of select="target/@type" />* target</xsl:if>
  <xsl:if test="aux"><xsl:text>, </xsl:text><xsl:value-of select="aux/@type" />* aux</xsl:if>
  <xsl:apply-templates select="arg" />
  <xsl:text>){/*EventManager.send(EventID::</xsl:text><xsl:value-of select="@name" /><xsl:text>,room,actor,target,aux</xsl:text>
  <xsl:for-each select="arg"><xsl:text>,arg_</xsl:text><xsl:value-of select="@name" /></xsl:for-each>
  <xsl:text>);*/}
</xsl:text>
</xsl:for-each>

<!-- footer -->
<xsl:text><![CDATA[
} // namespace Events
]]></xsl:text>

<!-- Compiler -->
<xsl:text>int SEventManager::compile (EventID id, std::string source, std::string filename, unsigned long fileline) {</xsl:text>
<xsl:for-each select="event">
  <!--
  <xsl:text>if(id == EventID::</xsl:text>
  <xsl:value-of select="@name" />
  <xsl:text>) return Scriptix::ScriptFunction::compile(S("event </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>"), source, S("self,event,room,actor,target,aux</xsl:text>
  <xsl:for-each select="arg">
    <xsl:text>, </xsl:text>
    <xsl:value-of select="@name" />
  </xsl:for-each>
  <xsl:text>"), filename, fileline);</xsl:text>
  -->
</xsl:for-each>

<!--<xsl:text>return Scriptix::ScriptFunction::compile(S("event ") + id.get_name(), source, S("self,event,room,actor,target,aux,data1,data2,data3,data4,data5"), filename, fileline);</xsl:text>-->
<xsl:text>return 0;}</xsl:text>

<xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template match="arg">
    <xsl:text>, </xsl:text>
    <xsl:choose>
      <xsl:when test="@type='String'">
        <xsl:text>const std::string&amp; </xsl:text>
      </xsl:when>
      <xsl:when test="@type='Integer'">
        <xsl:text>long </xsl:text>
      </xsl:when>
      <xsl:when test="@type='Boolean'">
        <xsl:text>bool </xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="@type" />
        <xsl:text>* </xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    <xsl:text>arg_</xsl:text><xsl:value-of select="@name" />
  </xsl:template>
</xsl:stylesheet>
<!-- vim: set expandtab tabstop=2 shiftwidth=2 : -->
