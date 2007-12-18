<html>
	<head>
		<title>IPTC AB</title>
		<link rel="stylesheet" type="text/css" href="styles.css">
			<map name="nav1">
				<area shape="RECT" coords="14,12,80,74" href="http://www.iptc.se" target="_blank">
			</map>
	</head>
	<body BGCOLOR="#C0C0B0" text="#000000" link="#0033cc" alink="#0033cc" vlink="#0033cc" topmargin="0" rightmargin="0" leftmargin="0" marginheight="0" marginwidth="0">
<% 
	var sysWEBTITLE1 = "";
	var sysWEBTITLE2 = "";
	parseSysconfig( "/etc/sysconfig/albatross/global" ); 
%>

		<table BGCOLOR="#FFFFFF" width="100%">
				<tr>
				<td width="85" BGCOLOR="#FFFFFF"><center>
						<img src="/images/tl_corner.jpg" usemap="#nav1" WIDTH="85" HEIGHT="75" border="0"></center>
				</td>
				<td width="90" BGCOLOR="#FFFFFF">
					<b>IPTC AB</b><br>
					<a href="http://www.iptc.se" target="_parent">www.iptc.se</a>
				</td>
				<td leftmargin="10" BGCOLOR="#E0E0D0" class="title">
					&nbsp;&nbsp;&nbsp;<% write( sysWEBTITLE1 ); %><br>
					&nbsp;&nbsp;&nbsp;<% write( sysWEBTITLE2 ); %>
				</td>
			</tr>
		</table>
	</body>
</html>
