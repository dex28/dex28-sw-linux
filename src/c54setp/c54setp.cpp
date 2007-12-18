#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/select.h>

#include <sys/ioctl.h>
#include "../hpi/hpi.h"

#include "c54setp.h"

static char* dsp_cfg = "/etc/sysconfig/albatross/dsp.cfg";

static bool update_port_settings( char* port, const PORT_PARAMETERS& defaults, int warm_restart )
{
    int abs_ch = atoi( port ) - 1;

    PORT_PARAMETERS params;
    if ( ! params.Load( port ) || params.USE_DEFAULTS )
    {
        params = defaults;
        params.LOCAL_UDP += abs_ch;
        }

    // params.Dump ();

    DSPCFG dspCfg;
    if ( ! dspCfg.Find( abs_ch, dsp_cfg ) )
    {
        fprintf( stderr, "Port is not configured.\n" );
        return false;
        }

    if ( ! dspCfg.isRunning () )
    {
        fprintf( stderr, "DSP is not running on port.\n" );
        return false;
        }

    char dspboard[ 128 ];
    strcpy( dspboard, dspCfg.devname );
    strcat( dspboard, "io" );

    return params.SaveTo( dspboard, dspCfg.channel, warm_restart );
    }

static bool query_port_settings( char* port )
{
    int abs_ch = atoi( port ) - 1;

    DSPCFG dspCfg;
    if ( ! dspCfg.Find( abs_ch, dsp_cfg ) )
    {
        fprintf( stderr, "Port is not configured.\n" );
        return false;
        }

    if ( ! dspCfg.isRunning () )
    {
        fprintf( stderr, "DSP is not running on port.\n" );
        return false;
        }

    int channel = dspCfg.channel;
    char dspboard[ 128 ];
    strcpy( dspboard, dspCfg.devname );
    strcat( dspboard, "io" );

    int fd = open( dspboard, O_RDWR );
    if ( fd < 0 )
    {
        fprintf( stderr, "Failed to open %s.\n", dspboard );
        return false;
        }

    unsigned char line[ 256 ];
    line[ 0 ] = 0x01; // B-Channel IOCTL
    line[ 1 ] = channel; // Channel

    ///////////////////////////////////////////////////////////////////////////
    line[ 2 ] = 0x00; // IOCTL: Query B-Channel Settings
    write( fd, line, 3 );

    close( fd );
    return true;
    }

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        fprintf( stderr, "usage: c54setp {all|<port>} [cold|query]\n" );
        return 1;
        }

    char* port = argv[ 1 ];
    int warm_restart = argc < 3 || strcmp( argv[ 2 ], "cold" ) != 0;
    int query = argc >= 3 && strcmp( argv[ 2 ], "query" ) == 0;

    if ( strcmp( port, "default" ) == 0 )
        port = "all";

    if ( query )
    {
        if ( strcmp( port, "all" ) == 0 ) // All channels
        {
            query_port_settings( "1" );
            query_port_settings( "2" );
            query_port_settings( "3" );
            query_port_settings( "4" );
            query_port_settings( "5" );
            query_port_settings( "6" );
            query_port_settings( "7" );
            query_port_settings( "8" );
            }
        else // Particular channel
        {
            query_port_settings( port );
            }
        }
    else
    {
        PORT_PARAMETERS defaults;
        if ( !  defaults.Load( "default" ) )
        {
            fprintf( stderr, "Failed to load default settings\n" );
            return 1;
            }

        if ( strcmp( port, "all" ) == 0 ) // single port
        {
            update_port_settings( "1", defaults, warm_restart );
            update_port_settings( "2", defaults, warm_restart );
            update_port_settings( "3", defaults, warm_restart );
            update_port_settings( "4", defaults, warm_restart );
            update_port_settings( "5", defaults, warm_restart );
            update_port_settings( "6", defaults, warm_restart );
            update_port_settings( "7", defaults, warm_restart );
            update_port_settings( "8", defaults, warm_restart );
            }
        else // Particular channel
        {
            update_port_settings( port, defaults, warm_restart );
            }
        }

    return 0;
    }

#include <math.h>

