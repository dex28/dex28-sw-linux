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
#include "xpi.h"

#if 0
#define CLR_CPU "\033[37m"
#define CLR_DEV "\033[32m"
#define CLR_OFF "\033[30m"
#else
#define CLR_CPU ""
#define CLR_DEV ""
#define CLR_OFF ""
#endif

static void getTimestamp( char* timestamp )
{
    timeval tv;
    gettimeofday( &tv, NULL );
    struct tm* tm = localtime( &tv.tv_sec );

    sprintf( timestamp, "%04d-%02d-%02d %02d-%02d-%02d.%03lu",
        1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec,
        tv.tv_usec / 1000 // millisec
        );
    }

class XPI
{
    enum STATE
    {
        IDLE,
        SENT_C0,
        SENT_00,
        SENT_01,
        WAIT_41
        };
    
    STATE state;
    int timer;

    FILE* logf;
    bool isMCPU;
    int maxboardc;
    int f_D;

    bool isEIRQ;

    bool force_rx_header;
    bool force_tx_header;
    int ctx_count;
    int ctx_cksum;
    unsigned char ctx[ 1024 ];
    int crx_count;
    int crx_cksum;
    unsigned char crx[ 1024 ];
    char timestamp[ 128 ];

public:

    XPI( void )
    {
        state = IDLE;
        timer = -1;

        logf = NULL;
        isMCPU = false;
        maxboardc = 2;
        f_D = -1;
        isEIRQ = false;

        force_rx_header = true;
        force_tx_header = true;
        ctx_count = 0;
        ctx_cksum = 0xFF;
        crx_count = 0;
        crx_cksum = 0xFF;
        strcpy( timestamp, "" );
        }

    bool Initialize( void )
    {
        f_D = open( "/dev/xpi", O_RDWR );

        if ( f_D < 0 )
            return false;

        ioctl( f_D, IOCTL_XPI_SET_CHANNEL, 0 ); // Select XPI channel

        isMCPU = ioctl( f_D, IOCTL_XPI_GET_MCPU_FLAG, 0 ); // Is acting as CPU-D?

        if ( isMCPU )
            printf( "IDOX / Acting as CPU-D\n" );
        else
            printf( "IDOX / Acting as sniffer\n" );
        }

    void OnTimeout( void )
    {
        if ( ! isMCPU )
            return;

        if ( state == WAIT_41 )
        {
            state = WAIT_41;
            timer = 5;
            }
        else if ( state == SENT_C0 )
        {
            write( f_D, "\x00", 1 );

            state = SENT_00;
            timer = 5;
            }
        else if ( state == SENT_00 )
        {
            write( f_D, "\x01", 1 );

            state = SENT_01;
            timer = 5;
            }
        else if ( state == IDLE )
        {
            if ( isEIRQ )
            {
                write( f_D, "\xC0", 1 );
                state = SENT_C0;
                timer = 5;
                }
            }
        }

    void ReplayNextLine( void )
    {
        if ( ! logf )
            return;

        char line[ 256 ];

        while ( fgets( line, sizeof( line ), logf ) )
        {
            if ( strncmp( line + 24, "SC", 2 ) != 0 ) 
                continue;
            if ( strncmp( line + 28, "CPU", 2 ) != 0 ) 
                continue;

            unsigned char data[ 256 ];
            int datac = 0;
            int v;
            while( 1 == sscanf( line + 33 + 3*datac, " %x", &v ) && datac < 18 )
                data[ datac++ ] = v;
            if ( datac && data[0] != 0xc0 && data[0] != 0x41 )
            {
                write( f_D, data, datac );

                if ( data[0] == 0x81 )
                {
                    state = WAIT_41;
                    timer = 5;
                    }

                break;
                }
            }

        if ( feof( logf ) )
        {
            printf( "\n---------------- File reply done.\n" );
            fclose( logf );
            logf = NULL;
            }
        }

    void OnUserInput( const char* str )
    {
        if ( ! isMCPU )
            return;

        if ( str[0] == '-' )
        {
            switch( str[1] )
            {
                case '1': // Turn off and reset all boards
                    for ( int i = 0; i < maxboardc; i++ )
                    {
                        ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( i << 2 ) | 0x0 ); // off and reset all
                        usleep( 4000 );
                        }
                    break;

                case '2': // Turn on all boards
                    for ( int i = 0; i < maxboardc; i++ )
                    {
                        ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( i << 2 ) | 0x1 ); // on
                        usleep( 4000 );
                        }
                    break;

                case '3': // Which boards are installed?
                    for ( int i = 0; i < maxboardc; i++ )
                        ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( i << 2 ) | 0x3 ); // is installed?
                    break;

