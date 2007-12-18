

#include <sched.h> // sched_setscheduler()
#include <unistd.h> // sleep()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include "../hpi/hpi.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

bool manual_start = false;
long sleep_delay = 1000000;

class DSPCFG
{
    int ID;
    char lockname[ 128 ];

public:

    int dev;
    char devname[ 64 ];
    int channelcount;

    int cpuusage;
    unsigned long uptime;
    int chstat[ 10 ];

    DSPCFG( void )
    {
        ID = 0;
        channelcount = 0;
        strcpy( devname, "" );
        strcpy( lockname, "" );
        dev = -1;
        }

    ~DSPCFG ()
    {
        if ( dev >= 0 )
            close( dev );
        }

    bool Open( int id, const char* dname, int chc, const char* lname )
    {
        ID = id;
        strcpy( devname, dname );
        channelcount = chc;
        strcpy( lockname, lname );
        dev = open( devname, O_RDWR );
        return dev >= 0;
        }

    void GetWdReport( void )
    {
        unsigned char dspwd_report[ 256 ] = { 0 };

        if ( dev >= 0 && ioctl( dev, IOCTL_HPI_GET_DSPWD_REPORT, &dspwd_report ) >= 0 )
        {
            cpuusage = ( dspwd_report[ 5 ] << 8 ) + dspwd_report[ 6 ]; // got in promiles

            uptime
                = ( dspwd_report[ 1 ] << 24 ) + ( dspwd_report[ 2 ] << 16 )
                + ( dspwd_report[ 3 ] << 8 ) + dspwd_report[ 4 ];

            for ( int i = 0; i < channelcount; i++ )
                chstat[ i ]  = dspwd_report[ 8 + 2 * i ];
            }
        }
    };

DSPCFG dsp[ 4 ];
int dspCount = 0;

int main( int argc, char** argv )
{
    if ( argc >= 2 )
    {
        manual_start = true;
        sleep_delay = atol( argv[1] ) * 1000;
        }
#if 0
	struct sched_param param;
	param.sched_priority=1;
	if ( sched_setscheduler(0, SCHED_RR, &param) == -1 ) 
		printf( "Error while sched_setscheduler(SCHED_RR)\n" );
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Read dsp configuration and create objects
    //
    FILE* f = fopen( "/etc/sysconfig/albatross/dsp.cfg", "r" );
    if ( f )
    {
        for( int i = 0; i < 4; i++ ) // Max 4 DSPs !
        {
            char line[ 256 ];
            if ( ! fgets( line, sizeof( line ), f ) )
                break;

            char devname[ 64 ];
            char coffname[ 128 ];
            int channels;
            char lockname[ 128 ];

            if ( 4 != sscanf( line, " %s %s %d %s", devname, coffname, &channels, lockname ) )
                continue;

            if ( dsp[ dspCount ].Open( dspCount, devname, channels, lockname ) )
                dspCount++;

            }

        fclose( f );
        }

    /////////////////////////////////////////////////////////////////////////

	for ( ;; )
	{
        for ( int i = 0; i < dspCount; i++ )
            dsp[ i ].GetWdReport ();
           
	    int cpu_usage, cpu_total;
	    extern void getProcStatInfo(int& value, int& total );
	    getProcStatInfo( cpu_usage, cpu_total );

	    unsigned long mem_usage, mem_total;
	    extern void getProcMemInfo(unsigned long& value, unsigned long& total );
	    getProcMemInfo( mem_usage, mem_total );

        unsigned long rx_bytes, rx_total, tx_bytes, tx_total;
        extern void getProcNetInfo( unsigned long& rx_value, unsigned long& rx_total, unsigned long& tx_value, unsigned long& tx_total );
        getProcNetInfo( rx_bytes, rx_total, tx_bytes, tx_total );

        /////////////////////////////////////

        if ( manual_start )
        {
            static unsigned long tstamp = 0;
            printf( "[%lu] ", ++tstamp );
            }

        if ( dspCount == 0 )
        {
            printf( "X" );
            }
        else
        {
            for ( int i = 0; i < dspCount; i++ )
            {
                for ( int j = 0; j < dsp[ i ].channelcount; j++ )
                {
                    const char s[] = { '-', '?', '*', 'R', 'T', 'F' };
                    printf( "%c", 
                        s[ dsp[ i ].chstat[ j ] % sizeof(s) ]
                        );
                    }
                }
            }

        printf( " %d %lu %lu",
	        cpu_usage, mem_usage, mem_total
            );

        printf( " %lu %lu %lu %lu",
	        rx_bytes, rx_total, tx_bytes, tx_total
            );

        for ( int i = 0; i < dspCount; i++ )
        {
            printf( " %d", dsp[ i ].cpuusage );
            }

        printf( "\r\n" );

		fflush( stdout );

		usleep( sleep_delay );
	}

	return 0;
}


