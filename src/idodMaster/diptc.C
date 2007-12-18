#include <stdio.h>

int main ()
{
	char buf[42][ 52 ] = { 0 };

	FILE* f = fopen( "iptc.bin", "rb+" );
	for ( int i = 0; i < 42; i++ )
	{
		fseek( f, (41 - i) * 52, SEEK_SET );
		fread( buf[i], 1, 50, f );
		for ( int j = 0; j < 50; j++ )
			if ( buf[i][j] ) buf[i][j] = ' ';
			else buf[i][j] = '*';
		buf[i][50] = 0;
	}

	for ( int i = 0; i < 42; i++ )
	{
		char* chp = buf[i];
		buf[i][50] = 0;

		fprintf( stderr, "[%s]\n", buf[i] );

		for ( ;; )
		{
			while( *chp == ' ' ) chp++; // skip spaces

			if ( *chp != '*' ) // not found begin of '*' array
				break;

			char* beg = chp++;
			while( *chp == '*' ) chp++; // find end of '*' array

			int x1 = 15 + beg - buf[i];
			int x2 = 15 + chp - buf[i] - 1; 
			int y = 15 + i;
			printf( "DrawGraphics( %4d, %4d, %4d, %4d, 1, ( 3 << 5 ) | 1 );\n", 
				x1, y, x2, y );
			chp++; // continue next array
		}

	}

	return 0;
}
	
