#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>

//
// alaw2linear() - Convert an A-law value to 16-bit linear PCM
//
//
#define SIGN_BIT    (0x80)      // Sign bit for a A-law byte. 
#define QUANT_MASK  (0xf)       // Quantization field mask. 
#define NSEGS       (8)         // Number of A-law segments. 
#define SEG_SHIFT   (4)         // Left shift for segment number. 
#define SEG_MASK    (0x70)      // Segment field mask. 

void alaw2linear( unsigned char a_val, unsigned char* pcm )
{
    a_val ^= 0x55;

    int t = (a_val & QUANT_MASK) << 4;
    int seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
    switch ( seg )
    {
        case 0:
            t += 8;
            break;
        case 1:
            t += 0x108;
            break;
        default:
            t += 0x108;
            t <<= seg - 1;
        }

    t = ( a_val & SIGN_BIT ) ? t : -t;
    pcm[ 0 ] = ( t >> 8 ) & 0xFF;
    pcm[ 1 ] = ( t >> 0 ) & 0xFF;
    }

struct RIFF_WAVE                         // Little-Endian short & long members
{
    // RIFF chunk
    unsigned char  riff_chunkID[4];      // 'RIFF'
    unsigned long  riff_chunkSize;       // sizeof RIFF chunk body = 48 + sizeof data samples
    unsigned char  riff_format[4];       // 'WAVE'
    // FMT chunk
    unsigned char  fmt_chunkID[4];       // 'fmt '
    unsigned long  fmt_chunkSize;        // sizeof FMT chunk body = 18
    unsigned short fmt_audioFormat;      // WAVE_FORMAT_ALAW = 0x0006
    unsigned short fmt_numChannels;      // 1
    unsigned long  fmt_samplesPerSec;    // 8000
    unsigned long  fmt_avgBytesPerSec;   // 8000
    unsigned short fmt_blockAlign;       // 1
    unsigned short fmt_bitsPerSample;    // 8
    unsigned short fmt_extSize;          // 0
    // FACT chunk
    unsigned char  fact_chunkID[4];      // 'fact'
    unsigned long  fact_chunkSize;       // sizeof FACT chunk body = 4
    unsigned long  fact_sampleLength;    // sizeof data samples
    // DATA chunk
    unsigned char  data_chunkID[4];      // 'data' = 0x61746164
    unsigned long  data_chunkSize;       // sizeof data samples
                                         // data samples comes here
    } __attribute__((packed));

RIFF_WAVE wave_header;

int main( int argc, char** argv )
{
    if ( argc < 3 )
        return -1;

    const char* bps = argv[1];
    const char* wav_filename = argv[2];
    const char* DN = argc >= 4 ? argv[3] : "Unknown";
    const char* remote_party = argc >= 5 ? argv[4] : "Unknown";

    char res_filename[ 256 ];
    strcpy( res_filename, wav_filename );

    ///////////////////////////////////////////////////////////////////////////
    // CONVERT TO MP3
    //
    FILE* inf = fopen( wav_filename, "r" );
    if ( inf == NULL )
        return -1;

    fread( &wave_header, 1, sizeof( wave_header ), inf );

    char* dotp = strrchr( res_filename, '.' );
    if ( dotp ) *dotp = 0;

    char* basename = strrchr( res_filename, '/' );
    if ( basename ) ++basename;
    else basename = res_filename;

    char cmd[ 512 ];
    sprintf( cmd, "/albatross/bin/lame -S -f -b %s -r -s 8 -m m"
        " --ta '%s' --tl '%s' --tt '%s' - '%s.mp3'", 
        bps, DN, remote_party, basename, res_filename 
        );

    strcat( res_filename, ".mp3" );

    FILE* outf = popen( cmd, "w" );
    if ( outf == NULL )
        return -1;

    for ( ;; )
    {
        unsigned char buf[ 576 * 2 ];
        int len = fread( buf, 1, sizeof( buf ), inf );
        if ( len <= 0 )
            break;

        unsigned char pcm[ 2 * sizeof( buf ) ]; 
        unsigned char* ptr = pcm;
        for ( int i = 0; i < len; i++, ptr += 2 )
            alaw2linear( buf[ i ], ptr );

        fwrite( pcm, 2, len, outf );
        fflush( outf );
        }

    int rc = pclose( outf );
    fclose( inf );

    if ( rc < 0 )
        return -1;

    // Unlink original file if lame was successfull
    //
    unlink( wav_filename );

    return 0;
    }