void getProcStatInfo(int& value, int& total )
{
    enum { hist_size = 10 };
    static int hist_ptr = -1;
    static unsigned long hist_cpu[ hist_size ] = { 0 };
    static unsigned long hist_sum_cpu = 0;
    static unsigned long hist_tot[ hist_size ] = { 0 };
    static unsigned long hist_sum_tot = 0;

    static long last_values[4] = { 0, 0, 0, 0 };

    FILE* proc_stat = fopen("/proc/stat", "r");

    if (! proc_stat) 
    {
        fprintf(stderr, "ERROR: Could not open /proc/stat");
        return;
        }

    char line[256];
    fgets( line, sizeof( line ), proc_stat );

    if ( strncmp( line, "cpu ", 4 ) == 0 ) 
    {
        unsigned long user_read, system_read, nice_read, idle_read;
        long user, system, nice, idle;

        sscanf( line, "%*s %lu %lu %lu %lu", &user_read, &system_read, &nice_read,
                &idle_read);

        if ( hist_ptr == -1 ) // first time only
        {
            last_values[0] = user_read;
            last_values[1] = system_read;
            last_values[2] = nice_read;
            last_values[3] = idle_read;
            }

        user   = (long)user_read   - last_values[0];
        system = (long)system_read - last_values[1];
        nice   = (long)nice_read   - last_values[2];
        idle   = (long)idle_read   - last_values[3];

        long cpu_usage = user + system + nice;
        long tot_usage = cpu_usage + idle;

        if ( ++hist_ptr >= hist_size ) 
            hist_ptr = 0;

        hist_sum_cpu -= hist_cpu[ hist_ptr ];
        hist_sum_cpu += ( hist_cpu[ hist_ptr ] = cpu_usage );

        hist_sum_tot -= hist_tot[ hist_ptr ];
        hist_sum_tot += ( hist_tot[ hist_ptr ] = tot_usage );

        last_values[0] = user_read;
        last_values[1] = system_read;
        last_values[2] = nice_read;
        last_values[3] = idle_read;

        if ( hist_sum_tot == 0 )
            value = 0;
        else
            value = 1000 * hist_sum_cpu / hist_sum_tot;

        total = 1000;
        }

    fclose(proc_stat);
    }


void
getProcMemInfo( unsigned long& value, unsigned long& total )
{
    unsigned long mem_total = 1, mem_used = 0, mem_free = 0;

    FILE* proc_meminfo = fopen( "/proc/meminfo", "r" );

    if( ! proc_meminfo ) 
    {
        fprintf(stderr, "ERROR: Could not open /proc/meminfo");
        return;
        }

    char line[ 256 ];

    while( fgets(line, sizeof( line ), proc_meminfo) )
    {
        unsigned long v = 0;

        if ( 1 == sscanf( line, "MemTotal: %lu", &v ) )
        {
            mem_total = v;
            }
        else if ( 1 == sscanf( line, "MemFree: %lu", &v ) )
        {
            mem_free += v;
            }
        else if ( 1 == sscanf( line, "Buffers: %lu", &v ) )
        {
            mem_free += v;
            }
        else if ( 1 == sscanf( line, "Cached: %lu", &v ) )
        {
            mem_free += v;
            break;
            }
        else if ( 3 == sscanf( line, "Mem: %lu %lu %lu", &mem_total, &mem_used, &mem_free ) )
        {
            mem_total /= 1024;
            mem_free /= 1024;
            break;
            }
        }

    fclose( proc_meminfo );

    value = mem_total - mem_free;
    total = mem_total;
    }

void getProcNetInfo(unsigned long& rx_value, unsigned long& rx_total, unsigned long& tx_value, unsigned long& tx_total )
{
    static unsigned long last_rx_total = 0, last_tx_total = 0;

    FILE* proc_stat = fopen("/proc/net/dev", "r");

    if (! proc_stat) 
    {
        fprintf(stderr, "ERROR: Could not open /proc/net/dev");
        return;
        }

    char line[256];
    while( fgets( line, sizeof( line ), proc_stat ) )
    {
        int rc = sscanf( line, " eth0: %lu %*s %*s %*s %*s %*s %*s %*s %lu", &rx_total, &tx_total );
        if ( 2 == rc  )
        {
            rx_value = rx_total - ( last_rx_total ? last_rx_total : rx_total );
            tx_value = tx_total - ( last_tx_total ? last_tx_total : tx_total );
            last_rx_total = rx_total;
            last_tx_total = tx_total;
            }
        }

    fclose(proc_stat);
    }

