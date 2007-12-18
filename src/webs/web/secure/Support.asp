<html>
<head>
<title>System&nbsp;Info</title>
<link rel="stylesheet" sel="text/css" href="/styles.css">
<script src="/doc/help.js"></script>
</head>
<body topmargin=4 leftmargin=8 rightmargin=4>
<%
	var sel = getQueryVal( "sel", "Support" );
	var url = "Support.asp?sel=";

	showTabBeg( "Support" );
	showTab( sel, url, "Support",  "Support" );
	showTab( sel, url, "Feedback", "Feedback", "prototype.htm" );
	showTabEnd( 2 );

    if ( sel == "Support" )
    {
		write(
		"<p class=normaltext style='margin-top:0;padding-top:0;'>",
		"This DEX28 Administration System for contains a comprehensive online",
		"help system that gives explanations and instructions about configuring",
		"the " + getAlbaModel () + ".</p>",
		"<ul class=normaltext style='margin-top:0;padding-top:0;'><LI><a href=javascript:cxhelp('start_gen.htm')>How to use the help system</a></LI>",
        "<LI><a href=javascript:cxhelp('toc.htm')>Table of Contents</a></LI></ul>",
		"<p class=normaltext style='margin-top:0;padding-top:0;'>",
		"If additional assistance is required, please select one of the following links:</p>",
		"<ul class=normaltext style='margin-top:0;padding-top:0;'><LI ><a href=javascript:cxhelp('supp_sp.htm')>Support from IPTC Service Partner</a></LI>",
        "<LI><a href=javascript:cxhelp('supp_iptc.htm')>Support from IPTC</a></LI>",
		"<li><a href=javascript:cxhelp('supp_repair.htm')>Returning products for repair</a></li></ul>");     
        write(
		"<form action='/cgi-bin/snapshot'>",
        "<hr><font class=normaltext>When requesting support, always send current system snapshot file together with the request.</font><br><br>",
		"<input type=submit name=OK value=' System Snapshot '>",
		"</form>",
		);
        
    }
    else if ( sel == "Feedback" )
    {
        var sysVERSION = "";
		parseSysconfig( "/etc/sysconfig/albatross/version" ); // read Y from X=Y into sysX

		write( 
		"<p class=normaltext style='margin-top:0;padding-top:0;'>",
		"IPTC is always looking for product improvements, especially during Alpha and Beta test releases.",
		"If you'd like to help us by providing feedback you can do so by clicking on the button below",
		"which will connect you to IPTC's feedback form.</p>",
		"<p class=normaltext>",
		"None of the fields are mandatory - just provide the information you wish.<br>",
		"Please do not use this link for Technical Support. If you need assistance click",
		"on the Support option on the left hand menu.</p>",
		"<form action='http://www.iptc.se/Feedback/default.asp'>",
		"<input type=submit name=OK value=' Feedback '>",
		"<input type=hidden name=product value='" + getAlbaModel () + "'>",
		"<input type=hidden name=version value='" + sysVERSION + "'>",
		"<input type=hidden name=serialno value='" + getAlbaSerialNo () + "'>",
		"<input type=hidden name=uuid value='" + getAlbaUUID () + "'>",
		"</form>",
		);
    }
    else
    {
		write( "Selection is not yet handled in ASP." );
    }
%>    

</body>
</html>
