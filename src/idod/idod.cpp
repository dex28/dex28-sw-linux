#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/select.h>

#include <sys/ioctl.h>
#include "../hpi/hpi.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h> // IPTOS_*

#include "MD5.h"
#include "../c54setp/c54setp.h"

using namespace std;

int debug = 1;
int D4oIPver = 0x0100; // V1.0 default
char* dsp_cfg = "/etc/sysconfig/albatross/dsp.cfg";

class MonotonicTimer
{
    timespec start;

    public: MonotonicTimer ()
    {
        Restart ();
        }

    public: void Restart ()
    {
        clock_gettime( CLOCK_MONOTONIC, &start );
        }

    public: long Elapsed ()
    {
        timespec end;
        clock_gettime( CLOCK_MONOTONIC, &end );

        long seconds  = end.tv_sec  - start.tv_sec; 
        long nseconds = end.tv_nsec - start.tv_nsec; 
     
        return long( ( seconds * 1e3 + nseconds / 1e6 ) + 0.5 ); 
        }
    };

static inline int read_stream( int fd, unsigned char* buf, int len )
{
    int readc = 0;

    while( readc < len )
    {
        int rc = read( fd, buf + readc, len - readc );
        if ( rc < 0 )
            return rc;
        if ( rc == 0 )
            break;
        readc += rc;
        }

    return readc;
    }

static inline int read_stream_CRLF( int fd, char* buf, int len )
{
    int readc = 0;

    while( readc < len )
    {
        int rc = read( fd, buf + readc, 1 );
        if ( rc < 0 )
        {
            buf[ readc ] = 0; // terminate string
            return rc;
            }
        if ( rc == 0 )
            break;
        // asert( rc == 1 );
        ++readc;

        if ( buf[ readc - 1 ] == '\n' ) // if current is LF
            if ( readc >= 2 && buf[ readc - 2 ] == '\r' ) // if CR is before
            {
                // found CR LF
                buf[ readc - 2 ] = 0; // relpace CR LF with '\0';
                readc -= 2;
                break;
                }
        }

    buf[ readc ] = 0; // terminate string
    return readc;
    }


// Generate random challenge
//
void get_challenge( char* challenge )
{
    FILE* f = fopen( "/dev/urandom", "rb" );
    if ( ! f )
    {
        fprintf( stderr, "Severe error: /dev/urandom could not be read\n" );
        exit( -1 );
        }

    unsigned char rnd[ 16 ];
    fread( rnd, 1, sizeof( rnd ), f );
    fclose( f );

    MD5 md5ctx;
    md5ctx.Init ();
    md5ctx.Update( rnd, sizeof( rnd ) );

    md5ctx.FinalHex( challenge );
    }

// Authenticate user based on username and its response
// which is challenge encrypted hashed with users password
//
bool authenticate( char* challenge, char* username, char* gotResponse )
{
    char uid[ 64 ];
    char password[ 64 ];

    // Find user in users.cfg file

    FILE* f = fopen( "/etc/sysconfig/albatross/users.cfg", "r" );
    char line[ 256 ];
    while( fgets( line, sizeof( line ), f ) )
    {
        if ( 1 == sscanf( line, "NAME=%s", uid )  // NAME=
            && strcmp( uid, username ) == 0
            )
        {
            fgets( line, sizeof( line ), f ); // PASS=
            sscanf( line, "PASS=%s", password );

            MD5 md5ctx;
            md5ctx.Init ();
            md5ctx.Update( challenge, 32 );
            md5ctx.Update( password, strlen( password ) );
            char challengeResp[ 64 ];
            md5ctx.FinalHex( challengeResp );

            return strcmp( challengeResp, gotResponse ) == 0;
            }
        }

    return false;
    }

// Authorize user for the usage of DN.
//
bool authorize( char* username, char* DN, char* mode )
{
    return true;
    }

