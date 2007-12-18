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
	showTab( "*",  "Config7.asp", "Date",       "" );
	showTab( "*",  "Config2.asp", "Advanced",   "Advanced Settings", "conf_adv.htm#advanced" );
	showTab( "*",  "Config6.asp", "Backup", "" );
	showTabEnd( 6 );

    var rc_local = "/etc/rc.d/rc.local";
    if ( getAlbaPlatform() == "arm" )
        rc_local = "/etc/init.d/local"

	if ( getQueryVal( "saverc", "" ) != "" )
	{
	    saveToFile( getQueryVal( "rclocal", "" ), rc_local );
	}
%>
 
<form id=Form1 name=Form1 method=POST style='margin-top:0;padding-top:0;'>  
<table border=0 cellspacing=2 cellpadding=2>
     <tr bgcolor=#D0D0C0>
      		<td>&nbsp;<b>User's&nbsp;Init&nbsp;Script:</b></td>
      </tr>
      <tr bgcolor=#F0F0F0>
      		<td>
            <textarea name='rclocal' cols=72 rows=10><% cgiExec( "cat " + rc_local, "", "", "" ); %></textarea> 
            </td>
      </tr>
      <tr bgcolor=#F0F0F0>
    		<td>&nbsp;
      		<INPUT type=submit value=" Save " name=saverc>&nbsp;      
			<INPUT type=reset value=" Reset " name=reset>
			</td>
      </tr>
</table>
</form>

</body>
</html>
