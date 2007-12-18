<html>
<head>
<title>System&nbsp;Info</title>
<link rel="stylesheet" sel="text/css" href="/styles.css">
<script src="/doc/help.js"></script>
<SCRIPT LANGUAGE=javascript>
<!--
function set_sws()
{
    if( confirm("You have choosed to set device for switch (PBX) side of operation.\nDo you want to proceed?") )
        document.location.href = "SysInfo.asp?setmode=sws&";
    }
function set_ext()
{
    if( confirm("You have choosed to set device for extension side of operation.\nDo you want to proceed?") )
        document.location.href = "SysInfo.asp?setmode=ext&";
    }
function cfg_complete()
{
    document.location.href = "SysInfo.asp?setmode=query&";
    }

convTime=0;

function clockCounter() 
{
	tmp = convTime;
	sec = tmp % 60;
	tmp = ( tmp - sec ) / 60;
	min = tmp % 60;
	tmp = ( tmp - min ) / 60;
	hour = tmp % 24;
	tmp = ( tmp - hour ) / 24;
		
	if( min <= 9 ) 
		min = "0" + min;
	if( sec <= 9 ) 
		sec = "0" + sec;
	if( hour <= 9 ) 
		hour = "0" + hour;

	var content = convTime == 0 ? "" 
        : "Elapsed Time: <b>" + hour.toString() +":" +min.toString() +":" +sec.toString() + "</b>";

	if(document.getElementById) 
    {
		document.getElementById("clockDiv").innerHTML=content;		
	}
	else if (document.layers) 
	{
		docnn = document.clockDiv;
		docnn.document.write(content); 
		docnn.document.close();
	}
	else if (document.all) 
	{
		document.all("clockDiv").innerHTML = content;
	}
				
	convTime++;

    if ( convTime >= 20 )
        cfg_complete ();
    else
 	    again = setTimeout( 'clockCounter()', 1000 );	

    return true;
}
/-->
</SCRIPT>
                        
</head>
<body 
    <%
        if ( getQueryVal( "setmode", "" ) == "sws" )
        {
            write( "onload=\"clockCounter()\"" );
            cgiExec( "( TERM=x; sleep 1; fwChangeGender sws ) > /tmp/fwCG &" );
        }
        else if ( getQueryVal( "setmode", "" ) == "ext" )
        {
            write( "onload=\"clockCounter()\"" );
            cgiExec( "( TERM=x; sleep 1; fwChangeGender ext ) > /tmp/fwCG &" );
        }
    %>
    topmargin=4 leftmargin=8 rightmargin=4
    >
