<html>
<head>
<title>System&nbsp;Tools</title>
<link rel="stylesheet" type="text/css" href="/styles.css">
<script src="/doc/help.js"></script>
<SCRIPT LANGUAGE=javascript>
<!--

uploadTime=0;

function clockCounter() 
{
    if ( uploadTime == 1 )
        document.Form1.disabled = true;    

	tmp = uploadTime;
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

	var content = uploadTime == 0 ? "" 
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
				
	uploadTime++;

 	again = setTimeout( 'clockCounter()', 1000 );	

    return true;
}

function uploadFile()
{
    if ( uploadTime != 0 )
        return false;

    if ( document.Form1.filename.value == "" )
    {
        alert( "Please, enter or select filename and click 'Upload'" );
        return false;
    }

	var content = "<font color=darkred><b>Uploading File:</b><br>" + document.Form1.filename.value + "</font>";

    if(document.getElementById) 
    {
        document.getElementById("msgDiv").innerHTML=content;		
    }
    else if (document.layers) 
    {
        docnn = document.msgDiv;
        docnn.document.write(content); 
        docnn.document.close();
    }
    else if (document.all) 
    {
        document.all("msgDiv").innerHTML = content;
    }

    clockCounter ();
    return true;
}

//-->
</SCRIPT>
</head>

<body topmargin=4 leftmargin=8 rightmargin=4>

<%
	showTabBeg( "System&nbsp;Tools" );
	showTab( "*", "SysTools1.asp", "System",   "" );
	showTab( "*", "SysTools2.asp", "Password", "" );
	showTab( "*", "SysTools3.asp", "Network",  "" );
	showTab( "*", "SysTools4.asp", "Update",  "Update The Firmware", "sys_tools.htm#update" );
	showTabEnd( 4 );
%>

<p class=normaltext style='margin-top:0;padding-top:0;'>
<form action="/cgi-bin/listUpdates" name="Form2" method=GET>
If this device has access to public internet, you can list available firmware updates
and perform <b>online</b> update by selecting option "<b>List Updates</b>". <!--If you 
intend to use Proxy server for your LAN, fill the appropriate field as well.-->
</p>
<p class=normaltext style='margin-top:0;padding-top:0;'>
<input type=submit name="list" value=" List Updates ">&nbsp;
<!-- [ using Proxy server: <input type=text name="proxy" value="" size=30> &nbsp;]  -->
</p>

<p class=normaltext >
Otherwise, you can perform <b>offline</b> update from localy available firmware update. In this case, please select local
file containing firmware update, and then select "<b>Upload</b>" to transfer it to DEX28.
</form>
</p>

<form action="/cgi-bin/updateFirmware" name="Form1" enctype="multipart/form-data" method=POST onsubmit="return uploadFile();">
<table border=0 cellspacing=2 cellpadding=2>  
	<tr>
		<td bgcolor=#F0F0F0>&nbsp;<b>File:</b>&nbsp;
		<input type="file" name="filename" id=filename size=50>&nbsp;
		</td>
		<td>
		<input type=submit value=" Upload " id=update name=update>
		</td>
	</tr>
</table>
</form>

<div class=normaltext id="msgDiv">&nbsp;</div>
<br>
<div class=normaltext id="clockDiv">&nbsp;</div>
</body>
</html>
