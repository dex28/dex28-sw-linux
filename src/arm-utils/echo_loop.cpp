#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/select.h>
#include <termios.h>

int main( int argc, char** argv )
{
    char teststr[] = "LOOPTEST";

    tcflush( 2, TCOFLUSH );
    write( 2, teststr, sizeof( teststr ) - 1 );
    write( 2, "\r\n", 2 );

    char incoming[ 5000 ];
    unsigned chp = 0;

    for ( int tocnt = 0; tocnt < 7; )
    {
        fd_set readfds;
        FD_ZERO( &readfds );
        FD_SET( 0, &readfds );

        timeval tvSelect;
        tvSelect.tv_sec = 0;
        tvSelect.tv_usec = 2000000;

        int rc = select( 1, &readfds, NULL, NULL, &tvSelect );
        if ( rc < 0 )
            break;
        else if ( rc == 0 )
        {
            tcflush( 2, TCOFLUSH );
            write( 2, teststr, sizeof( teststr ) - 1 );
            write( 2, "\r\n", 2 );

            printf( "GOT: Timeout\n" ); fflush( stdout );
            ++tocnt;
            continue;
            }

        int len = read( 0, incoming + chp, sizeof(incoming)-chp );
        if ( len <= 0 )
            break;
        chp += len;
        incoming[ chp ] = 0;

        printf( "GOT: %d bytes\n", len );
        fflush( stdout );

        if ( strstr( incoming, teststr ) )
        {
            printf( "GOT: %s\n", teststr ); 
            fflush( stdout );
            tcflush( 2, TCOFLUSH );
            return 0;
            }
        }

    printf( "FAILED\n" ); fflush( stdout );
    tcflush( 2, TCOFLUSH );
    return 0;
    }
