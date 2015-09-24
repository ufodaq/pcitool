<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:template match="register|field">
    <xsl:if test="local-name() = 'field'">
      <xsl:value-of select="preceding-sibling::text()"/>
    </xsl:if>

    <xsl:element name="{local-name()}">
      <xsl:apply-templates select="@*"/>
      <xsl:for-each select="view">
        <xsl:copy>
          <xsl:apply-templates select="@*"/>
        </xsl:copy>
      </xsl:for-each>
      <xsl:apply-templates select="field"/>
      <xsl:if test="field">
        <xsl:value-of select="text()[last()]"/>
      </xsl:if>
    </xsl:element>
  </xsl:template>

  <xsl:template match="//model/bank|//model/transform|//model/enum|//model/unit">
    <xsl:value-of select="preceding-sibling::text()"/>
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="model">
    <xsl:copy>
      <xsl:apply-templates select="bank"/>
      <xsl:apply-templates select="transform"/>
      <xsl:apply-templates select="enum"/>
      <xsl:apply-templates select="unit"/>
      <xsl:value-of select="text()[last()]"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
</xsl:stylesheet>
