/*
 *  albatross.c -- Albatross Management
 *
 *  Copyright (c) IPTC AB., 2003. All Rights Reserved.
 */

/******************************** Description *********************************/
/*
 *  Albatross Management routines for Albatross device that can be called from 
 *  ASP functions
 */

/********************************* Includes ***********************************/

#include    "emfdb.h"
#include    "webs.h"
#include    "um.h"

/********************************** Defines ***********************************/

int albaPortCount = 0;

char ALBA_MODEL[ 64 ] = "";
char ALBA_PORTS[ 64 ] = "";
char ALBA_SERIALNO[ 64 ] = "";
char ALBA_UUID[ 128 ] = "";
char ALBA_PLATFORM[ 128 ] = "";

/************************** Code for ASP functions ****************************/

// return global variable ALBA_MODE previously
// got from sysconfig/albatross/global file

static int getAlbaModel(int eid, webs_t wp, int argc, char_t **argv)
{
    ejSetResult( eid, ALBA_MODEL );

    return 0;
}

// return global variable ALBA_PORTS previously
// got from sysconfig/albatross/global file

static int getAlbaPorts( int eid, webs_t wp, int argc, char_t **argv)
{
    ejSetResult( eid, ALBA_PORTS );

    return 0;
}

// return global variable ALBA_SERIALNO previously
// got from sysconfig/albatross/global file

static int getAlbaSerialNo( int eid, webs_t wp, int argc, char_t **argv)
{
    ejSetResult( eid, ALBA_SERIALNO );

    return 0;
}

// return global variable ALBA_UUID previously
// got from sysconfig/albatross/global file

static int getAlbaUUID( int eid, webs_t wp, int argc, char_t **argv)
{
    ejSetResult( eid, ALBA_UUID );

    return 0;
}

// return global variable ALBA_PLATFORM previously
// got from sysconfig/albatross/global file

static int getAlbaPlatform( int eid, webs_t wp, int argc, char_t **argv)
{
    ejSetResult( eid, ALBA_PLATFORM );

    return 0;
}

// getQueryVal() get parameters from ASP pages

static int getQueryVal( int eid, webs_t wp, int argc, char_t **argv )
{
    char_t* var;
    char_t* defaultValue;

    if ( ejArgs( argc, argv, T("%s %s"), &var, &defaultValue ) < 2 ) 
        return 1; // argument count is wrong

    ejSetResult( eid, websGetVar( wp, var, defaultValue ) );
    return 0;
}

// isUndef( "variable", "default value" )

static int isUndef( int eid, webs_t wp, int argc, char_t ** argv )
{
    char_t* var = NULL;
    char_t* defaultValue = NULL;

    if ( ejArgs( argc, argv, T("%s %s"), &var, &defaultValue ) < 2 ) 
        return 1; // argument count is wrong

    char_t* value = NULL;
	int ejVarType = ejGetVar( eid, var, &value );
	if (ejVarType < 0) 
        ejSetResult( eid, defaultValue );
    else
        ejSetResult( eid, value );

    return 0;
}

// saveToFile(str,filename) : saves string contents into file

static int saveToFile(int eid, webs_t wp, int argc, char_t **argv)
{
    char_t* value;
    char_t* filename;
    FILE* f;

    if ( ejArgs( argc, argv, T("%s %s"), &value, &filename ) < 2 ) 
        return 1; // argument count is wrong

    f = fopen( filename, "w" );
    if ( ! f )
        return 1; // failed to open file    

    while( *value )
    {
        if ( value[ 0 ] == '\r' && value[ 1 ] == '\n' )
            ; // skip CR
        else
            fputc( *value, f );

        value++;
        }
    fclose( f );

    ejSetResult( eid, "1" );
    return 0;
}

// read file and parse values in the form A = B or A = "B"
// and asign values to EJS variables with name sysA values B

static int parseAssignments( int eid, char* prefix, char* filename, char* singleVar, char* value ) 
{
    char buffer[ 256 ];
    char szVar[ 256 ];
    int prefix_len = prefix ? strlen( prefix ) : 0;
    strcpy( szVar, prefix ? prefix : "" );

    FILE* f = fopen( filename, "r" );
    if ( ! f )
         return 0;

    while( fgets( buffer, sizeof( buffer ), f ) )
    {
        char* chp = strchr( buffer, '=' );

        if ( chp )
        {
            // Parse A=B

            char* szValue = chp + 1;    

            // remove = and trailing spaces of A
            for ( *chp-- = 0; chp >= buffer; chp-- )
                if ( isspace( *chp ) )
                    *chp = 0;  
                else
                    break;

            // skip leading spaces of A
            for ( chp = buffer; *chp && isspace( *chp ); chp++ )
                {}

            // chp now points to A, or to empty string
            //
            if ( *chp )
            {
                strcpy( szVar + prefix_len, chp );

                // skip leading " if any
                int begins_with_quote = 0;
                if ( *szValue == '"' )
                {
                    begins_with_quote = 1;
                    szValue++;
                }

                // find end of B
                for( chp = szValue; *chp; chp++ )
                    {}

                // remove spaces from the right side of B
                for( chp--; chp >= szValue; chp-- )
                    if ( isspace( *chp ) )
                        *chp = 0;
                    else
                        break;

                if ( begins_with_quote )
                {
                    // remove trailing " if any
                    char* chp = strrchr( szValue, '"' );
                    if ( chp )
                        *chp = 0;
                    }

                if ( singleVar ) // set single global value
                {
                    if ( strcmp( singleVar, szVar ) == 0 )
                    {
                    	if ( value ) strcpy( value, szValue );
                        break; // no need for more variables to scan
                    }
                }
                else // set EJS variable
                {
                    ejSetVar( eid, szVar, szValue );
                    trace( 5, "[%s]=[%s]\n", szVar, szValue );
                }
            }
        }
    }

    fclose( f );
    return 0;
}

