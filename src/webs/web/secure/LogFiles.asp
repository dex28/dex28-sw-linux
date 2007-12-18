<html>
<head>
<link rel="stylesheet" sel="text/css" href="/styles.css">
<title>LogFiles</title>
<script src="/doc/help.js"></script>

</head>
<body topmargin=4 leftmargin=8>
<%
	var sel = getQueryVal( "sel", "Messages" );
	var url = "LogFiles.asp?sel=";
	
	showTabBeg( "Log&nbsp;Files" );
	showTab( sel, url, "Messages",  "System Messages Log File ", "log.htm#messages" );
	showTab( sel, url, "Security", "Security Log", "log.htm#security" );
	showTab( sel, url, "Transfer", "Transfer Log", "log.htm#transfer" );
	showTab( sel, url, "Logins",  "Login History", "log.htm#logins" );
	showTab( sel, url, "Samba",   "Samba Log File", "log.htm#samba" );
	showTab( sel, url, "DSP",   "DSP Log File <a href='LogFiles.asp?sel=DSP&QB=1' style='font-size:10;text-decoration:none'>[Query]</a>", "log.htm#dsp" );
	showTabEnd( 6 );
	
    if ( sel == "Messages" )
    {
		cgiExec( "[ -f /var/log/messages ] && tail -n 160 /var/log/messages", "<pre style='margin-top:0;padding-top:0;'>", "", "</pre>"  ); 
    }
    else if ( sel == "Samba" )
    {
		cgiExec( "[ -f /var/log/samba/smbd.log ] && tail -n 160 /var/log/samba/smbd.log", "<pre style='margin-top:0;padding-top:0;'>", "", "</pre>"  ); 
    }
    else if ( sel == "Security" )
    {
		cgiExec( "[ -f /var/log/secure ] && tail -n 160 /var/log/secure", "<pre style='margin-top:0;padding-top:0;'>", "", "</pre>"  ); 
    }
    else if ( sel == "Transfer" )
    {
		cgiExec( "[ -f /var/log/xferlog ] && tail -n 160 /var/log/xferlog", "<pre style='margin-top:0;padding-top:0;'>", "", "</pre>"  ); 
    }
    else if ( sel == "Logins" )
    {
		cgiExec( "last -40", "<pre style='margin-top:0;padding-top:0;'>", "", "</pre>"  ); 
    }
    else if ( sel == "DSP" )
    {
		if ( getQueryVal( "QB", "" ) != "" )
			cgiExec( "/albatross/bin/c54setp all query 1>/dev/null 2>/dev/null", "", "", ""  ); 
		cgiExec( "[ -f /var/log/albatross ] && tail -n 160 /var/log/albatross", "<pre style='margin-top:0;padding-top:0;'>", "", "</pre>"  ); 
    }
    else
    {
		write( "Selection is not yet handled in ASP." );
    }
%>    

</body>
</html>
