
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <time.h>
#include <sys/time.h>

using namespace std;

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

int main ()
{
    MonotonicTimer t;

    for ( int i = 0; i < 1000; ++i ) 
    {
        long mtime = t.Elapsed ();
        t.Restart ();
     
        printf( "Elapsed time: %ld ms\n", mtime ); 

        sleep( 1 );
    }
}
