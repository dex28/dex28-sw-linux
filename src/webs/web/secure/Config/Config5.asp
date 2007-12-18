<html>
<head>
<link rel="stylesheet" type="text/css" href="/styles.css">
<title>Configuration</title>
<script src="/doc/help.js"></script>
</head>
<body topmargin=4 leftmargin=8 rightmargin=4>

<%
	showTabBeg( "Configuration (" + getAlbaIpAddress () + ")" );
	showTab( "*",  "Config1.asp", "Network",    "" );
	showTab( "*",  "Config3.asp", "Ports",    "" );
	if ( isAlbaMode( "GATEWAY" ) || isAlbaMode( "VR" ) )
	{
		showTab( "*",  "Config4.asp", "Users",      "" );
	}
	else if ( isAlbaMode( "EXTENDER" ) )
	{
		showTab( "*",  "Config5.asp", "Extensions", "Extensions Configuration", "conf_ext.htm" );
	}
	showTab( "*",  "Config7.asp", "Date",       "" );
	showTab( "*",  "Config2.asp", "Advanced",   "" );
	showTab( "*",  "Config6.asp", "Backup", "" );
	showTabEnd( 6 );
	
	var OK = 0;
	
	if ( getQueryVal( "save", "" ) != "" )
	{
		if ( saveExtenderPorts () == "1" )
			OK = 1;
	}
%>

<form id=Form1 name=Form1 method=GET style='margin-top:0;padding-top:0;'>  

<table border=0 cellspacing=2 cellpadding=1 >
<tbody align=center>
		<tr><td></td></tr>
        <tr bgcolor=#D0D0C0>
        	<td><b>&nbsp;Port&nbsp;</b></td>
                <td><b>DN</b></td>
                <td><b>IP&nbsp;Address</b></td>
                <td><b>TCP&nbsp;Port</b></td>
                <td><b>Username</b></td>
                <td><b>Password</b></td>
                <td><b>Mode</b></td>
                <td><b>A</b></td>
                <td><b>R</b></td>
        </tr>
        <% listExtenderPorts (); %>
 </tbody> 
</table>
<hr>
<p class=normaltext style='margin-top:0;padding-top:0;margin-bottom:0;padding-bottom:0;'>
<font color=darkblue><b>Advanced Parameters:</b></font>
</p>
<table border=0 cellspacing=2 cellpadding=1 >
<tbody align=center>
		<tr><td></td></tr>
        <tr bgcolor=#D0D0C0>
        	<td><b>&nbsp;Port&nbsp;</b></td>
                <td><b>TAU-D Option</b></td>
                <td><b>Retry Count</b></td>
        </tr>
        <% listExtPortsMisc (); %>
 </tbody> 
</table>

<br>&nbsp;
	<INPUT type=submit value=" Save " name=save>&nbsp;     
	<INPUT type=reset value=" Reset " name=reset>
</form>

<%
	if ( OK )
		write( "<p>Updated successfully!" );
%>
</body>
</html>