int binary_mode( char* remoteIpAddr, unsigned short remoteUdpPort, char* DN, char* uid, char* mode )
{
    int portID = -1;

    PORT_PARAMETERS params;
    params.LOCAL_UDP = 15000;
    if ( DN[ 0 ] == '#' || DN[ 0 ] == '*' || DN[ 0 ] == '.' ) // Abs port.
    {
        sscanf( DN + 1, "%d", &portID ); // PORT number embedded in DN
        --portID; // go to zero based port index

        if ( ! params.Load( DN + 1, /* DN search */ false  ) || params.USE_DEFAULTS )
        {
            params.Load( "default" );
            params.LOCAL_UDP += portID;
            }
        }
    else // search by DN that must be found !
    {
        if ( ! params.Load( DN, /* DN search */ true ) )
        {
            printf( "ERROR Invalid directory number %s\r\n", DN ); fflush( stdout );
            return 1;
            }

        sscanf( params.PORT, "%d", &portID ); // we have found PORT number
        --portID; // go to zero based port index

        if ( params.USE_DEFAULTS )
        {
            params.Load( "default" );
            params.LOCAL_UDP += portID;
            }
        }

    if ( portID < 0 )
    {
        printf( "ERROR Invalid directory number %s\r\n", DN ); fflush( stdout );
        return 1;
        }

    DSPCFG dspCfg;
    if ( ! dspCfg.Find( portID, dsp_cfg ) )
    {
        printf( "ERROR Invalid channel #%d for DN %s\r\n", portID, DN ); fflush( stdout );
        return 1;
        }

    int channel = dspCfg.channel;
    char dspboard[ 256 ];
    strcpy( dspboard, dspCfg.devname );
    strcat( dspboard, "io" );

    if ( ! dspCfg.isRunning () )
    {
        printf( "ERROR DSP is not running for DN %s\r\n", DN ); fflush( stdout );
        return 1;
        }

    fprintf( stderr, "Local UDP: %d, DSCP UDP %d, TCP %d\n",
        params.LOCAL_UDP, params.DSCP_UDP, params.DSCP_TCP );

    // Open local UDP channel

    unsigned short localUdpPort = params.LOCAL_UDP;

    sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY; // OR: inet_addr( "interface" ); 
    local.sin_port = htons( localUdpPort ); // Port MUST be in Network Byte Order

    int inUdpSocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Open inbound UDP socket
    if ( inUdpSocket < 0 )
    {
        localUdpPort = 0;
        }
    else if ( bind( inUdpSocket, (sockaddr*)&local, sizeof( local ) ) != 0 )
    {
        // Inbound UDP port associated to channel is already used.
        // Say that we do not have local udp port, and allocate any free UDP used just as outbound.
        localUdpPort = 0;
        close( inUdpSocket );
        inUdpSocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ); // UDP socket just for sending
        }

    printf( "OK %hu\r\n", localUdpPort ); fflush( stdout );
