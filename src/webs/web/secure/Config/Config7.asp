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
		showTab( "*",  "Config5.asp", "Extensions", "" );
	}
	showTab( "*",  "Config7.asp", "Date",       "Date & Time Settings", "conf_date.htm#advanced" );
	showTab( "*",  "Config2.asp", "Advanced",   "" );
	showTab( "*",  "Config6.asp", "Backup", "" );
	showTabEnd( 6 );

	if ( getQueryVal( "saventp", "" ) != "" )
	{
	    saveToFile( getQueryVal( "ntp_servers", "" ), "/etc/sysconfig/ntp_servers" );
	}
	else if ( getQueryVal( "settz", "" ) != "" )
	{
		if ( getQueryVal( "zone", "" ) != "" )
			cgiExec( "cp /usr/share/zoneinfo/" + getQueryVal( "zone", "" ) + " /etc/localtime" );
	}
	else if ( getQueryVal( "applydt", "" ) != "" )
	{
        if ( getQueryVal( "date", "" ) != "" && getQueryVal( "time", "" ) != "" )
        {
            var dt = setDate( getQueryVal( "date", "" ) + " " + getQueryVal( "time", "" ) );
		    cgiExec(  "date " + dt + " 1>/dev/null 2>/dev/null", "", "", "" );
        }
	}
	else if ( getQueryVal( "savedt", "" ) != "" )
	{
        if ( getQueryVal( "date", "" ) != "" && getQueryVal( "time", "" ) != "" )
        {
            var dt = setDate( getQueryVal( "date", "" ) + " " + getQueryVal( "time", "" ) );
		    saveToFile( "BASEDATE=" + dt, "/etc/sysconfig/basedate" );
        }
	}
%>
 
<form id=Form3 name=Form3 method=GET style='margin-top:0;padding-top:0;'>  
<table border=0 cellspacing=2 cellpadding=2>
	<tr bgcolor=#D0D0C0>
      		<td>&nbsp;<b>Current&nbsp;Date&nbsp;&amp;&nbsp;Time:</b></td>
      </tr>
      <tr bgcolor=#F0F0F0>
      		<td>&nbsp;<input type=text name=date size=9 style='text-align:center' value='<% cgiExec( "date +%F", "", "", "" ); %>'>
      		&nbsp;<input type=text name=time size=7 style='text-align:center' value='<% cgiExec( "date +%T", "", "", "" ); %>'>
      		&nbsp;</td>
      </tr>
      <tr bgcolor=#F0F0F0>
    		<td align=center>
      		<INPUT type=submit value=" Apply " name=applydt>&nbsp; 
      		<INPUT type=submit value=" Save as Base " name=savedt>&nbsp; 
			<INPUT type=reset value=" Reset " name=reset>
			</td>
      </tr>
</table>
</form> <hr>

<form id=Form1 name=Form1 method=GET style='margin-top:0;padding-top:0;'>  
<table border=0 cellspacing=2 cellpadding=2>
	<tr bgcolor=#D0D0C0>
      		<td>&nbsp;<b>Time&nbsp;Zone:</b> (Current is <b><% cgiExec( "date +%Z" ); %></b>)</td>
      </tr>
      <tr bgcolor=#F0F0F0>
      		<td><select name=zone>
<%
	cgiExec( "awk 'substr($1,1,1) != \"#\" { print \"<option value=\\\"\"$3\"\\\">\"$3\"</option>\" }' /usr/share/zoneinfo/zone.tab | sort", "", "", "" );
%>
		</select>
      		&nbsp;</td>
      </tr>
      <tr bgcolor=#F0F0F0>
    		<td align=center>
      		<INPUT type=submit value=" Set " name=settz>&nbsp; 
			<INPUT type=reset value=" Reset " name=reset>
			</td>
      </tr>
</table>
</form> <hr>

<form id=Form2 name=Form2 method=POST style='margin-top:0;padding-top:0;'>  
<table border=0 cellspacing=2 cellpadding=2>
     <tr bgcolor=#D0D0C0>
      		<td>&nbsp;<b>NTP&nbsp;Servers:</b></td>
      </tr>
      <tr bgcolor=#F0F0F0>
      		<td>
            <textarea name='ntp_servers' cols=40 rows=7><% cgiExec( "cat /etc/sysconfig/ntp_servers", "", "", "" ); %></textarea> 
            </td>
      </tr>
      <tr bgcolor=#F0F0F0>
    		<td>&nbsp;
      		<INPUT type=submit value=" Save " name=saventp>&nbsp;      
			<INPUT type=reset value=" Reset " name=reset>
			</td>
      </tr>
</table>
</form>

</body>
</html>
