#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

char line[ 65536 ]; // buffer

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        printf( "Missing argument: file name with form-data.\n" );
        return 1;
        }

    FILE* file = fopen( argv[1], "r+b" );
    if ( ! file )
    {
        printf( "Could not open %s\n", argv[1] );
        return 1;
        }

    if ( fseek( file, 0, SEEK_SET ) != 0 )
    {
        printf( "Failed to rewind file.\n" );
        return 1;
        }

    char content_type[ 256 ] = { 0 };
    char boundary[ 256 ] = "";
    int boundary_len = -1;
    char filename[ 256 ] = "";

    for ( ;; )
    {
        if ( ! fgets( line, sizeof( line ), file ) )
        {
            printf( "Content type not found.\n" );
            return 1;
            }

        if ( strcmp( line, "\r\n" ) == 0 )
            break; // head delimiter found

        if ( boundary_len < 0 ) // first time only
        {
            strcpy( boundary, "\r\n" );
            strcpy( boundary + 2, line );
            boundary_len = strlen( boundary );
            }

        char tag[ 128 ];
        char value[ 512 ];
        if ( 2 == sscanf( line, "%[^:]: %[^\r]", tag, value  ) )
        {
            if ( strcasecmp( tag, "Content-Type" ) == 0 )
                strcpy( content_type, value );
            else if ( strcasecmp( tag, "Content-Disposition" ) == 0 )
            {
                char* fn = strstr( value, "filename=\"" );
                if ( fn )
                {
                    if ( 1 == sscanf( fn, "filename=\"%[^\"]", line ) )
                    {
                        char* fd = strrchr( line, '/' );
                        if ( ! fd ) fd = strrchr( line, '\\' );
                        if ( fd ) strcpy( filename, fd+1 );
                        }
                    }
                }
            }
        }

    if( strcasecmp( content_type, "application/x-gzip-compressed" ) != 0 
    &&  strcasecmp( content_type, "application/x-compressed-tar" ) != 0 
    &&  strcasecmp( content_type, "application/x-compressed-gtar" ) != 0 
    &&  strcasecmp( content_type, "application/x-gtar" ) != 0 
    &&  strcasecmp( content_type, "application/x-gzip" ) != 0 
    &&  strcasecmp( content_type, "application/x-tar" ) != 0 
    &&  strcasecmp( content_type, "application/x-tgz" ) != 0 
    &&  strcasecmp( content_type, "application/x-tar-gz" ) != 0 
    &&  strcasecmp( content_type, "application/tgz" ) != 0 
    &&  strcasecmp( content_type, "application/octet-stream" ) != 0 
    &&  strcasecmp( content_type, "file/tgz" ) != 0 
        ) 
    {
        printf( "Unrecognized content type %s\n", content_type );
        return 1;
        }

    if ( strcasecmp( content_type, "application/octet-stream" ) == 0 
    && strlen( filename ) == 0
        )
    {
        printf( "Missing file with firmware update." );
        return 1;
        }

    long readpos = ftell( file );
    long writepos = 0;
 
    for ( int len = 0 ;; )
    {
        // Go back to 'reading' position block of data

        if ( fseek( file, readpos, SEEK_SET ) != 0 )
        {
            printf( "Failed to seek file.\n" );
            return 1;
            }

        int rc = fread( line + len, 1, sizeof( line ) - len, file );
        if ( rc <= 0 )
            return 4; // premature EOF !

        len += rc;
        readpos = ftell( file ); // remember 'reading' position

        // Go back to 'writing' position to write the same block
        // up to possibly detected boundary

        if ( fseek( file, writepos, SEEK_SET ) != 0 )
        {
            printf( "Failed to seek file.\n" );
            return 1;
            }

        for( int beg = 0; beg < len; )
        {
            char* chp = (char*)memchr( line + beg, '\r', len - beg ); // found CR [LF] ?
            if ( ! chp )
            {
                // not found CR: we can flush complete buffer
                fwrite( line + beg, 1, len - beg, file );
                len = 0;
                break; // break inner loop & fetch new buffer
                }

            // flush everything up to CR
            int p = chp - line;
            if ( p > beg )
                fwrite( line + beg, 1, p - beg, file );
            beg = p;

            if ( beg + boundary_len < len )
            {
                // if found boundary exit
                if ( memcmp( line + beg, boundary, boundary_len ) == 0 )
                {
                    // flush cached data
                    fflush( file );
                    // truncate file to currently written length
                    ftruncate( fileno(file), ftell(file) );
                    fclose( file );
                    printf( "%s", filename );
                    return 0; // only exit point with RC=OK
                    }

                // flush CR as well and continue searching boundary
                fwrite( line + beg, 1, 1, file );
                ++beg;
                }
            else
            {
                // move CR with trailing data into beginning
                memmove( line, line + beg, len - beg );
                len -= beg;
                break; // fetch new data
                }

            }

        writepos = ftell( file ); // remember 'writing' position
        }

    printf( "How did this happen?\n" );
    return 1;
    }

