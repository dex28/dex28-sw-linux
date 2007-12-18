
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

extern unsigned long crc32( unsigned long crc, const unsigned char* buf, unsigned int len );

class u_boot_env
{
    enum 
    {
        // ENV_TOTAL_SIZE must be n * flash_erase_block_size, where n is integer >= 1
        //
        ENV_TOTAL_SIZE = 0x10000, 
        ENV_HEADER_SIZE = sizeof(unsigned long)*2 + sizeof(unsigned short),
        ENV_DATA_SIZE = ENV_TOTAL_SIZE - ENV_HEADER_SIZE
    };

    struct environment
    {
        unsigned long crc;
        unsigned long revno_crc;
        unsigned short revision_no;
        char data[ ENV_DATA_SIZE ];
    } __attribute__ ((packed));

    // Load()/Save() device
    const char* devname;
    long offset;

    // Tracked info about environment
    long data_len;
    unsigned long real_crc;
    unsigned long real_revno_crc;

    ////////////////////////////////////////////////////////////////////////////
    environment sect;
    char end_of_data[2]; // must follow environment data and contain double '\0'
    ////////////////////////////////////////////////////////////////////////////

public:

    enum
    {
        OBSOLETE_FLAG = 0,
        ACTIVE_FLAG = 1
    };

    void update_crc( void )
    {
        sect.crc = crc32( 0, (unsigned char*)sect.data, sizeof(sect.data) );
        sect.revno_crc = crc32( 0, (unsigned char*)&sect.revision_no, sizeof(sect.revision_no) );

        real_crc = sect.crc;
        real_revno_crc = sect.revno_crc;
    }

    bool is_crc_OK( void ) const
    {
        return ( real_crc == sect.crc ) && ( real_revno_crc == sect.revno_crc );
    }

    int get_revision_no( void ) const
    {
        return sect.revision_no;
    }

    void init_empty_and_invalid( void )
    {
        data_len = 2;
        memset( &sect, 0, sizeof( sect ) );
        end_of_data[0] = '\0';
        end_of_data[1] = '\0';
        real_crc = 1;
        real_revno_crc = 1;
    }

    void delete_data( void )
    {
        data_len = 2;
        memset( sect.data, 0, sizeof( sect.data ) );
        update_crc ();
    }

    bool load( const char* p_devname, long p_offset = 0 )
    {
        // Note: Returns false only when I/O operations are failed!

        init_empty_and_invalid ();

        devname = p_devname;
        offset = p_offset;

        int fd = open( devname, O_RDONLY );
		if ( fd < 0 )
        {
            fprintf( stderr, "Can't open %s: %s\n", devname, strerror(errno) );
            return false;
        }

        mtd_info_user mtd;
        if ( ioctl( fd, MEMGETINFO, &mtd ) < 0 )
        {
            fprintf( stderr, "Loading from Non-MTD file %s\n", devname );
        }
        else if ( mtd.type == MTD_NORFLASH )
        {
            fprintf( stderr, "Loading from NOR Flash %s: Size = %lu, Erase Size = %lu\n", 
                devname,
                (unsigned long)mtd.size, (unsigned long)mtd.erasesize );
        }

        if ( offset != 0 && lseek( fd, offset, SEEK_SET ) == -1) 
        {
            fprintf( stderr, "seek error on %s: %s\n", devname, strerror(errno) );
            close( fd );
            return false;
        }

        int rc = read( fd, &sect.crc, ENV_TOTAL_SIZE );
        if ( rc != ENV_TOTAL_SIZE )
        {
            fprintf( stderr, "read error on %s: %s\n", devname,  strerror(errno) );
            close( fd );
            return false;
        }

        if ( close( fd ) < 0 ) 
        {
            fprintf( stderr, "I/O error on %s: %s\n", devname, strerror (errno) );
            return false;
        }

        real_crc = crc32( 0, (unsigned char*)sect.data, sizeof(sect.data) );
        real_revno_crc = crc32( 0, (unsigned char*)&sect.revision_no, sizeof(sect.revision_no) );

        if ( sect.crc != real_crc )
            fprintf ( stderr, "Bad Data CRC %08lx vs Real %08lx\n", 
                sect.crc, real_crc );
        if ( sect.revno_crc != real_revno_crc )
            fprintf ( stderr, "Bad RevNo CRC %08lx vs Real %08lx\n", 
                sect.revno_crc, real_revno_crc );

        if ( ! is_crc_OK () )
        {
            init_empty_and_invalid ();
        }
        else
        {
            // Update data_len
            //
            char* env = sect.data;
            while( *env )
                env += strlen(env) + 1;
            data_len = env - sect.data;
            if ( data_len & 1 )
                ++data_len;
        }

        fprintf( stderr, "Done.\n" );

        return true;
    }

