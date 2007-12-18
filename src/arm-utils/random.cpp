
#include <stdio.h>


// Generate random challenge
//
int main( int argc, char** argv )
{
    unsigned long max = 32768;
    if ( argc >= 2 )
        sscanf( argv[1], "%lu", &max );

    FILE* f = fopen( "/dev/urandom", "rb" );
    if ( ! f )
    {
        printf( "0\n" );
        return 1;
        }

    unsigned short rnd = 0;
    fread( &rnd, 1, sizeof( rnd ), f );
    fclose( f );

    unsigned long result = (unsigned long)( double( max ) * rnd / 65536.0 );
    if ( result >= max )
        result--;

    printf( "%lu\n", result );

    return 0;
    }
