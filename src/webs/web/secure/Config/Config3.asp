<html>
<head>
<link rel="stylesheet" type="text/css" href="/styles.css">
<title>Configuration</title>
<script src="/doc/help.js"></script>
<SCRIPT LANGUAGE="JavaScript">
<!--
function cond_dis( obj, value )
{
    if ( obj )
        obj.disabled = value;
}

function setdis_all( value )
{
    cond_dis( document.Form1.LOCAL_UDP,    value );
    cond_dis( document.Form1.DSCP_UDP,     value );
    cond_dis( document.Form1.DSCP_TCP,     value );
    cond_dis( document.Form1.CODEC,        value );
    cond_dis( document.Form1.JIT_MINLEN,   value );
    cond_dis( document.Form1.JIT_MAXLEN,   value );
    cond_dis( document.Form1.JIT_ALPHA,    value );
    cond_dis( document.Form1.JIT_BETA,     value );
    cond_dis( document.Form1.LEC,          value );
    cond_dis( document.Form1.LEC_LEN,      value );
    cond_dis( document.Form1.LEC_MINERL,   value );
    cond_dis( document.Form1.LEC_INIT,     value );
    cond_dis( document.Form1.NFE_MINNFL,   value );
    cond_dis( document.Form1.NFE_MAXNFL,   value );
    cond_dis( document.Form1.NLP,          value );
    cond_dis( document.Form1.NLP_MINVPL,   value );
    cond_dis( document.Form1.NLP_HANGT,    value );
    cond_dis( document.Form1.FMTD,         value );
    cond_dis( document.Form1.PKT_COMPRTP,  value );
    cond_dis( document.Form1.PKT_ISRBLKSZ, value );
    cond_dis( document.Form1.PKT_BLKSZ,    value );
    cond_dis( document.Form1.PKT_SIZE,     value );
    cond_dis( document.Form1.GEN_P0DB,     value );
    cond_dis( document.Form1.BCH_LOOP,     value );
    cond_dis( document.Form1.BOMB_KEY,     value );
    cond_dis( document.Form1.NAS_URL1,     value );
    cond_dis( document.Form1.NAS_URL2,     value );
    cond_dis( document.Form1.STARTREC_KEY, value );
    cond_dis( document.Form1.STOPREC_KEY,  value );
    cond_dis( document.Form1.SPLIT_FSIZE,  value );
    
    if ( document.Form1.saveRemote.checked == true )
    {
		var addr = window.prompt( "Remote IP Address:", "" );
		if ( addr == null )
		{
			// cancel save
			document.Form1.save.value = "";
		}
		else if ( addr != "" )
		{
			// redirect save to different server
			document.Form1.saveRemote.value = addr;
			document.Form1.action="http://" + addr + "/secure/Config/Config3.asp";
		}
    }
}

function check_dis()
{
    if ( ! document.Form1.USE_DEFAULTS )
        return;

    if ( document.Form1.USE_DEFAULTS.checked == true )
        setdis_all( true );
    else
        setdis_all( false );
}        
//-->
</SCRIPT>
</head>
<body topmargin=4 leftmargin=8 rightmargin=4 onload="check_dis();">

<%
	showTabBeg( "Configuration (" + getAlbaIpAddress () + ")" );
	showTab( "*",  "Config1.asp", "Network",    "" );
    showTab( "*",  "Config3.asp", "Ports",    "Port Settings", "conf_gw.htm#port" );
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
	showTab( "*",  "Config6.asp", "Backup", "" );
	showTabEnd( 6 );
	
	var OK = 0;
	
	if ( getQueryVal( "save", "" ) != "" )
	{
		if ( saveGatewayPorts () == "1" )
			OK = 1;
	}
%>
 
<form id=Form1 name=Form1 method=GET style='margin-top:0;padding-top:0;'>  
<% listGatewayPorts (); %>
<table style='margin-top:0;padding-top:0;'>
<tr>
<td>
&nbsp;<INPUT type=checkbox value"" name=saveRemote>Save Remote?<br><br>
&nbsp;<INPUT type=submit value=" Save " name=save onclick='setdis_all(false);'>
&nbsp;<INPUT type=submit value=" Defaults " name=defaults>
&nbsp;<INPUT type=reset value=" Reset " name=reset>
</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td class=fineprint><font color=#000080>
    <INPUT type=radio value='none' name=apply> Do Not Apply Settings<br>
    <INPUT type=radio value='warm' name=apply> Apply Settings (Warm)<br>
    <INPUT type=radio value='cold' name=apply checked> Apply Settings (Cold)<br>
    </font>
</td>
</tr>
</table>
</form>

<%
	if ( OK )
		write( "<p>Updated successfully!" );
%>
</body>
</html>