    void pad_with_FFs( void )
    {
        // fill space after environment with FFs
        //
        for( char* ptr = sect.data + data_len; ptr < end_of_data; ++ptr )
            *ptr = 0xFF; 

        update_crc ();
    }

    static char* envmatch( char* s1, char* s2)
    {
        // s1 is either a simple 'name', or a 'name=value' pair.
        // s2 is a 'name=value' pair.
        // If the names match, return the value of s2, else NULL.
        //
        while ( *s1 == *s2++ )
            if ( *s1++ == '=' )
                return s2;
        if ( *s1 == '\0' && s2[-1] == '=')
            return s2;

        return NULL;
    }

    void show_info( char* title = NULL )
    {
        fprintf( stderr, "-------------------------------------------------------------------\n" );
        if ( title ) fprintf( stderr, "%s\n", title );
        fprintf( stderr, "Size: %lu, CRC is %s [%08lx/%08lx], Revision: %04hx, Device: %s\n",
            data_len, is_crc_OK() ? "OK" : "Bad", sect.crc, sect.revno_crc,
            sect.revision_no, devname );

        if ( ! is_crc_OK () )
        {
            if ( sect.crc != real_crc )
                fprintf ( stderr, "Bad Data CRC %08lx vs Real %08lx\n", 
                    sect.crc, real_crc );
            if ( sect.revno_crc != real_revno_crc )
                fprintf ( stderr, "Bad RevNo CRC %08lx vs Real %08lx\n", 
                    sect.revno_crc, real_revno_crc );
        }
    }

    void printenv( void )
    {
        if ( ! is_crc_OK () )
            return;

        for ( char* env = sect.data; *env; env += strlen(env) + 1 ) 
        {
            printf( "%s\n", env );
        }
    }

    void printenv( char* name )
    {
        if ( ! is_crc_OK () )
            return;

        for ( char* env = sect.data; *env; env += strlen(env) + 1 ) 
        {
            const char* value = envmatch( name, env );
            if ( value )
            {
                printf( "%s\n", value );
                break;
            }
        }
    }

    bool setenv( char* name )
    {
        if ( ! is_crc_OK () )
        {
            fprintf ( stderr, "Bad CRC; cannot add \"%s\"\n", name );
            return false;
        }

        fprintf( stderr, "setenv: %s\n", name );

        int new_len = strlen( name );
#if 0
        // Ethernet Address and serial# can be set only once
        //
        if ( envmatch( "ethaddr", name ) )
        {
            fprintf ( stderr, "Cannot overwrite \"ethaddr\"!\n" );
            return false;
        }
        else if ( envmatch( "serial#", name ) )
        {
            fprintf ( stderr, "Cannot overwrite \"serial#\"!\n" );
            return false;
        }
#endif
        // Search if variable with this name already exists
        //
        char* env = sect.data;
        char* next = sect.data; // will point to next AV pair
        while( *env )
        {
            next = env + strlen( env ) + 1;

            if ( envmatch( name, env ) )
            {
                // fprintf ( stderr, "Deleting \"%s\"\n", env );
                break;
            }

            env = next;
        }

        // Delete any existing definition
        //
        while( *next )
        {
            while( *next )
                *env++ = *next++;
            *env++ = '\0';
            ++next;
        }

        char* value = strchr( name, '=' );
        if ( value && value[1] ) // value is not empty
        {
            // Append the new value at the end
            //
            if ( env + new_len + 2 >= end_of_data )
            {
                fprintf ( stderr, "No space left for \"%s\"\n", name );
            }
            else
            {
                // fprintf ( stderr, "Adding \"%s\"\n", name );
                next = name;
                while( *next )
                    *env++ = *next++;
                *env++ = '\0';
            }
        }

        *env++ = '\0'; // Terminate the list

        // Update data_len
        data_len = env - sect.data;
        if ( data_len & 1 )
            ++data_len;
            
        update_crc ();

        return true;
    }

