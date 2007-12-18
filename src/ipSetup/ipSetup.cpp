#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#ifdef CONFIG_READLINE

#include <readline/readline.h>
#include <readline/history.h>

#else

#include <string.h>

char* readline( char* prompt )
{
    printf( "%s", prompt );

    char* buf = (char*)calloc( 256, 1 );

    fgets( buf, 255, stdin );
    if ( strlen( buf ) > 0 )
        buf[ strlen( buf ) - 1] = 0;

    return buf;
    }

#endif

void editv( char* prompt, char* value )
{
    for ( ;; )
    {
        char buffer[ 256 ];
        sprintf( buffer, "%s [%s]: ", prompt, value );
        char* newValue = readline( buffer );
        if ( newValue == NULL )
        {
            strcpy( value, "" );
            printf( "\n" );
            }
        else
        {
            if ( *newValue == '\0' )
                return;

            if ( newValue[ 0 ] == ' ' )
                strcpy( value, "" );
            else
                strcpy( value, newValue );
            free( newValue );
            }
        }
    }

static int parseAssignments( char* filename, char* singleVar = NULL, char* value = NULL );

static void inline getEnv( char* dest, char* name, char* default_value = "" )
{
    char* value = getenv( name );
    if ( value ) strcpy( dest, value );
    else if ( default_value ) strcpy( dest, default_value );
    else strcpy( dest, "" );
    }

struct Config
{
    char run_serialno[ 64 ];
    char run_uuid[ 128 ];
    char run_hostname[ 128 ];
    char run_domainname[ 128 ];
    char run_addr[ 64 ];
    char run_mask[ 64 ];
    char run_bcast[ 64 ];
    char run_gateway[ 64 ];
    char run_sws_mode[ 64 ];

    char hostname[ 128 ];
    char domainname[ 64 ];
    char ipaddress[ 64 ];
    char broadcast[ 64 ];
    char netmask[ 64 ];
    char gateway[ 64 ];
    char nameserver1[ 64 ];
    char nameserver2[ 64 ];
    char bootproto[ 64 ];

    char old_hostname[ 128 ];
    };

static void get_running_config( struct Config* cfg )
{
    strcpy( cfg->run_serialno, "(n/a)" );
    strcpy( cfg->run_uuid, "(n/a)" );

    strcpy( cfg->run_hostname, "(n/a)" );
    strcpy( cfg->run_domainname, "(n/a)" );

    strcpy( cfg->run_addr, "(Unknown)" );
    strcpy( cfg->run_mask, "(Unknown)" );
    strcpy( cfg->run_bcast, "(Unknown)" );
    strcpy( cfg->run_gateway, "(Unknown)" );
    strcpy( cfg->run_sws_mode, "(Unknown)" );

    FILE* f;

    f = popen( "grep UUID /etc/sysconfig/albatross/global", "r" );
    if ( f )
    {
        fscanf( f, "UUID=%s", cfg->run_uuid );
        pclose( f );
        }

    f = popen( "grep SERIALNO /etc/sysconfig/albatross/global", "r" );
    if ( f )
    {
        fscanf( f, "SERIALNO=%s", cfg->run_serialno );
        pclose( f );
        }

    f = popen( "hostname -s", "r" );
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
            if ( 3 == sscanf( buffer, " inet addr:%s Bcast:%s Mask:%s", cfg->run_addr, cfg->run_bcast, cfg->run_mask ) )
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

    f = fopen( "/etc/sysconfig/albatross/mode", "r" );
    if ( f )
    {
        fscanf( f, "MODE=%s", cfg->run_sws_mode );
        fclose( f );

        if ( strcmp( cfg->run_sws_mode, "EXTENDER" ) == 0 )
            strcpy( cfg->run_sws_mode, "ext" );
        else if ( strcmp( cfg->run_sws_mode, "GATEWAY" ) == 0 )
            strcpy( cfg->run_sws_mode, "sws" );
        else if ( strcmp( cfg->run_sws_mode, "VR" ) == 0 )
            strcpy( cfg->run_sws_mode, "vr" );
        }

    fflush( stdout );
    }

