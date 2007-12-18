<html>
<head>
<title>Albatross</title>
<link rel="stylesheet" type="text/css" href="/styles.css">
<script src="/doc/help.js"></script>
</head>
<SCRIPT LANGUAGE=javascript>
<!--
<%
	uptime = getUpTime();
	if ( uptime == "" )
		write("uptime = 0;");
	else
		write("uptime = " +uptime +";");

    tcpport = getAlbaD4oIPPort ();
    write( tcpport );
%>

function clockCounter() 
{
    if ( uptime <= 0 )
		return;

	uptime++;
	tmp = uptime;
	sec = tmp % 60;
	tmp = ( tmp - sec ) / 60;
	min = tmp % 60;
	tmp = ( tmp - min ) / 60;
	hour = tmp % 24;
	tmp = ( tmp - hour ) / 24;
		
	if( min <= 9 ) 
		min = "0" + min;
	if( sec <= 9 ) 
		sec = "0" + sec;

    var days = ( tmp != 1 ) ? " days, " : " day, ";
	var content = "<b>" + tmp.toString() + days + hour.toString() +":" +min.toString() +":" +sec.toString() + "</b>";

	if(document.getElementById) 
    {
		document.getElementById("clockDiv").innerHTML=content;		
	}
	else if (document.layers) 
	{
		docnn = document.clockDiv;
		docnn.document.write(content); 
		docnn.document.close();
	}
	else if (document.all) 
	{
		document.all("clockDiv").innerHTML = content;
	}
				
 	again = setTimeout( 'clockCounter()', 1000 );	
}

function detach_status()
{
        var features = 'width=560,height=132,toolbar=no,status=no,scrollbars=no,resizable=no,menubar=no';
        window.open('/secure/status.asp?tcp=<% write( tcpport ); %>','_blank', features );
}
//-->
</SCRIPT>

<body topmargin=4 leftmargin=8 rightmargin=4 onload="clockCounter();">

<%
    showTabBeg( "Status" );
	showTab( "*", "Status1.asp", "Device & Port",  "Device and Port Status", "status.htm" );
	showTab( "*", "Status2.asp", "Connections", "", "" );
	showTabEnd( 2 );

	parseRunningNetworkConfig ();
%>

<center>

<table cellpadding="0" cellspacing="0" border="0" width="516">
    <tr><td colspan=3 height=5></td></tr>
	<tr>
		<td><img src="/images/corner_java_tl.gif" width=8 height=8></td>
		<td><img src="/images/border_java_t.gif" height=8 width=560></td>
		<td><img src="/images/corner_java_tr.gif" width=8 height=8></td>
	</tr>
	<tr>
		<td valign=top><img src="/images/border_java_l.gif" height=132 width=8></td>
		<td valign=bottom nowrap=true>
			<APPLET name="Albatross" CODE="Albatross.class" ARCHIVE="alba.jar" WIDTH="560" HEIGHT="132" VIEWASTEXT id="Applet1" codebase=/JAVA>
			<param name="ImagePath" value="/images/">
			<param name="WebPhoneURL" value='javascript:;//'><!-- relative to /JAVA -->
			<param name="Frame" value="_self">
			<param name="RefreshRate" value="2000">
			<param name="tcpPort" value="<% write( tcpport ); %>">
			<param name="NrOfPorts" value="<% write( getAlbaPorts () ); %>">
			<%
			for ( i = 1; i <= getAlbaPorts (); i++ ) 
                        {
				write("<param name='Port" + (i-1) +"' value='" + i +"'>\n" );
			}
			%>
			</APPLET></td>
		<td valign=top><img src="/images/border_java_r.gif" height=132 width=8></td>
	</tr>
	<tr>
		<td valign=top><img src="/images/corner_java_bl.gif" width=8 height=8></td>
		<td valign=top><img src="/images/border_java_b.gif" height=8 width=560></td>
		<td valign=top><img src="/images/corner_java_br.gif" width=8 height=8></td>
	</tr>
</table>
</center>
<br>
<table cellpadding=0 cellspacing=0 border=0>
<tr class=normaltext><td>Uptime:</td><td>&nbsp;&nbsp;</td><td id="clockDiv">&nbsp;</td></tr>
<tr class=normaltext><td>Hostname:</td><td>&nbsp;&nbsp;</td><td><b><% write( runHOSTNAME ); %>.<% write( runDOMAINNAME ); %></b></td></tr>
<tr class=normaltext><td>IP Address:</td><td>&nbsp;&nbsp;</td><td><b><% write( runIPADDR ); %></b></td></tr>
<tr class=normaltext><td>MAC Address:</td><td>&nbsp;&nbsp;</td><td><b><% write( runHWADDR ); %></b></td></tr>
</table>
<%
    sys_id = getCmdOut( "fw_env /dev/mtd1 /dev/mtd2 ?invalid_sysarea 2>/dev/null" );
    if ( "" != sys_id )
    {
        write( "<p class=normaltext><b><font color=red>Firmware update was not properly completed!</b> " );
        write( "Flash area " + sys_id + " is invalid.</font><br>" );
        write( "Please, go to <a href='/secure/SysTools/SysTools4.asp'>System Tools - Update</a> and complete installation of firmware update.</p>" );
    }
%>
<p>
<a class=normaltext href="javascript:detach_status();">Detach Status Window</a>
</p>
<%
    cgiExec( "fwDuHtml", "<hr>", "" );
%>
</body>
</html>
