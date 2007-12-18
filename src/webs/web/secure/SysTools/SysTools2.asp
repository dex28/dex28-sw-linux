<html>
<head>
<title>System&nbsp;Tools</title>
<link rel="stylesheet" type="text/css" href="/styles.css">
<script src="/doc/help.js"></script>

<SCRIPT LANGUAGE="JavaScript">
<!-- 
//-->
</SCRIPT>
</head>
<body topmargin=4 leftmargin=8 rightmargin=4>

<%
	showTabBeg( "System&nbsp;Tools" );
	showTab( "*", "SysTools1.asp", "System",   "" );
	showTab( "*", "SysTools2.asp", "Password", "Change Password", "sys_tools.htm#pass" );
	showTab( "*", "SysTools3.asp", "Network",  "" );
	showTab( "*", "SysTools4.asp", "Update",  "" );
	showTabEnd( 4 );

	var oldpwd = getQueryVal( "oldpwd", "" );
	var newpwd = getQueryVal( "newpwd", "" );
	var rnewpwd = getQueryVal( "rnewpwd", "" );
%>
 
<p class=normaltext style='margin-top:0;padding-top:0;'>

<%
if ( getQueryVal( "OK", "" ) == "" )
{
	write( "<font color=#800000>Enter old password, new password twice and click on OK to change web administration password.</font><br>" );
}
else
{
	var error = 0;

	if ( oldpwd == "" )
	{
        	write( "<font color=red>The old password can't be empty!</font><br>" );
		error = 1;
	}
	if ( newpwd == "" )
	{
        	write( "<font color=red>New password can't be empty!</font><br>" );
		error = 1;
	}
	else if ( newpwd != rnewpwd )
	{
        	write( "<font color=red>New password should be typed twice the same!</font><br>" );
		error = 1;
	}

	if ( ! error )
	{
		// TODO: actual change password
                if ( changeAdminPassword( oldpwd, newpwd ) )
		    write( "<font color=blue>Password has been successfully changed.</font><br>" );
                else
		    write( "<font color=red>Change password failed.</font><br>" );
		oldpwd = "";
		newpwd = "";
		rnewpwd = "";
	}
}  
%>

</p>

<table border=0 cellspacing=2 cellpadding=2>  

	<form id=Form1 name=Form1 method=GET>
	<tr bgcolor=#F0F0F0>
		<td>&nbsp;Old&nbsp;Change Password:&nbsp;&nbsp;</td>
		<td>&nbsp;<INPUT type="password" id=oldpwd name=oldpwd value="<% write( oldpwd ); %>">&nbsp;</td>
	</tr>
	<tr bgcolor=#F0F0F0>
		<td>&nbsp;New&nbsp;Password:&nbsp;&nbsp;</td>
		<td>&nbsp;<INPUT type="password" id=newpwd name=newpwd value="<% write( newpwd ); %>">&nbsp;</td>
	</tr>
	<tr bgcolor=#F0F0F0>
		<td>&nbsp;Retype&nbsp;New&nbsp;Password:&nbsp;&nbsp;</td>
		<td>&nbsp;<INPUT type="password" id=rnewpwd name=rnewpwd value="<% write( rnewpwd ); %>">&nbsp;</td>
	</tr>
        <tr>
        		<td height="8"></td>
                </tr>
	<tr>
		<td colspan=2 align="left">
			<INPUT type=submit value="  OK  " name=OK> 
			<INPUT type=reset value=" Reset " name=reset>
		</td>
	</tr>
	</form>
</table>
</body>
</html>