bool PORT_PARAMETERS::SaveTo( char* dspboard, int channel, int warm_restart )
{
    int fd = open( dspboard, O_RDWR );
    if ( fd < 0 )
    {
        fprintf( stderr, "Failed to open %s.\n", dspboard );
        return false;
        }

    unsigned char data[ 256 ];
    data[ 0 ] = 0x01; // B-Channel IOCTL
    data[ 1 ] = channel; // Channel

    ///////////////////////////////////////////////////////////////////////////
    int ptr = 2;
    data[ ptr++ ] = 0x01; // IOCTL: Update CODEC Settings
    data[ ptr++ ] = CODEC;
    data[ ptr++ ] = PKT_COMPRTP;
    data[ ptr++ ] = ( PKT_SIZE >> 8 ) & 0xFF;
    data[ ptr++ ] = PKT_SIZE & 0xFF;
    data[ ptr++ ] = BCH_LOOP;

    // TX_AMP normalized to 0dB = 256, 18dB = 2033, -18dB = 32
    //
    unsigned int amp = 256;
    switch ( TX_AMP )
    {
        case -18: amp =   32; break;
        case -12: amp =   64; break;
        case  -6: amp =  128; break;
        case   0: amp =  256; break;
        case   6: amp =  512; break;
        case  12: amp = 1024; break;
        case  18: amp = 2048; break;
        default : amp  = (unsigned int)round( 256.0 * pow( 10.0, TX_AMP / 20.0 ) );
        }
    data[ ptr++ ] =  ( amp >> 8 ) & 0xFF;
    data[ ptr++ ] =  amp & 0xFF;

    write( fd, data, ptr );

    ///////////////////////////////////////////////////////////////////////////
    ptr = 2;
    data[ ptr++ ] = 0x02; // IOCTL: Update Jitter Buffer Settigns
    data[ ptr++ ] = ( JIT_MINLEN >> 8 ) & 0xFF;
    data[ ptr++ ] = JIT_MINLEN & 0xFF;
    data[ ptr++ ] = ( JIT_MAXLEN >> 8 ) & 0xFF;
    data[ ptr++ ] = JIT_MAXLEN & 0xFF;
    data[ ptr++ ] = JIT_ALPHA;
    data[ ptr++ ] = JIT_BETA;
    write( fd, data, ptr );

    ///////////////////////////////////////////////////////////////////////////
    ptr = 2;
    data[ ptr++ ] = 0x03; // IOCTL: Update LEC Settings
    data[ ptr++ ] = warm_restart;
    data[ ptr++ ] = ( GEN_P0DB >> 8 ) & 0xFF;
    data[ ptr++ ] = GEN_P0DB & 0xFF;
    data[ ptr++ ] = PKT_BLKSZ;
    data[ ptr++ ] = PKT_ISRBLKSZ;
    data[ ptr++ ] = LEC;
    data[ ptr++ ] = ( LEC_LEN >> 8 ) & 0xFF;
    data[ ptr++ ] = LEC_LEN & 0xFF;
    data[ ptr++ ] = LEC_INIT;
    data[ ptr++ ] = LEC_MINERL;
    data[ ptr++ ] = ( NFE_MINNFL >> 8 ) & 0xFF;
    data[ ptr++ ] = NFE_MINNFL & 0xFF;
    data[ ptr++ ] = ( NFE_MAXNFL >> 8 ) & 0xFF;
    data[ ptr++ ] = NFE_MAXNFL & 0xFF;
    data[ ptr++ ] = NLP;
    data[ ptr++ ] = ( NLP_MINVPL >> 8 ) & 0xFF;
    data[ ptr++ ] = NLP_MINVPL & 0xFF;
    data[ ptr++ ] = ( NLP_HANGT >> 8 ) & 0xFF;
    data[ ptr++ ] = NLP_HANGT & 0xFF;
    data[ ptr++ ] = FMTD;
    write( fd, data, ptr );

    ///////////////////////////////////////////////////////////////////////////
    ptr = 2;
    data[ ptr++ ] = 0x05; // IOCTL: Update VRVAD Setings
    data[ ptr++ ] = ( VRVAD_THRSH >> 8 ) & 0xFF;
    data[ ptr++ ] = VRVAD_THRSH & 0xFF;
    data[ ptr++ ] = ( VRVAD_HANGT >> 8 ) & 0xFF;
    data[ ptr++ ] = VRVAD_HANGT & 0xFF;
    write( fd, data, ptr );

    close( fd );
    return true;
}