    bool save( void )
    {
        pad_with_FFs ();

        int fd = open( devname, O_RDWR );
        if ( fd < 0 )
        {
            fprintf( stderr, "Can't open %s: %s\n", devname, strerror( errno ) );
            return false;
        }

        bool is_flash = false;
        erase_info_t erase;
        erase.start = offset;
        erase.length = ENV_TOTAL_SIZE;

        mtd_info_user mtd;
        if ( ioctl( fd, MEMGETINFO, &mtd ) < 0 )
        {
            fprintf( stderr, "Saving to Non-MTD file %s\n", devname );
            // proceed, assume that it is plain file requiring no special erase 
        }
        else if ( mtd.type == MTD_NORFLASH )
        {
            is_flash = true;
            fprintf( stderr, "Saving to NOR flash %s: Size = %lu, Erase Size = %lu\n", 
                devname,
                (unsigned long)mtd.size, (unsigned long)mtd.erasesize );
            if ( ( ENV_TOTAL_SIZE % mtd.erasesize ) != 0 )
            {
                fprintf( stderr, "Environment size must be multiple of erase size\n" );
                goto close_and_return_error;
            }
        }
        else // we support writing only to NOR flash of all MTD devices
        {
            fprintf( stderr, "Device %s is not NOR flash!\n", devname );
            goto close_and_return_error;
        }

        if ( is_flash )
        {
            fprintf( stderr, "Unlocking flash...\n" );

            ioctl( fd, MEMUNLOCK, &erase );

            fprintf( stderr, "Erasing old environment...\n" );

            if ( ioctl( fd, MEMERASE, &erase ) != 0 ) 
            {
                fprintf( stderr, "MTD erase error on %s: %s\n", devname, strerror (errno));
                goto close_and_return_error;
            }
        }

        fprintf( stderr, "Writing environment...\n" );

        // Write data
        //
        if ( lseek( fd, offset + ENV_HEADER_SIZE, SEEK_SET ) == -1 ) 
        {
            fprintf( stderr, "seek error on %s: %s\n", devname, strerror (errno));
            goto close_and_return_error;
        }
        long len_to_write = is_flash ? data_len : sizeof( sect.data );
        if ( write( fd, &sect.data, len_to_write ) != len_to_write ) 
        {
            fprintf( stderr, "write error on %s: %s\n", devname, strerror (errno));
            goto close_and_return_error;
        }

        // Write header
        //
        if ( lseek( fd, offset, SEEK_SET ) == -1 ) 
        {
            fprintf( stderr, "seek error on %s: %s\n", devname, strerror (errno));
            goto close_and_return_error;
        }
        if ( write( fd, &sect, ENV_HEADER_SIZE ) != ENV_HEADER_SIZE ) 
        {
            fprintf( stderr, "write error on %s: %s\n", devname, strerror (errno));
            goto close_and_return_error;
        }

        if ( is_flash )
        {
            fprintf( stderr, "Locking flash...\n" );
            ioctl( fd, MEMLOCK, &erase );
        }

        if ( close( fd ) < 0 ) 
        {
            fprintf( stderr, "I/O error on %s: %s\n", devname, strerror (errno) );
            return false;
        }

        fprintf( stderr, "Done\n" );

        return true; // everything is OK

    close_and_return_error:

        close( fd );
        return false;
    }

