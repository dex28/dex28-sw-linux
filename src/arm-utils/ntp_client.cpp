#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

static void reload_config( int sig )
{
    // do nothing; signal will interrupt sleep()
}

int main( int argc, char** argv )
{
    daemon( /*nochdir*/ 0, /*noclose*/ 0 );

	signal( SIGHUP, reload_config );

    // Write our PID.
    //
    FILE* f = fopen( "/var/run/ntp_client.pid", "w" );
    if ( f )
    {
        fprintf( f, "%d\n", getpid () );
        fclose( f );
    }

    char* a[ 11 ] = { "ntpdate", "-s", NULL };
    int a_end = 10;
    int a_start = 2;

    int retry_timeout = 0;

    for ( ;; )
    {
        // Update a[]
        // a[ 0 ] points always to constant
        // a[ 1..8 ] points to char[] containing NTP server address
        // a[ 9 ] points always has NULL value
        //
        FILE* f = fopen( "/etc/sysconfig/ntp_servers", "r" );
        if ( f )
        {
            //
            for( int i = a_start; i < a_end; i++ )
            {
                char* word = a[ i ] ? a[ i ] : new char[ 128 ];
                word[0] = 0;
                if ( 0 == fscanf( f, " %s", word ) || word[0] == 0 )
                {
                    a[ i ] = 0;
                    delete word;
                    break;
                }

                a[ i ] = word;
            }

            fclose( f );
        }
#if 0
        printf( "Loaded configuration:\n" );
        for ( int i = 0; i < 10; i++ )
            if ( a[i] )
                printf( "[%s]\n", a[i] );
#endif

        pid_t pid = fork ();
        if ( pid == 0 )
        {
            execvp( a[0], a );
            exit( 0 );
        }

        // printf( "started ntpdate %d\n", pid );

        int status = 0;
        waitpid( pid, &status, 0 );
        if ( WIFEXITED( status ) && 0 == WEXITSTATUS( status ) )
        {
            sleep( 8 * 3600l ); // OK: update after 8 hours
            retry_timeout = 0;
        }
        else
        {
            // RETRY: after 1 hour, then after 2 hours ... up to 8 hours
            if ( retry_timeout < 8 )
                ++retry_timeout;
            sleep( retry_timeout * 3600l ); 
        }
    }
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