/*
    remoteIpAddr = "10.0.1.20";
    remoteUdpPort = 15001;
*/
    sockaddr_in remote;
    remote.sin_family = AF_INET;
    inet_aton( remoteIpAddr, &remote.sin_addr );
    remote.sin_port = htons( remoteUdpPort );

    if ( params.DSCP_TCP )
    {
        setsockopt( 0, IPPROTO_IP, IP_TOS, &params.DSCP_TCP, sizeof( params.DSCP_TCP ) );
        setsockopt( 1, IPPROTO_IP, IP_TOS, &params.DSCP_TCP, sizeof( params.DSCP_TCP ) );
        }

    if ( params.DSCP_UDP )
    {
        if ( inUdpSocket >= 0 )
            setsockopt( inUdpSocket, IPPROTO_IP, IP_TOS, &params.DSCP_UDP, sizeof( params.DSCP_UDP ) );
        }

    int f_D = open( dspboard, O_RDWR );
    int f_B = open( dspboard, O_RDWR );

    unsigned char line[ 4096 ];

    sprintf( (char*)line, "%-2d %-14s %5hu %-8s %-8s %s V1.1/%d.%d",
        portID + 1, remoteIpAddr, remoteUdpPort, DN, uid, mode, 
        (D4oIPver >> 8 ) & 0xFF, D4oIPver & 0xFF  // remote protocol version
        );

    ::ioctl( f_D, IOCTL_HPI_SET_CHANNEL_INFO, 1 );
    ::write( f_D, (char*)line, strlen( (char*)line ) );

    ::ioctl( f_D, IOCTL_HPI_SET_D_CHANNEL, channel );
    ::ioctl( f_B, IOCTL_HPI_SET_B_CHANNEL, channel );

    // Send request for DASL power down/up cycle (BP patch)
    {
        static unsigned char PDU [] = { 0, 0x00, 0x51, 0x00 }; //  Set line down
        PDU[ 0 ] = channel;
        write( f_D, PDU, sizeof( PDU ) );
        }

    int maxfds = inUdpSocket;
    if ( f_D > maxfds )
        maxfds = f_D;
    if ( f_B > maxfds )
        maxfds = f_B;

    MonotonicTimer heartBeat;

    for (;;)
    {
        fd_set readfds;
        FD_ZERO( &readfds );
        FD_SET( 0, &readfds );
        FD_SET( f_D, &readfds );
        FD_SET( f_B, &readfds );
        if ( inUdpSocket >= 0 && localUdpPort != 0 )
            FD_SET( inUdpSocket, &readfds );

        timeval tvSelect;
        tvSelect.tv_sec = 0;
        tvSelect.tv_usec = 500000; // 500ms timeout

        int rc = select( maxfds + 1, &readfds, NULL, NULL, &tvSelect );
        if ( rc < 0 )
            break;

        // From D4oIP v1.1 there should be regular heartbeat
        //
        if ( D4oIPver >= 0x0101 )
        {
            long tElapsed = heartBeat.Elapsed ();
            if ( tElapsed >= 2000 ) // every 2 seconds
            {
                heartBeat.Restart ();
                write( 1, "\x00\x03\x00\x56\x00", 5 );
                }
            }

        if ( rc == 0 ) // timeout
            continue;

        ///////////////////////////////////////////////////////////////////////
        // B Channel from GW
        ///////////////////////////////////////////////////////////////////////
        //
        if ( FD_ISSET( f_B, &readfds ) )
        {
            int rd = read_stream( f_B, line, 2 );

            if ( 0 == rd )
                break; // EOF

            if ( 2 != rd )
                break;

            int len = ( line[ 0 ] << 8 )  + line[ 1 ];

            rd = read_stream( f_B, line, len );

            if ( 0 == rd )
                break; // EOF

            if ( len != rd )
                break;

            // first byte is DSP IO_B subchannel, skip it
            //
            if ( inUdpSocket >= 0 )
                sendto( inUdpSocket, (char*)line + 1, len - 1, 0, (sockaddr*)&remote, sizeof( remote ) );
            }

        ///////////////////////////////////////////////////////////////////////
        // B Channel from remote DTS
        ///////////////////////////////////////////////////////////////////////
        //
        if ( inUdpSocket >= 0 && FD_ISSET( inUdpSocket, &readfds ) )
        {
            sockaddr_in from;
            socklen_t fromlen = sizeof( from );
            int retval = recvfrom( inUdpSocket, (char*)line + 1, sizeof( line ), 0,
                (sockaddr*) &from, &fromlen );

            if ( retval >= 1 )
            {
                line[ 0 ] = channel;
                write( f_B, line, retval + 1 );
                }
            }

        ///////////////////////////////////////////////////////////////////////
        // D Channel from GW
        ///////////////////////////////////////////////////////////////////////
        //
        if ( FD_ISSET( f_D, &readfds ) )
        {
            int rd = read_stream( f_D, line, 2 );

            if ( 0 == rd )
                break; // EOF

            if ( 2 != rd )
                break;

            int len = ( line[ 0 ] << 8 ) + line[ 1 ];

            rd = read_stream( f_D, line + 1, len ); 
            // subchannel goes to line[1] and data from line[2]
            // we will put length in line[0] and line[1]

            if ( 0 == rd )
                break; // EOF

            if ( len != rd )
                break;

            len--; // Trim channel id

            line[ 0 ] = ( len >> 8 ) & 0xFF;
            line[ 1 ] = len & 0xFF;

            if ( line[ 2 ] == 0x00 && line[ 3 ] == 0x54 ) // Request to quit
            {
                fprintf( stderr, "Request to quit arrived\r\n" );
                break;
                }

            if ( debug > 2 )
            {
                fprintf( stderr, "From SSW:" );
                for ( int i = 0; i < len; i++ )
                    fprintf( stderr, " %02x", (int)line[ i + 2 ] );
                fprintf( stderr, "\n" );
                }

            write( 1, line, len + 2 );
            heartBeat.Restart ();
            }

        ///////////////////////////////////////////////////////////////////////
        // D Channel from remote DTS
        ///////////////////////////////////////////////////////////////////////
        //
        if ( FD_ISSET( 0, &readfds ) )
        {
            int rd = read_stream( 0, line, 2 );

            if ( 0 == rd )
                break; // EOF

            if ( 2 != rd )
                break;

            int len = ( line[ 0 ] << 8 )  + line[ 1 ];

            rd = read_stream( 0, line + 1, len );

            if ( 0 == rd )
                break; // EOF

            if ( len != rd )
                break;

            if ( debug > 2 )
            {
                fprintf( stderr, "From DTS:" );
                for ( int i = 0; i < len; i++ )
                    fprintf( stderr, " %02x", (int)line[ i + 1 ] );
                fprintf( stderr, "\n" );
                }

            line[ 0 ] = channel; // prefix packet with channel
            write( f_D, line, len + 1 );
            }
        }

    if ( inUdpSocket >= 0 )
        close( inUdpSocket );

    close( f_B );
    close( f_D );

    fprintf( stderr, "idod: completed.\r\n" );
    return 0;
    }