    bool load( u_boot_env* env, bool header_only = false )
    {
        if ( ! env || ! env->is_crc_OK () )
        {
            init_empty_and_invalid ();
            return false;
        }

        if ( header_only )
        {
            init_empty_and_invalid ();
            // get only incremented revision number from othe environment's header
            sect.revision_no = env->sect.revision_no + 1;
        }
        else
        {
            memcpy( &sect, &env->sect, sizeof( sect ) );
            data_len = env->data_len;
            ++(sect.revision_no); // update revision number
        }

        update_crc ();

        return true;
    }

    u_boot_env( void )
    {
        devname = NULL;
        offset = 0;

        init_empty_and_invalid ();
    }
};

/* ========================================================================= */
/*
 * This part is derived from crc32.c from the zlib-1.1.3 distribution
 * by Jean-loup Gailly and Mark Adler.
 *
 * crc32() -- compute the CRC-32 of a data stream
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */
/* ========================================================================= */

#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

/* ========================================================================= */
unsigned long crc32
( 
    unsigned long crc, 
    const unsigned char* buf, 
    unsigned int len 
    )
{
    static bool crc_table_empty = true;
    static unsigned long crc_table[256];

// Generate a table for a byte-wise 32-bit CRC calculation on the polynomial:
//   x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.
//
// Polynomials over GF(2) are represented in binary, one bit per coefficient,
// with the lowest powers in the most significant bit.  Then adding polynomials
// is just exclusive-or, and multiplying a polynomial by x is a right shift by
// one.  If we call the above polynomial p, and represent a byte as the
// polynomial q, also with the lowest power in the most significant bit (so the
// byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
// where a mod b means the remainder after dividing a by b.
//
// This calculation is done using the shift-register method of multiplying and
// taking the remainder.  The register is initialized to zero, and for each
// incoming bit, x^32 is added mod p to the register if the bit is a one (where
// x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
// x (which is shifting right by one and adding x^32 mod p if the bit shifted
// out is a one).  We start with the highest power (least significant bit) of
// q and repeat for all eight bits of q.
//
// The table is simply the CRC of all possible eight bit values.  This is all
// the information needed to generate CRC's on data a byte at a time for all
// combinations of CRC register values and incoming bytes.
//
    if (crc_table_empty)
    {
        // terms of polynomial defining this crc (except x^32):
        static const unsigned char p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};

        // make exclusive-or pattern from polynomial (0xedb88320L) 
        unsigned long poly = 0L; // polynomial exclusive-or pattern
        for ( unsigned n = 0; n < sizeof(p)/sizeof(unsigned char); n++ )
            poly |= 1L << (31 - p[n]);

        for (int n = 0; n < 256; n++)
        {
            unsigned long c = (unsigned long)n;
            for (int k = 0; k < 8; k++)
                c = c & 1 ? poly ^ (c >> 1) : c >> 1;
            crc_table[n] = c;
        }

        crc_table_empty = false;
    }

    crc = crc ^ 0xffffffffL;
    while ( len >= 8 )
    {
        DO8(buf);
        len -= 8;
    }
    if ( len )
    {
        do { DO1( buf ); } 
        while ( --len );
    }

    return crc ^ 0xffffffffL;
}

/* ========================================================================= */

struct UBOOT_ENV
{
    // Environment holder: initialized empty but invalid
    // Should call update_crc() to make it empty and valid.
    //
    u_boot_env mtd1;
    u_boot_env mtd2;

    u_boot_env* cur_env;
    u_boot_env* old_env;
    u_boot_env* new_release;

    bool Load( char* file_mtd1, char* file_mtd2 );
    bool ExecuteCommand( char* cmd );
};
    
