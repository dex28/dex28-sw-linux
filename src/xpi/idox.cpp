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

int cardid = 0x80;
int ackid = 0x40;

static bool asciiDump = false;

class SendQueue
{
    struct msg
    {
        unsigned char data[ 32 ];
        int len;
        msg* next;
        };

    msg* first;
    msg* last;

public:

    SendQueue( void )
    {
        first = NULL;
        last = NULL;
        }

    void PutUrgent( unsigned char* data, int len )
    {
        if ( ! first )
        {
            first = last = new msg;
            first->next = NULL;
            }
        else
        {
            msg* old = first;
            first = new msg;
            first->next = old;
            }

        memcpy( last->data, data, len );
        last->len = len;
        }

    void PutDelay( int ms )
    {
        if ( ! last )
        {
            first = last = new msg;
            last->next = NULL;
            }
        else
        {
            last->next = new msg;
            last = last->next;
            last->next = NULL;
            }

        last->len = - ms; // negative len == delay in ms
        }

    void Put( unsigned char* data, int len )
    {
        if ( ! last )
        {
            first = last = new msg;
            last->next = NULL;
            }
        else
        {
            last->next = new msg;
            last = last->next;
            last->next = NULL;
            }

        memcpy( last->data, data, len );
        last->len = len;
        }

    bool IsEmpty( void ) const
    {
        return first == NULL;
        }

    int Get( unsigned char* data )
    {
        if ( first == NULL )
            return 0;

        int len = first->len;
        if ( len > 0 )
            memcpy( data, first->data, len );

        msg* old = first;

        first = first->next;
        if ( ! first )
            last = NULL;

        delete old;
        return len;
        }
    };

class XPI
{
    enum L1_STATE
    {
        L1_DISABLED,
        L1_IDLE,
        L1_RECEIVE_CTX,
        L1_RECEIVE_CRX,
        L1_POLL_EIRQ,
        L1_WAIT_CTX_ACK,
        L1_WAIT_CRX_ACK
        };

    enum L2_STATE
    {
        L2_READY,
        L2_WAIT_INITRESP
        };
    
    int minboardc;
    int maxboardc;
    bool isMCPU;

    L1_STATE l1_state;
    int l1_timer;
    bool isEIRQ;
    int eirq_pollc;

    int ctx_count;
    int ctx_cksum;
    unsigned char ctx[ 1024 ];

    int crx_count;
    int crx_cksum;
    unsigned char crx[ 1024 ];

    L2_STATE l2_state;
    int l2_timer;
    SendQueue sendQ;

    int f_D;
    char timestamp[ 128 ];
    bool confirm_send;

    void Goto( L2_STATE newl2_state, int timeout = -1 )
    {
        l2_state = newl2_state;
        l2_timer = timeout < 0 ? -1 : timeout / 20; // timeout in ms
        }

    void Goto( L1_STATE newl1_state, int timeout = -1 )
    {
        //printf( "State %d -> %d\n", l1_state, newl1_state );
        l1_state = newl1_state;
        l1_timer = timeout < 0 ? -1 : timeout / 20; // timeout in ms
        }

    void getTimestamp( void )
    {
        timeval tv;
        gettimeofday( &tv, NULL );
        struct tm* tm = localtime( &tv.tv_sec );
#if 0
        sprintf( timestamp, "%04d-%02d-%02d %02d:%02d:%02d.%03lu",
            1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            tv.tv_usec / 1000 // millisec
            );
#else
        sprintf( timestamp, "%02d:%02d:%02d.%03lu",
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            tv.tv_usec / 1000 // millisec
            );
#endif
        }

public:

    XPI( void )
    {
        l1_state = L1_IDLE;
        l1_timer = -1;
        isEIRQ = false;

        ctx_count = 0;
        ctx_cksum = 0xFF;
        crx_count = 0;
        crx_cksum = 0xFF;

        l2_state = L2_READY;
        l2_timer = -1;

        minboardc = 0;
        maxboardc = 1;
        isMCPU = false;
        f_D = -1;
        strcpy( timestamp, "" );

        confirm_send = false;
        }

