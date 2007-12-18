<html>
<head>
<title>Albatross</title>
<link rel="stylesheet" type="text/css" href="/styles.css">
<script src="/doc/help.js"></script>
</head>

<body topmargin=4 leftmargin=8 rightmargin=4>
<!--onload=setTimeout('window.location.replace("Status2.asp")',10000);-->

<%
    showTabBeg( "Status" );
	showTab( "*", "Status1.asp", "Device & Port",  "" );
	showTab( "*", "Status2.asp", "Connections", "TCP Connections Status", "stat_conn.htm" );
	showTabEnd( 2 );

    write( "<font class=normaltext><b>Active Connections:</b><br>&nbsp;</font>" );
    cgiExec( "echo '#  Remote IP      UDP   DN       Username'; cat /proc/hpidevInfo","<pre style='margin-top:0;padding-top:0;'><font color=#800000><b>", "</b></font>", "</pre>" );
    cgiExec( "/bin/netstat -tn | grep -E 'ESTAB|Address' | sed s/ESTABLISHED/CONNECTED/","<pre style='margin-top:0;padding-top:0;'><font color=#800000><b>", "</b></font>", "</pre>" );
%>
</body>
</html>
