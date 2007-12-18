/*
    Project:     Albatross / c54load
  
    Module:      main.cpp
  
    Description: Albatross c54xx COFF loader
                 Main module. Test engine.
  
    Copyright (c) 2002-2005 By Mikica B Kocic
    Copyright (c) 2002-2005 By IPTC Technology Communications AB
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <getopt.h>
#include <errno.h>

#include "hpiwrapper.h"

///////////////////////////////////////////////////////////////////////////////
// Global parameters:
//
//    pid filename
//    prefix directory for coff files
//    prefix directory for lock files
//
// Parameters per DSP:
//
//    hpi-dsp device name
//    coff filename
//    channel count
//    lock filename
//
//    initial configuration words (coff dependent)
//    
///////////////////////////////////////////////////////////////////////////////


DSP_Monitor dsp[ 4 ];
int dspCount = 0;

char mode[ 64 ] = "";

///////////////////////////////////////////////////////////////////////////

int  opt_verbose = 0;
bool opt_help = false;
bool opt_hpi = false;
bool opt_reset = false;
bool opt_speed = false;
long opt_speed_cycles = 500;
bool opt_test = false;

bool opt_daemon = false;
char* opt_pidfile = NULL;

bool opt_run = false;
char* opt_cfgfile = NULL;

char* opt_chdir = NULL;

char* opt_logfile = NULL;

///////////////////////////////////////////////////////////////////////////

void Kill_c54load_Instances( void )
{
    if ( ! opt_pidfile || ! opt_pidfile[0] )
        return;

    pid_t pid = 0;

    for( int i = 0; i < 10; i++ )
    {
        struct stat buf;
        if ( 0 > stat( opt_pidfile, &buf ) ) // no pid file found
            return;

        if ( pid == 0 ) // read file; first time only
        {
            FILE* f = fopen( opt_pidfile, "r" );
            if ( f )
            {
                fscanf( f, "%d", &pid );
                fclose( f );
                }

            fprintf( stderr, "Another instance of c54load [pid=%d] is already running.\n", pid );
            }

        if ( i == 0 )
            fprintf( stderr, "Sending SIGTERM to pid %d...\n", pid );
        else if ( i == 8 )
            fprintf( stderr, "Sending SIGKILL to pid %d...\n", pid );
        else // send signal 0 -- means search process
            fprintf( stderr, "Pid %d still exists...\n", pid );

        if ( 0 > kill( pid, i == 0 ? SIGTERM : i == 8 ? SIGKILL : 0 ) && errno == ESRCH )
        {
            // process not found
            unlink( opt_pidfile ); // remove pid file and return
            return;
            }

        sleep( 1 ); // give the process chanse to exit 
        }

    fprintf( stderr, "Could not kill other instance. Quitting...\n" );
    exit( 1 );
    }

///////////////////////////////////////////////////////////////////////////

static pid_t run( int abs_channel )
{
    pid_t pid = fork ();

    if ( pid != 0 )
    {
        trace( 1, "----- Starded child %d\n", pid );
        return pid;
        }
    
    // We are child...

    // Clean up
    //
    close( 0 );
    close( 1 );
    close( 2 );
    open( "/dev/null", O_RDONLY ); // stdin
    open( "/dev/null", O_RDONLY ); // stdout
    open( "/dev/null", O_RDONLY ); // stderr

    // Reset signal handlers
    //
    // sigprocmask( SETMASK, &omask, NULL ):

    signal( SIGINT,  SIG_DFL );
    signal( SIGQUIT, SIG_DFL );
    signal( SIGTERM, SIG_DFL );
    signal( SIGHUP,  SIG_DFL );

    setsid ();

    char argument[ 32 ];
    sprintf( argument, "%d", abs_channel );

    if ( strcasecmp( mode, "VR" ) == 0 ) // Voice recording
    {
        char* cmd [] = { "idodVR", argument, NULL };
        execv( "./idodVR", cmd );
        }
    else // DTS Master
    {
        char* cmd [] = { "idodMaster", argument, NULL };
        execv( "./idodMaster", cmd );
        }

    exit( 1 );

    return pid;
    }

void DSP_Monitor::Init_ChildProcesses( void )
{
    while( fRunning )
    {
        // Run actions
        //
        int abs_channel = 0;

        for ( int i = 0; i < dspCount; i++ )
        {
            for ( int j = 0; j < dsp[ i ].channelc; j++, abs_channel++ )
            {
                if ( dsp[ i ].child_pid[ j ] == 0 )
                {
                    dsp[ i ].child_pid[ j ] = run( abs_channel );
                    }
                }
            }

        sleep( 1 ); // don't consume all CPU time -- sleep a bit

        int status;
        int wpid = wait( &status );

        if ( ! fRunning )
            break;

        trace( 1, "----- Process %d exited; Scheduling for restart.\n", wpid );

        while( wpid > 0 )
        {
            for ( int i = 0; i < dspCount; i++ )
            {
                for ( int j = 0; j < dsp[ i ].channelc; j++ )
                {
                    if ( wpid == dsp[ i ].child_pid[ j ] )
                    {
                        dsp[ i ].child_pid[ j ] = 0; // restart
                        }
                    }
                }

            wpid = waitpid( -1, &status, WNOHANG );
            }
        }

    trace( 1, "----- Child monitor: Done.\n" );
    }

///////////////////////////////////////////////////////////////////////////////

pid_t child_monitor_pid = 0;

void DSP_Monitor::Kill_ChildProcesses( void )
{
    system( "killall idod 1>/dev/null 2>/dev/null" );
    system( "killall webStat 1>/dev/null 2>/dev/null" );

    // forward signal to child, which will kill its children
    if ( child_monitor_pid )
        kill( child_monitor_pid, SIGTERM );
    }


void sighup_handler( int )
{
    if ( opt_verbose >= 1 )
        fprintf( stderr, "[%d]: Caught SIGHUP\n", getpid () );

    if ( child_monitor_pid ) // we are master, not child forker
    {
        for ( int i = 0; i < dspCount; i++ )
            dsp[ i ].PostRestart ();
        }
    }

void sigterm_handler( int sig )
{
    if ( opt_verbose >= 1 )
        fprintf( stderr, "[%d]: Caught signal %d\n", getpid (), sig );

    DSP_Monitor::TerminateGracefully ();
    }

///////////////////////////////////////////////////////////////////////////

void display_usage( void )
{
    printf( 
        "Usage: c54load [OPTION]...\n"
        "   -h, --hpi              Automatically load/unload HPI module.\n"
        "   -R, --reset            Reset DSPs.\n"
        "   -s, --speed[=cycles]   Test DSP HPI transfer speed.\n"
        "                          Default: 500 cycles, write 2KiB block each.\n"
        "   -t, --test             Test DSP memory.\n"
        "   -d, --daemon[=pidfile] Start as daemon. Implies --run option.\n"
        "                          Default: /var/run/c54load.pid\n"
        "   -r, --run[=cfgfile]    Load and run coff file described in DSP config file.\n"
        "                          Default: /etc/sysconfig/albatross/dsp.cfg\n"
        "   -D, --dir=curdir       Change current working directory.\n"
        "   -l, --logfile=filename Save log into file.\n"
        "                          Default: /var/log/albatross\n"
        "   -V, --help             Display this help and exit.\n"
        "   -v, --verbose          Show trace information.\n"
        );
    }

///////////////////////////////////////////////////////////////////////////

int main( int argc, char** argv )
{
    for( ;; )
    {
        int option_index = 0;
        static struct option long_options[] = 
        {
            { "hpi",     0, 0, 0   }, // -h
            { "reset",   0, 0, 0   }, // -R
            { "speed",   2, 0, 0   }, // -s [cycles]
            { "test",    0, 0, 0   }, // -t
            { "daemon",  2, 0, 0   }, // -d [pidfile]
            { "run",     2, 0, 0   }, // -r [cfgfile]
            { "dir",     1, 0, 0   }, // -D curdir
            { "help",    0, 0, 0   }, // -V
            { "verbose", 0, 0, 0   }, // -v
            { "logfile", 1, 0, 0   }, // -l logfile
            { 0,         0, 0, 0   }
            };

        int c = getopt_long (argc, argv, "hRs::td::r::D:Vvl:", long_options, &option_index );
        if ( c == -1 )
            break;

        if ( c == 0 )
        {
            /*
            printf ("option %s", long_options[option_index].name);
            if (optarg)
                printf (" with arg %s", optarg);
            printf ("\n");
            */

            switch( option_index )
            {
                case 0:  c = 'h'; break; // hpi
                case 1:  c = 'R'; break; // reset
                case 2:  c = 's'; break; // speed
                case 3:  c = 't'; break; // test
                case 4:  c = 'd'; break; // daemon
                case 5:  c = 'r'; break; // run
                case 6:  c = 'D'; break; // dir
                case 7:  c = 'V'; break; // help
                case 8:  c = 'v'; break; // verbose
                case 9:  c = 'l'; break; // logfile
                default: c = '?'; break; // ?unknown?
                }
            }

        switch( c ) 
        {
            case 'h': opt_hpi = true;
                break;

            case 'R':
                opt_reset = true;
                break;

            case 's':
                opt_speed = true;
                if ( optarg )
                    opt_speed_cycles = strtol( optarg, NULL, 10 );
                break;

            case 't':
                opt_test = true;
                break;

            case 'd':
                opt_daemon = true;
                opt_run = true;
                opt_pidfile = optarg;
                break;

            case 'r':
                opt_run = true;
                opt_cfgfile = optarg;
                break;

            case 'D':
                opt_chdir = optarg;
                break;

            case 'V':
                opt_help = true;
                break;

            case 'v':
                ++opt_verbose;
                break;

            case 'l':
                opt_logfile = optarg;
                break;

            case '?':
                display_usage ();
                return 1;
            }
        }

    if ( ! opt_pidfile ) 
        opt_pidfile = "/var/run/c54load.pid";

    if ( ! opt_cfgfile )
        opt_cfgfile = "/etc/sysconfig/albatross/dsp.cfg";

    if ( ! opt_logfile )
        opt_logfile = "/var/log/albatross";

    if ( opt_help )
    {
        display_usage ();
        return 0;
        }

    if ( opt_verbose >= 1 )
    {
        printf( "Running parameters:\n" );
        printf( "    --hpi      %d\n", opt_hpi );
        printf( "    --reset    %d\n", opt_reset );
        printf( "    --speed    %d\n", opt_speed );
        printf( "    --test     %d\n", opt_test ); 
        printf( "    --daemon   %d, pidfile=[%s]\n", opt_daemon, opt_pidfile );
        printf( "    --run      %d, cfgfile= [%s]\n", opt_run, opt_cfgfile );
        printf( "    --dir      %d, chdir=[%s]\n", opt_chdir != NULL, opt_chdir ? opt_chdir : "" );
        printf( "    --help     %d\n", opt_help );
        printf( "    --verbose  %d\n", opt_verbose);
        if (  optind < argc )
        {
            printf ("Non-option ARGV-elements: ");
            for( ; optind < argc ; optind++ ) 
                printf( "%s ", argv[ optind ] );
            printf ("\n");
            }
        }

    if ( opt_chdir && chdir( opt_chdir ) < 0 )
    {
        perror( "chdir failed" );
        fprintf( stderr, "Cannot set current directory: [%s]. Exitting...\n", opt_chdir );
        return 1;
        }

    Kill_c54load_Instances ();

    if ( opt_daemon )
    {
        daemon( /*nochdir*/ 1, /*noclose*/0 );

        // Write our PID (if filename exists and it is not empty string).
        //
        if ( opt_pidfile && opt_pidfile[0] )
        {
            FILE* f = fopen( opt_pidfile, "w" );
            if ( f )
            {
                fprintf( f, "%d\n", getpid () );
                fclose( f );
                }
            }
        }

    if ( opt_logfile && opt_logfile[0] )
        logfile = fopen( opt_logfile, "a+" );

    {
        char buf[256];
        getcwd( buf, sizeof( buf ) );
        logf( "Current dir: %-.255s\n", buf );
        }

    ///////////////////////////////////////////////////////////////////////////
    // (re)load hpi module
    //
    if ( opt_hpi )
    {
        struct stat buf;
        while( 0 == stat( "/proc/hpidev", &buf ) )
        {
            // FIXME logf( "Resetting DSP(s)...\n" );
            // system( "./c54load --reset --logfile=" );

            usleep( 500000 );

            logf( "Killing remaining DSp clients...\n" );
            system( "killall -KILL idod 1>/dev/null 2>/dev/null" );
            system( "killall -KILL webStat 1>/dev/null 2>/dev/null" );
            system( "killall -KILL idodMaster 1>/dev/null 2>/dev/null" );
            system( "killall -KILL idodVR 1>/dev/null 2>/dev/null" );

            usleep( 500000 );

            logf( "Removing hpi module...\n" );
            system( "rmmod hpi" );
            }

        logf( "Installing hpi module...\n" );
        system( "insmod ./hpi.ko" );
        }

    ///////////////////////////////////////////////////////////////////////////
    // Read dsp configuration and create objects
    //
    FILE* f = fopen( opt_cfgfile, "r" );
    if ( ! f )
    {
        logf( "Failed to open configuration file.\n" );
        return -1;
        }

    for( int i = 0, abs_channel_base = 0; i < 4; i++ ) // Max 4 DSPs !
    {
        char line[ 512 ];
        if ( ! fgets( line, sizeof( line ), f ) )
            break;

        char devname[ 64 ];
        char coffname[ 128 ];
        int channels;
        char lockname[ 128 ];

        int n = 0;

        if ( 4 != sscanf( line, " %s %s %d %s%n", devname, coffname, &channels, lockname, &n ) )
            continue;

        if ( dsp[ dspCount ].Open( dspCount, devname, coffname, abs_channel_base, channels, lockname ) )
        {
            // Read override values for COFF sysparm section
            int param;
            for ( char* chp = line + n; n > 0 && 1 == sscanf( chp, " %i%n", &param, &n ); chp += n )
                dsp[ dspCount ].AddSysParam( param );

            abs_channel_base += channels;
            dspCount++;
            }
        }

    fclose( f );

    if ( dspCount == 0 )
    {
        logf( "No DSP(s) found.\n" );
        return -1;
        }

    ///////////////////////////////////////////////////////////////////////////
    // Perform required task
    //

    bool failed = false;

    if ( opt_reset )
    {
        for ( int i = 0; i < dspCount; i++ )
            if ( ! dsp[ i ].ResetDSP( /* send EOF */ true ) )
                failed = true;
        }

    if ( opt_test )
    {
        for ( int i = 0; i < dspCount; i++ )
            if ( ! dsp[ i ].MemTest () )
                failed = true;
        }

    if ( opt_speed )
    {
        for ( int i = 0; i < dspCount; i++ )
            if ( ! dsp[ i ].MemSpeedMeasure( opt_speed_cycles ) )
                failed = true;
        }

    if ( ! failed && opt_run )
    {
        signal( SIGTERM, sigterm_handler );
        signal( SIGQUIT, sigterm_handler );
        signal( SIGINT,  sigterm_handler );

        signal( SIGHUP,  sighup_handler );

        char* idod_lockfile = "/var/run/idod.lock";
        unlink( idod_lockfile );

        FILE* f = fopen( "/etc/sysconfig/albatross/mode", "r" );
        if ( f )
        {
            char ch;
            if ( 1 == fscanf( f, " MODE=%c", &ch ) )
            {
                if ( ch == '"' ) fscanf( f, "%[^\"]", mode );
                else 
                {
                    mode[ 0 ] = ch;
                    fscanf( f, "%s", mode + 1 );
                }
            
            fclose( f );
            }

        logf( "Retrieved MODE: [%s]\n", mode );

        if ( strcasecmp( mode, "GATEWAY" ) == 0 ) // Switch side
        {
            // Override param 0 (representing M/~S bitmap) with 
            // bitmap value all 0's, i.e. TDM SLAVE for all channels
            //
            for ( int i = 0; i < dspCount; i++ )
                dsp[ i ].SetSysParam( 0, 0x0 );
            }
        else if ( strcasecmp( mode, "VR" ) == 0 ) // Switch side, voice recording
        {
            // Override param 0 (representing M/~S bitmap) with 
            // bitmap value '10'b = 0x2, i.e. TDM MASTER for CH#1 and TDM SLAVE for CH#0
            //
            for ( int i = 0; i < dspCount; i++ )
                dsp[ i ].SetSysParam( 0, 0x2 );

            // start idodVR monitor and get its pid (used by signal handlers)
            //
            child_monitor_pid = fork ();

            if ( 0 == child_monitor_pid ) // we are child
            {
                for ( int i = 0; i < dspCount; i++ )
                    dsp[ i ].Close (); // close fds

                close( 0 );
                open( "/dev/null", O_RDONLY ); // stdin

                DSP_Monitor::Init_ChildProcesses ();
                exit( 0 );
                }

            trace( 1, "Started child monitor, pid %d\n", child_monitor_pid );
            }
        else // Extension side
        {
            // Create lock file that will prevent 'idod' clients do not start
            //
            int fd = creat( idod_lockfile, O_CREAT );
            if ( fd >=0 ) close( fd );
 
            // Override param 0 (representing M/~S bitmap) with 
            // bitmap value all 1's, i.e. TDM MASTER for all channels
            //
            for ( int i = 0; i < dspCount; i++ )
                dsp[ i ].SetSysParam( 0, ( 1 << dsp[ i ].GetChannelCount () ) - 1 );

            if ( strcasecmp( mode, "EXTENDER" ) == 0 ) // extension side, idodMaster clients
            {
                // start idodMaster monitor and get its pid (used by signal handlers)
                //
                child_monitor_pid = fork ();

                if ( 0 == child_monitor_pid ) // we are child
                {
                    for ( int i = 0; i < dspCount; i++ )
                        dsp[ i ].Close (); // close fds

                    close( 0 );
                    open( "/dev/null", O_RDONLY ); // stdin

                    DSP_Monitor::Init_ChildProcesses ();
                    exit( 0 );
                    }

                trace( 1, "Started child monitor, pid %d\n", child_monitor_pid );
                }
            }
        }

        // Start load COFF, run & monitor thread for each dsp
        //
        for ( int i = 0; i < dspCount; i++ )
            if ( ! dsp[ i ].StartThread () )
                failed = true;

        if ( ! failed )
        {
            // Wait all threads to get started ( started threads must be == dsp count )
            //
            DSP_Monitor::WaitAllThreadsStarted( dspCount );

            // Wait ANY thread to quit ( running threads must be = dsp count )
            //
            DSP_Monitor::WaitAnyThreadStopped( dspCount );

            // If any thread exists, then post signal to other to exit too.
            //
            DSP_Monitor::TerminateGracefully ();

            // Wait all threads to exit.
            //
            for ( int i = 0; i < dspCount; i++ )
            {
                if ( ! dsp[ i ].JoinThread () )
                    failed = true;
                }
            }

        DSP_Monitor::Kill_ChildProcesses ();
        }

    ///////////////////////////////////////////////////////////////////////////
    // Unload hpi module
    //
    if ( opt_hpi )
    {
        for ( int i = 0; i < dspCount; i++ )
        {
            dsp[ i ].ResetDSP( /* send EOF */ true );
            dsp[ i ].Close ();
            }

        for( ;; )
        {
            logf( "Removing hpi module...\n" );

            system( "killall -KILL idod 1>/dev/null 2>/dev/null" );
            system( "killall -KILL webStat 1>/dev/null 2>/dev/null" );
            system( "killall -KILL idodMaster 1>/dev/null 2>/dev/null" );
            system( "killall -KILL idodVR 1>/dev/null 2>/dev/null" );

            usleep( 500000 );

            system( "rmmod hpi" );

            struct stat buf;
            if ( 0 != stat( "/proc/hpidev", &buf ) )
                break;
            }
        }

    logf( "c54load completed.\n" ); 

    ///////////////////////////////////////////////////////////////////////////
    // Remove our PID
    //
    if ( opt_daemon && opt_pidfile && opt_pidfile[0] )
        unlink( opt_pidfile );

    ///////////////////////////////////////////////////////////////////////////
    // Close logfile
    //
    if ( logfile )
        fclose( logfile );

    return 0;
    }

void operator delete [] (void *p)
{
    free( p );
    trace( 2, "delete[](%p)\n", p );
    }

void operator delete (void *p)
{
    free( p );
    trace( 2, "delete(%p)\n", p );
    }

void* operator new (size_t t)
{
    void* p = calloc( 1, t );
    trace( 2, "new(%u) -> %p\n", t, p );
    return p;
    }

void* operator new[] (size_t t)
{
    void* p = calloc( 1, t );
    trace( 2, "new[](%u) -> %p\n", t, p );
    return p;
    }
