<html>
<head>
<link rel="stylesheet" type="text/css" href="/styles.css">
<title>Configuration</title>
<script src="/doc/help.js"></script>

<SCRIPT LANGUAGE="JavaScript">
<!--
function check()
{
    if ( document.Form2.DHCP.checked == true )
    {
        document.Form2.ipaddress.disabled = true;
        document.Form2.ipaddress.value    = " by DHCP ";
        document.Form2.netmask.disabled   = true; 
        document.Form2.netmask.value      = " by DHCP ";
        document.Form2.broadcast.disabled = true;
        document.Form2.broadcast.value    = " by DHCP ";
        document.Form2.gateway.disabled   = true;
        document.Form2.gateway.value      = " by DHCP ";
    }
    if ( document.Form2.DHCP.checked == false )
    {
        document.Form2.ipaddress.disabled = false;          
        document.Form2.netmask.disabled   = false; 
        document.Form2.broadcast.disabled = false;
        document.Form2.gateway.disabled   = false;

        if ( document.Form2.ipaddress.value == " by DHCP " )
             document.Form2.ipaddress.value =  "";
        if ( document.Form2.netmask.value   == " by DHCP " )
             document.Form2.netmask.value   =  "";
        if ( document.Form2.broadcast.value == " by DHCP " )
             document.Form2.broadcast.value =  "";
        if ( document.Form2.gateway.value   == " by DHCP " )
             document.Form2.gateway.value   =  "";
    }
}        
//-->
</SCRIPT>
</head>
<body topmargin=4 leftmargin=8 rightmargin=4 onload="check()">

<%
    showTabBeg( "Configuration (" + getAlbaIpAddress () + ")" );
    showTab( "*",  "Config1.asp", "Network",    "Network Settings", "conf_net.htm#network" );
    showTab( "*",  "Config3.asp", "Ports",    "" );
    if ( isAlbaMode( "GATEWAY" ) || isAlbaMode( "VR" ) )
    {
        showTab( "*",  "Config4.asp", "Users", "" );
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
        if ( saveNetworkConfig () == "1" )
            OK = 1;
    }
    else if ( getQueryVal( "apply", "" ) != "" )
    {
        if ( saveNetworkConfig () == "1" )
            OK = 1;
    }

    var sysHOSTNAME = "";
    var sysDOMAINNAME = "";
    var sysWORKGROUP = "IPTC";
    var sysNETBIOSNAME = "VRss";
    var sysBOOTPROTO = "";
    var sysIPADDR = "";
    var sysNETMASK = "";
    var sysBROADCAST = "";
    var sysGATEWAY = "";
    var sysDNS1 = "";
    var sysDNS2 = "";

    parseSavedNetworkConfig ();
    parseRunningNetworkConfig ();
%>

<form name=Form2 id=Form2 method=GET  style='margin-top:0;padding-top:0;' > 

<table border=0 cellspacing=2 cellpadding=0>  
    <tr bgcolor=#D0D0C0>
        <td colspan=3>&nbsp;<b>Identity</b></td>
    </tr>
    <tr bgcolor=#F0F0F0>
        <td>&nbsp;&nbsp;Host&nbsp;Name:&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td>&nbsp;<INPUT type="text" name=hostname 
            value="<% write( sysHOSTNAME ); %>">&nbsp;
        &nbsp;<% write( runHOSTNAME ); %>&nbsp;&nbsp;&nbsp;</td>
    </tr>
    <tr bgcolor=#F0F0F0>
        <td>&nbsp;&nbsp;Domain&nbsp;Name:&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td>&nbsp;<INPUT type="text" name=domainname 
            value="">&nbsp;
        &nbsp;<% write( runDOMAINNAME ); %>&nbsp;&nbsp;&nbsp;</td>
    </tr>
<%
    if ( isAlbaMode( "VR" ) )
    {
        write( "<tr bgcolor=#F0F0F0>\n" );
        write( "<td>&nbsp;&nbsp;Workgroup:&nbsp;&nbsp;&nbsp;&nbsp;</td>\n" );
        write( "<td>&nbsp;<INPUT type=text name=workgroup " );
        write( "value='" + sysWORKGROUP + "'>&nbsp;" );
        write( "&nbsp;&nbsp;&nbsp;&nbsp;</td>\n" );
        write( "</tr>\n" );
        write( "<tr bgcolor=#F0F0F0>\n" );
        write( "<td>&nbsp;&nbsp;NetBIOS&nbsp;Name:&nbsp;&nbsp;&nbsp;&nbsp;</td>\n" );
        write( "<td>&nbsp;<INPUT type=text name=netbiosname " );
        write( "value='" + sysNETBIOSNAME  + "'>&nbsp;" );
        write( "&nbsp;&nbsp;&nbsp;&nbsp;</td>\n" );
        write( "</tr>\n" );
    }
