<html>
<head>
<title><% write( getAlbaModel (), getAlbaIpAddress () ); %></title>
</head>
<body topmargin=0 bottommargin=0 leftmargin=0 rightmargin=0>

<APPLET name="Albatross" CODE="Albatross.class" ARCHIVE="alba.jar" WIDTH="560" HEIGHT="132" VIEWASTEXT id="Applet1" codebase=/JAVA>
<param name="ImagePath" value="/images/">
<param name="WebPhoneURL" value='javascript:;//'><!-- relative to /JAVA -->
<param name="Frame" value="_self">
<param name="RefreshRate" value="2000">
<param name="tcpPort" value="<% write( getQueryVal( "tcp", "7077" ) ); %>">
<param name="NrOfPorts" value="<% write( getAlbaPorts () ); %>">
<%
for ( i = 1; i <= getAlbaPorts (); i++ ) 
{
	write("<param name='Port" + (i-1) +"' value='" + i +"'>\n" );
}
%>
</APPLET>

</body>
</html>
