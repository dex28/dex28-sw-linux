
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char* CFGFILE = "/etc/init.d/services";
const char* TMPFILE = "/tmp/services.tmp";

// CFGFILE format:
// <svcname> <start#> <kill#> 0 1 2 3 4 5 6
// 0-6 one of 's' (start) or 'k' (kill)

int main( int argc, char** argv )
{
    if ( argc >= 2 && strcmp( argv[1], "--help" ) == 0 )
    {
        printf( "usage: %s <svcname> [ {+|-}<levels> ]...\n", argv[ 0 ] );
        return 0;
        }

    const char* svcName = argc > 1 ? argv[1] : NULL;

    FILE* inf = fopen( CFGFILE, "r" );
    if ( ! inf )
    {
        fprintf( stderr, "Could not open %s for reading\n", CFGFILE );
        return 1;
        }

    FILE* outf = NULL;

    if ( argc > 2 ) // if there are +/- levels args
    {
        outf = fopen( TMPFILE, "w" );
        if ( ! outf )
        {
            fprintf( stderr, "Could not open %s for writing\n", TMPFILE );
            fclose( inf );
            return 1;
            }
        }

    char line[ 256 ];
    while( fgets( line, sizeof( line ), inf ) )
    {
        char pName[ 64 ]; 
        char pS[ 64 ]; 
        char pK[ 64 ]; 
        char pI[ 7 ][ 32 ];

        if ( 10 == sscanf( line, "%s %s %s %s %s %s %s %s %s %s", 
                           pName, pS, pK, pI[0], pI[1], pI[2], pI[3], pI[4], pI[5], pI[6] )
            )
        {
            if ( svcName == NULL || strcmp( pName, svcName ) == 0 )
            {
                if ( svcName != NULL )
                {
                    printf( "%-11s S  K   0 1 2 3 4 5 6\n", "#" );
                    }

                printf( "%-11s %-2s %-2s  %s %s %s %s %s %s %s\n", 
                     pName, pS, pK, pI[0], pI[1], pI[2], pI[3], pI[4], pI[5], pI[6] );

                for ( int i = 2; i < argc; i++ ) // Go through arguments
                {
                    const char* startKillFlag = 0;

                    if ( argv[i][0] == '+' )
                    {
                        startKillFlag = "s";
                        }
                    else if ( argv[i][0] == '-' )
                    {
                        startKillFlag = "k";
                        }
                    else
                    {
                        fprintf( stderr, "Ignored: %s\n", argv[i] );
                        }

                    if ( startKillFlag )
                    {
                        for ( unsigned int j = 1; j < strlen( argv[i] ); j++ ) // Go through levels
                        {
                            int level = argv[ i ][ j ] - '0';
                            if ( level >=0 && level <= 6 )
                            {
                                strcpy( pI[ level ], startKillFlag );
                                }
                            }

                        printf( "%-11s      = %s %s %s %s %s %s %s\n", 
                            argv[i], pI[0], pI[1], pI[2], pI[3], pI[4], pI[5], pI[6] );
                        }
                    }

                }

            if ( outf )
            {
                fprintf( outf, "%-11s %-2s %-2s  %s %s %s %s %s %s %s\n", 
                         pName, pS, pK, pI[0], pI[1], pI[2], pI[3], pI[4], pI[5], pI[6] );
                }
            }
        }

    fclose( inf );

    if ( outf )
    {
        fclose( outf );
        execl( "/bin/mv", "/bin/mv", TMPFILE, CFGFILE, NULL );
        fprintf( stderr, "exec mv failed; /bin/mv not found\n" );
        return 1;
        }

    return 0;
    }