static int parseSysconfig(int eid, webs_t wp, int argc, char_t **argv)
{
    char_t* filename = NULL;
    char_t* prefix = NULL;
    ejSetResult( eid, "" );
    
    if (ejArgs(argc, argv, T("%s %s"), &filename, &prefix ) < 1)
        return 1; // argument count is wrong

    if ( ! prefix || ! prefix[0] )
        prefix = "sys";

    return parseAssignments( eid, prefix, filename, NULL, NULL );
}

/******************************************************************************/
//Reads LINUX uptime from /proc/uptime file

static int getUpTime(int eid, webs_t wp, int argc, char_t **argv)
{
    char buffer[ 256 ];
    //unsigned int tmp = 0, hour = 0, min = 0, sec = 0;
    FILE* fp = NULL;
    float uptime = 0;
    
    ejSetResult( eid, "" );

    if ((fp = fopen("/proc/uptime", "r")) == NULL)
        return 1;
    fscanf(fp, "%f", &uptime);
    fclose(fp);

    sprintf( buffer, "%u", (uint)uptime );

    ejSetResult( eid, buffer );

    return 0;
}

/******************************************************************************/
// getAlbaMode

#define ALBA_MODE_DIRTY "[DIRTY]"

static char ALBA_MODE[ 64 ] = ALBA_MODE_DIRTY;

extern int isAlbaMode( char* mode_of_operation )
{
    if ( strcmp( ALBA_MODE, ALBA_MODE_DIRTY ) == 0 )
    {
        strcpy( ALBA_MODE, "" );
        parseAssignments( 0, NULL, "/etc/sysconfig/albatross/mode", "MODE", ALBA_MODE );
    }

    return strcasecmp( ALBA_MODE, mode_of_operation ) == 0;
    }

static int isAlbaModeWebs(int eid, webs_t wp, int argc, char_t **argv)
{
    char* mode_of_operation = NULL;
    if (ejArgs(argc, argv, T("%s"), &mode_of_operation ) < 1)
        return 1;

    if ( isAlbaMode( mode_of_operation ) )
        ejSetResult( eid, "1" );
    else
        ejSetResult( eid, "0" );

    return 0;
}

static int saveAlbaModeWebs(int eid, webs_t wp, int argc, char_t **argv)
{
    char* mode_of_operation = NULL;
    char* apply = NULL;
    if (ejArgs(argc, argv, T("%s %s"), &mode_of_operation, &apply ) < 1)
        return 1;

    FILE* f = fopen( "/etc/sysconfig/albatross/mode", "w" );
    if ( f )
    {
        fprintf( f, "MODE=%s\n", mode_of_operation );
        fclose( f );
    }

    strcpy( ALBA_MODE, ALBA_MODE_DIRTY );

    ejSetResult( eid, "" );

    return 0;
}

/******************************************************************************/
// getAlbaD4oIPPort

static void getAlbaXinetdAlbatrossPort( char* buffer, int buflen )
{
    int port = 7077;    

    FILE* f = fopen( "/etc/xinetd.d/albatross", "r" );
    if ( f )
    {
        while( fgets( buffer, buflen, f ) )
        {
            if ( 1 == sscanf( buffer, " port = %d", &port ) )
                break;
        }

        fclose( f );
    }

    sprintf( buffer, "%d", port );
}

static int getAlbaD4oIPPort(int eid, webs_t wp, int argc, char_t **argv)
{
    char buffer[ 256 ];
    getAlbaXinetdAlbatrossPort( buffer, sizeof( buffer ) );
    ejSetResult( eid, buffer );

    return 0;
}

/******************************************************************************/
// Save network data in file /etc/sysconfig/network

