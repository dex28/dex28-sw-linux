<html>
<head>
<link rel="stylesheet" type="text/css" href="/styles.css">
<title>System&nbsp;Tools</title>
<script src="/doc/help.js"></script>
<SCRIPT LANGUAGE=javascript>
<!--

uploadStarted = false;

function uploadFile ()
{
    if ( uploadStarted )
        return false;

    if ( document.Form1.filename.value == "" )
    {
        alert( "Please, enter or select filename and click 'Restore'" );
        return false;
    }

    return true;
}
//-->
</SCRIPT>
</head>

</head>
<body topmargin=4 leftmargin=8>

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
	showTab( "*",  "Config2.asp", "Advanced",   "" );
	showTab( "*",  "Config6.asp", "Backup", "Backup/Restore Configuration", "conf_br.htm#backup" );
	showTabEnd( 6 );
%>
<table border=0 cellspacing=2 cellpadding=2 width=100%>  

    <tr bgcolor=#F0F0E0>
        <td colspan=2>
        <font class=normaltext color=darkblue style="font-size:12pt;font-weight:bold">
        &nbsp;Backup Configuration
        </font>
        </td>
    </tr>
<form name="Form3" method=GET action=/cgi-bin/backupConfig>
    <tr>
        <td><font style="font-size:3pt">&nbsp;</font><br>
        <font class=normaltext color=black>
        Please press the 'Backup' button to save configuration data to your PC.
        </font>
        </td>
        <td valign=bottom align=right>
        <INPUT type="submit" value=" Backup "  name=download>
        </td>
    </tr>
</form>

    <tr><td colspan=2>&nbsp;</td></tr>

    <tr bgcolor=#F0F0E0>
        <td colspan=2>
        <font class=normaltext color=darkblue style="font-size:12pt;font-weight:bold">
        &nbsp;Restore Configuration
        </font>
        </td>
    </tr>
<form action="/cgi-bin/restoreConfig" name="Form1" enctype="multipart/form-data" method="POST" onsubmit="return uploadFile();">
    <tr>
        <td><font style="font-size:3pt">&nbsp;</font><br>
        <font class=normaltext color=black>
        Please select the location of a previously backed-up configuration 
        file on your PC using the browse button below, then press the 'Restore' button.
        </font><br>&nbsp;<br>
        <input type="file" name="filename" size=50>
        </td>
        <td valign=bottom align=right>
		<input type=submit value=" Restore " id=submit name=restore>
        </td>
    </tr>
</form>

    <tr><td colspan=2>&nbsp;</td></tr>

    <tr bgcolor=#F0F0E0>
        <td colspan=2>
        <font class=normaltext color=darkblue style="font-size:12pt;font-weight:bold">
        &nbsp;Reset to Factory Default
        </font>
        </td>
    </tr>
<form action="/cgi-bin/restoreConfig" name="Form2" method="GET">
    <tr>
        <td><font style="font-size:3pt">&nbsp;</font><br>
        <font class=normaltext color=black>
        Please press the 'Reset' button to restore configuration to factory default.
        </font>
        <font class=normaltext><b>Note</b>: All current configuration will be lost.</font>
        </td>
        <td valign=bottom align=right>
        <input type=submit value=" Reset " name="FACTORY_DEFAULTS">
        </td>
    </tr>
</form>
</table>

</body>
</html>