bool UBOOT_ENV::Load( char* file_mtd1, char* file_mtd2 )
{
    cur_env = NULL;
    old_env = NULL;
    new_release = NULL;

    if ( ! mtd1.load( file_mtd1 ) )
        return false;

    if ( ! mtd2.load( file_mtd2 ) )
        return false;

    if ( mtd1.is_crc_OK () && ! mtd2.is_crc_OK () )
    {
        cur_env = &mtd1;
        old_env = &mtd2;
    }
    else if ( ! mtd1.is_crc_OK () && mtd2.is_crc_OK () )
    {
        cur_env = &mtd2;
        old_env = &mtd1;
    }
    else if ( ! mtd1.is_crc_OK () && ! mtd2.is_crc_OK () )
    {
    }
    else if ( mtd1.is_crc_OK () && mtd2.is_crc_OK () )
    {
        short delta_revision = mtd1.get_revision_no () - mtd2.get_revision_no ();

        if ( delta_revision >= 1 )
        {
            cur_env = &mtd1;
            old_env = &mtd2;
        }
        else
        {
            cur_env = &mtd2;
            old_env = &mtd1;
        }
    }

    if ( ! cur_env )
    {
        mtd1.update_crc ();
        mtd2.update_crc ();
        cur_env = &mtd1;
        old_env = &mtd2;
    }

    old_env->show_info( "Old environment:" );
    cur_env->show_info( "Current environment:" );

    return true;
}

bool UBOOT_ENV::ExecuteCommand( char* cmd )
{
    char line[ 256 ];
    strcpy( line, cmd );

    // Skip leading spaces
    char* name = line;
    while( name[0] && isspace( name[0] ) )
        ++name;

    // Command '?[name]': print new_release environment
    if ( name[0] == '?' )
    {
        if ( new_release )
        {
            new_release->show_info( "New release (editing):" );
            if ( name[1] ) new_release->printenv( name + 1 );
            else new_release->printenv ();
        }
        else
        {
            cur_env->show_info( "Current environment:" );
            if ( name[1] ) cur_env->printenv( name + 1 );
            else cur_env->printenv ();
        }
        return true;
    }

    // Command 'quit': exit application
    if ( strncmp( name, ":quit", 5 ) == 0 )
    {
        return false;
    }

    // Command 'save': Save new_release and make it current
    if ( strncmp( name, ":save", 5 ) == 0 )
    {
        if ( ! new_release )
        {
            fprintf( stderr, "Nothing changed. Nothing to save!\n" );
        }
        else
        {
            // Save new release
            if ( ! new_release->save () )
                return false;
            old_env = cur_env;
            cur_env = new_release;
            new_release = NULL; // Release new release :)
        }
        return true;
    }

    // Command 'empty': Clear all contents of the new_release
    if ( strncmp( name, ":empty", 6 ) == 0 )
    {
        // Start new relase in old environment holder,
        // if not already started.
        if ( ! new_release )
        {
            new_release = old_env;
            new_release->load( cur_env, /* header only */ true );
            new_release->show_info( "Started new release (empty):" );
        }
        else
        {
            new_release->delete_data ();
            new_release->show_info( "New release (made empty):" );
        }
        return true;
    }

    // Nothing to do if:
    // Nothing to do if:
    //   --  line is empty, or 
    //   --  '=' is missing, or
    //   --  '=' is followed by space, or
    //   --  '=' is preceeded by space
    //
    char* eqptr = strchr( name, '=' );
    if ( ! name[0] || ! eqptr || isspace(eqptr[1]) || isspace(eqptr[-1]) )
        return true;

    // Start new relase in old environment holder,
    // if not already started.
    if ( ! new_release )
    {
        new_release = old_env;
        new_release->load( cur_env );
        new_release->show_info( "Started new release:" );
    }

    // Make changes to new_release
    //
    new_release->setenv( name );
    return true;
}

UBOOT_ENV env;

int main( int argc, char** argv )
{
    if ( argc < 3 )
        return 1;

    if ( ! env.Load( argv[ 1 ], argv[ 2 ] ) )
        return 1;

    // Read and execute commands from argv[], first.
    for ( int i = 3; i < argc; i++ )
        if ( ! env.ExecuteCommand( argv[i] ) )
            return 1;

    if ( argc < 4 ) // no argv[] commands
    {
        // Read and execute commands from stdin
        char line[ 256 ];
        while( fgets( line, sizeof( line ) - 2, stdin ) )
        {
            // remove EOL character (if any)
            char* eol = strrchr( line, '\n' );
            if ( eol ) *eol = '\0';

            if ( ! env.ExecuteCommand( line ) )
                return 1;
        }
    }

    return 0;
}

