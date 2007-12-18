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

#include <signal.h>

#include "../c54setp/c54setp.h"

using namespace std;

volatile bool running = true;

void sigterm_handler( int )
{
    fprintf( stderr, "Quiting...\n" );
    running = false;
    }

void set_trace( char* dspboard, int channel, int value )
{
    int fd = open( dspboard, O_RDWR );
    if ( fd < 0 )
    {
        fprintf( stderr, "Failed to open %s.\n", dspboard );
        return;
        }

    unsigned char line[ 8 ];
    line[ 0 ] = 0x01; // B-Channel IOCTL
    line[ 1 ] = channel; // Channel
    line[ 2 ] = 0x04; // trace on/off
    line[ 3 ] = value;
    write( fd, line, 4 );

    close( fd );
    }

int main( int argc, char** argv )
{
    if ( argc < 3 )
    {
        fprintf( stderr, "usage: %s <dspdev> <ch>\n", argv[ 0 ] );
        return 1;
        }

    signal( SIGTERM, sigterm_handler );
    signal( SIGQUIT, sigterm_handler );
    signal( SIGINT,  sigterm_handler );

    char* dspboard = argv[ 1 ];
    int channel = atol( argv[ 2 ] );

    int f_B = open( dspboard, O_RDWR );
    if ( f_B < 0 )
        return 1;

    unsigned char line[ 4096 ];

    ::ioctl( f_B, IOCTL_HPI_SET_B_CHANNEL, 0x80 | channel );

    set_trace( dspboard, channel, 1 );

    while( running )
    {
        fd_set readfds;
        FD_ZERO( &readfds );
        FD_SET( f_B, &readfds );

        timeval tvSelect;
        tvSelect.tv_sec = 0;
        tvSelect.tv_usec = 100000; // 100ms timeout

        int rc = select( f_B + 1, &readfds, NULL, NULL, &tvSelect );
        if ( rc == 0 )
        {
            continue; // nothing selected
            }
        else if ( rc < 0 )
        {
            continue; // error or signal arrived
            }

        ///////////////////////////////////////////////////////////////////////
        // B Channel from GW
        ///////////////////////////////////////////////////////////////////////
        //
        if ( FD_ISSET( f_B, &readfds ) )
        {
            int rd = read( f_B, line, 2 );

            if ( 0 == rd )
                break; // EOF

            if ( 2 != rd )
            {
                fprintf( stderr, "%s: Could not read packet length\n", dspboard );
                break;
                }

            int len = ( line[ 0 ] << 8 )  + line[ 1 ];

            rd = read( f_B, line, len );

            if ( 0 == rd )
                break; // EOF

            if ( len != rd )
            {
                fprintf( stderr, "%s: Expected %d, read %d\n", dspboard, len, rd );
                break;
                }

            len -= 2;
            unsigned char* b = line + 2;

            // MSB-LSB -> Intel LSB-MSB PCM
            //
            for ( int i = 0; i < len; i += 2 )
            {
                int t = b[ i ]; b[ i ] = b[ i + 1 ]; b[ i + 1 ] = t;
                }

            // Write Intel PCM Stereo
            //
            fwrite( b, 1, len, stdout );
            }
        }

    fprintf( stderr, "Main loop done.\n" );

    fflush( stdout );

    set_trace( dspboard, channel, 0 );

    close( f_B );

    return 0;
    }