%>
    <tr>
                <td height="10"></td>
    </tr>       
    <tr bgcolor=#D0D0C0>
        <td colspan=3>&nbsp;<b>Ethernet&nbsp;Interface</b></td>
    </tr>
    <tr bgcolor=#F0F0F0>
        <td>&nbsp;&nbsp;DHCP&nbsp;Enabled:&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td>&nbsp;<INPUT type="checkbox" id=DHCP name=dhcp onClick="check()"
            <% if ( sysBOOTPROTO == "dhcp" ) write( "checked=yes" ); %>
            >&nbsp;
        &nbsp;</td>
    </tr>
    <tr bgcolor=#F0F0F0>
        <td>&nbsp;&nbsp;IP&nbsp;Address:&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td>&nbsp;<INPUT type="text" id=ipaddress name=ipaddress 
            value="<% write( sysIPADDR ); %>">&nbsp;
        &nbsp;<% write( runIPADDR ); %>&nbsp;&nbsp;&nbsp;</td>
    </tr>
    <tr bgcolor=#F0F0F0>
        <td>&nbsp;&nbsp;Network&nbsp;Mask:&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td>&nbsp;<INPUT type="text" id=netmask name=netmask 
            value="<% write( sysNETMASK ); %>">&nbsp;
        &nbsp;<% write( runNETMASK ); %>&nbsp;&nbsp;&nbsp;</td>
    </tr>
    <tr bgcolor=#F0F0F0>
        <td>&nbsp;&nbsp;Broadcast&nbsp;Address:&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td>&nbsp;<INPUT type="text" id=broadcast name=broadcast 
            value="<% write( sysBROADCAST ); %>">&nbsp;
        &nbsp;<% write( runBROADCAST ); %>&nbsp;&nbsp;&nbsp;</td>
    </tr>
    <tr bgcolor=#F0F0F0>
        <td>&nbsp;&nbsp;Detault&nbsp;Gateway:&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td>&nbsp;<INPUT type="text" id=gateway name=gateway 
            value="<% write( sysGATEWAY ); %>">&nbsp;
        &nbsp;<% write( runGATEWAY ); %>&nbsp;&nbsp;&nbsp;</td>
    </tr>
    <tr>
        <td height="10"></td>
    </tr>
    <tr bgcolor=#D0D0C0>
        <td colspan=3>&nbsp;<b>DNS</b></td>
    </tr>
    <tr bgcolor=#F0F0F0>
        <td>&nbsp;&nbsp;Name&nbsp;server&nbsp;1:&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td>&nbsp;<INPUT type="text" name=dns1 value="<% write( sysDNS1 ); %>">&nbsp;
        </td>
    </tr>
    <tr bgcolor=#F0F0F0>
        <td>&nbsp;&nbsp;Name&nbsp;server&nbsp;2:&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td>&nbsp;<INPUT type="text" name=dns2 value="<% write( sysDNS2 ); %>">&nbsp;
        </td>
    </tr>
    <tr bgcolor=#D0D0C0>
        <td colspan=3>&nbsp;<b>Services</b></td>
    </tr>
    <tr bgcolor=#F0F0F0>
        <td>&nbsp;&nbsp;Albatross&nbsp;D4oIP&nbsp;Port:&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td>&nbsp;<INPUT type="text" name=d4oip_port value="<% write( getAlbaD4oIPPort () ); %>">&nbsp;
        </td>
    </tr>
    <tr bgcolor=#F0F0F0>
        <td>&nbsp;&nbsp;Webs&nbsp;HTTP&nbsp;Port:&nbsp;&nbsp;&nbsp;&nbsp;</td>
        <td>&nbsp;<INPUT type="text" name=http_port value="<% write( isUndef( "sysHTTP_PORT", "80" ) ); %>">&nbsp;
        </td>
    </tr>
    <tr>
                <td height="10"></td>
    </tr>       
    <tr >
        <td colspan=2 align=left>&nbsp;
            <INPUT type=submit value=" Save " name=save>&nbsp;
            <INPUT type=submit value=" Apply " name=apply>&nbsp;
            <INPUT type=reset value=" Reset " name=reset>
        </td>
    </tr>
</table>

</form>
<%
    if ( OK )
    {
        write( "<p class=normaltext>Updated successfully!" );
        if ( getQueryVal( "apply", "" ) != "" )
        {
            write( "<br><font color=darkred><b>Applying network settings...</b></font>" );
            write( "<br><font color=darkred>Please, wait few seconds before refreshing this page to see result.</font>" );
        }
        write( "</p>" );
    }
%>
</body>
</html>
