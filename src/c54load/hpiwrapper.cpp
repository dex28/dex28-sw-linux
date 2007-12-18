/*
    Project:     Albatross / c54load
  
    Module:      hpiwrapper.cpp
  
    Description: Albatross c54xx COFF loader
                 HPI device c++ wrapper class implementation
  
    Copyright (c) 2002-2005 By Mikica B Kocic
    Copyright (c) 2002-2005 By IPTC Technology Communications AB
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "c54load.h"
#include "hpiwrapper.h"

Mutex stdoutMutex;

FILE* logfile = NULL;

void trace( int level, const char* format... )
{
    extern int opt_verbose;
    if ( opt_verbose < level )
        return;

    va_list marker;
    va_start( marker, format );

    stdoutMutex.Lock ();

    vprintf( format, marker );
    fflush( stdout );

    stdoutMutex.Unlock ();
    }

void logf( const char* format... )
{
    va_list marker;

    stdoutMutex.Lock ();

    va_start( marker, format );
    vprintf( format, marker );
    fflush( stdout );

    if ( logfile )
    {
        timeval tv;
        gettimeofday( &tv, NULL );
        tm* tm = localtime( &tv.tv_sec );

        fprintf( logfile, "%04d-%02d-%02d %02d:%02d:%02d.%03ld: ", 
            1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            tv.tv_usec / 1000 );

        va_start( marker, format );
        vfprintf( logfile, format, marker );
        fflush( logfile );
        }

    stdoutMutex.Unlock ();
    }

///////////////////////////////////////////////////////////////////////////////

volatile bool DSP_Monitor::fRunning = true;

volatile long DSP_Monitor::thread_count = 0;

volatile long DSP_Monitor::thread_started_count = 0;

Mutex DSP_Monitor::thread_count_mutex;

bool DSP_Monitor::Open( int id, const char* pdevname, const char* coffname, int abs_ch_b, int chn_cnt, const char* lockname )
{
    ID = id;

    reset_count = 0;

    dev = -1;
    dev_dbg = -1;

    strcpy( devname, pdevname );
    strcat( devname, "d" );
    dev_dbg = ::open( devname, O_RDONLY );

    strcpy( devname, pdevname );
    dev = ::open( devname, O_RDWR );

    if ( dev < 0 ) // failed to open controling device
    {
        ::close( dev_dbg );
        dev_dbg = -1;
        }

    strcpy( coff_filename, coffname );

    abs_channel_base = abs_ch_b;
    channelc = chn_cnt;
    for ( int i = 0; i < channelc; i++ )
        child_pid[ i ] = 0;

    strcpy( lock_filename, lockname );

    trace( 2, "%s: coff = %s, lock = %s, dev = %d, dbg = %d\n", devname, coffname, lockname, dev, dev_dbg );

    return isOpen ();
    }

void DSP_Monitor::Close( void )
{
    if ( dev_dbg >= 0 )
        ::close( dev_dbg );

    if ( dev >= 0 )
        ::close( dev );

    dev_dbg = -1;
    dev = -1;
    }

bool DSP_Monitor::ResetDSP( bool kill_debuggers )
{
    // Remove lock file
    //
    unlink( lock_filename );

    // Reset DSP
    //
    reset_count++;

    ::ioctl( dev, IOCTL_HPI_RESET_DSP, int( kill_debuggers ) );

    if ( kill_debuggers )
    {
        ::close( dev_dbg );
        dev_dbg = -1;

        char line[ 128 ];
        strcpy( line, devname );
        strcat( line, "d" );
        dev_dbg = ::open( line, O_RDONLY );
        }

    logf( "%s: DSP RESET; Reset count = %d; Dbg fd = %d\n", 
        devname, reset_count, dev_dbg );

    return dev >= 0 && dev_dbg >= 0;
    }

void DSP_Monitor::MemDump( unsigned long addr, long len )
{
    unsigned short data[ 1024 ];

    if ( len > 1024 )
        len = 1024;

    ::lseek( dev, addr, SEEK_SET );
    ::read( dev, data, len * sizeof(unsigned short) );

    for ( long row = 0; row < len; row += 8, addr += 8 )
    {
        stdoutMutex.Lock ();

        printf( "%s: %08lx : ", devname, addr );

        for ( long col = 0; row + col < len && col < 8; col++ )
            printf( "  %04hx", data[ row + col ] );

        printf( "\n" );

        stdoutMutex.Unlock ();
        }
    }

bool DSP_Monitor::MemWrite
(
    void* buffer, int page, unsigned long addr, int nbytes
    )
{
    /////////////////////////////////////////////////////////////////
    // FIXME: Change CPU address to XHPIA address accordingly to page
    /////////////////////////////////////////////////////////////////
    //
    if ( addr >= 0x8000 )
        addr |= 0x10000;

    // Write data to DSP memory
    //
    ::lseek( dev, addr, SEEK_SET );
    ::write( dev, buffer, nbytes );
#if 0
    stdoutMutex.Lock ();

    printf( "MemWrite: page %x, addr %lx, words %d\n", page, addr, nbytes/2 );

    unsigned short* buf = (unsigned short*) buffer;
    for ( int i = 0; i < 16; i++ )
        printf( "%04x%s", buf[ i ], i % 16 == 15 ? "\n" : " " );

    printf( "\n" );

    stdoutMutex.Unlock ();

    ::lseek( dev, addr, SEEK_SET );
    ::read( dev, buffer, 32 );

    stdoutMutex.Lock ();

    printf( "MemRead:  page %x, addr %lx, words %d\n", page, addr, nbytes/2 );

    buf = (unsigned short*) buffer;
    for ( int i = 0; i < 16; i++ )
        printf( "%04x%s", buf[ i ], i % 16 == 15 ? "\n" : " " );

    printf( "\n" );

    stdoutMutex.Unlock ();
#endif
    return true;
    }

bool DSP_Monitor::MemWrite // CLOAD compatible write to target memory
(
    void* thisp, void* buffer, int page, unsigned long addr, int nbytes
    )
{
    return ((DSP_Monitor*)thisp)->MemWrite( buffer, page, addr, nbytes );
    }

bool DSP_Monitor::LoadAndRunCOFF( const char* filename )
{
    logf( "%s: Loading COFF: %s\n", devname, filename );

    CLOAD coff;
    coff.mem_write_this = this;
    coff.mem_write = MemWrite;

    //coff.SetDebugLevel( 1 );

    if ( ! coff.Load( filename, 1 ) )
        return false;

    if ( ! coff.GetEntryPoint () )
        return false;

    if ( coff.GetSysParmAddr () != 0 && coff.GetSysParmSize () > 0 )
    {
        unsigned short data[ 256 ];
        int datac = coff.GetSysParmSize ();
        if ( datac > 256 ) datac = 256;

        ::lseek( dev, coff.GetSysParmAddr (), SEEK_SET );
        ::read( dev, data, 2 * datac );

        stdoutMutex.Lock ();

        if ( logfile )
        {
            fprintf( logfile, "%s: System parameters at 0x%04x (size=%d):\n", devname, 
                coff.GetSysParmAddr (), coff.GetSysParmSize () );

            fprintf( logfile,  "Original:" );
            for ( int i = 0; i < datac; i++ )
                fprintf( logfile,  " %04hx", data[ i ] );
            fprintf( logfile,  "\n" );
            }

        // Override parameters

        // Predefined System Parameters
        data[ 0 ] = ID; // DSP Identifier
        data[ 1 ] = channelc; // Channel Count

        // User Accessible Override Parameters
        for ( int i = 0; i < sysparm_count && 2 + i < datac; i++ )
            data[ 2 + i ] = sysparm[ i ];

        if ( logfile )
        {
            fprintf( logfile,  "Override:" );
            for ( int i = 0; i < datac; i++ )
                fprintf( logfile,  " %04hx", data[ i ] );
            fprintf( logfile,  "\n" );
            }

        // Write overriden parameters
        ::lseek( dev, coff.GetSysParmAddr (), SEEK_SET );
        ::write( dev, data, 2 * datac );

        stdoutMutex.Unlock ();
        }

    // if entry point is in DARAM4-7, then
    // XPC should be set on boot branch.
    //
    unsigned long addr = coff.GetEntryPoint ();
    if ( addr >= 0x8000 )
        addr |= 0x10000;

    logf( "%s: Running from 0x%08lx\n", devname, addr );

#if 0
    unsigned short buf[ 16 ];

    ::lseek( dev, addr, SEEK_SET );
    ::read( dev, buf, sizeof( buf ) );

    stdoutMutex.Lock ();

    printf( "MemRead: addr %lx, words %d\n", addr, sizeof(buf)/2 );

    for ( unsigned i = 0; i < sizeof(buf)/2; i++ )
        printf( "%04hx%s", buf[ i ], i % 16 == 15 ? "\n" : " " );

    printf( "\n" );

    stdoutMutex.Unlock ();
#endif

    ::ioctl( dev, IOCTL_HPI_RUN_PROGRAM, addr );

    return true;
    }

bool DSP_Monitor:: MemSpeedMeasure( long cycles )
{
    fprintf( stderr, "%s: Speed test started... ", devname );
    clock_t t1 = clock ();

    unsigned short ffs[ 1024 ];
    memset( ffs, 0xFF, sizeof( ffs ) );

    unsigned short zeros[ 1024 ];
    memset( zeros, 0x00, sizeof( zeros ) );

    long bytesXfered = 0;

    for ( long i = 0; i < cycles; i++ )
    {
        lseek( dev, 0x1100, SEEK_SET );
        write( dev, ffs, sizeof( ffs ) );
        bytesXfered += sizeof( ffs );

        lseek( dev, 0x1100, SEEK_SET );
        write( dev, zeros, sizeof( zeros ) );
        bytesXfered += sizeof( zeros );
        }

    clock_t t2 = clock ();

    double t = double( t2 - t1 ) / CLOCKS_PER_SEC;

    fprintf( stderr, "\r%s: Elapsed %.3f s; T= %.3f us/word, "
            "Speed: %.0f Bps (%.0f bps)\n",
        devname, t, t * 1e6 * 2 / bytesXfered,
        bytesXfered / t, 8 * bytesXfered / t
        );

    return true;
    }

// DSP memory banks description; Used e.g. to test memory
//
struct BANKDESC
{
    unsigned long addr;          // Bank origin (as XHPIA address)
    long size;                   // Bank size (in words)
    char* comment;               // Description
    };

static BANKDESC c5416_banks [] =
{
    { 0x000080, 0x7F80, "DARAM0-3" },
    { 0x018000, 0x8000, "DARAM4-7" },
    { 0x028000, 0x8000, "SARAM0-3" },
    { 0x038000, 0x8000, "SARAM4-7" },
    { 0,        0,      NULL       }
    };

static BANKDESC c5410A_banks [] =
{
    { 0x000080, 0x7F80, "DARAM0-3" },
    { 0x018000, 0x8000, "DARAM4-7" },
    { 0,        0,      NULL       }
    };

static BANKDESC c5409A_banks [] =
{
    { 0x000080, 0x7F80, "DARAM0-3" },
    { 0,        0,      NULL       }
    };

static BANKDESC c5402A_banks [] =
{
    { 0x000080, 0x3F80, "DARAM0-1" },
    { 0,        0,      NULL       }
    };

bool DSP_Monitor::MemTest( void )
{
    int DSP_ID = ::ioctl( dev, IOCTL_HPI_GET_DSP_CHIP_ID, 0 );

    BANKDESC* banks = 0;
    switch( DSP_ID )
    {
        case 0x02: banks = c5402A_banks; break;
        case 0x09: banks = c5409A_banks; break;
        case 0x10: banks = c5410A_banks; break;
        case 0x16: banks = c5416_banks;  break;
        }

    if ( ! banks )
    {
        fprintf( stderr, "%s: Unknown DSP Chip ID: %02x\n", devname, DSP_ID );
        return false;
        }

    int failedBanks = 0;

    for ( int i = 0; banks && banks[ i ].comment != NULL; i++ )
    {
        BANKDESC* bank = &banks[ i ];

        fprintf( stderr, "%s: Testing TMS320VC54%02x %s: 0x%06lx:0x%04lx: ", 
            devname, DSP_ID, bank->comment, bank->addr, bank->size
            );

        int failedWords = 0;

        enum { CHUNK_WORDC = 128 };
        unsigned short data[ CHUNK_WORDC ];
        unsigned short data2[ CHUNK_WORDC ];
        int jcount = bank->size * sizeof( unsigned short ) / sizeof( data );

        // Write FFFF data to DSP memory
        //
        memset( data, 0xFF, sizeof( data ) );
        ::lseek( dev, bank->addr, SEEK_SET );
        for ( int j = 0; j < jcount; j++ )
        {
            ::write( dev, data, sizeof( data ) );
            }

        // Read and verify data from DSP memory
        //
        ::lseek( dev, bank->addr, SEEK_SET );
        for ( int j = 0; j < jcount; j++ )
        {
            memset( data2, 0x00, sizeof( data2 ) );
            ::read( dev, data2, sizeof( data2 ) );

            for ( int k = 0; k < CHUNK_WORDC; k++ )
                if ( data[ k ] != data2[ k ] )
                    failedWords++;
            }

        // Write 0000 data to DSP memory
        //
        memset( data, 0x00, sizeof( data ) );
        ::lseek( dev, bank->addr, SEEK_SET );
        for ( int j = 0; j < jcount; j++ )
        {
            ::write( dev, data, sizeof( data ) );
            }

        // Read and verify data from DSP memory
        //
        ::lseek( dev, bank->addr, SEEK_SET );
        for ( int j = 0; j < jcount; j++ )
        {
            memset( data2, 0xFF, sizeof( data2 ) );
            ::read( dev, data2, sizeof( data2 ) );
            for ( int k = 0; k < CHUNK_WORDC; k++ )
                if ( data[ k ] != data2[ k ] )
                    failedWords++;
            }

        if ( failedWords )
        {
            failedBanks++;

            fprintf( stderr, "FAILED: %d of %ld words\n",
                failedWords, bank->size
                );
            }
        else
        {
            fprintf( stderr, "OK\n" );
            }
        }

    return failedBanks == 0;
    }

void* DSP_Monitor::RunAndMonitor_thread_wrapper( void *ptr )
{
    if ( ! ptr ) return NULL;

    DSP_Monitor* dsp = (DSP_Monitor*) ptr;

    dsp->IncThreadCount ();
    dsp->IncThreadStartedCount ();

    bool ok = dsp->RunAndMonitor ();

    dsp->DecThreadCount ();

    static int failed_ptr = 0;

    return ok ? NULL : &failed_ptr ;
    }

bool DSP_Monitor::StartThread( void )
{
    return 0 == pthread_create( &thread, NULL, RunAndMonitor_thread_wrapper, (void*)this ); // 0 means OK
    }

bool DSP_Monitor::JoinThread( void )
{
    printf( "Joining thread for %s\n", devname );
    void* failed_ptr;
    pthread_join( thread, &failed_ptr );

    return failed_ptr == NULL; // NULL means OK
    }

bool DSP_Monitor::RunAndMonitor( void )
{
    trace( 1, "%s: -------------------------------------------------------------\n", devname );

    if ( ! LoadAndRunCOFF( coff_filename ) )
    {
        logf( "%s: Failed to run %s\n", devname, coff_filename );
        return false;
        }

    bool lock_file_created = false;
    time_t last_wd = time( NULL );
    restart_posted = false;

    int maxDesc = -1;
    fd_set readfds_proto;
    FD_ZERO( &readfds_proto );
    FD_SET( dev_dbg, &readfds_proto );
    if ( dev_dbg > maxDesc )
        maxDesc = dev_dbg;

    while( fRunning )
    {
        timeval tv = { 0, 100000 }; // 100ms

        fd_set readfds = readfds_proto;
        int rc = select( maxDesc + 1, &readfds, NULL, NULL, &tv );
        if ( rc < 0 ) // error or signal
            continue;
        else if ( rc == 0 ) // timeout
            continue;

        time_t now = time( NULL );

        if ( FD_ISSET( dev_dbg, &readfds ) )
        {
            char line[ 1024 ];
		    int rd = read( dev_dbg, line, sizeof( line ) );

		    if ( 0 == rd )
            {
                logf( "%s: Got EOF\n", devname );

                ResetDSP( /*kill debuggers*/ true );

                if ( ! LoadAndRunCOFF( coff_filename ) )
                {
                    logf( "%s: Failed to run %s\n", devname, coff_filename );
                    return false;
                    }

                last_wd = time( NULL );
                lock_file_created = false;
                restart_posted = false;

                FD_ZERO( &readfds_proto );
                maxDesc = -1;
                FD_SET( dev_dbg, &readfds_proto );
                if ( dev_dbg > maxDesc )
                    maxDesc = dev_dbg;

                continue; // EOF
                }

            line[ rd ] = 0; // ASCIIZ string

            for ( char* chp = line; *chp; )
            {
                if ( chp[ 0 ] == 'W' && chp[ 1 ] == 'D' && chp[ 2 ] == '\n' )
                {
                    last_wd = now;

                    unsigned char dspwd_report[ 256 ];
                    ::ioctl( dev, IOCTL_HPI_GET_DSPWD_REPORT, &dspwd_report );
                    //
                    // WD Structure: (all data in network order, i.e. MSB first)
                    //
                    // uint16    -> length in bytes of the following:
                    // uint32    -> uptime in seconds
                    // uint16    -> cpu ussage in promilles
                    // uint16[]  -> status & statics
                    //
                    int len = dspwd_report[ 0 ];
                    unsigned long upTime 
                        = ( dspwd_report[ 1 ] << 24 ) + ( dspwd_report[ 2 ] << 16 )
                        + ( dspwd_report[ 3 ] << 8 ) + dspwd_report[ 4 ];
                    double cpuUsage = double( ( dspwd_report[ 5 ] << 8 ) + dspwd_report[ 6 ] ) /  10.0; // got in promiles

                    stdoutMutex.Lock ();

                    printf( "%s: (%2hu) %4.1f%% %6lu,", 
                        devname, (int)dspwd_report[ 0 ], cpuUsage, upTime );

                    for ( int i = 7; i < len; i += 2 )
                    {
                        printf( " %4x", ( dspwd_report[ i ] << 8 ) + dspwd_report[ i + 1 ] );
                        }

                    printf( "\n" );

                    stdoutMutex.Unlock ();

                    // Create lock file first time we have received WD,
                    // and perform initial channel settings.
                    //
                    if ( ! lock_file_created )
                    {
                        lock_file_created = true;
                        FILE* f = fopen( lock_filename, "w" );
                        if ( f ) fclose( f );

                        for ( int i = 0; i < channelc; i++ )
                        {
                            char cmd[ 80 ];
                            sprintf( cmd, "./c54setp %d cold", abs_channel_base + i + 1 );
                            system( cmd );
                            }
                        }

                    chp += 3; // skip "WD\n"
                    }
                else
                {
                    char* eol = strchr( chp, '\n' );
                    if ( ! eol )
                    {
                        logf( "%s: %s<NOEOL>\n", devname, chp );
                        break;
                        }
                    else
                    {
                        *eol = 0;
                        logf( "%s: %s\n", devname, chp );
                        chp = eol + 1;
                        }
                    }
                }
            }

        // Restart posted or 1s Watchdog failed.
        //
        if ( restart_posted || now - last_wd > 2 ) 
        {
            if ( restart_posted )
                logf( "%s: SIGHUP received. Reloading DSP...\n", devname );
            else
                logf( "%s: Watchdog failed. Reloading DSP...\n", devname );

            ResetDSP( /*kill debuggers*/ true );

            if ( ! LoadAndRunCOFF( coff_filename ) )
            {
                logf( "%s: Failed to run %s\n", devname, coff_filename );
                return false;
                }

            last_wd = time( NULL );
            lock_file_created = false;
            restart_posted = false;

            FD_ZERO( &readfds_proto );
            maxDesc = -1;
            FD_SET( dev_dbg, &readfds_proto );
            if ( dev_dbg > maxDesc )
                maxDesc = dev_dbg;
            }
        }

    trace( 1, "%s: --------------------------------------------------------------\n", devname );

    close( dev_dbg ); // close STDIO
    dev_dbg = -1;

    for ( int i = 0; i < 20; i++ ) // Wait max 2 seconds for clients to exit
    {
        int count = ::ioctl( dev, IOCTL_HPI_GET_IODEV_COUNT, 0 );
        trace( 2, "%s: iodev count %d\n", devname, count );
        if ( count <= 2 ) // we own 2 fds
            break;

        usleep( 100000 ); // 100ms
        }

    ResetDSP ();

    return true;
    }
