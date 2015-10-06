<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
  <xsl:output method="text" media-type="text/x-csrc" />
  <xsl:template match="doxygen">
    <xsl:apply-templates />
  </xsl:template>
  <xsl:template match="compounddef[@kind = 'file']">
    #include "assertions.h"
    <xsl:variable name="testbg" select="detaileddescription/para/xrefsect[xreftitle[string() = 'Test']]//para[starts-with(normalize-space(), 'Background:')]" />
    <xsl:apply-templates select="$testbg//programlisting|$testbg//computeroutput" />
    #include "<xsl:value-of select="//compounddef[@kind = 'file']/compoundname" />"
    <xsl:apply-templates select="innerclass" />
    <xsl:apply-templates select="sectiondef" />

    int main(void) {
       fputs("<xsl:value-of select="//compounddef[@kind = 'file']/compoundname" />:\n", stderr);
       <xsl:for-each select="//memberdef">
         <xsl:variable name="tests" select="detaileddescription/para/xrefsect[xreftitle[string() = 'Test']]" />
         <xsl:if test="$tests">
           test_<xsl:value-of select="position()" />();
         </xsl:if>
         <xsl:if test="not($tests) and @kind = 'function'">
           <xsl:message terminate="no">Warning: <xsl:value-of select="name"/>() has no tests</xsl:message>
         </xsl:if>
       </xsl:for-each>
       return 0;
    }
  </xsl:template>

  <xsl:template match="compounddef">
    <xsl:variable name="testbg" select="detaileddescription/para/xrefsect[xreftitle[string() = 'Test']]//para[starts-with(normalize-space(), 'Background:')]" />    
    <xsl:apply-templates select="$testbg//programlisting|$testbg//computeroutput" />
    <xsl:apply-templates select="innerclass" />    
    <xsl:apply-templates select="sectiondef" />
  </xsl:template>

  <xsl:template match="sectiondef">
    <xsl:apply-templates select="memberdef" />
  </xsl:template>

  <xsl:template match="innerclass">
    <xsl:apply-templates select="document(concat(@refid, '.xml'), /)" />    
  </xsl:template>

  <xsl:template match="memberdef">
    <xsl:variable name="tests" select="detaileddescription/para/xrefsect[xreftitle[string() = 'Test']]" />
    <xsl:if test="$tests">
      <xsl:variable name="testbg" select="$tests//para[starts-with(normalize-space(), 'Background:')]" />
      <xsl:apply-templates select="$testbg//programlisting|$testbg//computeroutput" />
      static void test_<xsl:number level="any" count="memberdef" />(void) {
      fputs("<xsl:value-of select="name"/>:\n", stderr);
      <xsl:apply-templates select="$tests/xrefdescription/para" />
      }
    </xsl:if>
  </xsl:template>
  
  <xsl:template match="para">
    <xsl:variable name="tag" select="normalize-space(text())" />
    <xsl:if test="not(starts-with($tag, 'Background:'))">
      <xsl:variable name="notice">
        <xsl:choose>
          <xsl:when test="starts-with($tag, 'Given ') or starts-with($tag, 'When ') or starts-with($tag, 'And when ') or starts-with($tag, 'Clean')"></xsl:when>
          <xsl:when test="starts-with($tag, 'Then ')"><xsl:value-of select="substring-after($tag, 'Then ')" /></xsl:when>
          <xsl:when test="starts-with($tag, 'And ')"><xsl:value-of select="substring-after($tag, 'And ')" /></xsl:when>
          <xsl:when test="starts-with($tag, 'But ')"><xsl:value-of select="substring-after($tag, 'But ')" /></xsl:when>                    
          <xsl:otherwise><xsl:value-of select="$tag" /></xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:if test="string($notice)">
        fputs("<xsl:for-each select="ancestor::listitem"><xsl:text>&#x20;&#x20;</xsl:text></xsl:for-each><xsl:value-of select="$notice" />...", stderr);
        fflush(stderr);
      </xsl:if>
      <xsl:for-each select="table">
        {
        <xsl:variable name="table" select="." />
        <xsl:variable name="indexvar">__index<xsl:value-of select="count(preceding::table)" /></xsl:variable>
        unsigned <xsl:value-of select="$indexvar" />;
        <xsl:for-each select="row[1]/entry[@thead = 'yes']/para">
          <xsl:variable name="column" select="position()" />
          <xsl:value-of select="string(computeroutput[1])" /> __<xsl:value-of select="string(computeroutput[2])" />__array[] = {
          <xsl:for-each select="$table/row[position() &gt; 1]/entry[position() = $column]/para">
            <xsl:value-of select="string(computeroutput)"/>,
          </xsl:for-each>
          };
        </xsl:for-each>
        
        for (<xsl:value-of select="$indexvar" /> = 0;
        <xsl:value-of select="$indexvar" /> &lt; <xsl:value-of select="count(row) - 1" />;
        <xsl:value-of select="$indexvar" />++)
        {
        <xsl:for-each select="row[1]/entry[@thead = 'yes']/para">
          <xsl:value-of select="string(computeroutput[1])" /> <xsl:text>&#32;</xsl:text><xsl:value-of select="string(computeroutput[2])" /> =
          __<xsl:value-of select="string(computeroutput[2])" />__array[<xsl:value-of select="$indexvar" />];
        </xsl:for-each>
        <xsl:if test="string($notice) and not($table/../itemizedlist) and not($table/../orderedlist)">fputc('.', stderr); fflush(stderr);
        </xsl:if>
      </xsl:for-each>
      <xsl:apply-templates select="programlisting|computeroutput" />
      <xsl:if test="orderedlist|itemizedlist">
        <xsl:if test="string($notice)">fputc('\n', stderr);</xsl:if>
        {
        <xsl:apply-templates select="(orderedlist|itemizedlist)/listitem/para" />
        }
        <xsl:if test="string($notice)">fputs("\n<xsl:for-each select="ancestor::listitem"><xsl:text>&#x20;&#x20;</xsl:text></xsl:for-each>", stderr);</xsl:if>
      </xsl:if>
      <xsl:for-each select="table">}}</xsl:for-each>
      <xsl:if test="string($notice)">
        fputs("ok\n", stderr);
      </xsl:if>
    </xsl:if>
  </xsl:template>

  <xsl:template match="programlisting">
    <xsl:apply-templates select="codeline" />
  </xsl:template>
  
  <xsl:template match="codeline|computeroutput">
    <xsl:apply-templates/><xsl:text>&#10;</xsl:text>
  </xsl:template>
  <xsl:template match="sp"><xsl:text>&#32;</xsl:text></xsl:template>
  <xsl:template match="highlight">
    <xsl:apply-templates />
  </xsl:template>
  
  <xsl:template match="*">
    <xsl:message terminate="yes">
      Where am I? <xsl:for-each select="ancestor-or-self::*"><xsl:value-of select="name()"/>:</xsl:for-each>
    </xsl:message>
  </xsl:template>
</xsl:stylesheet>
