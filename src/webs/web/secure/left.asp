<html>
<head>
<title>IPTC AB</title> </head>
<link rel="stylesheet" type="text/css" href="/stylessMenu.css">
<SCRIPT language=JavaScript><!--

if (document.images) 
{
	var item_nosel = new Image();
	item_nosel.src = "/images/item_nosel.gif";
	var item_sel = new Image();
	item_sel.src = "/images/item_sel.gif";
}

var selected_item = new String;
var helpWindow=0;

function selectItem(item_name)
{   
	if (selected_item!="" && document.images) 
	    document.images[selected_item].src = item_nosel.src;

	if (item_name!="" && document.images) 
	    document.images[item_name].src = item_sel.src;

	selected_item = item_name;
}

function pageLoaded()
{
	selectItem('img2');
}
//-->
</SCRIPT>


<body bgColor="#330099" text="#FFFFFF" link="#ffffff" alink="#ffffff" vlink="#ffffff" topmargin="8" rightmargin="0" leftmargin="0" marginheight="4" marginwidth="0" onLoad="pageLoaded();">
<table align="left" cellspacing="0" cellpadding="0" border="0" width="170">
     <tr>
     <td width="10">&nbsp;</td> 
     <td>
	<table align="left" cellspacing="0" cellpadding="0" border="0"> 
     		<tr>
        		<td colspan="2" valign=bottom><a href="Status/Status1.asp" class=menui target="main" onclick="selectItem('img2')" >
        		<img src="/images/item_nosel.gif" name="img2" border="0" align="left" hspace="0" WIDTH="22" HEIGHT="18">Status</a></td>
     		</tr>
	    	<tr>
            		<td height="4"></td></tr>
		<tr>
	  		<td colspan="2"><a href="SysInfo.asp" class=menui target="main" onclick="selectItem('img3')">
          		<img src="/images/item_nosel.gif" name="img3" border="0" align="left" hspace="0" WIDTH="22" HEIGHT="18">System&nbsp;Info</a></td>
		</tr>
        	<tr>
        		<td height="4"></td>
                </tr>
            	<tr>
			<td colspan="2"><a href="Config/Config1.asp" class=menui target="main" onclick="selectItem('img5')">
                        <img src="/images/item_nosel.gif" name="img5" border="0" align="left" hspace="0" WIDTH="22" HEIGHT="18">Configuration</a></td>
		</tr>
                <tr>
                	<td height="4"></td>
                </tr> 
             	<tr>            
			<td colspan="2"><a href="SysTools/SysTools1.asp" class=menui target="main" onclick="selectItem('img4')">
                	<img src="/images/item_nosel.gif" name="img4" border="0" align="left" hspace="0" WIDTH="22" HEIGHT="18">System&nbsp;Tools</a></td>
	    	</tr>
		<tr>
                	<td height="4"></td>
                </tr>
		<tr>
			<td colspan="2"><a href="LogFiles.asp" class=menui target="main" onclick="selectItem('img6')">
                        <img src="/images/item_nosel.gif" name="img6" border="0" align="left" hspace="0" WIDTH="22" HEIGHT="18">Log&nbsp;Files</a></td>
		</tr>
                <tr>
                	<td height="4"></td>
                </tr>
		<tr>
			<td colspan="2"><a href="/doc/documentation.asp" class=menui target="main" onclick="selectItem('img7')">
                        <img src="/images/item_nosel.gif" name="img7" border="0" align="left" hspace="0" WIDTH="22" HEIGHT="18">Documentation</a></td>
                </tr>
		<tr>
                	<td height="4"></td>
                </tr>
		<tr>
			<td colspan="2"><a href="Support.asp" class=menui target="main" onclick="selectItem('img8')">
                        <img src="/images/item_nosel.gif" name="img8" border="0" align="left" hspace="0" WIDTH="22" HEIGHT="18">Support</a></td>
		</tr>
		<tr>
                	<td height="4"></td>
                </tr>
	</table>		
     </td>
</table>
</body>
</html>
         
