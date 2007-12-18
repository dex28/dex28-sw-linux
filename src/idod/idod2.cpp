
#include <cstdio>
#include <cctype>
#include <cstring>

#include <sys/ioctl.h>
#include "albatros/hpi/hpi.h"

using namespace std;

int main( int argc, char** argv )
{
	unsigned char line[ 4096 ];
	FILE* f = NULL;
   
	if ( argc >= 1 )
		f = fopen( argv[ 1 ], "rb" );


	if ( ! f )
	{
		// printf( "%s: <not found>\n", argv[ 1 ]  );
		return -1;
	}

    // std::ioctl( fileno( f ), IOCTL_HPI_SET_FILTER_VALUE, 0x01 );
    // std::ioctl( fileno( f ), IOCTL_HPI_SET_FILTER_MASK, 8 ); // first 8 bits

	for (;;)
	{
		int rd = fread( line, 1, 2, f );

		if ( 0 == rd )
			break; // EOF

		if ( 2 != rd )
		{
			break;
		}

		int len = ( line[ 0 ] << 8 )  + line[ 1 ];

		rd = fread( line + 2, 1, len, f );

		if ( 0 == rd )
			break; // EOF

		if ( len != rd )
		{
			break;
		}

        fwrite( line, 1, len + 2, stdout );
		fflush( stdout );
	}

    return 0;
}
