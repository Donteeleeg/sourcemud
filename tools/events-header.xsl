<?xml version="1.0"?>
<xsl:stylesheet version="1.1" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:output method="text"/>
  <xsl:template match="/events">

<!-- header -->
<xsl:text><![CDATA[
// Generated by events-header.xsl
// DO NOT EDIT

#ifndef EVENT_IDS_H
#define EVENT_IDS_H

#include "mud/event.h"

class Room;
class Entity;
class Object;
class Portal;
class Zone;

namespace Events {
]]></xsl:text>

<!-- event ids -->
<xsl:for-each select="event">
  <xsl:text>extern EventID ON_</xsl:text><xsl:value-of select="translate(@name, 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/><xsl:text>;</xsl:text>

  <xsl:if test="type/@request='1'">
    <xsl:text>bool request</xsl:text><xsl:value-of select="@name" />
    <xsl:text>(Room* room</xsl:text>
    <xsl:if test="actor"><xsl:text>, </xsl:text><xsl:value-of select="actor/@type" />* actor</xsl:if>
    <xsl:if test="target"><xsl:text>, </xsl:text><xsl:value-of select="target/@type" />* target</xsl:if>
    <xsl:if test="aux"><xsl:text>, </xsl:text><xsl:value-of select="aux/@type" />* aux</xsl:if>
    <xsl:apply-templates select="arg" />
    <xsl:text>);</xsl:text>
  </xsl:if>

  <xsl:if test="type/@notify='1'">
    <xsl:text>void notify</xsl:text><xsl:value-of select="@name" />
    <xsl:text>(Room* room</xsl:text>
    <xsl:if test="actor"><xsl:text>, </xsl:text><xsl:value-of select="actor/@type" />* actor</xsl:if>
    <xsl:if test="target"><xsl:text>, </xsl:text><xsl:value-of select="target/@type" />* target</xsl:if>
    <xsl:if test="aux"><xsl:text>, </xsl:text><xsl:value-of select="aux/@type" />* aux</xsl:if>
    <xsl:apply-templates select="arg" />
    <xsl:text>);</xsl:text>
  </xsl:if>

  <xsl:if test="type/@command='1'">
    <xsl:text>bool do</xsl:text><xsl:value-of select="@name" />
    <xsl:text>(Room* room</xsl:text>
    <xsl:if test="actor"><xsl:text>, </xsl:text><xsl:value-of select="actor/@type" />* actor</xsl:if>
    <xsl:if test="target"><xsl:text>, </xsl:text><xsl:value-of select="target/@type" />* target</xsl:if>
    <xsl:if test="aux"><xsl:text>, </xsl:text><xsl:value-of select="aux/@type" />* aux</xsl:if>
    <xsl:apply-templates select="arg" />
    <xsl:text>);</xsl:text>
  </xsl:if>
</xsl:for-each>

<!-- footer -->
<xsl:text><![CDATA[
} // namespace Events

#endif // EVENT_IDS_H
]]></xsl:text>

  </xsl:template>

  <xsl:template match="arg">
    <xsl:text>, </xsl:text>
    <xsl:choose>
      <xsl:when test="@type='String'">
        <xsl:text>String </xsl:text>
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
    <xsl:value-of select="@name" />
  </xsl:template>
</xsl:stylesheet>
