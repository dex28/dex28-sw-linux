function openLink(url,w,h)
{
	newWin=window.open(url,"win",'toolbar=0,location=0,directories=0,status=1,menubar=0,scrollbars=1,resizable=1,width=' +w +',height=' +h);
	newWin.name = "newWin"; 
}

function trimString(string)
{	
	while(string.indexOf(" ") >= 0 )
		string = string.replace(" ","");
	return string;
}

function TestSN(IP1, IP2, IP3, IP4) {
	if ((IP1 > 255) || (IP1 < 0)) return false;
	if ((IP2 > 255) || (IP2 < 0)) return false;
	if ((IP3 > 255) || (IP3 < 0)) return false;
	if ((IP4 > 255) || (IP4 < 0)) return false;
	var IPX =5;
	
	if (IP1 < 255) {
	  if((IP2 > 0) || (IP3 > 0) || (IP4 > 0)) return false;
	  IPX = IP1;
	} 
	else {
		if (IP2 < 255) {
			if((IP3 > 0) || (IP4 > 0)) return false;
			IPX = IP2;
	    } 
	    else {
			if (IP3 < 255) {
				if ((IP4 > 0)) return false;
				IPX = IP3;
			} 
			else {
				IPX = IP4;
				if (IPX > 248 ) return false; 
			}
		}
    }

	// Determine if IPX is good
	switch (IPX) {
		case "255":
	    case "254":
   	    case "252":
	    case "248":
	    case "240":
	    case "224":
	    case "192":
		case "128":
	    case "0":
			return true;
	    default:
			return false;
	} 
  	
	return false;
}
 
function isValidIP( ip , own)
{
	ipaddress = trimString(ip.value);
	if( ipaddress.length < 7 )
		return false;
	ip.value = ipaddress;
	
	dot = ipaddress.indexOf(".");
	if( dot < 1 )
		return false;
	ip1 = ipaddress.substring(0,dot);
	ipaddress = ipaddress.substring(dot+1,15);
	dot = ipaddress.indexOf(".");
	if( dot < 1 )
		return false;
	ip2 = ipaddress.substring(0,dot);
	ipaddress = ipaddress.substring(dot+1,15);
	dot = ipaddress.indexOf(".");
	if( dot < 1 )
		return false;
	ip3 = ipaddress.substring(0,dot);
	ip4 = ipaddress.substring(dot+1,15);

	if ( isNaN(ip1) || ip1 > 255 || ip1 < 1 || (own == 1 && ip1 == 127) ) return false;
	if ( isNaN(ip2) || ip2 > 255 || ip2 < 0) return false;
	if ( isNaN(ip3) || ip3 > 255 || ip3 < 0) return false;
	if ( isNaN(ip4) || ip4 > 255 || ip4 < 1) return false;

	//alert(ip1 +" " +ip2 +" " +ip3+" " +ip4);
	return true;
}

function isValidNetmask( netmask )
{
	ipaddress = trimString(netmask.value);
	if( ipaddress.length < 7 )
		return false;
	netmask.value = ipaddress;

	dot = ipaddress.indexOf(".");
	if( dot < 1 )
		return false;
	ip1 = ipaddress.substring(0,dot);
	ipaddress = ipaddress.substring(dot+1,15);
	dot = ipaddress.indexOf(".");
	if( dot < 1 )
		return false;
	ip2 = ipaddress.substring(0,dot);
	ipaddress = ipaddress.substring(dot+1,15);
	dot = ipaddress.indexOf(".");
	if( dot < 1 )
		return false;
	ip3 = ipaddress.substring(0,dot);
	ip4 = ipaddress.substring(dot+1,15);

	if ( isNaN(ip1) || ip1 > 255 || ip1 < 1) return false;
	if ( isNaN(ip2) || ip2 > 255 || ip2 < 0) return false;
	if ( isNaN(ip3) || ip3 > 255 || ip3 < 0) return false;
	if ( isNaN(ip4) || ip4 > 255 || ip4 < 0) return false;

	//alert(ip1 +" " +ip2 +" " +ip3+" " +ip4);
	return TestSN(ip1,ip2,ip3,ip4);
}