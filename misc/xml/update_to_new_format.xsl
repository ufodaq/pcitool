<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="banks">
     <xsl:apply-templates select="@*|node()"/>
  </xsl:template>
  <xsl:template match="views">
     <xsl:apply-templates select="@*|node()"/>
  </xsl:template>
  <xsl:template match="units">
     <xsl:apply-templates select="@*|node()"/>
  </xsl:template>

  <xsl:template match="bank">
    <bank>
      <xsl:for-each select="bank_description/*">
	<xsl:attribute name="{name()}">
    	    <xsl:value-of select="text()"/>
	</xsl:attribute>
      </xsl:for-each>

      <xsl:apply-templates select="registers/*"/>
    </bank>
  </xsl:template>

  <xsl:template match="bank_description">
  </xsl:template>

  <xsl:template match="register_bits">
    <field>
      <xsl:for-each select="*">
	<xsl:if test="name() = 'views'">
	     <xsl:apply-templates select="view"/>
	</xsl:if>
	<xsl:if test="name() != 'views'">
    	    <xsl:attribute name="{name()}">
    		<xsl:value-of select="text()"/>
    	    </xsl:attribute>
    	</xsl:if>
      </xsl:for-each>
    </field>
  </xsl:template>
  
  <xsl:template match="register">
    <register>
      <xsl:for-each select="*">
	<xsl:if test="name() = 'registers_bits'">
	     <xsl:apply-templates select="register_bits"/>
	</xsl:if>
	<xsl:if test="name() = 'views'">
	     <xsl:apply-templates select="view"/>
	</xsl:if>
	<xsl:if test="name() != 'registers_bits' and name() != 'views'">
    	    <xsl:attribute name="{name()}">
    		<xsl:value-of select="text()"/>
    	    </xsl:attribute>
    	</xsl:if>
      </xsl:for-each>
    </register>
  </xsl:template>

  <xsl:template match="enum">
    <name>
      <xsl:attribute name="name">
        <xsl:value-of select="text()"/>
      </xsl:attribute>
     <xsl:apply-templates select="@*"/>
    </name>
  </xsl:template>
  
  <xsl:template match="view[@type]">
    <xsl:if test="@type = 'formula'">
	<transform>
	  <xsl:for-each select="*">
    	    <xsl:attribute name="{name()}">
    		<xsl:value-of select="text()"/>
    	    </xsl:attribute>
    	 </xsl:for-each>
	</transform>
    </xsl:if>
    <xsl:if test="@type = 'enum'">
	<enum>
	  <xsl:for-each select="*">
	    <xsl:if test="name() != 'enum'">
    	        <xsl:attribute name="{name()}">
    		    <xsl:value-of select="text()"/>
    		</xsl:attribute>
    	    </xsl:if>
    	 </xsl:for-each>
	 <xsl:apply-templates select="enum"/>
	</enum>
    </xsl:if>
  </xsl:template>
  
  <xsl:template match="view">
    <view>
	<xsl:attribute name="name"><xsl:value-of select="text()"/></xsl:attribute>
    </view>
  </xsl:template>
 
  <xsl:template match="convert_unit">
    <transform>
	<xsl:attribute name="unit"><xsl:value-of select="@value"/></xsl:attribute>
	<xsl:attribute name="transform"><xsl:value-of select="text()"/></xsl:attribute>
    </transform>
  </xsl:template>
  
  <xsl:template match="unit">
    <unit>
     <xsl:apply-templates select="@*"/>
     <xsl:apply-templates select="convert_unit"/>
    </unit>
  </xsl:template>

  <xsl:template match="@*|node()">
   <xsl:copy>
     <xsl:apply-templates select="@*|node()"/>
   </xsl:copy>
 </xsl:template>
</xsl:stylesheet>
