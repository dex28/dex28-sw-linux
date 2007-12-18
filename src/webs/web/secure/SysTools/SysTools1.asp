<html>
<head>
<title>System&nbsp;Tools</title>
<link rel="stylesheet" type="text/css" href="/styles.css">
<script src="/doc/help.js"></script>

<SCRIPT LANGUAGE=javascript>
<!--
function reboot()
{
	if( confirm("During reboot of Albatross no phones connected to this device will work. Is this OK?") )
		document.location.href = "/cgi-bin/control?function=reboot";
}
function shutdown()
{
	if( confirm("If you shut down Albatross no phones connected to this device will work. Is this OK?") )
		document.location.href = "/cgi-bin/control?function=shutdown";
}
function reload()
{
	if( confirm("During reload of DSP no phones connected to this device will work. Is this OK?") )
		document.location.href = "/cgi-bin/control?function=reload";
}
function killidod()
{
	if( confirm("If you kill connections no phones connected to this device will work. Is this OK?") )
		document.location.href = "/cgi-bin/control?function=killidod";
}
function purgelog()
{
    document.location.href = "/cgi-bin/control?function=purgelog";
}
//-->
</SCRIPT>
</head>
<body topmargin=4 leftmargin=8 rightmargin=4>

<%
	showTabBeg( "System&nbsp;Tools" );
	showTab( "*", "SysTools1.asp", "System",   "System and Process Control", "sys_tools.htm#system" );
	showTab( "*", "SysTools2.asp", "Password", "" );
	showTab( "*", "SysTools3.asp", "Network",  "" );
	showTab( "*", "SysTools4.asp", "Update",  "" );
	showTabEnd( 4 );
%>

<form style='margin-top:0;padding-top:0;'>
<table cellpadding="2" cellspacing="2" border="0">
	<tr>
		<td bgcolor=#F0F0F0>&nbsp;Reboot&nbsp;Albatross&nbsp;</td>
		<td>&nbsp;<INPUT type="button" value="   OK   " onclick=reboot();>&nbsp;</td>
	</tr>
	<tr>
		<td bgcolor=#F0F0F0>&nbsp;Shutdown&nbsp;Albatross&nbsp;</td>
		<td>&nbsp;<INPUT type="button" value="   OK   " onclick=shutdown();>&nbsp;</td>
	</tr>
	<tr>
		<td bgcolor=#F0F0F0>&nbsp;Reload&nbsp;DSPs&nbsp;</td>
		<td>&nbsp;<INPUT type="button" value="   OK   " onclick=reload();>&nbsp;</td>
	</tr>
	<tr>
		<td bgcolor=#F0F0F0>&nbsp;Kill&nbsp;Connections&nbsp;</td>
		<td>&nbsp;<INPUT type="button" value="   OK   " onclick=killidod();>&nbsp;</td>
	</tr>
	<tr>
		<td bgcolor=#F0F0F0>&nbsp;Purge Temp &amp; Log Files&nbsp;</td>
		<td>&nbsp;<INPUT type="button" value="   OK   " onclick=purgelog();>&nbsp;</td>
	</tr>
</table>
</form>

</body>
</html>