static int parseSavedNetworkConfig(int eid, webs_t wp, int argc, char_t **argv)
{
    char domainname[ 256 ]  = "";
    char nameserver1[ 256 ] = "";
    char nameserver2[ 256 ] = "";

    FILE* f = fopen( "/etc/resolv.conf", "r" );
    if ( f )
    {
        char buffer[ 256 ];
        while( fgets( buffer, sizeof( buffer ), f ) )
        {
            if ( nameserver1[ 0 ] == '\0'
                    && 1 == sscanf( buffer, "nameserver %s", nameserver1 ) )
                ;
            else if ( nameserver1[ 0 ] != '\0' 
                    && 1 == sscanf( buffer, "nameserver %s", nameserver2 ) )
                ;
            else if ( 1 == sscanf( buffer, "domain %s", domainname ) )
                ;
        }
        fclose( f );
    }

    ejSetVar( eid, "sysDOMAINNAME", domainname );
    ejSetVar( eid, "sysDNS1", nameserver1 );
    ejSetVar( eid, "sysDNS2", nameserver2 );

    if ( isAlbaMode( "VR" ) )
    {
        char workgroup[ 256 ] = "";
        char netbiosname[ 256 ] = "";
    
        f = fopen( "/etc/sysconfig/albatross/smb.conf", "r" );
        if ( f )
        {
            char buffer[ 256 ];
            while( fgets( buffer, sizeof( buffer ), f ) )
            {
                if ( workgroup[ 0 ] == '\0'
                        && 1 == sscanf( buffer, " workgroup = %s", workgroup ) )
                    ;
                else if ( netbiosname[ 0 ] == '\0' 
                        && 1 == sscanf( buffer, " netbios name = %s", netbiosname ) )
                    ;
            }
            fclose( f );
        }
    
        ejSetVar( eid, "sysWORKGROUP", workgroup );
        ejSetVar( eid, "sysNETBIOSNAME", netbiosname );
    }

#ifdef BUSYBOX
	parseAssignments( eid, "sys", "/etc/sysconfig/hostname", NULL, NULL );

    f = fopen( "/etc/network/interfaces", "r" );
    if ( f )
    {
        int inside_eth0 = 0;

        char buffer[ 256 ];
        while( fgets( buffer, sizeof( buffer ), f ) )
        {
            char iface[ 64 ];
            char param[ 256 ];
            if ( 2 == sscanf( buffer, " iface %s inet %s", iface, param ) )
            {
                if ( strcmp( iface, "eth0" ) == 0 )
                {
                    ejSetVar( eid, "sysBOOTPROTO", param );
                    inside_eth0 = 1;
                    }
                else
                {
                    inside_eth0 = 0;
                    }
                }

            if ( inside_eth0 )
            {
                if ( 1 == sscanf( buffer, " address %s", param ) )
                    ejSetVar( eid, "sysIPADDR", param );
                else if ( 1 == sscanf( buffer, " netmask %s", param ) )
                    ejSetVar( eid, "sysNETMASK", param );
                else if ( 1 == sscanf( buffer, " broadcast %s", param ) )
                    ejSetVar( eid, "sysBROADCAST", param );
                else if ( 1 == sscanf( buffer, " gateway %s", param ) )
                    ejSetVar( eid, "sysGATEWAY", param );
            }
        }

        fclose( f );
    }
#else
	parseAssignments( eid, "sys", "/etc/sysconfig/network", NULL, NULL );
	parseAssignments( eid, "sys", "/etc/sysconfig/network-scripts/ifcfg-eth0", NULL, NULL );
#endif

    parseAssignments( eid, "sysHTTP_", "/etc/sysconfig/webs", NULL, NULL );

    return 0;
    }

struct Config
{
    char run_hostname[ 128 ];
    char run_domainname[ 128 ];
    char run_addr[ 64 ];
    char run_mask[ 64 ];
    char run_bcast[ 64 ];
    char run_gateway[ 64 ];
    char run_hwaddr[ 64 ];

    char hostname[ 128 ];
    char domainname[ 64 ];
    char ipaddress[ 64 ];
    char broadcast[ 64 ];
    char netmask[ 64 ];
    char gateway[ 64 ];
    char nameserver1[ 64 ];
    char nameserver2[ 64 ];
    char bootproto[ 64 ];

    char workgroup[ 128 ];
    char netbiosname[ 128 ];

    char old_hostname[ 128 ];
};

static int getAlbaIpAddress(int eid, webs_t wp, int argc, char_t **argv)
{
    char ipaddr[ 64 ] = "Unknown IP Address";

    char_t* iface;
    
    if (ejArgs(argc, argv, T("%s"), &iface ) < 1)
       iface = "eth0";

    char cmd[ 64 ];
    sprintf( cmd, "ifconfig %s", iface );

    FILE* f = popen( cmd, "r" );
    if ( f )
    {
        char buffer[ 256 ];
        while( fgets( buffer, sizeof( buffer ), f ) )
        {
            if ( 1 == sscanf( buffer, " inet addr:%s", ipaddr ) )
                break;
        }
        pclose( f );
    }

    ejSetResult( eid, ipaddr );

    return 0;
}