<%
	var sel = getQueryVal( "sel", "General" );
	var url = "SysInfo.asp?sel=";
	
    showTabBeg( "System&nbsp;Info" );
	showTab( sel, url, "General", "General Information", "sysinfo.htm#gen" );
	showTab( sel, url, "CPU",    "Process Status", "sysinfo.htm#cpu" );
	showTab( sel, url, "Memory", "Memory Information", "sysinfo.htm#memory" );
	showTab( sel, url, "Disk",    "Disk Usage", "sysinfo.htm#disk" );
	showTab( sel, url, "Users",   "Logged-on Users", "sysinfo.htm#users" );
	showTab( sel, url, "Network", "Network Status and Information", "sysinfo.htm#net" );
	showTab( sel, url, "Kernel",  "LINUX Kernel Information", "sysinfo.htm#kernel" );
	showTabEnd( 7 );
	 
    if ( sel == "General" )
    {
        var sysVERSION = "";
		parseSysconfig( "/etc/sysconfig/albatross/version" ); // read Y from X=Y into sysX

		var sysWEBTITLE1 = "";
		var sysWEBTITLE2 = "";
        var sysLICENSE = "";

		parseSysconfig( "/etc/sysconfig/albatross/global" ); // read Y from X=Y into sysX

        write( "<table border=0><tr><td width=20>&nbsp;</td><td>" );
		write( "<table border=0>" );
		write( "<tr><td colspan=2 class=title>", sysWEBTITLE2, "</td></tr>" );

		write( "<tr><td colspan=2><font color=#BB0000><b>" );

		if ( isAlbaMode( "GATEWAY" ) )
			write( "Gateway Mode (Switch Side) of Operation" );
		else if ( isAlbaMode( "EXTENDER" ) )
			write( "Extension Side of Operation" );
		else if ( isAlbaMode( "VR" ) )
			write( "Voice Recording Mode" );
		else
			write( "Unknown configuration" );
        write( "</b><br>&nbsp;</font></td></tr>" );

		write( "<tr><td>Model:</td><td><b>", getAlbaModel (), "</b></td></tr>" );
		write( "<tr><td>Version:</td><td><b>", sysVERSION, "</b></td></tr>" );

        if ( getAlbaPlatform () != "" )
		    write( "<tr><td>Platform:</td><td><b>", getAlbaPlatform(), "</b></td></tr>" );

		write( "<tr><td>Hardware:</td><td><b>" ); 
		cgiExec( "uname -m" );
        write( "</b></td></tr>"  ); 

		write( "<tr><td>Serial Number</td><td><b>", getAlbaSerialNo (), "</b></td></tr>" );
		write( "<tr><td>UUID:</td><td><b>", getAlbaUUID (), "</b></td></tr>" );

        if ( getAlbaPlatform () == "arm" )
        {
		    write( "<tr><td>MAC Address:&nbsp;</td><td><b>" );
    	    cgiExec( "/usr/sbin/fw_env /dev/mtd1 /dev/mtd2 2>/dev/null ?ethaddr" );
            write( "</b></td></tr>" );
        }

		write( "<tr><td>Number of Ports:&nbsp;</td><td><b>", getAlbaPorts (), "</b></td></tr>" );
		write( "<tr><td>Licensed Ports:</td><td><b>", sysLICENSE, "</b></td></tr>" );
        if ( getAlbaPlatform () == "arm" ) 
        {
            write ( "<tr><td>Booted From:&nbsp;</td><td><b>Flash Area", getCmdOut( "fw_env /dev/mtd1 /dev/mtd2 ?bootcmd 2>/dev/null | awk '{ print $2 }'" ), "</b></td></tr>" );
        }
		write( "</table>" );
        write( "</td></tr></table>" );

        write( "<hr>" );
		
		if ( isAlbaMode( "GATEWAY" ) )
		{
		    write( "<center>" );
		    write( "<map name='FPMap0'>" );
		    write( "<area href='javascript:set_ext();' shape='rect' coords='271, 7, 490, 139' title='Click here to configure device as EXT'>" );
		    write( "</map>" );
		    write( "<img src='/images/cfg_gw.jpg' border=0 usemap='#FPMap0'>" );
		    write( "</center>" );
		}
		else if ( isAlbaMode( "EXTENDER" ) )
		{
		    write( "<center>" );
		    write( "<map name='FPMap0'>" );
		    write( "<area href='javascript:set_sws();' shape='rect' coords='3, 6, 214, 140' title='Click here to configure device as SWS'>" );
		    write( "</map>" );
	            write( "<img src='/images/cfg_ext.jpg' border=0 usemap='#FPMap0'>" );
		    write( "</center>" );
		}
		else if ( isAlbaMode( "VR" ) )
		{
		    write( "<center>" );
			write( "<img src='/images/cfg_vr.jpg' border=0>" );
		    write( "</center>" );
		}
    }
    else if ( sel == "CPU" )
    {
		cgiExec( "ps -edaf", "<pre style='margin-top:0;padding-top:0;'><b><font color=#800000>", "</font></b>", "</pre>"  ); 
    }
    else if ( sel == "Memory" )
    {
		cgiExec( "fwMemFree", "<pre style='margin-top:0;padding-top:0;'><b><font color=#800000>", "</font></b>", "</pre>"  ); 
		cgiExec( "cat /proc/meminfo","<pre>", "", "</pre>" );
    }
    else if ( sel == "Disk" )
    {
        cgiExec( "fwDuHtml", "", "" );
        write( "<br>" );
		cgiExec( "df -k", "<pre style='margin-top:0;padding-top:0;'><b><font color=#800000>", "</font></b>", "</pre>"  ); 
		cgiExec( "cat /proc/mounts", "<pre style='margin-top:0;padding-top:0;'><b><font color=#800000>Mount points:</font></b><br>", "", "</pre>"  ); 
        //if ( getAlbaPlatform() == "arm" )
		    //cgiExec( "cat /proc/yaffs", "<pre style='margin-top:0;padding-top:0;'><b><font color=#800000>YAFFS Info:</font></b><br>", "", "</pre>"  ); 
    }
    else if ( sel == "Users" )
    {
		cgiExec( "who -H", "<pre style='margin-top:0;padding-top:0;'><b><font color=#800000>", "</font></b>", "</pre>"  ); 
    }
    else if ( sel == "Network" )
    {
		cgiExec( "/sbin/ifconfig", "<pre style='margin-top:0;padding-top:0;'><font color=#800000><b>Interfaces:</b></font><br>&nbsp;<br>", "", "</pre>" );
        if ( getAlbaPlatform() != "arm" )
		    cgiExec( "/bin/netstat -i","<pre><font color=#800000><b>", "</b></font>", "</pre><br>" );
		cgiExec( "cat /proc/net/vlan/config","<pre><font color=#800000><b>", "</b></font>", "</pre>" );
		cgiExec( "for x in /proc/net/vlan/eth0.*; do [ -f $x ] && cat $x; done", "<pre>", "", "</pre>" );
		cgiExec( "/bin/netstat -rn","<pre><font color=#800000><b>", "</b></font>", "</pre>" );
		cgiExec( "/bin/netstat -tun","<pre><font color=#800000><b>", "</b></font>", "</pre>" );
        if ( getAlbaPlatform() != "arm" )
		    cgiExec( "/sbin/arp -vn","<pre><font color=#800000><b>", "</b></font>", "</pre>" );
    }
    else if ( sel == "Kernel" )
    {
		cgiExec( "cat /proc/version","<code style='margin-top:0;padding-top:0;'><font color=#800000><b>Version</b></font><br>", "", "</code>" );
		cgiExec( "cat /proc/cpuinfo","<pre><font color=#800000><b>CPU Information</b></font><br>", "", "</pre>" );
		cgiExec( "cat /proc/meminfo","<pre><font color=#800000><b>Memory Information</b></font><br>", "", "</pre>" );
    }
    else
    {
		write( "Selection is not yet handled in ASP." );
    }

    if ( sel == "General" )
    {
        if ( getQueryVal( "setmode", "" ) == "ext"|| getQueryVal( "setmode", "" ) == "sws" )
        {
            write( "<font class=normaltext><br><font color=darkred><b>Reconfiguration started. " );
            write( "<i>Please, wait!</i></b></font><br>" );
            write( "This page will be automatically refreshed after the reconfiguration is completed.<br></font>" );
        }
        else if ( getQueryVal( "setmode", "" ) == "query" )
        {
		    cgiExec( "cat /tmp/fwCG; rm -rf /tmp/fwCG", "<pre style='margin-top:0;padding-top:0;'><b><font color=#800000>Log:</font></b><br>", "", "</pre>"  ); 
        }
    } 
%>

<div class=normaltext id="clockDiv">&nbsp;</div>
</body>
</html>
