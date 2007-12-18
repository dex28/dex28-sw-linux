/*
    Project:     Albatros / cload
  
    Module:      hpiwrapper.h
  
    Description: Albatros c54xx COFF loader
                 HPI device c++ wrapper class declaration
  
    Copyright (c) 2002 By Mikica B Kocic
    Copyright (c) 2002 By IPTC Technology Communications AB
*/

#ifndef _HPIWRAPPER_H_INCLUDED
#define _HPIWRAPPER_H_INCLUDED

#include <sys/ioctl.h>
#include "../hpi/hpi.h"

#include <pthread.h>
#include <stdarg.h>

extern void trace( int level, const char* format... );
extern void logf( const char* format... );
extern class Mutex stdoutMutex;
extern FILE* logfile;

///////////////////////////////////////////////////////////////////////////////

class Mutex
{
    pthread_mutex_t mutex;

public:

    Mutex( void )
    {
        pthread_mutex_init( &mutex, NULL );
        }

    void Lock( void )
    {
        pthread_mutex_lock( &mutex );
        }

    void Unlock( void )
    {
        pthread_mutex_unlock( &mutex );
        }
    };

///////////////////////////////////////////////////////////////////////////////

class DSP_Monitor
{
    int ID;
    int dev;
    int dev_dbg;
    char devname[ 64 ];
    char coff_filename[ 128 ];
    char lock_filename[ 128 ];

    unsigned short sysparm[ 64 ]; // System parameters
    int sysparm_count;

    int abs_channel_base;
    int channelc;
    int child_pid[ 10 ];

    ///////////////////////////////////////////////////////////////////////////

    static volatile bool fRunning;
    pthread_t thread;

    volatile bool restart_posted;
    int reset_count;

    ///////////////////////////////////////////////////////////////////////////

    static volatile long thread_count;
    static volatile long thread_started_count;
    static Mutex thread_count_mutex;

    static void IncThreadStartedCount( void )
    {
        thread_count_mutex.Lock ();
        ++thread_started_count;
        trace( 2, "<---- Thread++ Count %d, Started %d\n", thread_count, thread_started_count );
        thread_count_mutex.Unlock ();
        }

    static void IncThreadCount( void )
    {
        thread_count_mutex.Lock ();
        ++thread_count;
        trace( 2, "<---- Thread++ Count %d, Started %d\n", thread_count, thread_started_count );
        thread_count_mutex.Unlock ();
        }

    static void DecThreadCount( void )
    {
        thread_count_mutex.Lock ();
        --thread_count;
        trace( 2, "<---- Thread-- Count %d, Started %d\n", thread_count, thread_started_count );
        thread_count_mutex.Unlock ();
        }

    bool RunAndMonitor( void );
    bool LoadAndRunCOFF( const char* filename );

    static void* RunAndMonitor_thread_wrapper( void *ptr );

    ///////////////////////////////////////////////////////////////////////////
    // CLOAD compatible write to target memory:
    //
    static bool MemWrite( void* thisp, void* buffer, int page, unsigned long addr, int nbytes );

public:

    static void TerminateGracefully( void )
    {
        fRunning = false;
        }

    static void WaitAllThreadsStarted( long check_count )
    {
        while( fRunning )
        {
            thread_count_mutex.Lock ();
            long count = thread_started_count;
            thread_count_mutex.Unlock ();

            trace( 2, "----> Thread Top Count %d\n", count );

            if ( count >= check_count )
                return;

            sleep( 1 );
            }
        }

    static void WaitAnyThreadStopped( long check_count )
    {
        while( fRunning )
        {
            thread_count_mutex.Lock ();
            long count = thread_count;
            thread_count_mutex.Unlock ();

            trace( 2, "----> Thread Count %d\n", count );

            if ( count < check_count )
                return;

            sleep( 1 );
            }
        }

    bool isOpen( void ) { return dev >= 0 && dev_dbg >= 0; }

    void PostRestart( void ) { restart_posted = true; }

    DSP_Monitor( void )
        : ID( 0 )
        , dev( -1 )
        , dev_dbg( -1 )
        , sysparm_count( 0 )
        , abs_channel_base( 0 )
        , channelc( 0 )
        , restart_posted( false )
        , reset_count( 0 )
    { 
        strcpy( devname, "" );
        strcpy( coff_filename, "" );
        strcpy( lock_filename, "" );
        }

    ~DSP_Monitor( void ) 
    { 
        Close (); 
        }

    int GetChannelCount( void ) const
    {
        return channelc;
        }

    void AddSysParam( int x )
    {
        sysparm[ sysparm_count++ ] = (unsigned short)x;
        }

    int GetSysParam( int pos )
    {
        return sysparm[ pos ];
        }

    void SetSysParam( int pos, int value )
    {
        if ( pos >= 0 && pos < sysparm_count )
            sysparm[ pos ] = value;
        }

    bool Open( int id, const char* devname, const char* coffname, int abs_ch_b, int channelc, const char* lockname );

    void Close( void );

    bool ResetDSP( bool kill_debuggers = true );

    bool MemTest( void );

    bool MemWrite( void* buffer, int page, unsigned long addr, int nbytes );

    bool MemSpeedMeasure( long cycles );

    void MemDump( unsigned long addr, long len );

    bool StartThread( void );
    bool JoinThread( void );

    static void Init_ChildProcesses( void );
    static void Kill_ChildProcesses( void );
    };

#endif // _HPIWRAPPER_H_INCLUDED
