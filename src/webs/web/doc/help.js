function cxhelp(thePage)
{
    the_url = "/doc/" + thePage;

    if ( window.parent.left.helpWindow && ! window.parent.left.helpWindow.closed ) 
    {   
        parent.left.helpWindow.HelpPage.location.href = the_url;
        parent.left.helpWindow.focus();
    }
    else
    {
        var myWidth = 600;
        var myHeight = 500;
        var features = 'width=' + myWidth + ',height=' + myHeight + ',toolbar=no,status=no,scrollbars=yes,resizable=yes,menubar=no';

        if ( window.screen )
        {
            var myLeft = ( screen.width - myWidth ) / 2;
            var myTop = ( screen.height - myHeight ) / 2;
            features += ',left=' + myLeft + ',top=' + myTop;
        }

        parent.left.helpWindow = window.open('','DEX28Help', features );
		
        newWin = parent.left.helpWindow;
		
        if ( newWin != null )
        {
            newWin.document.open('text/html');	// Required by Netscape 6.x
            newWin.document.write("<!DOCTYPE HTML PUBLIC \"-\/\/W3C\/\/DTD HTML 4.0 Transitional\/\/EN\">");
            newWin.document.write("<HTML><HEAD>");
            newWin.document.write("<META http-equiv=Content-Type content=\"text/html; charset=iso-8859-1\">");
            newWin.document.write("<TITLE>IPTC DEX28 VoIP Gateway Help<\/TITLE>");
            newWin.document.write("<\/HEAD>");
            newWin.document.write("<FRAMESET rows='55,*' frameborder=0 border=0>");
            newWin.document.write("<FRAME noresize frameborder=0 marginwidth=0 marginheight=0 name='Tools' scrolling=no SRC='/doc/tools.htm'>");
            newWin.document.write("<FRAME noresize frameborder=0 marginwidth=10 marginheight=0 name='HelpPage' SRC='"+the_url+"'>");
            newWin.document.write("<\/FRAMESET>");
            newWin.document.close();
            newWin.focus();
        }
    }
}

var failc = 0;

function cxtitle(content)
{
    var retry = 0;

    if ( top.Tools.document.getElementById ) 
    {
        if ( top.Tools.document.getElementById("cxtitle") )
        {
            top.Tools.document.getElementById("cxTitle").innerHTML=content;		
        }
        else
            retry = 1;
    }
    else if (top.Tools.document.layers) 
    {
        if ( top.Tools.document.cxTitle )
        {
            docnn = top.Tools.document.cxTitle
            docnn.document.write(content); 
            docnn.document.close();
        }
        else
            retry = 1;
    }
    else if (top.Tools.document.all) 
    {
        if ( top.Tools.document.all("cxTitle") )
        {
            top.Tools.document.all("cxTitle").innerHTML = content;
        }
        else
            retry = 1;
    }

    // top.Tools probably not loaded yet.
    // Retry in 300ms, max 10 times
    //
    if ( retry && failc < 10 )
    {
        setTimeout( 'cxtitle("'+ content + '");', 300 );
        failc++;
    }
}

