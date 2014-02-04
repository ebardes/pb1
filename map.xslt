<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:ma="http://schemas.malighting.de/grandma2/xml/MA" xmlns="http://schemas.malighting.de/grandma2/xml/MA">
  <xsl:output 
    mode="xml"
    encoding="utf-8"
    indent="no" 
  />

  <xsl:template match="/">
    <MA xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns="http://schemas.malighting.de/grandma2/xml/MA" xsi:schemaLocation="http://schemas.malighting.de/grandma2/xml/MA http://schemas.malighting.de/grandma2/xml/2.9.0/MA.xsd" major_vers="2" minor_vers="9" stream_vers="0">
      <Info datetime="2014-01-05T01:56:31" showfile="cmtpeterpan" />
      <DMXRemotes index="2">
	<xsl:apply-templates />
      </DMXRemotes>
    </MA>
  </xsl:template>

  <xsl:template match="ma:key">
      <RemoteDMX index="{@index}" addr="10.{@assign}" type="hardkey" tasten_code="{@scan}" />
  </xsl:template>
</xsl:stylesheet>
