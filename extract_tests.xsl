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
    <xsl:apply-templates select="$testbg/programlisting|$testbg/computeroutput" />
    #include "<xsl:value-of select="//compounddef[@kind = 'file']/compoundname" />"
    <xsl:apply-templates select="innerclass" />
    <xsl:apply-templates select="sectiondef" />

    int main(void) {
       SET_RANDOM_SEED();
      
       BEGIN_TESTSUITE("<xsl:value-of select="//compounddef[@kind = 'file']/compoundname" />");
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
    <xsl:apply-templates select="$testbg/programlisting|$testbg/computeroutput" />
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
      <xsl:apply-templates select="$testbg/programlisting|$testbg/computeroutput" />
      static void test_<xsl:number level="any" count="memberdef" />(void) {
      BEGIN_TESTCASE("<xsl:value-of select="name"/>");
      <xsl:apply-templates select="$tests/xrefdescription/para" />
      }
    </xsl:if>
  </xsl:template>
  
  <xsl:template match="para">
    <xsl:variable name="tag" select="normalize-space(text())" />
    <xsl:if test="not(starts-with($tag, 'Background:'))">
      <xsl:variable name="step">
        <xsl:choose>
          <xsl:when test="starts-with($tag, 'Clean')">CLEANUP</xsl:when>
          <xsl:when test="starts-with($tag, 'Given ') or starts-with($tag, 'And given ')">PREREQ</xsl:when>
          <xsl:when test="starts-with($tag, 'When ') or starts-with($tag, 'And when ')">CONDITION</xsl:when>
          <xsl:when test="starts-with($tag, 'Then ') or starts-with($tag, 'And ') or starts-with($tag, 'But ') or starts-with($tag, 'Verify ')">ASSERTION</xsl:when>
          <xsl:otherwise>DESCRIPTION</xsl:otherwise>
          </xsl:choose>
      </xsl:variable>
      <xsl:variable name="background" select="parent::xrefdescription/../../preceding-sibling::para[xrefsect[xreftitle[string() = 'Test'] and
                                              xrefdescription/para[starts-with(normalize-space(), 'Background:')]]][1]/xrefsect/xrefdescription/para" />
      BEGIN_TESTSTEP_<xsl:value-of select="$step" />(<xsl:value-of select="count(ancestor::listitem)" />, "<xsl:value-of select="$tag" />");      
      <xsl:for-each select="table|$background/table">
        <xsl:choose>
          <xsl:when test="count(row) = 2">
            {
            <xsl:variable name="indexvar" select="string(row[2]/entry[1]/para/computeroutput[2])" />
            <xsl:value-of select="string(row[2]/entry[1]/para/computeroutput[1])" /><xsl:text>&#32;</xsl:text><xsl:value-of select="$indexvar" />;

            for (<xsl:value-of select="$indexvar" /> = <xsl:value-of select="string(row[2]/entry[2]/para/computeroutput)" />;
            <xsl:value-of select="$indexvar" /> &lt;=  <xsl:value-of select="string(row[2]/entry[3]/para/computeroutput)" />;
            <xsl:value-of select="$indexvar" /> += <xsl:choose><xsl:when test="row[2]/entry[4]"><xsl:value-of select="string(row[2]/entry[4]/para/computeroutput)" /></xsl:when><xsl:otherwise>1</xsl:otherwise></xsl:choose>)
            {
            <xsl:if test="row[2]/entry[1]/para/computeroutput[3]">
              BEGIN_TESTITER_<xsl:value-of select="$step"/>(<xsl:value-of select="count(ancestor::listitem)"/>,
              <xsl:value-of select="position() - 1" />,
              "For <xsl:value-of select="$indexvar" />=%<xsl:value-of select="string(row[2]/entry[1]/para/computeroutput[3])" />",
              <xsl:value-of select="$indexvar" />);
            </xsl:if>
          </xsl:when>
          <xsl:otherwise>
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
              <xsl:value-of select="string(computeroutput[1])" /><xsl:text>&#32;</xsl:text><xsl:value-of select="string(computeroutput[2])" /> =
              __<xsl:value-of select="string(computeroutput[2])" />__array[<xsl:value-of select="$indexvar" />];
            </xsl:for-each>
            BEGIN_TESTITER_<xsl:value-of select="$step"/>(<xsl:value-of select="count(ancestor::listitem)"/>,
            <xsl:value-of select="position() - 1" />,
            <xsl:choose>
              <xsl:when test="row[1]/entry[@thead = 'yes']/para[computeroutput[3]]">
                "For "              
                <xsl:for-each select="row[1]/entry[@thead = 'yes']/para[computeroutput[3]]">
                  "<xsl:if test="position() > 1"><xsl:text>, </xsl:text></xsl:if><xsl:value-of select="string(computeroutput[2])" />=%<xsl:value-of select="string(computeroutput[3])" />"
                </xsl:for-each>
                <xsl:for-each select="row[1]/entry[@thead = 'yes']/para[computeroutput[3]]">
                  , <xsl:value-of select="string(computeroutput[2])" />
                </xsl:for-each>
              </xsl:when>
              <xsl:otherwise>
                "[%u]", <xsl:value-of select="$indexvar" />
              </xsl:otherwise>
            </xsl:choose>
            );
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
      <xsl:apply-templates select="programlisting|computeroutput" />
      <xsl:variable name="subtests" select="orderedlist|itemizedlist|$background/orderedlist|$background/itemizedlist" />
      <xsl:if test="$subtests">
        {
        BEGIN_TESTSUBSTEP_<xsl:value-of select="$step"/>();
        {
        <xsl:apply-templates select="$subtests/listitem/para[not(starts-with(normalize-space(), 'Clean'))]" />
        <xsl:apply-templates select="$subtests/listitem/para[starts-with(normalize-space(), 'Clean')]" />        
        }
        }
      </xsl:if>
      <xsl:for-each select="table|$background/table">}}</xsl:for-each>
      <xsl:choose>
        <xsl:when test="$subtests">END_TESTSTEP_<xsl:value-of select="$step" />(<xsl:value-of select="count(ancestor::listitem)"/>);</xsl:when>
        <xsl:otherwise>END_TESTSTEP_<xsl:value-of select="$step" />(0);</xsl:otherwise>
      </xsl:choose>
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