void hex_dump_mode( char* dspboard, int channel )
{
    int f_D = open( dspboard, O_RDWR );
    if ( f_D < 0 )
        return;

    unsigned char line[ 4096 ];
    printf( "Listening %s : %d\n", dspboard, channel );

    ::ioctl( f_D, IOCTL_HPI_SET_D_CHANNEL, channel );

    for (;;)
    {
        int rd = read_stream( f_D, line, 2 );

        if ( 0 == rd )
            break; // EOF

        if ( 2 != rd )
        {
            printf( "%s: Could not read packet length\n", dspboard );
            break;
            }

        int len = ( line[ 0 ] << 8 )  + line[ 1 ];

        rd = read_stream( f_D, line, len );

        if ( 0 == rd )
            break; // EOF

        if ( len != rd )
        {
            printf( "%s: Expected %d, read %d\n", dspboard, len, rd );
            break;
            }
/*
        printf( "%s:%d: ", dspboard, channel );
*/
        timeval tv;
        gettimeofday( &tv, NULL );
        tv.tv_sec %= 86400L;
        //tv.tv_sec -= 4 * 3600;
        //printf( line[ 1 ] ? "\033[32;1m" : "\033[37;1m" ); 
        printf( "%02lu:%02lu:%02lu.%03lu: %s", 
            tv.tv_sec / 3600, ( tv.tv_sec % 3600 ) / 60, tv.tv_sec % 60, // H:M:S
            tv.tv_usec / 1000, line[ 1 ] ? "DTS" : "SSW" );

        for ( int i = 2; i < len; i++ )
            printf( " %02x", int( line[ i ] ) );

        //printf( "\033[0m" ); 
        printf( "\n" ); 

        fflush( stdout );
        }

    printf( "%s: <EOF>\n", dspboard );
    }

int main( int argc, char** argv )
{
    if ( argc >= 3 )
    {
        hex_dump_mode( /*dspdev*/ argv[ 1 ], /*channel*/ strtol( argv[ 2 ], NULL, 10 ) );
        return 0;
        }

    char* remoteIpAddr = getenv( "REMOTE_HOST" );
    if ( ! remoteIpAddr )
    {
        remoteIpAddr = getenv( "TCPREMOTEIP" );
        if ( ! remoteIpAddr )
        {
            printf( "ERROR Remote address is not identified.\r\n" ); fflush( stdout );
            return 1;
            }
        }

    if ( debug )
    {
        close( 2 );
        open( "/dev/console", O_WRONLY );
        //open( "/dev/pts/0", O_WRONLY );
        }

    char challenge[ 80 ];
    get_challenge( challenge );
    printf( "HELLO %s 1.1\r\n", challenge ); fflush( stdout );

    char username[ 256 ] = "";
    char response[ 256 ] = "";
    unsigned short remoteUdpPort;
    char DN[ 256 ] = "";
    char mode[ 256 ] = "";
    int majorProtVer = 1;
    int minorProtVer = 0;

    char buffer[ 1024 ];
    read_stream_CRLF( 0, buffer, sizeof( buffer ) );

    int fieldc = sscanf( buffer, "HELLO %250s %250s %hu %250s %250s %d.%d", username, response, &remoteUdpPort, DN, mode, 
                        &majorProtVer, &minorProtVer );

    D4oIPver = ( majorProtVer << 8 ) | minorProtVer;

    if ( fieldc < 5 )
    {
        if ( strcmp( username, "webStat" ) == 0 )
        {
            execl( "/albatross/bin/webStat", "webStat", NULL );
            }

        printf( "ERROR Protocol error.\r\n" ); fflush( stdout );
        return 1;
        }

    {
        struct stat info;
        if ( 0 == stat( "/var/run/idod.lock", &info ) )
        {
            printf( "ERROR Device is configured as extension side.\r\n" ); fflush( stdout );
            return 1;
            }
        }

    if ( ! authenticate( challenge, username, response ) )
    {
        printf( "ERROR Authentication failed.\r\n" ); fflush( stdout );
        return 1;
        }

    if ( ! authorize( username, DN, mode ) )
    {
        printf( "ERROR Authorization failed.\r\n" ); fflush( stdout );
        return 1;
        }

    if ( debug )
    {
        fprintf( stderr, "idod: Authorized %s from %s for DN %s. Protocol V%d.%d\n", 
                 username, remoteIpAddr, DN, majorProtVer, minorProtVer );
        }

    return binary_mode( remoteIpAddr, remoteUdpPort, DN, username, mode );
    }