    bool Initialize( bool coldStart = false )
    {
        f_D = open( "/dev/xpi", O_RDWR );

        if ( f_D < 0 )
        {
            fprintf( stderr, "Failed to open /dev/xpi\n" );
            return false;
            }

        ioctl( f_D, IOCTL_XPI_SET_CHANNEL, 0 ); // Select XPI channel

        isMCPU = ioctl( f_D, IOCTL_XPI_GET_MCPU_FLAG, 0 ); // Is acting as CPU-D?

        getTimestamp ();
        if ( isMCPU )
            printf( "%s IDOX: Acting as CPU-D\n", timestamp );
        else
            printf( "%s IDOX: Acting as sniffer\n", timestamp );

        if ( coldStart )
        {
            int bPos = 0x00;

            ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( bPos << 2 ) | 0x0 ); // off and reset all

            usleep( 4000 ); // 4 ms

            ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( bPos << 2 ) | 0x1 ); // on

            usleep( 4000 ); // 4 ms

            ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( bPos << 2 ) | 0x3 ); // is installed?

            int rc = ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( bPos << 2 ) | 0x2 ); // is on and installed?

            if ( rc == 1 )
            {
                Goto( L2_WAIT_INITRESP, 3000 ); // 3 s
                Goto( L1_DISABLED );
                }
            else
            {
                Goto( L2_READY );
                Goto( L1_IDLE );
                }
            }
        }

    void Send( unsigned char* src, int len )
    {
        unsigned char data[ 32 ];
        memcpy( data, src, len );

        for( ;; )
        {
            int cksum = 0xFF;

            for ( int i = 0; i < len; i++ )
            {
                cksum ^= data[ i ];
                cksum <<= 1;

                if ( cksum & 0x100 )
                {
                    cksum &= 0xFF;
                    cksum |= 0x01;
                    }
                }

            data[ len ] = cksum;

            sendQ.Put( data, len + 1 ); // put into output queue data + CKS

            if ( ( data[ 0 ] & 0xE0 ) != 0xC0 )
                break;

            data[ 0 ] |= 0x20; // turn on bit D5
            // Send the same packet second time
            }
        }

    void On_L2_Timeout( void )
    {
        if ( ! isMCPU )
            return;

        if ( l2_state == L2_WAIT_INITRESP )
        {
            unsigned char data [] = { 0xc1, 0x04, 0x50, 0x01, 0x0f, 0x01 };
            Send( data, sizeof( data ) );
            Goto( L2_WAIT_INITRESP, 1000 ); // same state, timeout 1 s
            Goto( L1_IDLE );
            }
        }

    void On_L1_Timeout( void )
    {
        if ( ! isMCPU )
            return;

        if ( l1_state == L1_DISABLED )
        {
            getTimestamp ();
            printf( "%s Continuing...\n", timestamp );
            Goto( L1_IDLE );
            }
        else if ( l1_state == L1_IDLE )
        {
            }
        else if ( l1_state == L1_RECEIVE_CTX )
        {
            // abort frame
            crx_count = 0;
            Goto( L1_IDLE );
            }
        else if ( l1_state == L1_RECEIVE_CRX )
        {
            // abort frame
            ctx_count = 0;
            Goto( L1_IDLE );
            }
        else if ( l1_state == L1_POLL_EIRQ )
        {
            if ( eirq_pollc == -1 )
                eirq_pollc = minboardc;
            else
                ++eirq_pollc;

            if ( eirq_pollc >= maxboardc )
            {
                // Abort polling
                // Note: this is severe -- all boards should be reset!
                Goto( L1_IDLE );
                }
            else
            {
                unsigned char pollid = eirq_pollc;
                write( f_D, &pollid, 1 );
                Goto( L1_POLL_EIRQ, 100 );
                // note: eirq_pollc is autoincremented
                }
            }
        }

    void CTX_SendNextFrame( void )
    {
        if ( sendQ.IsEmpty () )
            return;

        unsigned char data[ 32 ];
        int len = sendQ.Get( data );

        if ( len < 0 ) // negative length == delay in ms
        {
            int delay = -len;
            getTimestamp ();
            printf( "%s Delay %d ms\n", timestamp, delay );
            Goto( L1_DISABLED, delay );
            return;
            }

        if ( confirm_send )
        {
            printf( "Send" ); 
                for ( int i = 0; i < len; i++ ) printf( " %02x", data[i] );
            printf( " ?" );
            char x[256]; fgets( x,sizeof(x),stdin );
            }

        write( f_D, data, len );

        ctx_count = 0;
        ctx_cksum = 0xFF;
        Goto( L1_RECEIVE_CTX, 100 );
        //usleep( 3000 );
        }

    void OnUserInput( const char* str )
    {
        if ( ! isMCPU )
            return;

        if ( str[0] == '-' )
        {
            switch( str[1] )
            {
                case '1': // Is board installed?
                    for ( int i = minboardc; i < maxboardc; i++ )
                        ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( i << 2 ) | 0x2 ); // is on and installed?
                    break;

                case '2': // Turn off and reset all boards
                    for ( int i = minboardc; i < maxboardc; i++ )
                    {
                        ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( i << 2 ) | 0x0 ); // off and reset all
                        usleep( 4000 );
                        }
                    break;

                case '3': // Turn on all boards
                    for ( int i = minboardc; i < maxboardc; i++ )
                    {
                        ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( i << 2 ) | 0x1 ); // on
                        usleep( 4000 );
                        }
                    break;

                case '4': // Is board on and installed?
                    for ( int i = minboardc; i < maxboardc; i++ )
                        ioctl( f_D, IOCTL_XPI_FC_COMMAND, ( i << 2 ) | 0x3 ); // is installed?
                    break;

                }
            }
        else if ( str[0] == '+' )
        {
            confirm_send = true;
            }
        else if ( isspace( str[0] ) )
        {
            unsigned char data[ 32 ];
            int datac;

            for ( datac = 0; datac < 17; datac++ )
            {
                int chpos = 3 * datac;
                if ( strncmp( str + chpos, "   ", 3 ) == 0 )
                    break;
                int v;
                if ( 1 != sscanf( str + chpos, " %x", &v ) )
                    break;
                data[ datac ] = v;
                }

            if ( datac && data[0] != 0xC0 && data[0] != ackid )
            {
                Send( data, datac );
                }
            }
        else
        {
            char fname[ 256 ];
            sscanf( str, " %s", fname );
            char path[ 256 ];
            sprintf( path, "t/%s", fname );
            SendBatch( path );
            }
        }

    void SendBatch( const char* fname )
    {
        FILE* logf = fopen( fname, "r" );
        if ( ! logf )
            return;

        getTimestamp ();
        printf( "%s Loading batch %s\n", timestamp, fname );

        unsigned char data[ 256 ];
        int datac = 0;

        char line[ 256 ];

        while ( fgets( line, sizeof( line ), logf ) )
        {
            int p1 = 0, p2 = 0, p3 = 0;

            if ( 1 == sscanf( line, " WAIT %d\n", &p1 ) && p1 > 0 )
            {
                sendQ.PutDelay( p1 );
                continue;
                }

            if ( strncmp( line + 13, "SC: CPU", 7 ) != 0 ) 
                continue;

            for ( datac = 0; datac < 18; datac++ )
            {
                int chpos = 22 + 3 * datac;
                if ( strncmp( line + chpos, "   ", 3 ) == 0 )
                    break;
                int v;
                if ( 1 != sscanf( line + chpos, " %x", &v ) )
                    break;
                data[ datac ] = v;
                }

            if ( datac && data[0] != 0xC0 && data[0] != ackid )
            {
                Send( data, datac - 1 );
                }
            }

        fclose( logf );
        }

    void CTX_Dump( const char* comment = NULL ) 
    {
        getTimestamp ();

        printf( "%s SC: CPU:", timestamp );

        for ( int i = 0; i < ctx_count; i++ )
            printf( " %02x", (int)ctx[i] );

        if ( asciiDump )
        {
            for ( int i = ctx_count; i < 18; i++ )
                printf( "   " );

            printf( "  " );

            if ( comment )
                printf( "%s", comment );
            else
            {
                for ( int i = 2; i < ctx_count - 1; i++ )
                    printf( "%c", isprint( ctx[i] ) ? ctx[i] : '.' );
                }
            }

        printf( "\n" );
        fflush( stdout );
        }

    void CTX_EndOfFrame ()
    {
        ctx_count = 0; 
        ctx_cksum = 0xFF;
    
        if ( ctx[0] == cardid ) // wait ACK
        {
            Goto( L1_WAIT_CRX_ACK, 100 );
            }
        else if ( isEIRQ )
        {
            Goto( L1_POLL_EIRQ, 100 );
            eirq_pollc = -1;
            write( f_D, "\xC0", 1 );
            }
        else
        {
            Goto( L1_IDLE );
            }
        }

    void CRX_Dump( const char* comment = NULL ) 
    {
        getTimestamp ();

        printf( "%s SC: DEV:", timestamp );

        for ( int i = 0; i < crx_count; i++ )
            printf( " %02x", (int)crx[i] );

        if ( asciiDump )
        {
            for ( int i = crx_count; i < 18; i++ )
                printf( "   " );

            printf( "  " );

            if ( comment )
                printf( "%s", comment );
            else
            {
                for ( int i = 2; i < crx_count - 1; i++ )
                    printf( "%c", isprint( crx[i] ) ? crx[i] : '.' );
                }
            }

        printf( "\n" );
        fflush( stdout );
        }

    void CRX_EndOfFrame( void )
    {
        if ( crx_cksum == 0 && crx[ 0 ] == cardid )
        {
            if ( isMCPU ) // send ACK
            {
                unsigned char data[1] = { ackid };
                write( f_D, data, 1 );
                }

            Goto( L1_WAIT_CTX_ACK, 100 );

            unsigned char chk1 [] = { cardid, 0x05, 0x50, 0x11, 0x02, 0x36, 0x05 };

            if ( l2_state == L2_WAIT_INITRESP && memcmp( chk1, crx, sizeof( chk1 ) ) == 0 )
            {
                Goto( L2_READY, -1 );
                unsigned char data [] = { 0xc1, 0x02, 0x40, 0x00 };
                Send( data, sizeof( data ) );
                FILE* inif = fopen( "idox.ini", "r" );
                if ( inif )
                {
                    char fname[ 256 ];
                    while( 1 == fscanf( inif, " %s", fname ) )
                        SendBatch( fname );
                    fclose( inif );
                    }
                }
            } 
        else
        {
            Goto( L1_IDLE );
            }

        crx_count = 0;
        crx_cksum = 0xFF;
        }

    void On_FC( void )
    {
        unsigned char fc[2];
        read( f_D, fc, 2 ); // Two octets

        int fc_cmd = fc[0];
        int fc_sense = fc[1];

        getTimestamp ();

        printf( "%s FC: CPU: A=0x%02x, D=%x%x -> SENSE=%x\n",
            timestamp, ( fc_cmd >> 2) & 0x3F, 
            ( fc_cmd >> 1 ) & 0x01, ( fc_cmd >> 0 ) & 0x01,
            ( fc_sense >> 0 ) & 0x01 );

        fflush( stdout );
        }

    void On_CTX( void )
    {
        if ( l1_state == L1_IDLE ) // begin frame
        {
            ctx_count = 0;
            ctx_cksum = 0xFF;
            }

        unsigned char octet;
        read( f_D, &octet, 1 ); // Read one octet

        if ( l1_state == L1_IDLE || l1_state == L1_RECEIVE_CTX )
        {
            if ( l1_state == L1_IDLE )
                Goto( L1_RECEIVE_CTX, 100 );

            ctx[ ctx_count ] = octet;

            ctx_cksum ^= ctx[ ctx_count ];
            ctx_cksum <<= 1;
            if ( ctx_cksum & 0x100 )
            {
                ctx_cksum &= 0xFF;
                ctx_cksum |= 0x01;
                }
    
            ++ctx_count;

            if ( ctx_count >= 2 
                && ctx_count == 3 + ( ctx[ 1 ] & 0x0F ) ) // got frame
            {
                CTX_Dump( ctx_cksum != 0 ? "[Bad CKS]": NULL );
                CTX_EndOfFrame ();
                }
            else if ( ctx_count >= 18 ) // frame overflow
            {
                CTX_Dump( "Size overflow; Aborted." );
                CTX_EndOfFrame ();
                }
            }
        else if ( l1_state == L1_WAIT_CTX_ACK && octet == ackid )
        {
            getTimestamp ();
            printf( "%s SC: CPU: %02x\n", timestamp, (int)octet );

            if ( isEIRQ )
            {
                Goto( L1_POLL_EIRQ, 100 );
                eirq_pollc = -1;
                write( f_D, "\xC0", 1 );
                }
            else
            {
                Goto( L1_IDLE );
                }
            }
        else if ( l1_state == L1_POLL_EIRQ && eirq_pollc == -1 && octet == 0xC0 )
        {
            // printf( " [C0] " );
            }
        else if ( l1_state == L1_POLL_EIRQ && eirq_pollc == octet )
        {
            // printf( " [%02x] ", octet );
            }
        else // ignore octet
        {
            getTimestamp ();
            printf( "%s SC: CPU: Ignored %02x [State = %d]\n", timestamp, (int)octet, l1_state );
            }
        }

    void On_CRX( void )
    {
        if ( l1_state == L1_IDLE || l1_state == L1_POLL_EIRQ ) // begin frame
        {
            crx_count = 0;
            crx_cksum = 0xFF;
            }

        unsigned char octet;
        read( f_D, &octet, 1 ); // Read one octet

        if ( l1_state == L1_IDLE || l1_state == L1_RECEIVE_CRX || l1_state == L1_POLL_EIRQ )
        {
            if ( l1_state == L1_IDLE || l1_state == L1_POLL_EIRQ );
                Goto( L1_RECEIVE_CRX, 100 );

            crx[ crx_count ] = octet;

            crx_cksum ^= crx[ crx_count ];
            crx_cksum <<= 1;
            if ( crx_cksum & 0x100 )
            {
                crx_cksum &= 0xFF;
                crx_cksum |= 0x01;
                }
    
            ++crx_count;

            if ( crx_count >= 2 
                && crx_count == 3 + ( crx[ 1 ] & 0x0F ) ) // Got frame
            {
                CRX_Dump( crx_cksum != 0 ? "[Bad CKS]": NULL );
                CRX_EndOfFrame ();
                }
            else if ( crx_count >= 18 ) // Overflow
            {
                CRX_Dump( "Size overflow; Aborted." );
                CRX_EndOfFrame ();
                }
            }
        else if ( l1_state == L1_WAIT_CRX_ACK && octet == ackid )
        {
            getTimestamp ();
            printf( "%s SC: DEV: %02x\n", timestamp, (int)octet );

            if ( isEIRQ )
            {
                Goto( L1_POLL_EIRQ, 100 );
                eirq_pollc = -1;
                write( f_D, "\xC0", 1 );
                }
            else
            {
                Goto( L1_IDLE );
                }
            }
        else
        {
            // ignore octet
            getTimestamp ();
            printf( "%s SC: DEV: Ignored %02x [State = %d]\n", timestamp, (int)octet, l1_state );
            }
        }

    void On_EIRQ( void )
    {
        unsigned char eirq_stat;
        read( f_D, &eirq_stat, 1 ); // Read one octet
        isEIRQ = eirq_stat;

        if ( ctx_count > 0 )
        {
            getTimestamp ();
            printf( "%s SC: DEV:", timestamp );

            for ( int i = 0; i < ctx_count; i++ )
                printf( "   " );

            printf( "| EIRQ %s\n", isEIRQ ? "On" : "Off" );

            fflush( stdout );
            }
        else
        {
            // printf( "[EIRQ %s]", isEIRQ ? "On" : "Off" );
            }


        if ( ! isMCPU )
            return;

        if ( l1_state == L1_IDLE && isEIRQ )
        {
            Goto( L1_POLL_EIRQ, 100 );
            eirq_pollc = -1;
            write( f_D, "\xC0", 1 );
            }
        }

    void MainLoop( void )
    {
        int maxfds = 0;
        if ( f_D > maxfds )
            maxfds = f_D;

        for (;;)
        {
            if ( isMCPU && l1_state == L1_IDLE )
                CTX_SendNextFrame ();

            fd_set readfds;
            FD_ZERO( &readfds );
            FD_SET( f_D, &readfds );

            if ( l1_state != L1_DISABLED )
                FD_SET( 0, &readfds );

            timeval tvSelect;
            tvSelect.tv_sec = 0;
            tvSelect.tv_usec = 20000; // 20ms

            int rc = select( maxfds + 1, &readfds, NULL, NULL, &tvSelect );
            if ( rc < 0 )
                break; // Error

            if ( rc == 0 )
            {
                if ( l1_timer >= 0 && --l1_timer <= 0 )
                {
                    l1_timer = -1;
                    On_L1_Timeout ();
                    }
                if ( l2_timer >= 0 && --l2_timer <= 0 )
                {
                    l2_timer = -1;
                    On_L2_Timeout ();
                    }
                }

            if ( FD_ISSET( 0, &readfds ) )
            {
                char str[ 256 ];
                memset( str, 0, sizeof( str ) );
                if ( fgets( str, sizeof( str ), stdin ) )
                    OnUserInput( str );
                }

            if ( FD_ISSET( f_D, &readfds ) )
            {
                unsigned char code;
                int rd = read( f_D, &code, 1 );

                if ( 0 == rd )
                    break; // EOF

                switch( code )
                {
                    case 0xF0: // FC, two octets
                        On_FC ();
                        break;
                        
                    case 0xF1: // CTX, one octet
                        On_CTX ();
                        break;

                    case 0xF2: // CRX, one octet
                        On_CRX ();
                        break;

                    case 0xF3: // EIRQ, no octets
                        On_EIRQ ();
                        break;
                    }
                }
            }
        }
    };

XPI xpi;

int main( int argc, char** argv )
{
    bool coldStart = false;

    for ( int i = 1; i < argc; i++ )
    {
        if ( strcmp( argv[i], "-r" ) == 0 ) 
            coldStart = true;
        else if ( strcmp( argv[i], "-a" ) == 0 ) 
            asciiDump = true;
        }

    if ( ! xpi.Initialize( coldStart ) )
        return -1;

    for ( int i = 1; i < argc; i++ )
    {
        if ( argv[i][0] != '-' )
            xpi.SendBatch( argv[i] );
        }

    xpi.MainLoop ();

    return 0;
    }

void operator delete [] (void *p)
{
    free( p );
    }

void operator delete (void *p)
{
    free( p );
    }

void* operator new (size_t t)
{
    void* p = calloc( 1, t );
    return p;
    }

void* operator new[] (size_t t)
{
    void* p = calloc( 1, t );
    return p;
    }