static void show_running_config( struct Config* cfg )
{
    get_running_config( cfg );

    printf( "------------------------------------------------------------------------------\n" );
    printf( "///                       Running Configuration                            ///\n" );
    printf( "------------------------------------------------------------------------------\n" );
    printf( "Serial No:    %s\n", cfg->run_serialno );
    printf( "UUID:         %s\n", cfg->run_uuid );
    printf( "SWS/EXT Mode: %s\n", cfg->run_sws_mode );
    printf( "Host Name:    %s\n", cfg->run_hostname );
    printf( "Domain Name:  %s\n", cfg->run_domainname );
    printf( "IP Address:   %s\n", cfg->run_addr );
    printf( "Netmask:      %s\n", cfg->run_mask );
    printf( "Broadcast:    %s\n", cfg->run_bcast );
    printf( "Gateway:      %s\n", cfg->run_gateway );
    printf( "------------------------------------------------------------------------------\n" );
    }

static void show_config( struct Config* cfg )
{
    printf( "------------------------------------------------------------------------------\n" );
    printf( "Configuration...: %-22s ( %s )\n", "SAVED", "RUNNING" );
    printf( "Host Name.......: %-22s ( %s )\n", cfg->hostname, cfg->run_hostname );
    printf( "Domain Name.....: %-22s ( %s )\n", cfg->domainname, cfg->run_domainname );
    if ( strcmp( cfg->bootproto, "static" ) == 0  )
    {
        printf( "IP Address......: %-22s ( %s )\n", cfg->ipaddress, cfg->run_addr );
        printf( "Netmask.........: %-22s ( %s )\n", cfg->netmask,   cfg->run_mask );
        printf( "Broadcast Addr..: %-22s ( %s )\n", cfg->broadcast, cfg->run_bcast );
        printf( "Default Gateway.: %-22s ( %s )\n", cfg->gateway, cfg->run_gateway );
        }
    else
    {
        printf( "IP Address......: %-22s ( %s )\n", "from DHCP", cfg->run_addr );
        printf( "Netmask.........: %-22s ( %s )\n", "from DHCP", cfg->run_mask ); 
        printf( "Broadcast Addr..: %-22s ( %s )\n", "from DHCP", cfg->run_bcast );
        printf( "Default Gateway.: %-22s ( %s )\n", "from DHCP", cfg->run_gateway );
        }
    printf( "Name Server 1...: %-22s ( %s )\n", cfg->nameserver1, cfg->nameserver1 );
    printf( "Name Server 2...: %-22s ( %s )\n", cfg->nameserver2, cfg->nameserver2 );
    printf( "------------------------------------------------------------------------------\n" );
    }

