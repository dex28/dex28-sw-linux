<html>
<head>
<link rel="stylesheet" type="text/css" href="/styles.css">
<title>Configuration</title>
<script src="/doc/help.js"></script>
<SCRIPT LANGUAGE="JavaScript">
<!--
function selUser(name)
{
    document.forms.Form1.username.value = name;
}
function selAuth(mode)
{
    document.forms.Form1.dn.value = mode;
}
//-->
</SCRIPT>
</head>
<body topmargin=4 leftmargin=8 rightmargin=4>

<% 
	var RetMsg = "";
	var authView = 0;
	
	var username = getQueryVal( "username", "" );

	if ( getQueryVal ( "authview" , "" ) != "" && username != "" )
		authView = 1;
		
	if ( getQueryVal ( "back" , "" ) != "" )
		authView = 0;

	showTabBeg( "Configuration (" + getAlbaIpAddress () + ")" );
	showTab( "*",  "Config1.asp", "Network",    "" );
	showTab( "*",  "Config3.asp", "Ports",    "" );

	if ( isAlbaMode( "GATEWAY" ) || isAlbaMode( "VR" ) )
	{
		if ( authView == 1 )
			showTab( "*",  "Config4.asp", "Users", "Authorizations for '" + username + "'", "conf_gw.htm#auth" );
		else
			showTab( "*",  "Config4.asp", "Users", "User Management","conf_gw.htm#user" );
	}
	else if ( isAlbaMode( "EXTENDER" ) )
	{
		showTab( "*",  "Config5.asp", "Extensions", "" );
	}

	showTab( "*",  "Config7.asp", "Date",       "" );
	showTab( "*",  "Config2.asp", "Advanced",   "" );
	showTab( "*",  "Config6.asp", "Backup", "" );
	showTabEnd( 6 );
%>

<form name=Form1 id=Form1 method=GET style='margin-top:0;padding-top:0;'>  

<%
	if ( authView == 0 ) // Users View
	{
		if ( getQueryVal( "add", "" ) != "" )
		{
			RetMsg = addUser( username );
		}
		else if ( getQueryVal( "delete", "" ) != "" )
		{
			RetMsg = deleteUser( username );
		}
		if ( getQueryVal( "save", "" ) != "" )
		{
			RetMsg = saveUsers ();
		}

		listUsers ();
		
        	write( "<INPUT type=submit value=' Save ' name=save>&nbsp;&nbsp;" );      
		write( "<INPUT type=reset value=' Reset ' name=reset>" );
		write( "<hr></hr>" ); 

		write( "<TABLE cellspacing=0 colpadding=0><TR><TD valign=middle>" );
		write( "<font class=normaltext>Username:</font>&nbsp;&nbsp;</TD><TD>" );
		write( "<INPUT type=text value='' name=username id=username>&nbsp;" );
		write( "<INPUT type=submit value=' Add ' name=add id=add>&nbsp;");     
		write( "<INPUT type=submit value=' Delete ' name=delete id=delete>&nbsp;");
		write( "<INPUT type=submit value=' Authorizations ' name=authview value='1' id=authview>&nbsp;" ); 

		write( "</TD></TR></TABLE>" );
		
		if ( getQueryVal( "add", "" ) != "" && RetMsg != "" )
		{
			write( "<p>", RetMsg );
		}
					
		if ( getQueryVal( "delete", "" ) != "" && RetMsg != ""  )
		{			
			write( "<p>", RetMsg );
		}
		if ( getQueryVal( "save", "" ) != "" && RetMsg != ""  )
		{
			write( "<p>", RetMsg );
		}
	}
	else  // Authorization View
	{
		if ( getQueryVal( "add", "" ) != "" )
		{
			RetMsg = addAuthforUser( username );
		}
		else if ( getQueryVal( "delete", "" ) != "" )
        {
			RetMsg = delAuthforUser( username );
		}       
		if ( getQueryVal( "save", "" ) != "" )
		{
			RetMsg = saveAuthforUser( username );
		}

		listAuthforUser( username );

		write( "<INPUT type=submit value=' Save ' name=save id=save>&nbsp;&nbsp;" );      
		write( "<INPUT type=reset value=' Reset ' name=reset>" );
		write( "<hr></hr>" ); 

		write( "<TABLE cellspacing=0 colpadding=0><TR><TD valign=middle>" );
		write( "<font class=normaltext>Directory Number:</font>&nbsp;&nbsp;</TD><TD>" );
		write( "<INPUT type=text value='' name=dn id=dn>&nbsp;" );
		write( "<INPUT type=hidden name=authview value='1'>&nbsp;" );		
		write( "<INPUT type=submit value=' Add ' name=add id=add>&nbsp;");     
		write( "<INPUT type=submit value=' Delete ' name=delete id=delete>&nbsp;"); 
		write( "<INPUT type=submit value=' Back ' name=back id=back >&nbsp;" );		
		write( "<INPUT type=hidden name=username value='" );
		write( username );
		write( "'>&nbsp;" );
		write( "</TD></TR></TABLE>" );
		
		if ( getQueryVal( "add", "" ) != "" && RetMsg != "" )
		{
			write( "<p>", RetMsg );
		}
					
		if ( getQueryVal( "delete", "" ) != "" && RetMsg != "" )
		{
			write( "<p>", RetMsg );
		}
					
		if ( getQueryVal( "save", "" ) != "" && RetMsg != "" )
		{
			write( "<p>", RetMsg );
		}
	}
%>
</form>

</body>
</html>