                case '4': // Which boards are on and installed?
                    for ( int i = 0; i < maxboardc; i++ )
                        ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( i << 2 ) | 0x2 ); // is on and installed?
                    break;
                }
            }
        else if ( str[0] == '\'' )
        {
            if ( logf )
                fclose( logf );

            char fname[ 256 ];
            sscanf( str + 1, " %s", fname );

            logf = fopen( fname, "r" );
            if ( logf )
                printf( "\n------------------ Replaying %s\n", fname );

            ReplayNextLine ();
            }
        }

    void OnReceived_FC( void )
    {
        unsigned char fc[2];
        read( f_D, fc, 2 ); // Two octets

        int fc_cmd = fc[0];
        int fc_sense = fc[1];

        printf( CLR_CPU "\n%s FC: CPU: A=0x%02x, D=%x%x -> SENSE=%x",
            timestamp, ( fc_cmd >> 2) & 0x3F, 
            ( fc_cmd >> 1 ) & 0x01, ( fc_cmd >> 0 ) & 0x01,
            ( fc_sense >> 0 ) & 0x01 );
        }

    void OnReceived_CTX( void )
    {
        if ( force_tx_header )
        {
            force_tx_header = false;
            force_rx_header = true;
            ctx_count = 0;
            ctx_cksum = 0xFF;
            printf( CLR_CPU "\n%s SC: CPU:", timestamp );
            }

        read( f_D, ctx + ctx_count, 1 ); // Read one octet

        ctx_cksum ^= ctx[ ctx_count ];
        ctx_cksum <<= 1;
        if ( ctx_cksum & 0x100 )
        {
            ctx_cksum &= 0xFF;
            ctx_cksum |= 0x01;
            }

        printf( " %02x", (int)ctx[ ctx_count ] );

        ++ctx_count;

        if ( ctx_count == 1 && ctx[ 0 ] == 0x41 ) // ACK
        {
            force_tx_header = true;

            if ( isMCPU && state == IDLE )
                ReplayNextLine ();
            }
        else if ( ctx_count >= 2 && ctx[ 0 ] != 0xC0 ) 
        {
            if ( ctx_count == 3 + ( ctx[ 1 ] & 0x0F ) ) // check length
            {
                for ( int i = ctx_count; i < 18; i++ )
                    printf( "   " );
                printf( "  " );
                for ( int i = 2; i < ctx_count - 1; i++ )
                    printf( "%c", isprint( ctx[i] ) ? ctx[i] : '.' );

                if ( ctx_cksum != 0 )
                    printf( "; Bad CKS [%02x]", ctx_cksum ); // print cksum
                force_tx_header = true;

                if ( isMCPU && state == IDLE )
                    ReplayNextLine ();
                }
            }
        }

    void OnReceived_CRX( void )
    {
        if ( force_rx_header )
        {
            force_rx_header = false;
            force_tx_header = true;
            crx_count = 0;
            crx_cksum = 0xFF;
            printf( CLR_DEV "\n%s SC: DEV:", timestamp );
            }

        read( f_D, crx + crx_count, 1 ); // Read one octet

        crx_cksum ^= crx[ crx_count ];
        crx_cksum <<= 1;
        if ( crx_cksum & 0x100 )
        {
            crx_cksum &= 0xFF;
            crx_cksum |= 0x01;
            }

        printf( " %02x", (int)crx[ crx_count ] );

        ++ crx_count;

        if ( crx_count == 1 && crx[ 0 ] == 0x41 ) // ACK
        {
            state = IDLE;
            timer = -1;

            force_rx_header = true;
            }
        else if ( crx_count >= 2 && crx[ 0 ] != 0xC0 ) 
        {
            if ( crx_count == 3 + ( crx[ 1 ] & 0x0F ) ) // check length
            {
                for ( int i = crx_count; i < 18; i++ )
                    printf( "   " );
                printf( "  " );
                for ( int i = 2; i < crx_count - 1; i++ )
                    printf( "%c", isprint( crx[i] ) ? crx[i] : '.' );

                if ( crx_cksum != 0 )
                    printf( "; Bad CKS [%02x]", crx_cksum ); // print cksum
                force_rx_header = true;

                if ( state == IDLE )
                    write( f_D, "\x41", 1 ); // Send ACK
                }
            }
        }

    void OnReceived_EIRQ( void )
    {
        unsigned char eirq_stat;
        read( f_D, &eirq_stat, 1 ); // Read one octet
        isEIRQ = eirq_stat;

        if ( ! isMCPU )
            return;

        printf( CLR_DEV "\n%s SC: DEV: EIRQ %s", timestamp, eirq_stat ? "On" : "Off" );
        }

    void MainLoop( void )
    {
        int maxfds = 0;
        if ( f_D > maxfds )
            maxfds = f_D;
/*
        OnUserInput( "-1" );
        OnUserInput( "-2" );
        OnUserInput( "-3" );
        OnUserInput( "-4" );
*/

        for (;;)
        {
            fd_set readfds;
            FD_ZERO( &readfds );
            FD_SET( f_D, &readfds );
            FD_SET( 0, &readfds );

            timeval tvSelect;
            tvSelect.tv_sec = 0;
            tvSelect.tv_usec = 20000; // 20ms

            int rc = select( maxfds + 1, &readfds, NULL, NULL, &tvSelect );
            if ( rc < 0 )
                break; // Error

            if ( rc == 0 )
            {
                //printf( "timer %d\n", timer );
                if ( timer >= 0 && --timer <= 0 )
                {
                    timer = -1;
                    OnTimeout ();
                    }
                }

            if ( FD_ISSET( 0, &readfds ) )
            {
                char str[ 256 ];
                if ( fgets( str, sizeof( str ), stdin ) )
                    OnUserInput( str );
                }

            if ( FD_ISSET( f_D, &readfds ) )
            {
                getTimestamp( timestamp );

                unsigned char code;
                int rd = read( f_D, &code, 1 );

                if ( 0 == rd )
                    break; // EOF

                switch( code )
                {
                    case 0xF0: // FC, two octets
                        OnReceived_FC ();
                        break;
                        
                    case 0xF1: // CTX, one octet
                        OnReceived_CTX ();
                        break;

                    case 0xF2: // CRX, one octet
                        OnReceived_CRX ();
                        break;

                    case 0xF3: // EIRQ, no octets
                        OnReceived_EIRQ ();
                        break;
                    }
                }

            fflush( stdout );
            }
        }
    };

XPI xpi;

int main( int argc, char** argv )
{
    if ( ! xpi.Initialize () )
        return -1;

    xpi.MainLoop ();

    return 0;
    }