int main ()
{
    ///////////////////////////////////////////////////////////////////////////

    // Check if we are running on console ports?
    //
    if ( strcmp( ttyname( 0 ), "/dev/ttyS0" ) != 0
    && strcmp( ttyname( 0 ), "/dev/ttyS1" ) != 0 )
    {
        return 0;
        }

    char* username = getenv( "USER" );

    if ( username && strcmp( username, "shutdown" ) == 0 )
    {
        system( "shutdown -h now" );
        return 0;
        }

    if ( username && strcmp( username, "reboot" ) == 0 )
    {
        system( "reboot" );
        return 0;
        }

    ///////////////////////////////////////////////////////////////////////////

    struct Config cfg;
    memset( &cfg, 0, sizeof( cfg ) );
    strcpy( cfg.hostname,  "DEX28.iptc.local" );
    strcpy( cfg.domainname, "" );
    strcpy( cfg.bootproto, "" );

    show_running_config( &cfg ); // get & show running configuration

    ///////////////////////////////////////////////////////////////////////////

    char* ans = readline( "Reset Web Admin Password (y/N)? " );
    if ( ans && ( ans[ 0 ] == 'y' || ans[ 0 ] == 'Y' ) )
    {
        rename( "/albatross/bin/umconfig.def", "/albatross/bin/umconfig.txt" );
#ifdef BUSYBOX
        system( "service webs stop" );
        system( "service webs start" );
#endif
        printf( "Web Admin Pasword is reset to default.\n" );
        }
    else
    {
        printf( "Keeping existing password...\n" );
        }
    if ( ans ) free( ans );

    ///////////////////////////////////////////////////////////////////////////

#ifdef BUSYBOX
    ans = readline( "Do you want to reconfigure SWS/EXT mode (y/N)? " );
    if ( ans && ( ans[ 0 ] == 'y' || ans[ 0 ] == 'Y' ) )
    {
        char old_mode[ 64 ];
        strcpy( old_mode, cfg.run_sws_mode );

        editv( "SWS/EXT mode", cfg.run_sws_mode );

        for ( unsigned int i = 0; i < strlen( cfg.run_sws_mode ); i++ )
        {
            if ( islower( cfg.run_sws_mode[i] ) )
                cfg.run_sws_mode[i] = tolower( cfg.run_sws_mode[i] );
            }

        char cmd[ 256 ];
        sprintf( cmd, "fwChangeGender %s", cfg.run_sws_mode );
        system( cmd );
/*
        if ( strcasecmp( cfg.run_sws_mode, "EXT" ) == 0 )
            strcpy( cfg.run_sws_mode, "EXT" );
        else if ( strcasecmp( cfg.run_sws_mode, "SWS" ) == 0 )
            strcpy( cfg.run_sws_mode, "SWS" );
        else if ( strcasecmp( cfg.run_sws_mode, "VR" ) == 0 )
            strcpy( cfg.run_sws_mode, "VR" );
        else
            strcpy( cfg.run_sws_mode, old_mode ); // keep old mode if not recognizable

        if ( strcmp( old_mode, cfg.run_sws_mode ) != 0 ) // changed mode
        {
            printf( "Mode [%s]->[%s]\n", old_mode, cfg.run_sws_mode );

            if ( strcasecmp( cfg.run_sws_mode, "EXT" ) == 0 )
                system( "echo 'MODE=EXTENDER' > /etc/sysconfig/albatross/mode" );
            else if ( strcasecmp( cfg.run_sws_mode, "SWS" ) == 0 )
                system( "echo 'MODE=GATEWAY' > /etc/sysconfig/albatross/mode" );
            else if ( strcasecmp( cfg.run_sws_mode, "VR" ) == 0 )
                system( "echo 'MODE=VR' > /etc/sysconfig/albatross/mode" );

#ifdef BUSYBOX
            system( "service webs restart" );
            system( "service c54load restart" );
#endif
            show_running_config( &cfg ); // get & show running configuration
            }
        else
        {
            printf( "Keeping existing [%s] mode...\n", cfg.run_sws_mode );
            }
*/
        fflush( stdout );
        sleep( 2 );

        return 0;
        }
    if ( ans ) free( ans );
#endif

    ///////////////////////////////////////////////////////////////////////////
    FILE* f;

    /*
     *  Read network configuration
     */

#ifdef BUSYBOX
    parseAssignments( "/etc/sysconfig/hostname" );
    getEnv( cfg.hostname, "sysHOSTNAME" );
#else
    parseAssignments( "/etc/sysconfig/network" );
    getEnv( cfg.hostname, "sysHOSTNAME" );
    getEnv( cfg.gateway,  "sysGATEWAY"  );
#endif

    strcpy( cfg.old_hostname, cfg.hostname );

    ///////////////////////////////////////////////////////////////////////////

#ifdef BUSYBOX
    strcpy( cfg.bootproto, "" );

    f = fopen( "/etc/network/interfaces", "r" );
    if ( f )
    {
        int inside_eth0 = 0;

        char buffer[ 256 ];
        while( fgets( buffer, sizeof( buffer ), f ) )
        {
            char iface[ 64 ];
            char bootproto[ 64 ];
            if ( 2 == sscanf( buffer, " iface %s inet %s", iface, bootproto ) )
            {
                if ( strcmp( iface, "eth0" ) == 0 )
                {
                    strcpy( cfg.bootproto, bootproto );
                    inside_eth0 = 1;
                    }
                else
                {
                    inside_eth0 = 0;
                    }
                }

            if ( inside_eth0 )
            {
                if ( 1 == sscanf( buffer, " address %s", cfg.ipaddress ) )
                    ;
                else if ( 1 == sscanf( buffer, " netmask %s", cfg.netmask ) )
                    ;
                else if ( 1 == sscanf( buffer, " broadcast %s", cfg.broadcast ) )
                    ;
                else if ( 1 == sscanf( buffer, " gateway %s", cfg.gateway ) )
                    ;
                }
            }

        fclose( f );
        }
#else
    parseAssignments( "/etc/sysconfig/network-scripts/ifcfg-eth0" );
    getEnv( cfg.ipaddress, "sysIPADDR"    );
    getEnv( cfg.netmask,   "sysNETMASK"   );
    getEnv( cfg.broadcast, "sysBROADCAST" );
    getEnv( cfg.gateway,   "sysGATEWAY"   );
    getEnv( cfg.bootproto, "sysBOOTPROTO" );
    getEnv( cfg.hostname,  "sysHOSTNAME"  );
#endif

    if ( ! cfg.bootproto[0] )
        strcpy( cfg.bootproto, "dhcp" );

    ///////////////////////////////////////////////////////////////////////////

    f = fopen( "/etc/resolv.conf", "r" );
    if ( f )
    {
        char buffer[ 256 ];
        while( fgets( buffer, sizeof( buffer ), f ) )
        {
            if ( cfg.nameserver1[ 0 ] == '\0'
                    && 1 == sscanf( buffer, " nameserver %s", cfg.nameserver1 ) )
                ;
            else if ( cfg.nameserver1[ 0 ] != '\0' 
                    && 1 == sscanf( buffer, " nameserver %s", cfg.nameserver2 ) )
                ;
            else if ( 1 == sscanf( buffer, "domain %s", cfg.domainname ) )
                ;
            }
        fclose( f );
        }

    ///////////////////////////////////////////////////////////////////////////

    show_config( &cfg );

    ans = readline( "Do you want to change network settings (y/N)? " );
    if ( ! ans || ( ans[ 0 ] != 'y' && ans[ 0 ] != 'Y' ) )
    {
        return 1;
        }
    if ( ans ) free( ans );

    printf( "\n" );
    printf( "********************************************************\n" );
    printf( "* Your are about to change network parameters.         *\n" );
    printf( "* All IP addresses should be entered in dot notation.  *\n" );
    printf( "* To delete a value, press space end enter.            *\n" );
    printf( "* To keep existing value, just press enter.            *\n" );
    printf( "********************************************************\n" );
    printf( "\n" );

    editv( "Host Name", cfg.hostname );

    char* chp = strchr( cfg.hostname, '.' );
    if ( chp )
    {
        *chp++ = 0; // cfg.hostname will became short hostname
        if ( *chp ) strcpy( cfg.domainname, chp );
        }

    editv( "Domain Name", cfg.domainname );

    for ( ;; )
    {
        editv( "IP Settings (dhcp or static)", cfg.bootproto );
        if ( strcmp( cfg.bootproto, "dhcp" ) == 0 )
            break;
        else if ( strcmp( cfg.bootproto, "static" ) == 0 )
        {
            editv( "IP Address", cfg.ipaddress );
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

            editv( "IP Netmask", cfg.netmask );
            editv( "IP Broadcast Address", cfg.broadcast );
            editv( "Default Gateway", cfg.gateway );
            editv( "Primary DNS", cfg.nameserver1 );
            editv( "Secondary DNS", cfg.nameserver2 );
            break;
            }
        }

    if ( ! cfg.hostname[ 0 ] )
        strcpy( cfg.hostname, "DEX28" );

    if ( ! cfg.domainname[ 0 ] )
        strcpy( cfg.domainname, "localdomain" );

    ///////////////////////////////////////////////////////////////////////////

    ans = readline( "\nDo you want to save configuration (y/N)? " );
    if ( ! ans || ( ans[ 0 ] != 'y' && ans[ 0 ] != 'Y' ) )
        return 1;
    if ( ans ) free( ans );

    /*
     *  Save network configuration to files
     */

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

    f = fopen( "/etc/hosts2", "w" );
    if ( ! f )
        return 1;
    if ( strcmp( cfg.bootproto, "static" ) != 0 ) // DHCP
    {
        fprintf( f, "127.0.0.1 %s.%s %s localhost.localdomain localhost\n",
            cfg.hostname, cfg.domainname, cfg.hostname );
    }
    else // static
    {
        fprintf( f, "127.0.0.1 localhost.localdomain localhost\n" );
        fprintf( f, "%s %s.%s %s\n", cfg.ipaddress, cfg.hostname, cfg.domainname, cfg.hostname );
    }
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

    printf( "\nParameters are successfully saved.\n" );

    ///////////////////////////////////////////////////////////////////////////

    char cmd[ 256 ];

    // Set hostname (always) -- because we have changed /etc/hosts !
    //
    sprintf( cmd, "%s.%s", cfg.hostname, cfg.domainname );
    sprintf( cmd, "hostname %s.%s", cfg.hostname, cfg.domainname );
    system( cmd );

    ///////////////////////////////////////////////////////////////////////////

    ans = readline( "\nDo you want to apply new network configuration (y/N)? " );
    if ( ! ans || ( ans[ 0 ] != 'y' && ans[ 0 ] != 'Y' ) )
    {
        get_running_config( &cfg );
        show_config( &cfg );
        return 1;
        }
    if ( ans ) free( ans );

    sprintf( cmd, "restartNetwork %s", strcmp( cfg.bootproto, "dhcp" ) == 0 ? "" : cfg.gateway );
    system( cmd );

    printf( "\nParameters are successfully applied.\n" );

    get_running_config( &cfg );
    show_config( &cfg );
    fflush( stdout );
    sleep( 1 );

    return 0;
    }

