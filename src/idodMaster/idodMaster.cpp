#include "DTS.h"

#include <getopt.h>

DTS dts;

int  opt_ID = -1;
int  opt_verbose = 0;
bool opt_help = false;
char* opt_cfgfile = "/etc/sysconfig/albatross/dsp.cfg";

///////////////////////////////////////////////////////////////////////////

void display_usage( void )
{
    printf( 
        "Usage: idodMaster [OPTION]... <channel>\n"
        "   -c, --cfg=cfgfile      DSP config file.\n"
        "                          Default: /etc/sysconfig/albatross/dsp.cfg\n"
        "   -V, --help             Display this help and exit.\n"
        "   -v, --verbose          Show trace information.\n"
        );
    }

///////////////////////////////////////////////////////////////////////////////

int main( int argc, char** argv )
{
    for( ;; )
    {
        int option_index = 0;
        static struct option long_options[] = 
        {
            { "cfg",     1, 0, 0   }, // -c cfgfile
            { "help",    0, 0, 0   }, // -V
            { "verbose", 0, 0, 0   }, // -v
            { 0,         0, 0, 0   }
            };

        int c = getopt_long (argc, argv, "c:Vv", long_options, &option_index );
        if ( c == -1 )
            break;

        if ( c == 0 )
        {
            switch( option_index )
            {
                case 0:  c = 'c'; break; // cfg
                case 1:  c = 'V'; break; // help
                case 2:  c = 'v'; break; // verbose
                default: c = '?'; break; // ?unknown?
                }
            }

        switch( c ) 
        {
            case 'c':
                opt_cfgfile = optarg;
                break;

            case 'V':
                opt_help = true;
                break;

            case 'v':
                ++opt_verbose;
                break;

            case '?':
                display_usage ();
                return 1;
            }
        }

    if ( opt_help )
    {
        display_usage ();
        return 0;
        }

    if ( optind < argc )
    {
        opt_ID = strtol( argv[ optind ], NULL, 10 );
        }
    else
    {
        display_usage ();
        return 1;
        }

    if ( opt_verbose >= 1 )
    {
        printf( "Running parameters:\n" );
        printf( "    <channel>  %d\n", opt_ID );
        printf( "    --cfg      cfgfile=[%s]\n", opt_cfgfile );
        printf( "    --help     %d\n", opt_help );
        printf( "    --verbose  %d\n", opt_verbose);
        if ( optind < argc )
        {
            printf ("Non-option ARGV-elements: ");
            for( int i = optind; i < argc ; i++ ) 
                printf( "%s ", argv[ i ] );
            printf ("\n");
            }
        }

    if ( ! dts.Open( opt_ID, opt_cfgfile ) )
        return -1;

    dts.Main ();

    return 0;
    }