static int getAlbaMacAddress(int eid, webs_t wp, int argc, char_t **argv)
{
    char mac_addr[ 64 ] = "Unknown MAC Address";

    char_t* iface;
    
    if (ejArgs(argc, argv, T("%s"), &iface ) < 1)
       iface = "eth0";

    char cmd[ 64 ];
    sprintf( cmd, "ifconfig %s", iface );

    FILE* f = popen( cmd, "r" );
    if ( f )
    {
        char buffer[ 256 ];
        while( fgets( buffer, sizeof( buffer ), f ) )
        {
            if ( 1 == sscanf( buffer, "eth0 Link encap:Ethernet HWaddr %s", mac_addr ) ) 
                continue;
                break;
            }
        pclose( f );
        }

    ejSetResult( eid, mac_addr );

    return 0;
}

static void get_running_config( struct Config* cfg )
{
    strcpy( cfg->run_hostname, "(n/a)" );
    strcpy( cfg->run_domainname, "(n/a)" );

    strcpy( cfg->run_addr, "(Unknown)" );
    strcpy( cfg->run_mask, "(Unknown)" );
    strcpy( cfg->run_bcast, "(Unknown)" );
    strcpy( cfg->run_gateway, "(Unknown)" );

    strcpy( cfg->run_hwaddr, "(Unknown)" );

    FILE* f = popen( "hostname -s", "r" );
    if ( f )
    {
        fscanf( f, "%s", cfg->run_hostname );
        pclose( f );
        }

    f = popen( "hostname -d", "r" );
    if ( f )
    {
        fscanf( f, "%s", cfg->run_domainname );
        pclose( f );
        }

    char buffer[ 256 ];

    f = popen( "ifconfig eth0", "r" );
    if ( f )
    {
        while( fgets( buffer, sizeof( buffer ), f ) )
        {
            if ( 1 == sscanf( buffer, "eth0 Link encap:Ethernet HWaddr %s", cfg->run_hwaddr ) ) 
                continue;
            else if ( 3 == sscanf( buffer, " inet addr:%s Bcast:%s Mask:%s", cfg->run_addr, cfg->run_bcast, cfg->run_mask ) )
                break;
        }
        pclose( f );
    }

    f = popen( "ip route list dev eth0", "r" );
    if ( f )
    {
        while( fgets( buffer, sizeof( buffer ), f ) )
        {
            if ( 1 == sscanf( buffer, "default via %s", cfg->run_gateway ) )
                break;
        }

        pclose( f );
    }
}

static int parseRunningNetworkConfig(int eid, webs_t wp, int argc, char_t **argv)
{
    struct Config cfg;
    get_running_config( &cfg );

    websSetVar( wp, "runHOSTNAME", cfg.run_hostname );
    websSetVar( wp, "runDOMAINNAME", cfg.run_domainname );
    websSetVar( wp, "runIPADDR", cfg.run_addr );
    websSetVar( wp, "runNETMASK", cfg.run_mask );
    websSetVar( wp, "runBROADCAST", cfg.run_bcast );
    websSetVar( wp, "runGATEWAY", cfg.run_gateway );
    websSetVar( wp, "runHWADDR", cfg.run_hwaddr );

    return 0;
}