// read file and parse values in the form A = B or A = "B"
// and asign values to EJS variables with name sysA values B

static int parseAssignments( char* filename, char* singleVar, char* value ) 
{
    char buffer[ 256 ];
    char szVar[ 256 ] = { "sys" };

    FILE* f = fopen( filename, "r" );
    if ( ! f )
         return 0;

    while( fgets( buffer, sizeof( buffer ), f ) )
    {
        char* chp = strchr( buffer, '=' );

        if ( chp )
        {
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

            // chp points to pure name A or empty string
            // (if A=B was actually just =B)
            //
            if ( *chp )
            {
                strcpy( szVar + 3, chp );

                // skip spaces on the left side of B
                while( *szValue && isspace( *szValue ) )
                    szValue++;

                // skip leading " if any
                if ( *szValue == '"' )
                    szValue++;

                // find end of B
                for( chp = szValue; *chp; chp++ )
                    {}

                // remove spaces from the right side of B
                for( chp--; chp >= szValue; chp-- )
                    if ( isspace( *chp ) )
                        *chp = 0;
                    else
                        break;

                // remove trailing " if any
                if ( *chp == '"' )
                    *chp = 0;

                if ( singleVar ) // set single global value
                {
                    if ( strcmp( singleVar, szVar ) == 0 )
                    {
                    	if ( value ) strcpy( value, szValue );
                        break; // no need for more variables to scan
                    }
                }
                else // set environment variable
                {
                    setenv( szVar, szValue, /*overwrite*/ 1 );
                }
            }
        }
    }

    fclose( f );
    return 0;
}
