<html>
<head>
<title>System&nbsp;Tools</title>
<link rel="stylesheet" type="text/css" href="/styles.css">
<script src="/doc/help.js"></script>

</head>
<body topmargin=4 leftmargin=8 rightmargin=4>

<%
	showTabBeg( "System&nbsp;Tools" );
	showTab( "*", "SysTools1.asp", "System",   "" );
	showTab( "*", "SysTools2.asp", "Password", "" );
	showTab( "*", "SysTools3.asp", "Network",  "Network Diagnostics", "sys_tools.htm#net" );
	showTab( "*", "SysTools4.asp", "Update",  "" );
	showTabEnd( 4 );
%>

<p class=normaltext style='margin-top:0;padding-top:0;'>
<font color=#800000>Fill in IP address of a remote host, and select type of diagnostics.</font>
</p>

<table border=0 cellspacing=2 cellpadding=2>  

	<form name="Form1" method=GET action=/cgi-bin/ping>
	<tr bgcolor=#F0F0F0>
		<td>&nbsp;Host&nbsp;IP&nbsp;Address:&nbsp;&nbsp;</td>
		<td>&nbsp;<INPUT type="text" name=ipaddress value="<% write( getQueryVal( "ipaddress", "" ) ); %>">&nbsp;</td>
	</tr>
	<tr bgcolor=#F0F0F0>
		<td>&nbsp;Resolve&nbsp;Names:&nbsp;&nbsp;</td>
		<td><INPUT type="checkbox" name=nameserver value=yes <% if ( getQueryVal( "nameserver", "" ) != "" ) write( "checked=true" ) ; %>></td>
	</tr>
	<tr>
		<td colspan=2>&nbsp;</td>
	</tr>
	<tr>
		<td colspan=2>
			<INPUT type="submit" value=" Ping  " id=ping name=ping>
			<INPUT type="submit" value=" Trace Route " id=trace name=tracert>
			<INPUT type="submit" value=" ARP Ping " id=arping name=arping>
		</td>
	</tr>
	</form>
</table>

<pre>
<!--%
	ipaddr = getQueryVal( "ipaddress" ,"" );
	ping = getQueryVal( "ping" ,"" );
	trace = getQueryVal( "trace" ,"" );
	namesrv = getQueryVal( "nameserver", "" );

	if ( ping != "" && ipaddr != "" )
	{
		if ( namesrv == "yes" )
			cgiExec( "ping -c 4 -w 5 " + ipaddr, "<b>", "</b>" );
		else
			cgiExec( "ping -n -c 4 -w 5 " + ipaddr, "<b>", "</b>" );
	}	
	else if ( trace != "" && ipaddr != "" )
	{
		if ( namesrv == "yes" )
			cgiExec( "traceroute " + ipaddr, "<b>", "</b>" );
		else
			cgiExec( "traceroute -n " + ipaddr, "<b>", "</b>" );
	}	
%-->
</pre>
</body>
</html>