static int saveNetworkConfig(int eid, webs_t wp, int argc, char_t **argv)
{
    char_t *save, *apply;
    FILE* f;

    ejSetResult( eid, "0" );

    save = websGetVar(wp, T("save"), NULL);
    apply = websGetVar(wp, T("apply"), NULL);

    if ( ! save && ! apply )
        return 0;

    ///////////////////////////////////////////////////////////////////////////

    struct Config cfg;
    memset( &cfg, 0, sizeof( cfg ) );

    char buffer[ 256 ];
    f = popen( "ifconfig eth0", "r" );
    if ( f )
    {
        while( fgets( buffer, sizeof( buffer ), f ) )
        {
            if ( 3 == fscanf( f, " inet addr:%s Bcast:%s Mask:%s", cfg.run_addr, cfg.run_bcast, cfg.run_mask ) )
                break;
        }
        pclose( f );
    }

    ///////////////////////////////////////////////////////////////////////////

    /*
     *  Read network configuration
     */

    save = websGetVar(wp, T("save"), NULL);
    
    /*
     *  Override network configuration from web params
     */

    strcpy( cfg.hostname,    websGetVar(wp, T("hostname"),   T("DEX28.iptc.local") ) );
    strcpy( cfg.domainname,  websGetVar(wp, T("domainname"), T("") ) );

    strcpy( cfg.workgroup,   websGetVar(wp, T("workgroup"),  T("IPTC") ) );
    strcpy( cfg.netbiosname, websGetVar(wp, T("netbiosname"), T("DEX28VR") ) );

    strcpy( cfg.bootproto,   websGetVar(wp, T("dhcp"), NULL) ? "dhcp" : "static" );
    strcpy( cfg.ipaddress,   websGetVar(wp, T("ipaddress"),  cfg.run_addr      ) );
    strcpy( cfg.netmask,     websGetVar(wp, T("netmask"),    T("")             ) );
    strcpy( cfg.broadcast,   websGetVar(wp, T("broadcast"),  T("")             ) );
    strcpy( cfg.gateway,     websGetVar(wp, T("gateway"),    T("")             ) );

    strcpy( cfg.nameserver1, websGetVar(wp, T("dns1"),       T("") ) );
    strcpy( cfg.nameserver2, websGetVar(wp, T("dns2"),       T("") ) );

    char* chp = strchr( cfg.hostname, '.' );
    if ( chp )
    {
        *chp++ = 0; // cfg.hostname will became short hostname
        if ( *chp ) strcpy( cfg.domainname, chp );
    }

    if ( ! cfg.domainname[ 0 ] )
        strcpy( cfg.domainname, "localdomain" );

    {
        char* slashp =  strchr( cfg.ipaddress, '/' );
        if ( slashp ) // format x.y.z.t/n
        {
            char cmd[ 256 ];
            sprintf( cmd, "ipcalc -m %s", cfg.ipaddress );
            FILE* f = popen( cmd, "r" );
            if ( f )
            {
                fscanf( f, "NETMASK=%s", cfg.netmask );
                pclose( f );
            }
            sprintf( cmd, "ipcalc -b %s", cfg.ipaddress );
            f = popen( cmd, "r" );
            if ( f )
            {
                fscanf( f, "BROADCAST=%s", cfg.broadcast );
                pclose( f );
            }
            *slashp = 0; // truncate to real ip address
        }
    }

#ifdef BUSYBOX

    f = fopen( "/etc/sysconfig/hostname2", "w" );
    if ( ! f )
        return 1;
    fprintf( f, "HOSTNAME=%s.%s\n", cfg.hostname, cfg.domainname );
    fclose( f );

    f = fopen( "/etc/network/interfaces2", "w" );
    if ( ! f )
        return 1;
    fprintf( f, "auto lo\n" );
    fprintf( f, "iface lo inet loopback\n" );
    fprintf( f, "\n" );
    fprintf( f, "auto eth0\n" );
    if ( strcmp( cfg.bootproto, "static" ) != 0 )
    {
        fprintf( f, "iface eth0 inet dhcp\n" );
        }
    else
    {
        fprintf( f, "iface eth0 inet static\n" );
        fprintf( f, "    address %s\n", cfg.ipaddress );
        if ( strcmp( cfg.netmask, "" ) != 0 )
            fprintf( f, "    netmask %s\n", cfg.netmask );
        if ( strcmp( cfg.broadcast, "" ) != 0 )
            fprintf( f, "    broadcast %s\n", cfg.broadcast );
        if ( strcmp( cfg.gateway, "" ) != 0 )
            fprintf( f, "    gateway %s\n", cfg.gateway );
        }

    fclose( f );

#else

    f = fopen( "/etc/sysconfig/network2", "w" );
    if ( ! f )
        return 1;
    fprintf( f, "NETWORKING=yes\n" );
    fprintf( f, "HOSTNAME=%s.%s\n", cfg.hostname, cfg.domainname );
    fprintf( f, "GATEWAY=%s\n", cfg.gateway );
    fclose( f );

    f = fopen( "/etc/sysconfig/network-scripts/if2cfg-eth0", "w" );
    if ( ! f )
        return 1;
    fprintf( f, "DEVICE=eth0\n" );
    fprintf( f, "ONBOOT=yes\n" );
    if ( strcmp( cfg.bootproto, "static" ) != 0 )
    {
        fprintf( f, "BOOTPROTO=dhcp\n" );
        }
    else
    {
        fprintf( f, "BOOTPROTO=static\n" );
        fprintf( f, "IPADDR=%s\n", cfg.ipaddress );
        fprintf( f, "NETMASK=%s\n", cfg.netmask );
        if ( strcmp( cfg.broadcast, "" ) != 0 )
            fprintf( f, "BROADCAST=%s\n", cfg.broadcast );
        fprintf( f, "GATEWAY=%s\n", cfg.gateway );
        }

    fclose( f );

#endif

    f = fopen( "/etc/resolv.conf2", "w" );
    if ( ! f )
        return 1;
    if ( cfg.nameserver1[ 0 ] )
        fprintf( f, "nameserver %s\n", cfg.nameserver1 );
    if ( cfg.nameserver2[ 0 ] )
        fprintf( f, "nameserver %s\n", cfg.nameserver2 );
    if ( cfg.domainname[ 0 ] )
    {
        fprintf( f, "domain %s\n", cfg.domainname );
        fprintf( f, "search %s\n", cfg.domainname );
    }
    fclose( f );

    if ( isAlbaMode( "VR" ) )
    {
        f = fopen( "/etc/sysconfig/albatross/smb.conf2", "w" );
        if ( ! f )
            return 1;
        if ( cfg.workgroup[0] )
            fprintf( f, "   workgroup = %s\n", cfg.workgroup );
        if ( cfg.netbiosname[0] )
            fprintf( f, "   netbios name = %s\n", cfg.netbiosname );
        fclose( f );
    }

    f = fopen( "/etc/hosts2", "w" );
    if ( ! f )
        return 1;
    fprintf( f, "127.0.0.1 localhost.localdomain localhost\n" );
    fprintf( f, "%s %s.%s %s\n", cfg.ipaddress, cfg.hostname, cfg.domainname, cfg.hostname );
    fclose( f );

    // First make backup of the config file
    // and then make new file actual
#ifdef BUSYBOX
    link( "/etc/sysconfig/hostname", "/etc/sysconfig/~hostname" );
    rename( "/etc/sysconfig/hostname2", "/etc/sysconfig/hostname" );

    link( "/etc/network/interfaces", "/etc/network/~interfaces" );
    rename( "/etc/network/interfaces2", "/etc/network/interfaces" );
#else
    link( "/etc/sysconfig/network", "/etc/sysconfig/~network" );
    rename( "/etc/sysconfig/network2", "/etc/sysconfig/network" );

    link( "/etc/sysconfig/network-scripts/ifcfg-eth0", "/etc/sysconfig/network-scripts/~ifcfg-eth0" );
    rename( "/etc/sysconfig/network-scripts/if2cfg-eth0", "/etc/sysconfig/network-scripts/ifcfg-eth0" );
#endif

    link( "/etc/resolv.conf", "/etc/~resolv.conf" );
    rename( "/etc/resolv.conf2", "/etc/resolv.conf" );

    link( "/etc/hosts", "/etc/~hosts" );
    rename( "/etc/hosts2", "/etc/hosts" );

    if ( isAlbaMode( "VR" ) )
    {
        link( "/etc/sysconfig/albatross/smb.conf", "/etc/sysconfig/albatross/~smb.conf" );
        rename( "/etc/sysconfig/albatross/smb.conf2", "/etc/sysconfig/albatross/smb.conf" );
    }

    ///////////////////////////////////////////////////////////////////////////

    strcpy( buffer, "" );
    parseAssignments( 0, NULL, "/etc/sysconfig/webs", "PORT", buffer );
    char* new_value = websGetVar( wp, "http_port", "80" );
    if ( strcmp( buffer, new_value ) != 0 )
    {
        trace( 2, "Saving sysconfig webs\n" );
        f = fopen( "/etc/sysconfig/webs", "w" );
        if ( f )
        {
            fprintf( f, "PORT=%s\n", new_value );
            fclose( f );
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    getAlbaXinetdAlbatrossPort( buffer, sizeof( buffer ) );
    new_value = websGetVar( wp, "d4oip_port", "7077" );
    if ( strcmp( buffer, new_value ) != 0 )
    {
        trace( 2, "Saving xinetd albatross\n" );
        f = fopen( "/etc/xinetd.d/albatross", "w" );
        if ( f )
        {
            fprintf( f,
                "service albatross\n"
                "{\n"
                "        disable         = no\n"
                "        type            = UNLISTED\n"
                "        port            = %s\n"
                "        flags           = REUSE\n"
                "        socket_type     = stream\n"
                "        wait            = no\n"
                "        user            = root\n"
                "        server          = /albatross/bin/idod\n"
                "        log_on_failure  += USERID\n"
                "}\n",
                new_value
                );

            fclose( f );
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    char cmd[ 256 ];

    // Set hostname (always) -- because we have changed /etc/hosts !
    //
    sprintf( cmd, "%s.%s", cfg.hostname, cfg.domainname );
    sprintf( cmd, "hostname %s.%s", cfg.hostname, cfg.domainname );
    system( cmd );

    if ( apply ) // spawn network restart
    {
        if ( strcmp( cfg.bootproto, "dhcp" ) == 0 )
            strcpy( cfg.gateway, "" );
        sprintf( cmd, "restartNetwork %s &", cfg.gateway );
        system( cmd );
    }

    ejSetResult( eid, "1" );

    return 0;   
}

/******************************************************************************/
static int changeAdminPassword(int eid, webs_t wp, int argc, char_t **argv)
{
    char* pwd;
    char* oldpwd;
    char* newpwd;

    if ( ejArgs(argc, argv, T("%s %s"), &oldpwd, &newpwd) < 2 ) 
         return 1;

    umOpen ();

    pwd = umGetUserPassword( T("system") );
    if ( pwd )
    {
         if ( strcmp( pwd, oldpwd ) == 0 
             && 0 == umSetUserPassword( T("system"), newpwd ) 
             )
         {
             umCommit( NULL );
             ejSetResult( eid, "1" );
             return 0;
         }
    }
    
    ejSetResult( eid, "0" );
    return 0;
}

/******************************************************************************/
static int commitUM(int eid, webs_t wp, int argc, char_t **argv)
{
    char_t * commit;
    if ( ejArgs(argc, argv, T("%s "), &commit) >= 1 ) {
        if( strlen( commit ) > 0 && strcmp( commit, "0" ) == 0 ) {
            if( umRestore( NULL ) != 0 )
                ejSetResult( eid, "-1" );
            else
                ejSetResult( eid, "0" );
        }
        else if( umCommit( NULL ) != 0 )
            ejSetResult( eid, "-1" );
        else
            ejSetResult( eid, "0" );

    }
    return 0;
}

/******************************************************************************/
// Executes system command with popen
//
// first argument: command
// second argument (optional): shown before the first row
// third argument (optional):  shown before the second row
// foruth argument (optional): shown after the last row
//
// Second and third arguments make possible to make highlighted
// first row like a header e.g. with <b> and </b>.
// Second and fourth arguments make possible to surround 
// command results with e.g. <pre> and </pre>
//
static int cgiExec( int eid, webs_t wp, int argc, char_t** argv )
{
    int lineno = 0;
    char buffer[ 512 ];
    FILE* fp = NULL;
    char* command;
    char* preHTML;
    char* headHTML;
    char* postHTML;
    char* eolHTML;
    
    ejSetResult( eid, "" );

    argc = ejArgs(argc, argv, T("%s %s %s %s %s"), &command, &preHTML, &headHTML, &postHTML, &eolHTML );
    if ( argc < 1 )
        return 1;

    if ( ( fp = popen( command, "r" ) ) == NULL)
    {
        return 1;
    }

    for( lineno = 0; fgets( buffer, sizeof( buffer), fp ); lineno++ )
    {
        if ( lineno == 0 && argc >= 2 )
             websWrite( wp, "%s", preHTML );

        websWriteBlock(wp, T(buffer), strlen(buffer) );

        if ( argc >= 5 )
             websWrite( wp, "%s", eolHTML );

        if ( lineno == 0 && argc >= 3 )
             websWrite( wp, "%s", headHTML );
    }

    if ( argc >= 4 )
        websWrite( wp, "%s", postHTML );

    pclose( fp );

    ejSetResult( eid, "0" );

    return 0;
}

static int getCmdOut( int eid, webs_t wp, int argc, char_t** argv )
{
    char buffer[ 512 ];
    char* command;
    
    ejSetResult( eid, "" );

    argc = ejArgs(argc, argv, T("%s"), &command );
    if ( argc < 1 )
        return 1;

    FILE* fp = popen( command, "r" );
    if ( fp == NULL )
    {
        return 1;
    }

    int i = 0;
    for( ; i < sizeof(buffer) ; )
    {
        int rc = fread( buffer + i, 1, sizeof(buffer) - i - 1, fp );
        if ( rc <= 0 )
            break;
        i += rc;
    }
    buffer[i] = 0;

    pclose( fp );

    char* to = buffer;
    char* from = buffer;
    while ( *from )
    {
        if ( ! isspace( *from ) )
            *to++ = *from++;
        else
        {
            *to++ = ' ';
            while( *from && isspace( *from ) )
                from++;
            }
        }

    ejSetResult( eid, buffer );

    return 0;
}
/******************************************************************************/
static int showTabBeg( int eid, webs_t wp, int argc, char_t** argv )
{
    char_t* title;

    if ( ejArgs( argc, argv, T("%s"), &title ) < 1 )
        return 1;

    // websSetVar( wp, T("TABSHEAD"), title );

    websWrite( wp,
    "<table width=100%% bgcolor=#FFFFFF height=18 border=0 cellspacing=0 cellpadding=0 >"
    "<thead class=title>%s<font style='font-size:3px'><br>&nbsp;</font></thead>"
        "<tr>",
    title
    );

    return 0;
}

/******************************************************************************/
static int showTabEnd( int eid, webs_t wp, int argc, char_t** argv )
{
    char* help = websGetVar( wp, T("CXHELP"), NULL );
    int tabCount = 0;

    if ( ejArgs( argc, argv, T("%d"), &tabCount ) < 1 )
        return 1;

    int colspan = tabCount * 3 + 2;

    websWrite( wp,
            "<td width=100%% align=middle valign=center>&nbsp</td><td>&nbsp;</td>"
            "</tr>"
        "<tr>"
            "<td colspan=%d height=6 valign=top>"
            "<IMG src=/images/under.gif align=top height=2 border=0 width=100%%>"
            "</td>"
        "</tr>"
        "<tr>"
            "<td colspan=%d class=title align=left>"
            "<b>%s</b>"
            "</td>"
            "<td align=right valign=bottom>",
        colspan, colspan - 1,
        websGetVar( wp, "TITLE", "" )
        );

    if ( help )
    {
        websWrite( wp,
            "<input type=image src='/images/help.gif' onclick='cxhelp(\"%s\")'>",
            help );
    }

    websWrite( wp, "</td></tr></table><hr>" );

    return 0;
}

/******************************************************************************/
// show tab item 
//
// first argument: id
// second argument: verbose
// third argument: URL prefix

//
static int showTab( int eid, webs_t wp, int argc, char_t** argv )
{
    char_t* sel;
    char_t* url;
    char_t* tabName;
    char_t* title;
    char_t* help = NULL;
    int isHighlighted = 0;

    /* trigger an ASP error if argc is less then we expect */
    if ( ejArgs( argc, argv, T("%s %s %s %s %s"), &sel, &url, &tabName, &title, &help ) < 4 )
        return 1;

    // if sel == "*" then tab is haighlighted if title <> ""
    // else if sel <> "" then tab is highlighted if current == id
    //
    isHighlighted = ( sel[0] == '*' ) 
    ? ( title[0] != 0 )
    : ( strcmp( sel, tabName ) == 0 ); 

    if ( isHighlighted )
        websSetVar( wp, T("TITLE"), title );

    if ( isHighlighted && help ) 
        websSetVar( wp, T("CXHELP"), help );

    websWrite( wp, 
        "<td bgcolor='%s' align=left valign=top>"
        "<IMG src=/images/leftTab.gif border=0>"
        "</td>",
        isHighlighted ? "#000080" : "#FFFFFF" 
        ); 
    
    websWrite( wp, 
        "<td bgcolor='%s' align=middle valign=center nowrap"
        " background=/images/cenTab.gif>"
        "<A HREF='%s%s' class=menutop style='%stext-decoration:none;'>"
        "%s</a>"
        "</td>",
        isHighlighted ? "#000080" : "#FFFFFF",
        url, sel[0] == '*' ? "" : tabName, 
        isHighlighted ? "color=white;" : "",
        tabName
        );

    websWrite( wp, 
        "<td bgcolor='%s' align=left valign=top>"
        "<IMG src=/images/rightTab.gif border=0>"
        "</td>",
        isHighlighted ? "#000080" : "#FFFFFF" 
        ); 

    return 0;
}

/******************************************************************************/
// setDate
//
// first argument: YYYY-mm-dd HH:MM:SS
//
static int setDate( int eid, webs_t wp, int argc, char_t** argv )
{
    int rc = 0;
    int y = 0, m = 0, d = 0, H = 0, M = 0, S = 0;
    char cmd[ 80 ];

    if ( argc < 1 )
        return 1;

    rc = sscanf( argv[ 0 ], "%d-%d-%d %d:%d:%d", &y, &m, &d, &H, &M, &S );
    if ( rc < 5 )
        return 1;

    sprintf( cmd, "%02d%02d%02d%02d%04d.%02d", m, d, H, M, y, S );

    ejSetResult( eid, cmd );

    return 0;
}

/************************** Code for Initialization ***************************/

// Defines all available ASP functions

void albaDefineFunctions(void)
{
    websSetRealm( "IPTC-DEX28" );

    parseAssignments( 0, NULL, "/etc/sysconfig/albatross/global", "MODEL", ALBA_MODEL );
    parseAssignments( 0, NULL, "/etc/sysconfig/albatross/global", "PORTS", ALBA_PORTS );
    parseAssignments( 0, NULL, "/etc/sysconfig/albatross/global", "SERIALNO", ALBA_SERIALNO );
    parseAssignments( 0, NULL, "/etc/sysconfig/albatross/global", "UUID", ALBA_UUID );
    parseAssignments( 0, NULL, "/etc/sysconfig/albatross/global", "PLATFORM", ALBA_PLATFORM );

    sscanf( ALBA_PORTS, "%d", &albaPortCount );

    websAspDefine(T("getQueryVal"), getQueryVal);
    websAspDefine(T("isUndef"), isUndef);

    websAspDefine(T("getUpTime"), getUpTime);
    websAspDefine(T("getAlbaModel"), getAlbaModel);
    websAspDefine(T("getAlbaPorts"), getAlbaPorts);
    websAspDefine(T("getAlbaSerialNo"), getAlbaSerialNo);
    websAspDefine(T("getAlbaUUID"), getAlbaUUID);
    websAspDefine(T("getAlbaPlatform"), getAlbaPlatform);
    websAspDefine(T("getAlbaIpAddress"), getAlbaIpAddress);
    websAspDefine(T("getAlbaMacAddress"), getAlbaMacAddress);
    websAspDefine(T("getAlbaD4oIPPort"), getAlbaD4oIPPort );

    websAspDefine(T("isAlbaMode"), isAlbaModeWebs);
    websAspDefine(T("saveAlbaMode"), saveAlbaModeWebs);

    websAspDefine(T("changeAdminPassword"), changeAdminPassword );
    websAspDefine(T("commitUM"), commitUM);

    websAspDefine(T("parseSysconfig"), parseSysconfig );
    websAspDefine(T("parseSavedNetworkConfig"), parseSavedNetworkConfig );
    websAspDefine(T("parseRunningNetworkConfig"), parseRunningNetworkConfig );
    websAspDefine(T("saveNetworkConfig"), saveNetworkConfig );

    websAspDefine(T("cgiExec"), cgiExec);
    websAspDefine(T("getCmdOut"), getCmdOut);

    websAspDefine(T("showTabBeg"), showTabBeg);
    websAspDefine(T("showTab"), showTab);
    websAspDefine(T("showTabEnd"), showTabEnd);

    websAspDefine(T("saveToFile"), saveToFile );

    websAspDefine(T("setDate"), setDate );
}

/******************************************************************************/
