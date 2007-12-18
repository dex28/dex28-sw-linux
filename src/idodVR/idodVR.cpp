#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/select.h>

#include <sys/ioctl.h>
#include "../hpi/hpi.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> // IPTOS_*
#include <arpa/inet.h>

#include "../c54setp/c54setp.h"
#include "../idodMaster/ELUFNC.h"

#define EOL "\r\n"

using namespace std;

enum
{
    UID_VOICE  = 500, // User ID
    GID_VOICE  = 500  // Group ID
    };

char* dsp_cfg = "/etc/sysconfig/albatross/dsp.cfg";

///////////////////////////////////////////////////////////////////////////
// trim( word ) - cuts all spaces from word

static inline char* trim( char* word )
{
    int i = 0;
    int j = 0;

    while ( word[ i ] != '\0' )
    {
        if ( word[ i ] != ' ' )
        {
            word[ j ] = word[ i ];
            ++i;
            ++j;
        }
        else
            ++i;
    }
    word[ j ] = '\0'; 
    return word;
}

///////////////////////////////////////////////////////////////////////////
// Equpiment Type

enum EQU_TYPE
{
    DTS_UNKNOWN      = -1,

    DTS_OPI_II_TYPE  = 0x0000, // OPI-II instrument
                               // ----------------
    DTS_OPI_II       = 0x0000, // OPI-II console only 
    DTS_OPI_II_B     = 0x0001, // OPI-II with Braille unit 

    DTS_DBC600_TYPE  = 0x0800, // DBC600 Family
                               // ----------------
    DTS_DBC601       = 0x0800, // DBC601
    DTS_DBC631       = 0x0801, // DBC631
    DTS_DBC661       = 0x0802, // DBC661
    DTS_DBC662       = 0x0803, // DBC662
    DTS_DBC663       = 0x0806, // DBC663

    DTS_DBC199       = 0x0D00, // DBC199

    DTS_DBC200_TYPE  = 0x0E00, // DBC200 Family
                               // ----------------
    DTS_DBC201       = 0x0E00, // DBC201
    DTS_DBC202       = 0x0E01, // DBC202
    DTS_DBC203       = 0x0E02, // DBC203
    DTS_DBC203_9     = 0x0E03, // DBC203 + DBY409
    DTS_DBC203_E     = 0x0E04, // DBC203 + EXT
    DTS_DBC203_N9    = 0x0E05, // DBC203 + none + DBY409
    DTS_DBC203_99    = 0x0E06, // DBC203 + DBY409 + DBY409
    DTS_DBC203_E9    = 0x0E07, // DBC203 + EXT + DBY409
    DTS_DBC203_NE    = 0x0E08, // DBC203 + none + EXT
    DTS_DBC203_9E    = 0x0E09, // DBC203 + DBY409 + EXT
    DTS_DBC203_EE    = 0x0E0A, // DBC203 + EXT + EXT
    DTS_DBC214       = 0x0E0B, // DBC214
    DTS_DBC214_9     = 0x0E0C, // DBC214 + DBY409
    DTS_DBC214_E     = 0x0E0D, // DBC214 + EXT
    DTS_DBC214_N9    = 0x0E0E, // DBC214 + none + DBY409
    DTS_DBC214_99    = 0x0E0F, // DBC214 + DBY409 + DBY409
    DTS_DBC214_E9    = 0x0E10, // DBC214 + EXT + DBY409
    DTS_DBC214_NE    = 0x0E11, // DBC214 + none + EXT
    DTS_DBC214_9E    = 0x0E12, // DBC214 + DBY409 + EXT
    DTS_DBC214_EE    = 0x0E13, // DBC214 + EXT + EXT

    DTS_EXTUNIT      = 0x0F00, // External unit
                               // ----------------
    DTS_DBY409       = 0x0F00, // DBY409
    DTS_DBY410       = 0x0F01, // DBY410

    DTS_DBC22X       = 0x1100, // DBC22X D4 Family
                               // ----------------
    DTS_DBC220       = 0x1100, // DBC220
    DTS_DBC222       = 0x1101, // DBC222
    DTS_DBC222_1DSS  = 0x1102, // DBC222, One DSS
    DTS_DBC223       = 0x1103, // DBC223
    DTS_DBC223_1DSS  = 0x1104, // DBC223, One DSS
    DTS_DBC223_2DSS  = 0x1105, // DBC223, Two DSS
    DTS_DBC223_3DSS  = 0x1106, // DBC223, Three DSS
    DTS_DBC223_4DSS  = 0x1107, // DBC223, Four DSS
    DTS_DBC225       = 0x1108, // DBC225
    DTS_DBC225_1DSS  = 0x1109, // DBC225, One DSS
    DTS_DBC225_2DSS  = 0x110A, // DBC225, Two DSS
    DTS_DBC225_3DSS  = 0x110B, // DBC225, Three DSS
    DTS_DBC225_4DSS  = 0x110C, // DBC225, Four DSS

    DTS_OWS          = 0x8000, // OWS
    DTS_OWS_V2       = 0x8002  // OWS Version 2
    };

enum DISPLAY_TYPE
{
    NO_DISPLAY,
    OPI_II_DISPLAY,
    CHARACTER_DISPLAY,
    GRAPHICAL_DISPLAY_223,
    GRAPHICAL_DISPLAY_225
    };

///////////////////////////////////////////////////////////////////////////////

#define  WAVE_FORMAT_ALAW                       0x0006 // Microsoft Corporation
#define  WAVE_FORMAT_MULAW                      0x0007 // Microsoft Corporation
#define  WAVE_FORMAT_G726_ADPCM                 0x0064 // APICOM

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

///////////////////////////////////////////////////////////////////////////////

// Note: The function will block until the whole packet has been read!
//
static inline int read_stream( int fd, unsigned char* buf, int len )
{
    int readc = 0;

    while( readc < len )
    {
        int rc = read( fd, buf + readc, len - readc );
        if ( rc < 0 )
            return rc;
        if ( rc == 0 )
            break;
        readc += rc;
        }

    return readc;
    }

///////////////////////////////////////////////////////////////////////////////

static char NAS_URL1[ 128 ] = "";
static char NAS_URL2[ 128 ] = "";

static char DN[ 64 ] = "";

static char timestamp[ 256 ] = "";
static char cur_date[ 16 ] = "";
static char cur_path[ 16 ] = "";
static char csv_filename[ 256 ] = "";

static int  f_DM = -1;
static int  f_DS = -1;
static int  f_B = -1;

static DISPLAY_TYPE hasDisplay = NO_DISPLAY;
static int rowCount = 0;
static int colCount = 0;
static bool reqRemap = false;
static char remote_party[ 128 ] = { 0 };
static int  bomb_key = -1;
static int  startrec_key = -1;
static int  stoprec_key = -1;
static bool manually_started_recording = false;
static bool manually_stop_recording = false;
static long split_filesize = 10 * 60 * 8000; // max samples per file (def 10 minutes)
static long max_filesize = 30 * 60 * 8000; // abs. max samples per file (def 30 minutes)
static bool IsOWS = false;
static unsigned short localUdpPort = 0;
static int udp_DSCP = 0, tcp_DSCP = 0;

static int  transmission_order = 0;
static int  fd_Wave = -1;
static char filename[ 256 ];
static int  payload_type = 0;
static long sample_count = 0;
static long use_wave_header = false;
static bool is_voice_detected = false;
static bool is_VAD_active = false;
static int  vad_threshold = 50;
static long vad_hangovertime = 8 * 5000; // 5 sec
static long vr_minfilesize =  8800; // 1.1 sec

static RIFF_WAVE wave_header =
{
    { 'R', 'I', 'F', 'F' }, 48, { 'W', 'A', 'V', 'E' },     // RIFF chunk
    { 'f', 'm', 't', ' ' } , 18, 6, 1, 8000, 8000, 1, 8, 0, // fmt chunk
    { 'f', 'a', 'c', 't' },  4, 0,                          // fact chunk
    { 'd', 'a', 't', 'a' },  0                              // data chunk
};

///////////////////////////////////////////////////////////////////////////////

void EquipmentTestRequestReset ()
{
    static unsigned char PDU [] = { 0, 0x20, FNC_EQUTESTREQ, FNC_TEST_RESET, 0x00, 0x00, 0x00, 0x00 };
    PDU[ 0 ] = 1; // write to DTS
    write( f_DM, PDU, sizeof( PDU ) );
    }

void ClearLED( int key )
{
    if ( key < 0 ) return;
    static unsigned char PDU [] = { 0, 0x20, FNC_CLEARLED, 0 };
    PDU[ 0 ] = 1; // write to DTS
    PDU[ 3 ] = key; // key number
    write( f_DM, PDU, sizeof( PDU ) );
    }

void FlashLED1( int key ) // Slow
{
    if ( key < 0 ) return;
    static unsigned char PDU [] = { 0, 0x20, FNC_FLASHLEDCAD1, 0 };
    PDU[ 0 ] = 1; // write to DTS
    PDU[ 3 ] = key; // key number
    write( f_DM, PDU, sizeof( PDU ) );
    }

void FlashLED2( int key ) // Fast
{
    if ( key < 0 ) return;
    static unsigned char PDU [] = { 0, 0x20, FNC_FLASHLEDCAD2, 0 };
    PDU[ 0 ] = 1; // write to DTS
    PDU[ 3 ] = key; // key number
    write( f_DM, PDU, sizeof( PDU ) );
    }

///////////////////////////////////////////////////////////////////////////////

static void getTimestamp( void )
{
    timeval tv;
    gettimeofday( &tv, NULL );
    struct tm* tm = localtime( &tv.tv_sec );

    sprintf( timestamp, "%04d-%02d-%02d %02d-%02d-%02d.%03lu",
        1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec,
        tv.tv_usec / 1000 // millisec
        );

    if ( strncmp( cur_date, timestamp, 10 ) != 0 )
    {
        strncpy( cur_date, timestamp, 10 );
        cur_date[ 10 ] = 0;

        strcpy( cur_path, cur_date );
        cur_path[ 7 ] = 0; // Separate YYYY-MM and DD

        if ( 0 ==  mkdir( cur_path, 0775 ) )
            chown( cur_path, UID_VOICE, GID_VOICE );

        cur_path[ 7 ] = '/'; // Now it will be YYYY-MM/DD

        if ( 0 ==  mkdir( cur_path, 0775 ) )
            chown( cur_path, UID_VOICE, GID_VOICE );

        char log_file[ 256 ];
        sprintf( log_file, "%s/%s %s.log", cur_path, DN, timestamp );
        freopen( log_file, "a", stdout );

        chown( log_file, UID_VOICE, GID_VOICE );
        }
    }

static void BeginRecording( void )
{
    if ( fd_Wave >= 0 ) // already open
        return;

    if ( is_VAD_active || IsOWS )
    {
        // Wait voice to be detected to start recording!
        if ( ! is_voice_detected )
            return;
        }

    getTimestamp ();

    use_wave_header = false;

    if ( payload_type == 0 || payload_type == 1 || payload_type >= 250 ) // G.711 A-Law & u-Law
    {
        use_wave_header = true;
        sprintf( filename, "%s/%s %s.wav", cur_path, DN, timestamp );
        }
    else if ( payload_type >= 2 && payload_type <= 5 ) // G.726 40, 32, 24 or 16
    {
        sprintf( filename, "%s/%s %s.g726-%d", cur_path, DN, timestamp, 8 * ( 5 - ( payload_type - 2 ) ) );
        }
    else
    {
        sprintf( filename, "%s/%s %s PT%02X.bin", cur_path, DN, timestamp, payload_type );
        }
    
    fd_Wave = open( filename, O_TRUNC | O_CREAT | O_RDWR, 0664 );
    sample_count = 0;

    if ( fd_Wave < 0 ) // failed, FIXME: report error
        return;

    printf( "%s: SYS: ---- %-19s %s, PT=%d" EOL, timestamp, "BeginRecording", filename, payload_type );

    if ( use_wave_header )
    {
        wave_header.fmt_audioFormat = payload_type == 1 ? WAVE_FORMAT_MULAW : WAVE_FORMAT_ALAW;
        wave_header.riff_chunkSize = 48 + 0x20000000;
        wave_header.fact_sampleLength = 0x20000000;
        wave_header.data_chunkSize = 0x20000000;
        write( fd_Wave, &wave_header, sizeof( wave_header ) );
        }

    chown( filename, UID_VOICE, GID_VOICE );
    }

static void ClearLEDs( void )
{
    // Turn-off led indicating recording
    if ( startrec_key >= 0 )
        ClearLED( startrec_key );

    // Turn-off led indicating bomb-threat
    if ( bomb_key >= 0 )
        ClearLED( bomb_key );
    }

static void EndRecording( bool because_of_VAD = false )
{
    if ( fd_Wave < 0 )
        return;

    if ( because_of_VAD )
    {
        // Remove empty voice samples recorded under VAD hangover time
        //
        long orig_sample_count = sample_count;

        if ( sample_count + 500 < vad_hangovertime )
            sample_count = 0;
        else
            sample_count -= vad_hangovertime - 500;

        off_t seekp = sample_count;

        switch ( payload_type )
        {
            case   0:
            case   1: seekp = sample_count * 8 / 8; break; // 64 kbps
            case   2: seekp = sample_count * 5 / 8; break; // 40 kbps
            case   3: seekp = sample_count * 4 / 8; break; // 32 kbps
            case   4: seekp = sample_count * 3 / 8; break; // 24 kbps
            case   5: seekp = sample_count * 2 / 8; break; // 16 kbps
            case   6: seekp = sample_count * 1 / 8; break; //  8 kbps
            case 250:
            case 251: seekp = sample_count * 8 / 8; break; // 64 kbps
            }

        lseek( fd_Wave, seekp, SEEK_SET );
        ftruncate( fd_Wave, seekp );

        printf( "%s: SYS: ---- %-19s %s, %lu -> %lu" EOL, timestamp, "Truncate" ,
            filename, orig_sample_count, sample_count ); fflush( stdout );
        }

    // Remove file if voice duration is less then minimum required
    //
    if ( sample_count < vr_minfilesize )
    {
        close( fd_Wave );
        fd_Wave = -1;

        unlink( filename );

        printf( "%s: SYS: ---- %-19s %s, %lu" EOL, timestamp, "Removed (too small)",
            filename, sample_count ); fflush( stdout );

        if ( hasDisplay == GRAPHICAL_DISPLAY_223 || hasDisplay == GRAPHICAL_DISPLAY_225 )
            remote_party[ 0 ] = 0;

        return;
        }

    if ( use_wave_header )
    {
        wave_header.riff_chunkSize = 48 + sample_count;
        wave_header.fact_sampleLength = sample_count;
        wave_header.data_chunkSize = sample_count;
        lseek( fd_Wave, 0, SEEK_SET );
        write( fd_Wave, &wave_header, sizeof( wave_header ) );
        }

    close( fd_Wave );
    fd_Wave = -1;

    char* remp = remote_party; // Skip spaces
    while( *remp && *remp == ' ' )
        ++remp;

    // Write CSV log
    //
    time_t now = time( NULL );
    struct tm* tm = localtime( &now );
    FILE* csv_log = fopen( csv_filename, "a" );
    if ( csv_log )
    {
        fprintf( csv_log, "%04d-%02d-%02d %02d:%02d:%02d,%s,%lu,%.3lf,'%s,'%s" EOL, 
            1900 + tm->tm_year, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec,
            filename, sample_count, double( sample_count ) / 8000.0, DN, remp
            );
        fclose( csv_log );
        chown( csv_filename, UID_VOICE, GID_VOICE );
        }

    if ( because_of_VAD )
    {
        printf( "%s: SYS: ---- %-19s %s, %lu (%.3lfs)" EOL, timestamp, "EndRecording(VAD)", 
            filename, sample_count, double( sample_count ) / 8000.0 ); fflush( stdout );
        }
    else
    {
        printf( "%s: SYS: ---- %-19s %s, %lu (%.3lfs)" EOL, timestamp, "EndRecording", 
            filename, sample_count, double( sample_count ) / 8000.0 ); fflush( stdout );
        }

    // Create post-recording transaction file, if:
    //  -- MP3 conversion is required, or
    //  -- ftp transfer is required
    //
    if ( payload_type == 250 || payload_type == 251 
         || strncmp( NAS_URL1, "ftp://", 6 ) == 0 
         || strncmp( NAS_URL2, "ftp://", 6 ) == 0 
        )
    {
        char tf_name[ 256 ];
        sprintf( tf_name, "/voice/todo/%s.sh", filename + 11 );
       
        FILE* tf = fopen( tf_name, "w" );
        if ( tf )
        {
            fprintf( tf, "#!/bin/sh\n\n" );
            fprintf( tf, "FILE='%s'\n\n", filename );
            fprintf( tf, "cd '/voice/%s'\n\n", DN );

            if ( payload_type == 250 || payload_type == 251 ) // MPEG-2.5 16/8kbps
            {
                // Convert WAV to MP3
                // ala2mp3 <bps> <filename> <DN> <remote_party>
                //
                fprintf( tf, "/albatross/bin/alaw2mp3 %s '%s' '%s' '%s'\n\n",
                    payload_type == 250 ? "16" : "8",
                    filename, DN, remp
                    );

                // Switch filename to MP3 file, if alaw2mp3 was successfull
                //
                char* dotp = strrchr( filename, '.' );
                if ( dotp ) strcpy( dotp, ".mp3" );
                fprintf( tf, "[ \"$?\" -eq 0 ] && FILE=\"%s\"\n\n", filename );
                }

            // FTP file to multiple locations
            //
            if ( strncmp( NAS_URL1, "ftp://", 6 ) == 0 )
            {
                fprintf( tf, "curl -T \"${FILE}\" --ftp-create-dirs --connect-timeout 10 '%s/%s/%s/'\n",
                    NAS_URL1, DN, cur_path );
                }
            if ( strncmp( NAS_URL2, "ftp://", 6 ) == 0 )
            {
                fprintf( tf, "curl -T \"${FILE}\" --ftp-create-dirs --connect-timeout 10 '%s/%s/%s/'\n",
                    NAS_URL2, DN, cur_path );
                }

            // Close file and make it executable
            //
            fclose( tf );
            chmod( tf_name, 0755 );
            }
        }

    if ( hasDisplay == GRAPHICAL_DISPLAY_223 || hasDisplay == GRAPHICAL_DISPLAY_225 )
        remote_party[ 0 ] = 0;
    }

///////////////////////////////////////////////////////////////////////////////

void DTS_Initialize( int type, int version, int extraVerInfo )
{
    int equType = ( type << 8 ) | version;

    hasDisplay = NO_DISPLAY; // Default is not to have a display
    rowCount = 0;
    colCount = 0;
    reqRemap = false;
    IsOWS = false;

    char* desc = "Unknown";

    switch( equType )
    {
        case DTS_OPI_II:
        case DTS_OPI_II_B:
            desc = "OPI-II";
            hasDisplay = OPI_II_DISPLAY;
            break;

        case DTS_DBC601:
            desc = "DBC601";
            hasDisplay = NO_DISPLAY;
            break;

        case DTS_DBC631:
            desc = "DBC631";
            hasDisplay = CHARACTER_DISPLAY; 
            rowCount = 2; colCount = 20; reqRemap = false;
            break;

        case DTS_DBC661:
            desc = "DBC661";
            hasDisplay = CHARACTER_DISPLAY; 
            rowCount = 4; colCount = 20; reqRemap = false;
            break;

        case DTS_DBC662:
            desc = "DBC662";
            hasDisplay = CHARACTER_DISPLAY; 
            rowCount = 4; colCount = 20; reqRemap = false;
            break;

        case DTS_DBC663:
            hasDisplay = CHARACTER_DISPLAY; 
            rowCount = 4; colCount = 20; reqRemap = false;
            break;

        case DTS_DBC199:
            desc = "DBC199";
            hasDisplay = NO_DISPLAY;
            break;

        case DTS_DBC201:
            desc = "DBC201";
            hasDisplay = NO_DISPLAY;
            if ( ( extraVerInfo & 0x0F ) == 1 )
                desc = "DBC210";
            break;

        case DTS_DBC202:
            desc = "DBC202";
            hasDisplay = CHARACTER_DISPLAY; 
            rowCount = 2; colCount = 20; reqRemap = false;
            if ( ( extraVerInfo & 0x0F ) == 2 )
                desc = "DBC212";
            else if ( ( extraVerInfo & 0x0F ) == 3 )
                desc = "DBC212(D4)";
            break;

        case DTS_DBC203:
        case DTS_DBC203_9:
        case DTS_DBC203_E:
        case DTS_DBC203_N9:
        case DTS_DBC203_99:
        case DTS_DBC203_E9:
        case DTS_DBC203_NE:
        case DTS_DBC203_9E:
        case DTS_DBC203_EE:
            desc = "DBC203";
            hasDisplay = CHARACTER_DISPLAY; 
            rowCount = 3; colCount = 40; reqRemap = true;
            if ( ( extraVerInfo & 0x0F ) == 2 )
                desc = "DBC213";
            else if ( ( extraVerInfo & 0x0F ) == 3 )
                desc = "DBC213(D4)";
            break;

        case DTS_DBC214:
        case DTS_DBC214_9:
        case DTS_DBC214_E:
        case DTS_DBC214_N9:
        case DTS_DBC214_99:
        case DTS_DBC214_E9:
        case DTS_DBC214_NE:
        case DTS_DBC214_9E:
        case DTS_DBC214_EE:
            desc = "DBC214";
            hasDisplay = CHARACTER_DISPLAY;
            rowCount = 6; colCount = 40; reqRemap = false;
            if ( ( extraVerInfo & 0x0F ) == 3 )
                desc = "DBC214(D4)";
            break;

        case DTS_DBC220:
            desc = "DBC220";
            hasDisplay = NO_DISPLAY;
            break;

        case DTS_DBC222:
        case DTS_DBC222_1DSS:
            desc = "DBC222";
            hasDisplay = CHARACTER_DISPLAY; 
            rowCount = 2; colCount = 20; reqRemap = false;
            break;

        case DTS_DBC223:
        case DTS_DBC223_1DSS:
        case DTS_DBC223_2DSS:
        case DTS_DBC223_3DSS:
        case DTS_DBC223_4DSS:
            desc = "DBC223";
            hasDisplay = GRAPHICAL_DISPLAY_223;
            break;

        case DTS_DBC225:
        case DTS_DBC225_1DSS:
        case DTS_DBC225_2DSS:
        case DTS_DBC225_3DSS:
        case DTS_DBC225_4DSS:
            desc = "DBC225";
            hasDisplay = GRAPHICAL_DISPLAY_225;
            break;

        case DTS_OWS:
        case DTS_OWS_V2:
            desc = "OWS";
            IsOWS = true;
            break;

        default:
            equType = DTS_UNKNOWN;
            break;
        }

    printf( "%s: DTS: ---- %-19s Type=%02X, Ver=%02X, Xtra=%02X" EOL,
        timestamp, desc, type, version, extraVerInfo
        );
    }

///////////////////////////////////////////////////////////////////////////////

static void DTS_logf( const char* fnc, unsigned char* data, int len )
{
    if ( len == 0 )
    {
        printf( "%s: DTS: (%02X) %s" EOL, timestamp, int( data[0] ), fnc );
        }
    else
    {
        printf( "%s: DTS: (%02X) %-19s", timestamp, int( data[0] ), fnc );

        for ( int i = 1; i < len; i++ )
            printf( " %02X", int( data[ i ] ) );

        printf( EOL ); 
        }
    }

static void ParsePDU_FromDTS( unsigned char* data, int data_len )
{
    *data++; // Skip OPC
    --data_len;
    
    if ( data_len <= 0 )
        return;

    switch( *data )
    {
        case FNC_DASL_LOOP_SYNC:
            DTS_logf( "LoopSync", data, data_len );
            break;

        case FNC_XMIT_FLOW_CONTROL:
            // DTS_logf( "XmitFlowControl", data, data_len );
            break;

        case FNC_RESET_REMOTE_DTSS:
            DTS_logf( "ResetRemoteDTSs", data, data_len );
            break;

        case FNC_KILL_REMOTE_IDODS:
            DTS_logf( "KillRemoteIDODs", data, data_len );
            break;

        case FNC_EQUSTA:
            //
            // Function: The signal sends status and equipment identity to the system
            // software. It is sent when the instrument has received an activation
            // signal if the initialisation test is without errors. It is also sent on a
            // special request or if a loop back order is received.
            // If the instrument is not in active mode, this signal is sent if the
            // instrument receives any signal or if any key or hook is pushed.
            // If transmission order = "Low speaking" is received in the instrument
            // when hook is on, the EQUSTA signal is sent back and the order is
            // ignored.
            //
            // Return signals: EQUACT or EQUSTAREQ or EQULOOP or DISTRMUPDATE
            //
            //  0  8, ! 01 H FNC (Function code)
            //  1  8, ! XX H EQUIPMENT TYPE
            //  2  8, ! XX H EQUIPMENT VERSION
            //  3  8, ! XX H LINE STATE
            //  4  8, ! XX H EQUIPMENT STATE
            //  5  8, ! XX H BIT FAULT COUNTER (not used)
            //  6  8, ! XX H BIT FAULT SECONDS (not used)
            //  7  8, ! XX H TYPE OF ACTIVITY (not used)
            //  8  8, ! XX H LOOP STATE
            //  9  8, ! XX H EXTRA VERSION INFORMATION
            // 10  8, ! XX H OPTION UNIT INFORMATION
            //
            if ( /*equState*/ ( data[ 4 ] & 0x3F ) == 0 ) // Passive
            {
                DTS_logf( "EquSta(Passive)", data, data_len );
                hasDisplay = NO_DISPLAY;
                rowCount = 0;
                colCount = 0;
                manually_started_recording = false;
                manually_stop_recording = false;
                EndRecording ();
                }
            else // Active
            {
                DTS_logf( "EquSta(Active)", data, data_len );

                DTS_Initialize( /*type*/ data[ 1 ], /*ver*/ data[ 2 ], /*extraver*/ data_len >= 10 ? data[ 9 ] : 0 );
                }
            break;

        case FNC_PRGFNCACT:
            //
            // Function: The signal sends information concerning any pushed programmable
            // function key. A signal can contain only one key number.
            //
            // Return signal to: RELPRGFNCREQ2
            //
            //  0  8, ! 02 H FNC (Function code)
            //  1  8, ! XX H Key number
            //
            DTS_logf( "PrgFncAct", data, data_len );

            if ( startrec_key == stoprec_key )
            {
                // Toggle start/stop
                //
                if ( data[ 1 ] == startrec_key )
                {
                    manually_started_recording = ! manually_started_recording;
    
                    if ( manually_started_recording )
                    {
                        printf( "%s: SYS: ---- %s" EOL, timestamp, "StartRecReq" );
                        manually_stop_recording = ( transmission_order == 0 ); // request manual stop if not in a call
                        FlashLED1( startrec_key ); // Indicate (on DTS) activated recording
                        }
                    else
                    {
                        printf( "%s: SYS: ---- %s" EOL, timestamp, "StopRecReq" );
                        EndRecording ();
                        ClearLEDs ();
                        }
                    }
                }
            else if ( data[ 1 ] == startrec_key )
            {
                printf( "%s: SYS: ---- %s" EOL, timestamp, "StartRecReq" );
                manually_started_recording = true;
                manually_stop_recording = ( transmission_order == 0 ); // request manual stop if not in a call
                FlashLED1( startrec_key ); // Indicate (on DTS) activated recording
                }
            else if ( data[ 1 ] == stoprec_key )
            {
                printf( "%s: SYS: ---- %s" EOL, timestamp, "StopRecReq" );
                manually_started_recording = false;
                manually_stop_recording = false;
                EndRecording ();
                ClearLEDs ();
                }

            if ( data[ 1 ] == bomb_key 
                && (
                     ( ( is_VAD_active || IsOWS ) && is_voice_detected ) 
                     || 
                     ( ! ( is_VAD_active || IsOWS ) && transmission_order > 0  )
                   ) 
                )
            {
                if ( startrec_key >= 0 && ! manually_started_recording ) // Start also recording!
                {
                    printf( "%s: SYS: ---- %s" EOL, timestamp, "StartRecReq" );
                    manually_started_recording = true;
                    manually_stop_recording = false;
                    FlashLED1( startrec_key ); // Indicate (on DTS) activated recording

                    if ( fd_Wave < 0 )
                        BeginRecording ();
                    }
                
                if ( mkdir( "../bomb", 0775 ) == 0 ) // Create folder
                    chown( "../bomb", UID_VOICE, GID_VOICE );

                char newpath[ 256 ]; 
                sprintf( newpath, "../bomb/%s", filename + 11 ); // skip cur_path prefix from filepath

                struct stat stat_buf;
                if ( stat( newpath, &stat_buf ) == 0 ) // link to voice file already exists
                {
                    printf( "%s: SYS: ---- %-19s Key=%d" EOL, timestamp, "ThreatContinued", bomb_key );
                    fflush( stdout );
                    }
                else // create link
                {
                    printf( "%s: SYS: ---- %-19s %s, Key=%d" EOL, timestamp, "ThreatActivated", filename, bomb_key );
                    fflush( stdout );

                    link( filename, newpath );

                    FlashLED2( bomb_key ); // Indicate (on DTS) activated bomb-threat
                    }
                }
            break;

        case FNC_FIXFNCACT:
            //
            // Function: The signal sends information concerning any pushed fixed function
            // key or hook state change. A signal can contain only one key code or
            // hook state change.
            //
            // Return signal to: RELFIXFNCREQ2
            //
            // D00 8, ! 05 H NBYTES
            // D01 8, ! 001/X/XXX/X OPC/X/SN/IND
            // D02 8, ! 04 H FNC (Function code)
            // D03 8, ! XX H Key code
            // D04 8, ! YY H CS
            //
            DTS_logf( "FixFncAct", data, data_len );
            break;

        case FNC_PRGFNCREL2:
            //
            // Function: The signal is a response to the request concerning any released
            // programmable function key in the instrument. The signal can only be
            // sent once after each received request signal.
            // If no programmable function key is pushed when the request signal
            // RELPRGFNCREQ2 is received, the signal PRGFNCREL2 is
            // immediately returned.
            // If no programmable function key is pushed when the request signal
            // RELPRGFNCREQ2 is received, the signal PRGFNCREL2 will be
            // sent when the actual key is released.
            //
            // Return signal to: RELPRGFNCREQ2
            //
            // D00 8, ! 04 H NBYTES
            // D01 8, ! 001/X/XXX/X OPC/X/SN/IND
            // D02 8, ! 05 H FNC (Function code)
            // D03 8, ! YY H CS
            //
            DTS_logf( "PrgFncRel2", data, data_len );
            break;

        case FNC_FIXFNCREL2:
            //
            // Function: The signal is a response to the request concerning any released
            // fixed function key in the instrument. The signal can only be sent once
            // after each received request signal.
            // If no fixed function key is pushed when the request signal
            // RELFIXFNCREQ2 is received, the signal PRGFNCREL2 is
            // immediately returned.
            // If any fixed function key is pushed when the request signal
            // RELFIXFNCREQ2 is received, the signal FIXFNCREL2 will be sent
            // when the actual key is released.
            //
            // Return signal to: RELFIXFNCREQ2
            //
            // D00 8, ! 04 H NBYTES
            // D01 8, ! 001/X/XXX/X OPC/X/SN/IND
            // D02 8, ! 06 H FNC (Function code)
            // D03 8, ! YY H
            //
            DTS_logf( "FixFncRel2", data, data_len );
            break;

        case FNC_EQULOCALTST:
            //
            // Function: The signal is sent before entering local test mode if the instrument is
            // active. The signal is also sent when the instrument has returned
            // from local test mode back to normaltraffic mode.
            // If the instrument is not active, no signal is sent before or after visiting
            // local test mode.
            //
            // Return signals to: EQUTSTREQ, Test code 147-151, 153
            //
            // D00 8, ! 05 H NBYTES
            // D01 8, ! 001/X/XXX/X OPC/X/SN/IND
            // D02 8, ! 07 H FNC (Function code)
            // D03 8, ! XX H TRAFFIC STATUS
            // D04 8, ! YY H CS
            // Detailed description:
            // D03 TRAFFIC STATUS
            // : 00 H = Instrument in normal traffic mod
            // (Return from local test mode)
            // : 01 H = Instrument in local test mode
            // (Entering local test mode)
            //
            DTS_logf( "EquLocalTst", data, data_len );
            break;

        case FNC_EXTERNUNIT:
            //
            // Function: The signal sends the signal received in the instrument from the
            // external unit, ex. a EQUSTA signal from a DBY409.
            //
            // Return signal to: None
            //
            // D00 8, ! 07-1B H NBYTES
            // D01 8, ! 001/X/XXX/X OPC/X/SN/IND
            // D02 8, ! 08 H FNC (Function code)
            // D03 8, ! XX H Extern unit address
            // D04 8, ! NNH NBYTES
            // D05 8, ! XX H Data 1
            // : :
            // D(n+4) 8, ! XX H Data n (Max 21)
            // D(n+5) 8, ! YY H CS
            //
            // Detailed description:
            // D03 EXTERN UNIT ADDRESS
            // B0-B3 : 0 - 14 = Extern unit address
            // B4-B6 : 0 = Extern unit interface
            // : 1 = Option unit interface
            // D04 NBYTES
            // Number of data bytes, including itself.
            // D05-D(n+4) DATA 1-N
            // Data received from an extern unit.
            //
            DTS_logf( "ExternUnit", data, data_len );
            break;

        case FNC_STOPWATCHREADY:
            //
            // Function: The signal is sent to the system software when the stopwatch has
            // finished counting down to zero.
            //
            // Return signal to: None
            //
            // D00 8, ! 04 H NBYTES
            // D01 8, ! 001/X/XXX/X OPC/X/SN/IND
            // D02 8, ! 09 H FNC (Function code)
            // D03 8, ! YY H CS
            //
            DTS_logf( "StopWatchReady", data, data_len );
            break;

        case FNC_MMEFNCACT:
            DTS_logf( "MmeFncAct", data, data_len );
            break;

        case FNC_HFPARAMETERRESP:
            //
            // Function: The signal contains the transmission parameters read from the DSP
            // based handsfree circuit.
            // A specific parameter address can contain 1 to 10 data bytes.
            //
            // Return signal to: HANDSFREEPARAMETER
            //
            // D00 8, ! 07-10 H NBYTES
            // D01 8, ! 001/X/XXX/X OPC/X/SN/IND
            // D02 8, ! 0B01 H FNC (Function code)
            // D04 8, ! XX H Parameter address
            // D05 8, ! XX H Parameter data 1
            // : :
            // D(n+4) 8, ! XX H Parameter data n
            // D(n+5) 8, ! YY H CS
            //
            // Detailed description:
            // D05 PARAMETER ADDRESS
            // B0-B6 : 1 = Speech to Echo Comparator levels, 4 data bytes
            // : 2 = Switchable Attenuation settings, 4 data bytes
            // : 3 = Speech Comparator levels, 8 data bytes
            // : 4 = Speech Detector levels, 5 data bytes
            // : 5 = Speech Detector timing, 10 data bytes
            // : 6 = AGC Receive, 7 data bytes
            // : 7 = AGC Transmit, 6 data bytes
            //
            DTS_logf( "HfParameterResp", data, data_len );
            break;

        case FNC_EQUTESTRES:
            //
            // Function: The signal responds to the signal EQUTESTREQ and sends results
            // according to the requested test code.
            // If the initialisation test wasn't ok, the signal EQUTESTRES test code
            // = 127 is returned when the instrument is activated by the signal
            // EQUACT.
            //
            DTS_logf( "EquTestRes", data, data_len );
            break;

        default:
            //
            // Unknown FNC or not properly parsed.
            //
            DTS_logf( "Unknown FNC", data, data_len );
            break;
        }
    }

///////////////////////////////////////////////////////////////////////////////

static void PBX_logf( const char* fnc, unsigned char* data, int len )
{
    if ( len == 0 )
    {
        printf( "%s: PBX: (%02X) %s" EOL, timestamp, int( data[0] ), fnc );
        }
    else
    {
        printf( "%s: PBX: (%02X) %-19s", timestamp, int( data[0] ), fnc );

        for ( int i = 1; i < len + 1; i++ )
            printf( " %02X", int( data[ i ] ) );

        printf( EOL ); 
        }
    }

static void ParsePDU_FromPBX( unsigned char* data, int data_len )
{
    *data++; // Skip OPC
    --data_len;
    
    // Loop parsing FNCs (until there is data)
    
    // Loop will break either if there is no data (data_len <= 0), or
    // the rest of PDU should be ignored (indicated arg_len <= 0)
    //
PARSE_NEXT_FNC:

    if ( data_len <= 0 )
        return;

    int arg_len = -1;
    
    switch( *data )
    {
        case FNC_DASL_LOOP_SYNC:
            arg_len = -1;
            PBX_logf( "LoopSync", data, 1 );
            break;

        case FNC_XMIT_FLOW_CONTROL:
            arg_len = -1;
            // PBX_logf( "XmitFlowControl", data, arg_len );
            break;

        case FNC_RESET_REMOTE_DTSS:
            arg_len = -1;
            PBX_logf( "ResetRemoteDTSs", data, arg_len );
            break;

        case FNC_KILL_REMOTE_IDODS:
            arg_len = -1;
            PBX_logf( "KillRemoteIDODs", data, arg_len );
            break;

        case FNC_VR_VAD:
            arg_len = -1;
            PBX_logf( "VoiceActivity", data, 1 );
            is_voice_detected = data[1]; // remember voice presence state
            if ( is_VAD_active || IsOWS )
            {
                // Stop recording if there is no voice detected!
                if ( ! is_voice_detected )
                    EndRecording( /* bacause of VAD */ true );
                }
            break;

        case FNC_EQUACT:                
            //
            // Function: The signal activates the equipment, i.e. the instrument 
            //    changes from passive to active state. It also sets the LED indicator 
            //    and tone ringer cadences. Normally the signal EQUSTA is returned as 
            //    a response to this signal. However, if some errors has been found 
            //    in the initialisation test, the signal EQUTESTRES, Test code 127, is
            //    returned.
            //
            // Return signals: EQUSTA or EQUTESTRES, Test code 127
            //
            // D(n)    8, ! 80 H FNC (Function code)
            // D(n+1)  8, ! XX H Internal ring signal, 1:st on interval
            // D(n+2)  8, ! XX H Internal ring signal, 1:st off interval
            // D(n+3)  8, ! XX H Internal ring signal, 2:nd on interval
            // D(n+4)  8, ! XX H Internal ring signal, 2:nd off interval
            // D(n+5)  8, ! XX H External ring signal, 1:st on interval
            // D(n+6)  8, ! XX H External ring signal, 1:st off interval
            // D(n+7)  8, ! XX H External ring signal, 2:nd on interval
            // D(n+8)  8, ! XX H External ring signal, 2:nd off interval
            // D(n+9)  8, ! XX H Call back ring signal, 1:st on interval
            // D(n+10) 8, ! XX H Call back ring signal, 1:st off interval
            // D(n+11) 8, ! XX H Call back ring signal, 2:nd on interval
            // D(n+12) 8, ! XX H Call back ring signal, 2:nd off interval
            // D(n+13) 8, ! XX H LED indicator cadence 0, 1:st on interval
            // D(n+14) 8, ! XX H LED indicator cadence 0, 1:st off interval
            // D(n+15) 8, ! XX H LED indicator cadence 0, 2:nd on interval
            // D(n+16) 8, ! XX H LED indicator cadence 0, 2:nd off interval
            // D(n+16) 8, ! XX H LED indicator cadence 0, 2:nd off interval
            // D(n+17) 8, ! XX H LED indicator cadence 1, On interval
            // D(n+18) 8, ! XX H LED indicator cadence 1, Off interval
            // D(n+19) 8, ! XX H LED indicator cadence 2, On interval
            // D(n+20) 8, ! XX H LED indicator cadence 2, Off interval
            //
            // Detailed description:
            // D(n+1)-D(n+12) = 00 - FF H Number of 50 ms intervals
            // D(n+13)-D(n+20) = 00 - FF H Number of 10 ms intervals
            //
            // Note 1: Not all intervals for any cadence can be zero.
            //
            // Note 2: To a DBY409 the EQUACT signal should only contain the following bytes:
            //
            // D(n)    8, ! 80 H FNC
            // D(n+1)  8, ! XX H LED indicator cadence 0, 1:st on interval
            // D(n+2)  8, ! XX H LED indicator cadence 0, 1:st off interval
            // D(n+3)  8, ! XX H LED indicator cadence 0, 2:nd on interval
            // D(n+4)  8, ! XX H LED indicator cadence 0, 2:nd off interval
            // D(n+5)  8, ! XX H LED indicator cadence 1, On interval
            // D(n+6)  8, ! XX H LED indicator cadence 1, Off interval
            // D(n+7)  8, ! XX H LED indicator cadence 2, On interval
            // D(n+8)  8, ! XX H LED indicator cadence 2, Off interval
            //
            arg_len = 20; // = 8; // if ( DTS == DBY409 )
            PBX_logf( "EquAct", data, arg_len );
            break;

        case FNC_EQUSTAREQ:
            //
            // Function: The signal sends request for DTS status.
            //
            // Return signals: EQUSTA
            //
            // D(n)   8, ! 81 H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "EquStaReq", data, arg_len );
            break;

        case FNC_CLOCKCORRECT:
            //
            // Function: The signal adjusts the local real time clock in the instrument.
            //
            // Return signals: None
            //
            // D(n)   8, ! 82 H FNC (Function code)
            // D(n+1) 8, ! XX H Type of time presentation
            // D(n+2) 8, ! XX H Clock display field
            // D(n+3) 8, ! XX H Start position within field
            // D(n+4) 8, ! XX H Hours in BCD-code
            // D(n+5) 8, ! XX H Minutes in BCD-code
            // D(n+6) 8, ! XX H Seconds in BCD-code
            //
            if ( IsOWS )
            {
                arg_len = -1;
                PBX_logf( "OWS", data, data_len );
                }
            else
            {
                arg_len = 6;
                int REPR = data[ 1 ];
                int R = ( data[ 2 ] >> 4 ) & 0x07;
                int C = data[ 2 ] & 0x1F;
                int P = data[ 3 ];
                int H = ( data[ 4 ] >> 4 ) * 10 + ( data[ 4 ] & 0xF );
                int M = ( data[ 5 ] >> 4 ) * 10 + ( data[ 5 ] & 0xF );
                int S = ( data[ 6 ] >> 4 ) * 10 + ( data[ 6 ] & 0xF );

                printf( "%s: PBX: (%02X) %-19s (%d,%d) P=%d, Repr=%02X: %02d:%02d:%02d" EOL,
                    timestamp, int( data[0] ), "ClockCorrect", R, C, P, REPR, H, M, S
                    );
                }
            break;

        case FNC_TRANSMISSION:
            //
            // Signal: TRANSMISSION
            // Function: The signal updates the transmission.
            //
            // Return signal: None
            //
            // D(n)   8, ! 84 H FNC (Function code)
            // D(n+1) 8, ! XX H Transmission order
            //
            // Detailed description:
            // D(n+1): TRANSMISSION ORDER
            // : 00 = No speaking
            // : 01 = Handset speaking
            // : 02 = Handsfree speaking
            // : 03 = No transmission change
            // : 04 = Public address
            // : 05 = Loud speaking
            // : 06 = Headset speaking
            //
            if ( IsOWS )
            {
                arg_len = -1;
                PBX_logf( "OWSTransmission", data, data_len );
                }
            else
            {
                arg_len = 1;
                int ORDER = data[ 1 ];
                static const char* desc[] =
                {
                    "No Speaking", "Handset speaking", "Handsfree speaking",
                    "No transmission change", "Public address", "Loud speaking", 
                    "Headset speaking" 
                    };

                printf( "%s: PBX: (%02X) %-19s Order=%02X: %s" EOL,
                    timestamp, int( data[0] ), "Transmission", 
                    ORDER, ORDER >= 0 && ORDER <= 6 ? desc[ ORDER ] : "?"
                    );

                if ( ORDER != 0x03 )
                    transmission_order = ORDER;

                if ( transmission_order == 0 )
                {
                    EndRecording ();

                    if ( ! manually_stop_recording )
                    { 
                        manually_started_recording = false;
                        printf( "%s: SYS: ---- %s" EOL, timestamp, "StopRecReq" );
                        ClearLEDs ();
                        }
                    }
                }

            break;

        case FNC_CLEARDISPLAY:
            //
            // Function: The signal clears the display completely.
            //
            // Return signals: None
            //
            // D(n)  8, ! 85 H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "ClearDisplay", data, arg_len );

            if ( hasDisplay == CHARACTER_DISPLAY )
            {
                memset( remote_party, ' ', 20 );
                remote_party[ 20 ] = 0;
                }
            break;

        case FNC_CLEARDISPLAYFIELD:
            //
            // Function: The signal clears a field in the display.
            //
            // Return signals: None
            //
            // D(n)   8, ! 86 H FNC (Function code)
            // D(n+1) 8, ! XX H Field address
            //
            // Detailed description:
            // D(n+1) FIELD ADDRESS
            // B0-B4: Row number of display field
            // B5-B7: Column number of display field
            //
            arg_len = 1;
            {
                int R = ( data[ 1 ] >> 4 ) & 0x07;
                int C = data[ 1 ] & 0x1F;

                printf( "%s: PBX: (%02X) %-19s (%d,%d)" EOL,
                    timestamp, int( data[0] ), "ClearDisplayField",
                    R, C
                    );

                if ( hasDisplay == CHARACTER_DISPLAY )
                {
                    if ( rowCount == 2 )
                    {
                        if ( R == 0 && C == 1 )
                            memset( remote_party, ' ', 10 );
                        else if ( R == 2 && C == 1 )
                            memset( remote_party + 10, ' ', 10 );
                        }
                    else if ( rowCount != 0 )
                    {
                        if ( R == 0 && C == 2 )
                            memset( remote_party, ' ', 10 );
                        else if ( R == 2 && C == 2 )
                            memset( remote_party + 10, ' ', 10 );
                        }
                    }
                }
            break;

        case FNC_INTERNRINGING:
            //
            // Function: The signal activates the internal ringing.
            //
            // Return signals: None
            //
            // D(n) 8, ! 88 H FNC (Function code)
            //
            if ( IsOWS )
            {
                arg_len = -1;
                PBX_logf( "OWS", data, data_len );
                }
            else
            {
                arg_len = 0;
                PBX_logf( "InternRinging", data, arg_len );
                }
            break;

        case FNC_INTERNRINGING1LOW:
            //
            // Function: The signal activates the internal ringing 1:st interval low.
            //
            // Return signals: None
            //
            // D(n) 8, ! 89 H FNC (Function code)
            //
            if ( IsOWS )
            {
                arg_len = -1;
                PBX_logf( "OWS", data, data_len );
                }
            else
            {
                arg_len = 0;
                PBX_logf( "InternRinging1Low", data, arg_len );
                }
            break;

        case FNC_EXTERNRINGING:
            //
            // Function: The signal activates the external ringing.
            //
            // Return signals: None
            //
            // D(n) 8, ! 8A H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "ExternRinging", data, arg_len );
            break;

        case FNC_EXTERNRINGING1LOW:
            //
            // Function: The signal activates the external ringing 1:st interval low
            //
            // Return signals: None
            // 
            // D(n) 8, ! 8B H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "ExternRinging1Low", data, arg_len );
            break;

        case FNC_CALLBACKRINGING:
            //
            // Function: The signal activates the call back ringing.
            //
            // Return signals: None
            //
            // D(n) 8, ! 8C H FNC (Function code)
            //
            if ( IsOWS )
            {
                arg_len = -1;
                PBX_logf( "OWS", data, data_len );
                }
            else if ( data_len >= 3 && data[ 1 ] == 0x0C && data[ 2 ] == 0x02 )
            {
                arg_len = -1;
                PBX_logf( "OWS", data, data_len );

                IsOWS = true;
                }
            else
            {
                arg_len = 0;
                PBX_logf( "CallbackRinging", data, arg_len );
                }
            break;

        case FNC_CALLBACKRINGING1LOW:
            //
            // Function: The signal activates the call back ringing 1:st interval low.
            //
            // Return signals: None
            //
            // D(n) 8, ! 8D H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "CallbackRinging1Low", data, arg_len );
            break;

        case FNC_STOPRINGING:
            //
            // Function: The signal stops the ringing.
            //
            // Return signals: None
            //
            // D(n) 8, ! 8E H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "StopRinging", data, arg_len );
            break;

        case FNC_CLEARLEDS:
            //
            // Function: The signal clears all LED indicators.
            //
            // Return signals: None
            //
            // D(n) 8, ! 8F H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "ClearLEDs", data, arg_len );
            break;

        case FNC_CLEARLED:
            //
            // Function: The signal clears one LED indicator.
            //
            // Return signals: None
            //
            // D(n)   8, ! 90 H FNC (Function code)
            // D(n+1) 8, ! XX H Indicator number
            //
            arg_len = 1;
            PBX_logf( "ClearLED", data, arg_len );
            break;

        case FNC_SETLED:
            //
            // Function: The signal sets one LED indicator to steady light.
            //
            // Return signals: None
            //
            // D(n)   8, ! 91 H FNC (Function code)
            // D(n+1) 8, ! XX H Indicator number
            //
            arg_len = 1;
            PBX_logf( "SetLED", data, arg_len );
            break;

        case FNC_FLASHLEDCAD0:
            //
            // Function: The signal sets one LED indicator to flash with cadence 0.
            //
            // Return signals: None
            //
            // D(n)   8, ! 92 H FNC (Function code)
            // D(n+1) 8, ! XX H Indicator number
            //
            arg_len = 1;
            PBX_logf( "FlashLEDCad0", data, arg_len );
            break;

        case FNC_FLASHLEDCAD1:
            //
            // Function: The signal sets one LED indicator to flash with cadence 1.
            //
            // Return signals: None
            //
            // D(n)   8, ! 93 H FNC (Function code)
            // D(n+1) 8, ! XX H Indicator number
            //
            arg_len = 1;
            PBX_logf( "FlashLEDCad1", data, arg_len );
            break;

        case FNC_FLASHLEDCAD2:
            //
            // Function: The signal sets one LED indicator to flash with cadence 2.
            //
            // Return signals: None
            //
            // D(n)   8, ! 94 H FNC (Function code)
            // D(n+1) 8, ! XX H Indicator number
            //
            arg_len = 1;
            PBX_logf( "FlashLEDCad2", data, arg_len );
            break;

        case FNC_WRITEDISPLAYFIELD:
            //
            // Function: The signal writes 10 characters to a display field.
            //
            // Return signals: None
            //
            // D(n)   8, ! 95 H FNC (Function code)
            // D(n+1) 8, ! XX H Display field address
            // D(n+2)    ! XX H
            //
            // D(n+11)   ! XX H 10 Characters to the display
            //
            // Detailed description:
            // D(n+1) DISPLAY FIELD ADDRESS
            // B0-B4: Row number of display field
            // B5-B7: Column number of display field
            //
            arg_len = 11;
            {
                int R = ( data[ 1 ] >> 4 ) & 0x07;
                int C = data[ 1 ] & 0x1F;
                int LEN = 10;

                printf( "%s: PBX: (%02X) %-19s (%d,%d): \"%.*s\"" EOL, 
                    timestamp, int( data[0] ), "WriteDisplayField", 
                    R, C, arg_len - 1, data + 2 
                    );

                if ( fd_Wave >= 0 ) // While in transmission
                {
                    if ( rowCount == 2 )
                    {
                        if ( R == 0 && C == 1 )
                            memcpy( remote_party, data + 2, LEN );
                        else if ( R == 2 && C == 1 )
                            memcpy( remote_party + 10, data + 2, LEN );
                        }
                    else if ( rowCount != 0 )
                    {
                        if ( R == 0 && C == 2 )
                            memcpy( remote_party, data + 2, LEN );
                        else if ( R == 2 && C == 2 )
                            memcpy( remote_party + 10, data + 2, LEN );
                        }
                    }
                }
            break;

        case FNC_RNGCHRUPDATE:
            //
            // Function: The signal updates the chosen tone ringer character in the
            // instrument.
            //
            // Return signals: None
            //
            // D(n) 8, ! 96 H FNC (Function code)
            // D(n+1) 8, ! XX H Tone ringer character order
            //
            // Detailed description:
            // D(n+1) TONE RINGER CHARACTER ORDER
            // B0-B7: One value form 00 to 09 in Hex code, meaning
            // that ten different ringing tone characters can be
            // chosen in the instrument.
            //
            // Note 1: Default value in the instrument in 05.
            //
            arg_len = 1;
            PBX_logf( "RngChrUpdate", data, arg_len );
            break;

        case FNC_STOPWATCH:
            //
            // Function: The signal controls a local stopwatch function in the 
            // instrument according to the description below.
            //
            // Return signals: None
            //
            // D(n) 8, ! 97 H FNC (Function code)
            // D(n+1) 8, ! XX H Stopwatch display field
            // D(n+2) 8, ! XX H Start position within field
            // D(n+3) 8, ! XX H Stopwatch order
            // D(n+4) 8, ! XX H Minutes in BCD-code
            // D(n+5) 8, ! XX H Seconds in BCD-code, see note 2
            //
            // Detailed description:
            // D(n+1) STOPWATCH DISPLAY FIELD
            //    B0-B4: Row number of display field
            //    B5-B7: Column number of display field
            // D(n+2) START POSITION WITHIN FIELD
            //    B0-B3: Start position of stopwatch in display field
            //    B4-B7: 0
            // D(n+3) STOPWATCH ORDER
            //   B0-B1 = 0 : Reset to zero
            //         = 1 : Stop counting
            //         = 2 : Start counting
            //         = 3 : Don't affect the counting
            //      B2 = 0 : Remove stopwatch from display
            //         = 1 : Present stopwatch on display
            //      B3 = 0 : Increment counting
            //         = 1 : Decrement counting
            //      B4 = 0 : Don't update stopwatch time
            //         = 1 : Update stopwatch with new time
            //
            // Note 1: The bit 3, 4 in Stopwatch order and minutes and seconds don't 
            // exist in DBC600 telephones.
            //
            // Note 2: If stopwatch order bit 4 = 0, when the minutes and seconds 
            // must not be sent.
            //
            arg_len = 5;
            PBX_logf( "Stopwatch", data, arg_len );
            break;

        case FNC_DISSPECCHR:
            //
            // Function: The signal defines a special character to the display. One special
            // character per signal can be defined formatted as a 8x5 dot matrix.
            // Maximum 8 different characters can be defined for one instrument.
            //
            // Return signals: None
            //
            // D(n)   8, ! 98 H FNC (Function code)
            // D(n+1) 8, ! ZZ H Character code
            // D(n+2) 8, ! XX H Data in dot row 1
            // D(n+3) 8, ! XX H Data in dot row 2
            // D(n+4) 8, ! XX H Data in dot row 3
            // D(n+5) 8, ! XX H Data in dot row 4
            // D(n+6) 8, ! XX H Data in dot row 5
            // D(n+7) 8, ! XX H Data in dot row 6
            // D(n+8) 8, ! XX H Data in dot row 7
            // D(n+9) 8, ! XX H Data in dot row 8
            //
            // Detailed description:
            // D(n+1) CHARACTER CODE
            //    B0-B7: 00 - 07 H, which means that 8 different characters can be defined.
            // D(n+2)-D(n+9) DATA IN DOT ROW 1 TO 8
            //    B0-B4: Dot definition in one row. "1" means active dot.
            //    B5-B7: 0
            //
            arg_len = 9;
            PBX_logf( "DispSpecChr", data, arg_len );
            break;

        case FNC_FLASHDISPLAYCHR:
            // 
            // Function: The signal activates blinking character string in thedisplay, max 10
            // characters.
            //
            // Return signals: None
            //
            // D(n)   8, ! 99 H FNC (Function code)
            // D(n+1) 8, ! XX H Display field address
            // D(n+2)    ! NY H Start position number of characters
            // D(n+3)    ! XX1 H Character to blink
            //
            // D(n+X)    ! XXN H
            //
            // Detailed description:
            // D(n+1) DISPLAY FIELD ADDRESS
            //    B0-B4: Row number of display field
            //    B5-B7: Column number of display field
            // D(n+2) START POSITION, NUMBER OF CHARACTERS
            //    B0-B3: Start position in field
            //    B4-B7: Number of characters, (1-10)
            // D(n+3)..D(n+X) CHARACTERS TO BLINK
            //    The number must be the same as in "number of characters" above,
            //    max 10 characters.
            //
            arg_len = 2 + ( data[ 2 ] >> 4 ) & 0x0F;
            {
                int R = ( data[ 1 ] >> 4 ) & 0x07;
                int C = data[ 1 ] & 0x1F;
                int P = data[ 2 ] & 0x0F;
                int LEN = ( data[ 2 ] >> 4 ) & 0x0F;

                printf( "%s: PBX: (%02X) %-19s (%d,%d) P=%d: \"%.*s\"" EOL, timestamp, int( data[0] ), 
                    "FlashDisplayChr", R, C, P, LEN, data + 3 
                    );
                }
            break;

        case FNC_ACTCLOCK:
            // 
            // Function: The signal activates the real time clock in the display.
            //
            // Return signals: None
            // 
            // D(n)   8, ! 9A H FNC (Function code)
            // D(n+1) 8, ! XX H Clock order
            //
            // Detailed description:
            // D(n+1) CLOCK ORDER
            // B0-B6 : 1 = Clock in display0
            //    B7 : 1 = Clock in display
            //       ; 0 = No clock in display
            //
            arg_len = 1;
            {
                printf( "%s: PBX: (%02X) %-19s Order=%02X: %s" EOL,
                    timestamp, int( data[0] ), "ActClock", 
                    int( data[ 1 ] ),
                    data[ 1 ] & 0x80 ? "Show clock" : "Hide clock"
                    );
                }
            break;

        case FNC_RELPRGFNCREQ2:
            // 
            // Function: The signal requests information concerning released programmable
            // function key from the instrument. If no programmable function key is
            // pushed when this signal is received, the signal PRGFNCREL2 is
            // returned back at once. If a programmable function key is pushed
            // when this signal is received, the signal PRGFNCREL2 is returned
            // back when the actual key become released. Just one return signal
            // PRGFNCREL2 is associated with any received signal
            // RELPRGFNCREQ2.
            //
            // Return signals: PRGFNCREL2
            //
            // D(n) 8, ! 9B H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "RelPrgFncReq2", data, arg_len );
            break;

        case FNC_RELFIXFNCREQ2:
            // 
            // Function: The signal requests information concerning released fix function key
            // from the instrument. If no fix function key is pushed when this signal
            // is received, the signal FIXFNCREL2 is returned back at once. If a fix
            // function key is pushed when this signal is received, the signal
            // FIXFNCREL2 is returned back when the actual key become
            // released.
            // Just one return signal FIXFNCREL2 is associated with any received
            // signal RELFIXFNCREQ2.
            //
            // Return signals: FIXFNCREL2
            //
            // D(n) 8, ! 9C H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "RelFixFncReq2", data, arg_len );
            break;

        case FNC_ACTCURSOR:
            // 
            // Function: The signal activates the blinking cursor in the display.
            //
            // Return signals: None
            //
            // D(n)   8, ! 9D H FNC (Function code)
            // D(n+1) 8, ! XX H Cursor display address
            // D(n+2) 8, ! XX H Cursor position within field
            //
            // Detailed description:
            // D(n+1) CURSOR DISPLAY ADDRESS
            //    B0-B4: Row number of display field
            //    B5-B7: Column number of display field
            // D(n+1) CURSOR POSITION WITHIN FIELD
            //    B0-B3: Cursor position within field, (0-9)
            //    B4-B7: 0
            //
            arg_len = 2;
            {
                int R = ( data[ 1 ] >> 4 ) & 0x07;
                int C = data[ 1 ] & 0x1F;
                int P = data[ 2 ] & 0x0F;

                printf( "%s: PBX: (%02X) %-19s (%d,%d) P=%d" EOL, timestamp, int( data[0] ), 
                    "ActCursor", R, C, P
                    );
                }
            break;

        case FNC_CLEARCURSOR:
            //
            // Function: The signal clears the blinking cursor from the display.
            //
            // Return signals: None
            //
            // D(n) 8, ! 9E H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "ClearCursor", data, arg_len );
            break;

        case FNC_FIXFLASHCHR:
            //
            // Function: The signal changes the blinking character string to permanent
            // presentation.
            //
            // Return signals: None
            // 
            // D(n) 8, ! 9F H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "FixFlashChr", data, arg_len );
            break;

        case FNC_RNGLVLUPDATE:
            //
            // Function: The signal updates the tone ringer level and controls if ringing shall
            // be with increasing level. It also decides if a test for four periods shall
            // be made.
            //
            // Return signals: None
            //
            // D(n) 8, ! A7 H FNC (Function code)
            // D(n+1) 8, ! XX H Tone ringer level order
            //
            // Detailed description:
            // D(n+1) TONE RINGER LEVEL ORDER
            //  B0-B7: Tone ringer level, 0dB-(-27dB) step -3dB
            //       : 0 = 0dB (Max)
            //       : 1 = -3dB
            //       : 9 = -27dB (Min)
            //     B4: 0 = Normal ringing
            //       : 1 = Increasing tone ringer level
            //     B5: 0 = No activation of tone ringer
            //       : 1 = Activation of tone ringer during 4 periods.
            //
            // Note 1: In the DBC199 telephone the tone ringer level (B0-B3) can only have two values,
            // 0 = Maximum level and 9 = Attenuated level.
            //
            // Note 2: In the DBC199 telephone the increasing tone ringer level (B4 = 1) means that the two
            // first periods are attenuated and the remaining are with maximum level.
            //
            arg_len = 1;
            {
                int RNG_LEVEL = - 3 * ( data[ 1 ] & 0x0F ); // in dB
                int INC_RING = ( data[ 1 ] & 0x10 ) >> 4;
                int ACT_4PER = ( data[ 1 ] & 0x20 ) >> 5;

                printf( "%s: PBX: (%02X) %-19s LEVEL=%ddB, %s, %s" EOL, timestamp, int( data[0] ), 
                    "RngLvlUpdate", RNG_LEVEL, 
                    INC_RING ? "Increasing" : "Normal", 
                    ACT_4PER ? "No act." : "4 periods act."
                    );
                }
            break;

        case FNC_TRANSLVLUPDATE:
            //
            // Function: The signal updates the transmission levels in the DTS. The signal
            // also tells the instrument what the PBX has D3 comparability. Before
            // this signal has been received the D3 instrument acts like a DBC600.
            //
            // Return signal: None
            //
            // D(n) 8, ! A8 H FNC (Function code)
            // D(n+1) 8, ! XX H Loudspeaker level
            // D(n+2) 8, ! XX H Earpiece level
            //
            // Detailed description:
            // D(n+1) LOUDSPEAKER LEVEL (RLR)
            // B0-B7 : Loudspeaker level, -6dB - +14dB step 2dB
            // : 00 = -6dB (23dBr, Max)
            // : 01 = -4dB (21dBr)
            // : 0A = 14dB (3dBr, Min)
            // D(n+2) EARPIECE LEVEL (RLR)
            // B0-B7 : Earpiece level, -5dB - +10dB step 1dB
            // : 00 = -5dB (-13dBr, Max)
            // : 01 = -4dB (-14dBr)
            // : 0F = 10dB (-28dBr, Min)
            //
            arg_len = 2;
            {
                int SPK_LEVEL = -6 + data[ 1 ] * 2; // in dB
                int EAR_LEVEL = -5 + data[ 1 ]; // in dB

                printf( "%s: PBX: (%02X) %-19s SPK=%ddB, EAR=%ddB" EOL, timestamp, int( data[0] ), 
                    "TransLvlUpdate", SPK_LEVEL, EAR_LEVEL
                    );
                }
            break;

        case FNC_IO_SETUP:
            //
            // Function: The signal updates the I/O optional board.
            //
            // Return signals: None
            //
            // D(n) 8, ! A9 H FNC (Function code)
            // D(n+1) 8, ! XX H I/O setup order 1
            // D(n+2) 8, ! XX H I/O setup order 2
            //
            // Detailed description:
            // D(n+1) I/O SETUP ORDER 1
            //   : 0 =Extern function, Updated by PBX
            //   : 1 =Extern function, Internally updated
            //   : 2 =Extra handset mode update
            //   : 3 =Impaired handset level
            //   : 4 =PC sound modes
            //   : 5 =Option unit mute
            // D(n+2) I/O SETUP ORDER 2
            // D(n+1): 0 = EXTERN FUNCTION, UPDATED BY PBX:
            // D(n+2) : 0 =Extern function, I/O off
            //   : 1 =Extern function, I/O on
            // D(n+1): 1 = EXTERN FUNCTION, INTERNALLY UPDATED:
            // D(n+2) : 0 =Extern function, Tone ringer
            //   : 1 =Extern function, Extern busy indicator
            // D(n+1): 2 = EXTRA HANDSET MODE UPDATE:
            //   B0 : 0 =Handset microphone off
            //      : 1 =Handset microphone on
            //   B1 : 0 =Extra handset microphone off
            //      : 1 =Extra handset microphone on
            // D(n+1): 3 = IMPAIRED HANDSET LEVEL:
            // D(n+2) : 0 =Impaired level off
            //      : 1 =Impaired level on
            //
            arg_len = 2;
            PBX_logf( "IOSetup", data, arg_len );
            break;

        case FNC_FLASHDISPLAYCHR2:
            //
            // Function: The signal activates blinking character string in the display, max 20
            // characters.
            //
            // Return signals: None
            //
            // D(n) 8, ! AA H FNC (Function code)
            // D(n+1) 8, ! XX H Display field address
            // D(n+2) 8, ! XX H Start position
            // D(n+3) 8, ! NN H Number of characters
            // D(n+4) 8, ! XX H Character to blink
            // ...
            // D(n+X) 8, ! XXN H
            //
            // Detailed description:
            // D(n+1) DISPLAY FIELD ADDRESS
            //   B0-B4: Row number of display field
            //   B5-B7: Column number of display field
            // D(n+2) START POSITION
            //   B0-B3: Start position in field, (0-9)
            //   B4-B7: 0
            // D(n+3) NUMBER OF CHARACTERS
            //   B0-B7: Number of characters, (1-20)
            // D(n+4) CHARACTERS STRING
            //   The number must be the same as in "number of characters" above,
            //   max 20 characters.
            //
            arg_len = 3 + data[ 3 ];
            {
                int R = ( data[ 1 ] >> 4 ) & 0x07;
                int C = data[ 1 ] & 0x1F;
                int P = data[ 2 ] & 0x0F;
                int LEN = data[ 3 ];

                printf( "%s: PBX: (%02X) %-19s (%d,%d) P=%d: \"%.*s\"" EOL, timestamp, int( data[0] ), 
                    "FlashDisplayChr2", R, C, P, LEN, data + 4 
                    );
                }
            break;

        case FNC_TRANSCODEUPDATE:
            //
            // Function: The signal updates transmissions coding principle and markets
            // adaptations for sidetone and transmit level in the DTS.
            //
            // Return signals: None
            //
            // D(n) 8, ! AB H FNC (Function code)
            // D(n+1) 8, ! XX H PCM code order
            // D(n+2) 8, ! XX H Sidetone level
            // D(n+3) 8, ! XX H Transmit level
            //
            // Detailed description:
            // D(n+1) PCM CODE ORDER
            //   : 0 = my-law
            //   : 1 = A-law
            // D(n+2) SIDETONE LEVEL (STMR = SLR - S - SK)
            //   0-B7 : Sidetone level, S=-8dB-(-23dB) step -1dB
            //   S : 00 = -8dB (Max.)
            //   S : 01 = -9dB
            //   S : 0F = -23dB (Min.)
            // D(n+3) TRANSMIT LEVEL (SLR = TK - T)
            //   B0-B7 : Transmit level, T=0dB-15dB step 1dB
            //   T : 00 = 0dB (Min.)
            //   T : 01 = 1dB
            //   T : 0F = 15dB (Max.)
            //
            // Note: The constants SK and TK equipment type dependent:
            // DBC200 : SK = 4.5, TK = 15.
            //
            arg_len = 3;
            {
                int PCM_CODE = data[ 1 ];
                int ST_LEVEL = -8 - ( data[ 2 ] & 0xF ); // in dB
                int TX_LEVEL = ( data[ 3 ] & 0xF ); // in dB

                printf( "%s: PBX: (%02X) %-19s PCM=%02X: %s, SideTone=%ddB, Transmit=%ddB" EOL, timestamp, int( data[0] ), 
                    "TransCodeUpdate", PCM_CODE, PCM_CODE == 0 ? "u-Law" : PCM_CODE == 1 ? "A-Law" : "?",
                       ST_LEVEL, TX_LEVEL
                    );
                }
            break;

        case FNC_EXTERNUNIT1UPDATE:
            // 
            // Function: The signal sends the data in the signal unchanged to external unit or
            // option un
            //
            // Return signals: None
            //
            // D(n) 8, ! AC-AD H FNC (Function code)
            // D(n+1) 8, ! XX H Extern unit address
            // D(n+2) 8, ,! 02-17 H NBYTES
            // D(m) 8, ! XX H Data 1
            // ...
            // D(m+N) Data n (max 22)
            //
            // Detailed description:
            // D(n) FUNCTION CODE
            //   : AC H = Extern unit 1 (if old format)
            //   : AD H = Extern unit 2 (if old format)
            // D(n+1) EXTERN UNIT ADDRESS
            //   0- B0-B3 : 0 - 14 = Extern unit address
            //   : 15 = Extern unit broadcast
            //   B4-B6 : 0 = Extern unit interface
            //   : 1 = Option unit interface
            //   B7 : 0 = Old format (D(n+1) = NBYTES)
            //   : 1 = Enhanced format
            // D(n+2) NBYTES
            //   Number of data bytes, including itself.
            // D(m)-D(m+N) DATA 1-N
            //   Data to be sent to extern unit 1.
            //
            // Note: The OPC/X/SN/IND byte shall not be included in the DATA 1-N field in signals to an
            // extern unit.
            //
            arg_len = 2 + data[ 2 ] - 1;
            PBX_logf( "ExternUnit1Update", data, arg_len );
            break;

        case FNC_EXTERNUNIT2UPDATE:
            // The same as previous; but UNIT2
            arg_len = 2 + data[ 2 ] - 1;
            PBX_logf( "ExternUnit2Update", data, arg_len );
            break;

        case FNC_WRITEDISPLAYCHR:
            //
            // Function: The signal writes character string in the display, max 40 characters.
            //
            // Return signals: None
            //
            // D(n) 8, ! AE H FNC (Function code)
            // D(n+1) 8, ! XX H Display field address
            // D(n+2) 8, ! XX H Start position
            // D(n+3) 8, ! XX H Number of characters
            // D(n+4) 8, ! XX1 H Character string
            // ...
            // D(n+X) ! XXN H
            //
            // Detailed description:
            // D(n+1) DISPLAY FIELD ADDRESS
            //   B0-B4: Row number of display field
            //   B5-B7: Column number of display field
            // D(n+2)S TART POSITION
            //   B0-B3: Start position in field, (0-9)
            //   B4-B7: 0
            // D(n+3) NUMBER OF CHARACTERS
            //   B0-B7: Number of characters, (1-40)
            // D(n+4) CHARACTERS STRING
            //   The number must be the same as in "number of characters" above,
            //   max 40 characters.
            arg_len = 3 + data[ 3 ];
            {
                int R = ( data[ 1 ] >> 4 ) & 0x07;
                int C = data[ 1 ] & 0x1F;
                int P = data[ 2 ];
                int LEN = data[ 3 ];

                printf( "%s: PBX: (%02X) %-19s (%d,%d) P=%d: \"%.*s\"" EOL, timestamp, int( data[0] ), 
                    "WriteDisplayChr", R, C, P, LEN, data + 4 
                    );
                }
            break;

        case FNC_ACTDISPLAYLEVEL:
            //
            // Function: The signal activates the level bar in the display.
            //
            // Return signals: None
            //
            // D(n) 8, ! AF H FNC (Function code)
            // D(n+1) 8, ! XX H Display level bar address
            // D(n+2) 8, ! XX H Display level bar start position within field
            // D(n+3) 8, ! XX H Length of level bar (1-40 char)
            // D(n+4) 8, ! XX H Level of bar (0-40 char)
            //
            // Detailed description:
            // D(n+1) DISPLAY LEVEL BAR ADDRESS
            //   B0-B4: Row number of display field
            //   B5-B7: Column number of display field
            // D(n+2) DISPLAY LEVEL BAR START POSITION WITHIN FIELD
            //   B0-B3: Display level bar position within field, (0-9)
            //   B4-B7: 0
            //
            arg_len = 4;
            {
                int R = ( data[ 1 ] >> 4 ) & 0x07;
                int C = data[ 1 ] & 0x1F;
                int P = data[ 2 ];
                int LEN = data[ 3 ];
                int LEVEL = data[ 4 ];

                printf( "%s: PBX: (%02X) %-19s (%d,%d) P=%d: LEVEL=%d of %d" EOL, timestamp, int( data[0] ), 
                    "ActDisplayLevel", R, C, P, LEVEL, LEN
                    );
                }
            break;

        case FNC_ANTICLIPPINGUPDATE:
            // Function: The signal updates the transmission Anti clipping to be on or off and
            // the Anti clipping threshold.
            //
            // Return signal: None
            //
            // D(n) 8, ! B0 H FNC (Function code)
            // D(n+1) 8, ! XX H Anti clipping on/off
            // D(n+2) 8, ! XX H Anti clipping threshold
            //
            // Detailed description:
            // D(n+2) D(n+1)ANTI CLIPPING ON/OFF
            //   : 00 = Anti clipping off
            //   : 01 = Anti clipping on
            // D(n+2) ANTI CLIPPING THRESHOLD
            //   B0-B7 : 00 = -15dm0 Threshold
            //   : 01 = -13dm0 Threshold
            //   : 02 = -9dm0 Threshold
            //   : 03 = -7dm0 Threshold
            //
            arg_len = 2;
            PBX_logf( "AntiClippingUpdate", data, arg_len );
            break;

        case FNC_EXTRARNGUPDATE:
            //
            // Function: The signal updates the 3:rd tone ringer cadences for Intern, Extern
            // and Call-back and all cadences for extra tone ringing.
            //
            // Return signals: EQUSTA or EQUTESTRES, Test code 127
            //
            // D(n) 8, ! B1 H FNC (Function code)
            // D(n+1) 8, ! XX H Internal ring signal, 3:rd on interval
            // D(n+2) 8, ! XX H Internal ring signal, 3:rd off interval
            // D(n+3) 8, ! XX H External ring signal, 3:rd on interval
            // D(n+4) 8, ! XX H External ring signal, 3:rd off interval
            // D(n+5) 8, ! XX H Call back ring signal, 3:rd on interval
            // D(n+6) 8, ! XX H Call back ring signal, 3:rd off interval
            // D(n+7) 8, ! XX H Extra ring signal, 1:st on interval
            // D(n+8) 8, ! XX H Extra ring signal, 1:st off interval
            // D(n+9) 8, ! XX H Extra ring signal, 2:nd on interval
            // D(n+10) 8, ! XX H Extra ring signal, 2:nd off interval
            // D(n+11) 8, ! XX H Extra ring signal, 3:rd on interval
            // D(n+12) 8, ! XX H Extra ring signal, 3:rd off interval
            //
            // Detailed description:
            // D(n+1)-D(n+12) = 00 - FF H Number of 50 ms intervals
            //
            // Note: In DBC200 telephones the Internal and External 3:rd interval is not used.
            //
            arg_len = 12;
            PBX_logf( "ExtraRngUpdate", data, arg_len );
            break;

        case FNC_EXTRARINGING:
            //
            // Function: The signal activates the extra ringing.
            //
            // Return signals: None
            //
            // D(n) 8, ! B2 H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "ExtraRinging", data, arg_len );
            break;

        case FNC_EXTRARINGING1LOW:
            //
            // Function: The signal activates the extra ringing 1:st interval low.
            //
            // Return signals: None
            //
            // D(n) 8, ! B3 H FNC (Function code)
            //
            arg_len = 0;
            PBX_logf( "ExtraRinging1Low", data, arg_len );
            break;

        case FNC_HEADSETUPDATE:
            //
            // Function: The signal defines which programmable key shall be used as a
            // headset ON/OFF key.
            // When local controlled headset mode the headset key will not be sent
            // to SSW.
            // The associated LED is locally controlled after this signal.
            //
            // Return signals: None
            //
            // D(n) 8, ! B4 H FNC (Function code)
            // D(n+1) 8, ! XX H Headset mode
            // D(n+2) 8, ! XX H Headset key number (0-255)
            //
            // Detailed description:
            // D(n+1) HEADSET MODE
            // : 0 = Normal handset mode
            // : 1 = Local controlled headset mode
            // : 2 = SSW controlled headset mode passive
            // : 3 = SSW controlled headset mode active
            // D(n+2) HEADSET KEY NUMBER
            // : 0E H = Default headset key code
            // : FF H = No headset key defined
            //
            arg_len = 2;
            PBX_logf( "HeadsetUpdate", data, arg_len );
            break;

        case FNC_HANDSFREEPARAMETER:
            //
            // Function: The signal updates the transmission parameters in the DSP based
            // handsfree circuit. A specific parameter address can contain 1 to 10
            // data bytes.
            //
            // Return signals: None or HFPARAMETERRESP
            //
            // D(n) 8, ! B5 HFNC (Function code)
            // D(n+1) 8, ! XX H Number of parameter address
            // D(n+2) 8, ! XX H Parameter address
            // D(n+3) ! XX H Parameter address
            // : :
            // D(n+k) ! XX H Parameter address
            //
            // Detailed description:
            // D(n+2) PARAMETER ADDRESS
            //    B7 : 0 = Write parameters
            //    : 1 = Read parameters
            //    B0-B6 : 1 = Speech to Echo Comparator levels, 4 data bytes
            //    : 2 = Switchable Attenuation settings, 4 data bytes
            //    : 3 = Speech Comparator levels, 8 data bytes
            //    : 4 = Speech Detector levels, 5 data bytes
            //    : 5 = Speech Detector timing, 10 data bytes
            //    : 6 = AGC Receive, 7 data bytes
            //    : 7 = AGC Transmit, 6 data bytes
            //
            // D(n+2) = SPEECH TO ECHO COMPARATOR LEVELS
            // D(n+3) : XX H Gain of acoustic echo, -48 to +48 dB step 0.38dB
            // D(n+4) : XX H Gain of line echo, -48 to +48 dB step 0.38dB
            // D(n+5) : XX H Echo time, 0 to 1020 ms step 4ms
            // D(n+6) : XX H Echo time, 0 to 1020 ms step 4ms
            //
            // D(n+2) = SWITCHABLE ATTENUATION SETTING
            // D(n+3) : XX H Attenuation if speech, 0 to 95 dB step 0.5dB
            // D(n+4) : XX H Wait time, 16ms to 4s step 16ms
            // D(n+5) : XX H Decay speed, 0.6 to 680 ms/dB step 2.66ms/dB
            // D(n+6) : XX H Switching time, 0.005 to 10 ms/dB
            //
            // D(n+2) = SPEECH COMPARATOR LEVELS
            // D(n+3) : XX H Reserve when speech, 0 to 48 dB step 0.19dB
            // D(n+4) : XX H Peak decrement when speech, 0.16 to 42 ms/dB
            // D(n+5) : XX H Reserve when noise, 0 to 48 dB step 0.19dB
            // D(n+6) : XX H Peak decrement when noise, 0.16 to 42 ms/dB
            // D(n+7) : XX H Reserve when speech, 0 to 48 dB step 0.19dB
            // D(n+8) : XX H Peak decrement when speech, 0.16 to 42 ms/dB
            // D(n+9) : XX H Reserve when noise, 0 to 48 dB step 0.19dB
            // D(n+10 : XX H Peak decrement when noise, 0.16 to 42 ms/dB
            //
            // D(n+2) = SPEECH DETECTOR LEVELS
            // D(n+3) : XX H Limitation of amplifier, -36 to -78 dB step 6.02dB
            // D(n+4) : XX H Level offset up to noise, 0 to 50 dB step 0.38dB
            // D(n+5) : XX H Level offset up to noise, 0 to 50 dB step 0.38dB
            // D(n+6) : XX H Limitation of LP2, 0 to 95 dB step 0.38
            // D(n+7) : XX H Limitation of LP2, 0 to 95 dB step 0.38
            //
            // D(n+2) = SPEECH DETECTOR TIMING
            // D(n+3) : XX H Time constant LP1, 1 to 512 ms
            // D(n+4) : XX H Time constant LP1, 1 to 512 ms
            // D(n+5) : XX H Time constant PD (signal), 1 to 512 ms
            // D(n+6) : XX H Time constant PD (noise), 1 to 512 ms
            // D(n+7) : XX H Time constant LP2 (signal), 4 to 2000 s
            // D(n+8) : XX H Time constant LP2 (noise), 1 to 512 ms
            // D(n+9) : XX H Time constant PD (signal), 1 to 512 ms
            // D(n+10 : XX H Time constant PD (noise), 1 to 512 ms
            // D(n+11) : XX H Time constant LP2 (signal), 4 to 2000 s
            // D(n+12) : XX H Time constant LP2 (noise), 1 to 512 ms
            //
            // D(n+2) = AGC RECEIVE
            // D(n+3) : XX H Loudspeaker gain, -12 to 12 dB
            // D(n+4) : XX H Compare level, 0 to -73 dB
            // D(n+5) : XX H Attenuation of control, 0 to -47 dB
            // D(n+6) : XX H Gain range of control, 0 to 18 dB
            // D(n+7) : XX H Setting time for higher levels, 1 to 340 ms/dB
            // D(n+8) : XX H Setting time for lower levels, 1 to 2700 ms/dB
            // D(n+9) : XX H Threshold for AGC-reduction, 0 to -95 dB
            //
            // D(n+2) = AGC TRANSMIT
            // D(n+3) : XX H Loudspeaker gain, -12 to 12 dB
            // D(n+4) : XX H Compare level, 0 to -73 dB
            // D(n+5) : XX H Gain range of control, 0 to 18 dB
            // D(n+6) : XX H Setting time for higher levels, 1 to 340 ms/dB
            // D(n+7) : XX H Setting time for lower levels, 1 to 2700 ms/dB
            // D(n+8) : XX H Threshold for AGC-reduction, 0 to -95 dB
            arg_len = -1; // Ignore the rest of PDU. FIXME
            PBX_logf( "HandsfreeParameter", data, data_len );
            break;

        case FNC_EQUSTAD4REQ:
            //
            // Function: The signal request for D4 information. If D4 terminal received this
            // signal, it will answer it with EQUSTA. But not the D3 terminals
            //
            // Return signals: EQUSTA (d4 INFORMATION )
            //
            // D(n) 8, ! D0 H FNC (Function code)
            // D(n+1) 8, ! XX H Exchange Type
            //
            // Detailed description:
            // D(n+1) Exchange Type
            // B0-B7: 0 = MD system
            //        1 = BP system
            //        2 = MDE system        
            //   
            arg_len = 1;
            {
                int PBXTYPE = data[ 1 ];
                static const char* desc[] = { "MD System", "BP System", "MDE System" };

                printf( "%s: PBX: (%02X) %-19s PBX=%02X: %s" EOL,
                    timestamp, int( data[0] ), "EquStaD4Req", 
                    PBXTYPE,
                    PBXTYPE >= 0 && PBXTYPE <= 2 ? desc[ PBXTYPE ] : "?"
                    );
                }
            break;

        case FNC_CLEARDISPLAYGR:
            // 
            // Function: The signal clear the display area defined by signal. clears all or part
            // of a graphic display
            //
            // Return signals: None
            // 
            // D(n) 8, ! D1 H FNC (Function code)
            // D(n+1) 16, ! XXXX H Coordinate x (start pixel) note: LSB first
            // D(n+3) 8, ! XX H Coordinate y (start pixel)
            // D(n+4) 16, ! XXXX H Coordinate x (end pixel) note: LSB first
            // D(n+6) 8, ! XX H Coordinate y (end pixel)
            // 
            arg_len = -1;
            {
                int X1 = ( data[ 2 ] << 8 ) | data[ 1 ];
                int Y1 = data[ 3 ];
                int X2 = ( data[ 5 ] << 8 ) | data[ 4 ];
                int Y2 = data[ 6 ];

                printf( "%s: PBX: (%02X) %-19s (%d,%d)-(%d,%d)" EOL,
                    timestamp, int( data[0] ), "ClearDisplayGr", X1, Y1, X2, Y2
                    );
                }
            break;

        case FNC_WRITEDISPLAYGR:
            // 
            // Function: The signal is going to write a character string in the graphic display.
            // The character string data not used will be filled with FF. Coordinate
            // (FFFF,FF) has special meaning: Write the string after the last string.
            //
            // Return signals: None
            //
            // D(n) 8, ! D2 H FNC (Function code)
            // D(n+1) 16, ! XXXX H Coordinate x (start pixel) note: LSB first
            // D(n+3) 8, ! XX H Coordinate y (start pixel)
            // D(n+4) 8, ! XX H NOT USED (comment by MBK)
            // D(n+5) 8, ! XX H Character description
            // D(n+6) 8, ! XX H Length of String
            // D(n+7) 8, ! XX1 H Character string
            // : :
            // D(n+X) : ! XXN H
            //
            // Detailed description:
            //
            // D(n+4) CHARACTER DESCRIPTION
            // B0-B4 = 1 : Small fonts in D4 mode
            //       = 2 : Medium fonts in D4 mode
            //       = 3 : Large fonts in D4 mode
            //       = 14 : DBC222, 223 fonts in D3 mode
            //       = 15 : DBC224,DBC225 fonts in D3 mode
            // B5-B6 = 0 : 0% of full colour
            //       = 1 : 30% of full colour
            //       = 2 : 60% of full colour
            //       = 3 : 100% of full colour
            // B7    = 0 : No blinking
            //       = 1 : Blinking     
            // 
            arg_len = -1; // Ignore the rest of PDU.
            {
                int X = ( data[ 2 ] << 8 ) | data[ 1 ];
                int Y = data[ 3 ];
                int ATTR = data[ 5 ];
                unsigned int LEN = data[ 6 ];
                if ( LEN >= sizeof( remote_party ) )
                    LEN = sizeof( remote_party ) - 2;

                printf( "%s: PBX: (%02X) %-19s (%d,%d) A=%02X: \"%.*s\"" EOL,
                    timestamp, int( data[0] ), "WriteDisplayGr", X, Y, ATTR,
                    LEN, data + 7
                    );

                if ( fd_Wave >= 0 ) // While in Transmission
                {
                    if ( hasDisplay == GRAPHICAL_DISPLAY_223 )
                    {
                        if ( Y == 17 && X != 0 )
                        {
                            memcpy( remote_party, data + 7, LEN );
                            remote_party[ LEN ] = 0;
                            }
                        }
                    else if ( hasDisplay == GRAPHICAL_DISPLAY_225 )
                    {
                        if ( Y == 34 && X != 0 ) 
                        {
                            memcpy( remote_party, data + 7, LEN );
                            remote_party[ LEN ] = 0;
                            }
                        }
                    }
                }
            break;

        case FNC_STOPWATCHGR:
            //
            // Function: The signal clear the display area defined by signal. clears all or part
            // of a graphic display
            //
            // Return signals: None
            //
            // D(n) 8, ! D3 H FNC (Function code)
            // D(n+1) 16, ! XXXX H Coordinate x (start pixel) note: LSB first
            // D(n+3) 8, ! XX H Coordinate y (start pixel)
            // D(n+4) 8, ! XX H Stopwatch order
            // D(n+5) 8, ! XX H Character description
            //
            // Detailed description:
            //
            // D(n+4) STOPWATCH ORDER
            // B0-B1 = 0 : Reset to zero
            //       = 1 : Stop counting
            //       = 2 : Start counting
            //       = 3 : Dont affect the counting
            // B2    = 0 : Remove stopwatch from display
            //       = 1 : Present stopwatch on display
            // B3-B7 = 0 : Not used
            //
            // D(n+5) CHARACTER DESCRIPTION
            // B0-B4 = 1 : Small fonts in D4 mode
            //       = 2 : Medium fonts in D4 mode
            //       = 3 : Large fonts in D4 mode
            //       = 14 : DBC222, 223 fonts in D3 mode
            //       = 15 : DBC224,DBC225 fonts in D3 mode
            // B5-B6 = 0 : 0% of full colour
            //       = 1 : 30% of full colour
            //       = 2 : 60% of full colour
            //       = 3 : 100% of full colour
            // B7 : Not used
            //
            arg_len = -1;
            {
                int X = ( data[ 2 ] << 8 ) | data[ 1 ];
                int Y = data[ 3 ];
                int ORDER = data[ 4 ];
                int ATTR = data[ 5 ];

                printf( "%s: PBX: (%02X) %-19s (%d,%d) A=%02X, Order=%02X" EOL,
                    timestamp, int( data[0] ), "StopWatchGr", X, Y, ATTR, ORDER
                    );
                }
            break;
            
        case FNC_CLOCKORGR:
            //
            // Function: The signal adjusts the local real time clock in the instrument.
            //
            // Return signals: None
            //
            // D(n) 8, ! D4 H FNC (Function code)
            // D(n+1) 8, ! XX H Type of time presentation
            // D(n+2) 16, ! XXXX H Coordinate x (start pixel) note: LSB first
            // D(n+4) 8, ! XX H Coordinate y (start pixel)
            // D(n+5) 8, ! XX H Hours in BCD-code
            // D(n+6) 8, ! XX H Minutes in BCD-code
            // D(n+7) 8, ! XX H Seconds in BCD-code
            // D(n+8) 8, ! XX H Character description
            //
            // Detailed description:
            //
            // D(n+1) TYPE OF TIME PRESENTATION
            // B0-B7 = 0 : 24-hour presentation
            // = 1 : 12-hour presentation
            //
            // D(n+5) HOURS IN BCD-CODE
            // MINUTES IN BCD-CODE
            // SECONDS IN BCD-CODE
            // B0-B3 Least significant figure in BCD code
            // B4-B7 Most significant figure in BCD code
            //
            // D(n+8) CHARACTER DESCRIPTION
            // B0-B4 = 1 : Small fonts in D4 mode
            //       = 2 : Medium fonts in D4 mode
            //       = 3 : Large fonts in D4 mode
            //       = 14 : DBC222, 223 fonts in D3 mode
            //       = 15 : DBC224,DBC225 fonts in D3 mode
            // B5-B6 = 0 : 0% of full colour
            //       = 1 : 30% of full colour
            //       = 2 : 60% of full colour
            //       = 3 : 100% of full colour
            // 
            arg_len = -1; // Ignore the rest of PDU. FIXME
            {
                int X = ( data[ 3 ] << 8 ) | data[ 2 ];
                int Y = data[ 4 ];
                int ATTR = data[ 8 ];
                int REPR = data[ 1 ];
                int H = ( data[ 5 ] >> 4 ) * 10 + ( data[ 5 ] & 0xF );
                int M = ( data[ 6 ] >> 4 ) * 10 + ( data[ 6 ] & 0xF );
                int S = ( data[ 7 ] >> 4 ) * 10 + ( data[ 7 ] & 0xF );

                printf( "%s: PBX: (%02X) %-19s (%d,%d) A=%02X, Repr=%02X: %02d:%02d:%02d" EOL,
                    timestamp, int( data[0] ), "ClockOrGr", X, Y, ATTR, REPR, H, M, S
                    );
                }
            break;

        case FNC_ACTCURSORGR:
            //
            // Function: This function code is equivalent to the existing ACTIVATE BLINKING
            // CURSOR. The signal activates the blinking cursor on the display.
            // Currently, the field and position where the cursor should be displayed
            // are specified by the EXCHANGE and so will be the starting pixel.
            // The only cursor management which is handled by the firmware is to
            // remove the cursor when a new one is displayed. Coordinate
            // (FFFF,FF) has special meaning: Put the corsor after the last string.
            //
            // Return signals: None
            //
            // D(n) 8, ! D5 H FNC (Function code)
            // D(n+1) 16, ! XXXX H Coordinate x (start pixel) note: LSB first
            // D(n+3) 8, ! XX H Coordinate y (start pixel)
            // D(n+4) 8, ! XX H ASCII code for cursor (Not Used)
            // D(n+5) 8, ! XX H Character description
            //
            // Detailed description:
            //
            // D(n+5) CHARACTER DESCRIPTION
            // B0-B4 = 1 : Small fonts in D4 mode
            //       = 2 : Medium fonts in D4 mode
            //       = 3 : Large fonts in D4 mode
            //       = 14 : DBC222, 223 fonts in D3 mode
            //       = 15 : DBC224,DBC225 fonts in D3 mode
            // B5-B6 = 0 : 0% of full colour
            //       = 1 : 30% of full colour
            //       = 2 : 60% of full colour
            //       = 3 : 100% of full colour
            // B7 : Not used
            //      
            arg_len = -1; // Ignore the rest of PDU. FIXME
            {
                int X = ( data[ 2 ] << 8 ) | data[ 1 ];
                int Y = data[ 3 ];
                int ATTR = data[ 5 ];

                printf( "%s: PBX: (%02X) %-19s (%d,%d) A=%02X" EOL,
                    timestamp, int( data[0] ), "ActCursorGr", X, Y, ATTR
                    );
                }
            break;

        case FNC_HWICONSGR:
            //
            // Function: This signal controls the local SHOWICONS function in the
            // instrument according to the description below.
            //
            // Return signals: None
            //
            // D(n) 8, ! D6 H FNC (Function code)
            // D(n+1) 16, ! XXXX H Coordinate x (start pixel) note: LSB first
            // D(n+3) 8, ! XX H Coordinate y (start pixel)
            // D(n+4) 8, ! XX H SHOWICONS order
            //
            // Detailed description:
            //
            // D(n+4) SHOWICONS ORDER
            // B0    = 0 Remove status icons from display
            //       = 1 Present status icons on display
            // B1-B7 = 0 Not used
            //
            arg_len = -1; // Ignore the rest of PDU.
            {
                int X = ( data[ 2 ] << 8 ) | data[ 1 ];
                int Y = data[ 3 ];
                int ORDER = data[ 4 ];

                printf( "%s: PBX: (%02X) %-19s (%d,%d) Order=%02X: %s" EOL,
                    timestamp, int( data[0] ), "HwIconsGr", X, Y, ORDER,
                    ORDER & 0x01 ? "Show" : "Hide"
                    );
                }
            break;

        case FNC_DRAWGR:
            //
            // Function: The signal displays a line/rectangle from start pixel to end pixel
            //
            // Return signals: None
            //
            // D(n) 8, ! D7 H FNC (Function code)
            // D(n+1) 8, ! XX H Draw description
            // D(n+2) 16, ! XXXX H Coordinate x (start pixel) note: LSB first
            // D(n+4) 8, ! XX H Coordinate y (start pixel)
            // D(n+5) 8, ! XX H Extra info (not used)
            // D(n+6) 16, ! XX H Coordinate x (end pixel)
            // D(n+8) 8, ! XX H Coordinate y (end pixel)
            // D(n+9) 8, ! XX H Character description
            //
            // Detailed description:
            //
            // D(n+1) DRAW DESCRIPTION
            // B0-B3 = 1 : horizontal line
            //       = 2 : vertical line
            //       = 3 : rectangle
            //
            // D(n+9) CHARACTER DESCRIPTION
            // B0-B4 = 1 : 1 pixel thick
            //       = 2 : 2 pixels thick
            //       = 3 : 3 pixels thick
            // B5-B6 = 0 : 0% of full colour
            //       = 1 : 30% of full colour
            //       = 2 : 60% of full colour
            //       = 3 : 100% of full colour
            // B7    = 0 : No blinking
            //       = 1 : Blinking
            //
            arg_len = -1; // Ignore the rest of PDU. FIXME
            {
                int DRAW = data[ 1 ] & 0xF;
                int X1 = ( data[ 3 ] << 8 ) | data[ 2 ];
                int Y1 = data[ 4 ];
                int X2 = ( data[ 5 ] << 7 ) | data[ 6 ];
                int Y2 = data[ 8 ];
                int ATTR = data[ 9 ];

                printf( "%s: PBX: (%02X) %-19s (%d,%d)-(%d,%d) A=%02X, %s" EOL,
                    timestamp, int( data[0] ), "DrawGr", X1, Y1, X2, Y2, ATTR, 
                    DRAW == 0 ? "DrawDesc = 0(!?)" : DRAW == 1 ? "H-Line" : DRAW == 2 
                              ? "V-Line" : DRAW == 3 ? "Rect" : "DrawDesc > 3(!?)"
                    );
                }
            break;
            
        case FNC_LISTDATA:
            // 
            // Function: Send list strings to be stored and displayed
            //
            // Return signals: None
            //
            // D(n) 8, ! D8 H FNC (Function code)
            // D(n+1) 8, ! XX H Number of strings to update
            // D(n+2) 8, ! XX H List buffer position for string 1
            // D(n+3) 8, ! XX H Number of alphabets in string 1
            // D(n+4) 8, ! XX H Alphabet 1 in string1
            // D(n+5) 8, ! XX H Num of characters for alphabet 1 in string 1
            // D(n+6) 8, ! XX H Char 1 for alphabet 1 in string 1
            // ...
            // D(m+1) 8, ! XX H Alphabet 2 in string1
            // D(m+2) 8, ! XX H Num of characters for alphabet 2 in string 1
            // D(m+3) 8, ! XX H Char 1 for alphabet 2 in string 1
            // ...
            // D(p+1) 8, ! XX H List buffer position for string 2
            // D(p+2) 8, ! XX H Number of alphabets in string 2
            // D(p+3) 8, ! XX H Alphabet 1 in string2
            // D(p+4) 8, ! XX H Num of characters for alphabet 1 in string 2
            // D(p+5) 8, ! XX H Char 1 for alphabet 1 in string 2
            // ...
            // D(q+1) 8, ! XX H Alphabet 2 in string2
            // D(q+2) 8, ! XX H Num of characters for alphabet 2 in string 2
            // D(q+3) 8, ! XX H Char 1 for alphabet 2 in string 2
            // ...      
            arg_len = -1; // Ignore the rest of PDU. FIXME
            PBX_logf( "ListData", data, data_len );
            break;

        case FNC_SCROLLLIST:
            // 
            // Function: Send list strings order.
            //
            // Return signals: None
            //
            // D(n) 8, ! D9 H FNC (Function code)
            // D(n+1) 16, ! XX H Total number of list elements in the list(LSB first)
            // D(n+3) 16, ! XX H List element position in the total list - scrollbar(LSB
            // first)
            // D(n+5) 8, ! XX H Displayed position to highlight (1-4)
            // D(n+6) 8, ! XX H List buffer position to display first
            // D(n+7) 8, ! XX H First character description (see below)
            // D(n+8) 8, ! XX H List buffer position to display second
            // D(n+9) 8, ! XX H Second character description (see below)
            // D(n+10) 8, ! XX H List buffer position to display third
            // D(n+11) 8, ! XX H Third character description (see below)
            // D(n+12) 8, ! XX H List buffer position to display fourth
            // D(n+13) 8, ! XX H Fourth character description (see below)
            //
            // Detailed description:
            //
            // D(n+6) Number of row / option to display
            // D(n+8) Number of row / option to display
            // D(n+10) Number of row / option to display
            // D(n+12) Number of row / option to display
            //         nn = Display row
            //         FF = Display blank row
            // D(n+7) CHARACTER DESCRIPTION
            // D(n+9) CHARACTER DESCRIPTION
            // D(n+11) CHARACTER DESCRIPTION
            // D(n+13 CHARACTER DESCRIPTION
            // B0-B4 1 = Font small
            //       2 = Font medium
            //       3 = Font large
            // B5-B6 = 0 : 0% of full colour
            //       = 1 : 30% of full colour
            //       = 2 : 60% of full colour
            //       = 3 : 100% of full colour
            // B7 : Not used
            //
            arg_len = -1; // Ignore the rest of PDU. FIXME
            PBX_logf( "ScrollList", data, data_len );
            break;

        case FNC_TOPMENUDATA:
            // 
            // Function: Send list strings to be stored and displayed
            //
            // Return signals: None
            //
            // D(n) 8, ! DA H FNC (Function code)
            // D(n+1) 8, ! XX H Number of strings to update
            // D(n+2) 8, ! XX H List buffer position for string 1
            // D(n+3) 8, ! XX H Number of alphabets in string 1
            // D(n+4) 8, ! XX H Alphabet 1 in string 1
            // D(n+5) 8, ! XX H Num of chars for alphabet 1 in string 1
            // D(n+6) 8, ! XX H Char 1 for alphabet 1 in string 1
            // ...
            // D(p+1) 8, ! XX H List buffer position for string 2
            // D(p+2) 8, ! XX H Number of alphabets in string 2
            // D(p+3) 8, ! XX H Alphabet 1 in string 2
            // D(p+4) 8, ! XX H Num of chars for alphabet 1 in string 2
            // D(p+5) 8, ! XX H Char 1 for alphabet 1 in string 2
            // ...
            //
            arg_len = -1; // Ignore the rest of PDU. FIXME
            PBX_logf( "TopMenuData", data, data_len );
            break;

        case FNC_SCROLLTOPMENU:
            //
            // Function: Highlight a top menu heading.
            //
            // Return signals: None
            //
            // D(n) 8, ! DB H FNC (Function code)
            // D(n+1) 8, ! XX H Number of column / heading to highlight,
            // one of 0 - 5
            //
            arg_len = -1; // Ignore the rest of PDU. FIXME
            PBX_logf( "ScrollTopMenu", data, data_len );
            break;

        case FNC_SCROLLSUBMENU:
            // 
            // Function: Highlight an option under a top menu heading.
            //
            // Return signals: None
            //
            // D(n) 8, ! DC H FNC (Function code)
            // D(n+1) 8, ! XX H Total number of options
            // D(n+2) 8, ! XX H Position of option in whole list - scrollbar
            // D(n+3) 8, ! XX H Number of displayed element to highlight (1-4)
            // D(n+4) 8, ! XX H Number of row/option to display first,
            //             FF = display blank row
            // D(n+5) 8, ! XX H Number of row/option to display second,
            //             FF = display blank row
            // D(n+6) 8, ! XX H Number of row/option to display third,
            //             FF = display blank row
            // D(n+7) 8, ! XX H Number of row/option to display fourth,
            //             FF = display blank row
            //
            arg_len = -1; // Ignore the rest of PDU. FIXME
            PBX_logf( "ScrollSubMenu", data, data_len );
            break;

        case FNC_EQULOOP:
            //
            // Function: The signal controls loop back activation and deactivation of the
            // available loops in the equipment. These are described below.
            //
            // Return signals: EQUSTA
            //
            // D(n)   8, ! E0 H FNC (Function code)
            // D(n+1) 8, ! XX H Loop data
            //
            // Detailed description:
            //
            // D(n+1) LOOP DATA
            //   B2 : 0 = Deactivate loop of B1 channel to line
            //      : 1 = Activate loop of B1 channel back to line
            //   B3 : 0 = Deactivate loop of B2 channel to line
            //      : 1 = Activate loop of B2 channel back to line
            //
            arg_len = 1;
            PBX_logf( "EquLoop", data, arg_len );
            break;

        case FNC_EQUTESTREQ:
            //
            // Function: The signal requests test of different functions in the instrument. The
            // type of test is specified by the test code which is associated with the
            // function code.
            //
            // Return signals: EQUTESTRES (depending on code)
            //
            arg_len = -1; // Ignore the rest of PDU. FIXME
            PBX_logf( "EquTestReq", data, data_len );
            break;

        case FNC_NULLORDER:
            arg_len = 0;
            break;

        default:
            //
            // Unknown FNC or not properly parsed.
            //
            arg_len = -1; // Ignore the rest of PDU.
            PBX_logf( "Unknown FNC", data, data_len );
            break;
        }
    
    if ( arg_len < 0 )
        return;

    // Find next FNC
    //
    ++arg_len; // include FNC
    data += arg_len;
    data_len -= arg_len;

    goto PARSE_NEXT_FNC;
    }

///////////////////////////////////////////////////////////////////////////////

void sigterm_handler( int )
{
    sleep( 1 );
    exit( 0 );
    }

int main( int argc, char** argv )
{
    freopen( "/dev/console", "a", stderr );

    if ( argc < 2 )
    {
        fprintf( stderr, "Usage: idodVR <channel#>" EOL );
        return -1;
        }

    signal( SIGTERM, sigterm_handler );

    int portID = strtol( argv[ 1 ], NULL, 10 );
    DSPCFG dspCfg;
    if ( ! dspCfg.Find( portID, dsp_cfg ) )
    {
        fprintf( stderr, "idodVR: Error: Invalid channel #%d" EOL, portID + 1 );
        return -1;
        }

    // Wait for /voice filesystem to be mounted:
    //
    // TODO: Try with mountpoint instead of greping /proc/mounts?
    //       Note that mountpoint is not present in FC2!
    //
    bool fsWarnIssued = false;
    while ( 0 != system( "grep -q '/voice ext' /proc/mounts" ) )
    {
        if ( ! fsWarnIssued )
        {
            fprintf( stderr, "idodVR: %d: Waiting /voice filesystem to be mounted\n", portID );
            fsWarnIssued = true;    
            }
        sleep( 1 );
        }
    if ( fsWarnIssued )
    {
        fprintf( stderr, "idodVR: %d: /voice filesystem is mounted; continuing...\n", portID );
        }

    // Wait DSP to be running
    //
    dspCfg.waitToBeRunning ();

    // DSP Channel
    //
    int channel = dspCfg.channel;
    char dspboard[ 256 ];
    strcpy( dspboard, dspCfg.devname );
    strcat( dspboard, "io" );

    if ( ! dspCfg.isRunning () )
    {
        fprintf( stderr, "idodVR: Error: Failed to open %s" EOL, dspboard );
        return -1;
        }

    // Load parameters
    //
    PORT_PARAMETERS::GetConnParams( portID, localUdpPort, udp_DSCP, tcp_DSCP );

    PORT_PARAMETERS::GetVR( portID, DN, 
        bomb_key, startrec_key, stoprec_key, split_filesize, 
        NAS_URL1, NAS_URL2, is_VAD_active, vad_threshold, vad_hangovertime, vr_minfilesize
        );

    trim( DN );
    if ( ! DN[0] )
        sprintf( DN, "%d", 101 + portID ); // default: 101, 102, 103...
    if ( stoprec_key < 0 )
        stoprec_key = startrec_key;

    fprintf( stderr, 
        "idodVR: %d: Loaded: DN=[%s], BK=%d, START=%d, STOP=%d,\n"
        "    MAX FS=%ld, MIN FS=%ld, FTP: [%s] [%s],\n"
        "    VAD=%s, THRSH=%d, HANGT=%lu, UDP=%hu\n", 
        portID, DN, bomb_key, startrec_key, stoprec_key, split_filesize, vr_minfilesize, NAS_URL1, NAS_URL2,
        is_VAD_active ? "on" : "off", vad_threshold, vad_hangovertime, localUdpPort
        );

    if ( split_filesize > 0 ) 
        split_filesize *= 8000; // got in seconds, store in samples

    if ( vad_hangovertime > 0 )
        vad_hangovertime *= 8; // got in milliseconds, store in samples

    if ( vr_minfilesize > 0 ) 
        vr_minfilesize *= 8; // got in milliseconds, store in samples

    // Transaction TODO Directory (owned by root)
    //
    mkdir( "/voice/todo", 0775 );

    // Working Directory (owned by voice UID)
    //
    sprintf( filename, "/voice/%s", DN );
    mkdir( filename, 0775 );
    chown( filename, UID_VOICE, GID_VOICE );
    chdir( filename );

    sprintf( csv_filename, "/voice/%s.csv", DN );

    // Open UDP channel
    //
    int inUdpSocket;
    
    sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY; // OR: inet_addr( "interface" ); 
	local.sin_port = htons( localUdpPort ); // Port MUST be in Network Byte Order

	inUdpSocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Open inbound UDP socket
	if ( inUdpSocket < 0 )
    {
        localUdpPort = 0;
        }
    else if ( bind( inUdpSocket, (sockaddr*)&local, sizeof( local ) ) != 0 )
    {
        close( inUdpSocket );
        inUdpSocket = -1;
        localUdpPort = 0;
        }

    getTimestamp ();

    printf( "%s: SYS: ---- %-19s %s"  EOL, timestamp, "DSP Device", dspboard );
    printf( "%s: SYS: ---- %-19s %d"  EOL, timestamp, "DSP Subchannel", channel );
    printf( "%s: SYS: ---- %-19s '%s'"  EOL, timestamp, "Directory Number", DN );
    printf( "%s: SYS: ---- %-19s '%s'"  EOL, timestamp, "NAS URL 1", NAS_URL1 );
    printf( "%s: SYS: ---- %-19s '%s'"  EOL, timestamp, "NAS URL 2", NAS_URL2 );
    printf( "%s: SYS: ---- %-19s %ld samples (%.1lfs)" EOL, timestamp, "Max Rec Filesize", split_filesize, double( split_filesize ) / 8000.0 );
    printf( "%s: SYS: ---- %-19s %ld samples (%.3lfs)" EOL, timestamp, "Min Rec Filesize", vr_minfilesize, double( vr_minfilesize ) / 8000.0 );
    printf( "%s: SYS: ---- %-19s %s"  EOL, timestamp, "VAD Active", is_VAD_active ? "true" : "false" );
    printf( "%s: SYS: ---- %-19s %d"  EOL, timestamp, "VAD Threshold", vad_threshold );
    printf( "%s: SYS: ---- %-19s %ld samples (%.3lfs)" EOL, timestamp, "VAD Hangover Time", vad_hangovertime, double( vad_hangovertime )/ 8000.0 );
    printf( "%s: SYS: ---- %-19s %d"  EOL, timestamp, "Bomb-Threat Key", bomb_key);
    printf( "%s: SYS: ---- %-19s %d"  EOL, timestamp, "Start Rec Key", startrec_key );
    printf( "%s: SYS: ---- %-19s %d"  EOL, timestamp, "Stop Rec Key", startrec_key );
    printf( "%s: SYS: ---- %-19s %d"  EOL, timestamp, "UDP Port", localUdpPort );
    fflush( stdout );

    f_DM = open( dspboard, O_RDWR );
    if ( f_DM < 0 )
    {
        printf( "idodVR: Error: Failed to open %s" EOL, dspboard );
        return -1;
        }

    f_DS = open( dspboard, O_RDWR );
    if ( f_DS < 0 )
    {
        printf( "idodVR: Error: Failed to open %s" EOL, dspboard );
        return -1;
        }

    f_B = open( dspboard, O_RDWR );
    if ( f_B < 0 )
    {
        printf( "idodVR: Error: Failed to open B channel #%d" EOL, portID );
        return -1;
        }

    ::ioctl( f_DM, IOCTL_HPI_SET_D_CHANNEL, channel + 1 ); // DTS
    ::ioctl( f_DS, IOCTL_HPI_SET_D_CHANNEL, channel ); // PBX
    ::ioctl( f_B, IOCTL_HPI_SET_B_CHANNEL, channel );

    // Put PDU that should be automatically written when device is closed/relased.
    // Ioctl param value 1 indicates that next write will contain PDU.
    //
    ::ioctl( f_DM, IOCTL_HPI_ON_CLOSE_WRITE, 1 ); 
    EquipmentTestRequestReset ();

    // Now, real equipment test request RESET
    EquipmentTestRequestReset ();

#if 0
    // DASL power down/up cycle on PBX
    {
        static unsigned char PDU [] = { 0, 0x00, 0x51, 0x00 }; //  Set line down
        PDU[ 0 ] = 0; // Write to PBX
        write( f_DS, PDU, sizeof( PDU ) );
        }
#endif

    int maxfds = 0;
    if ( f_DM > maxfds )
        maxfds = f_DM;
    if ( f_DS > maxfds )
        maxfds = f_DS;
    if ( f_B > maxfds )
        maxfds = f_B;
    if ( inUdpSocket > maxfds )
        maxfds = inUdpSocket;

    unsigned char line[ 4096 ];

    for (;;)
    {
        fd_set readfds;
        FD_ZERO( &readfds );
        FD_SET( f_DM, &readfds );
        FD_SET( f_DS, &readfds );
        FD_SET( f_B, &readfds );
        if ( inUdpSocket >= 0 )
            FD_SET( inUdpSocket, &readfds );

        int rc = select( maxfds + 1, &readfds, NULL, NULL, NULL );
        if ( rc < 0 )
            break;
        else if ( rc == 0 )
            continue;

        ///////////////////////////////////////////////////////////////////////
        // B Channel from PBX
        ///////////////////////////////////////////////////////////////////////
        //
        if ( FD_ISSET( f_B, &readfds ) )
        {
            int rd = read_stream( f_B, line, 2 );

            if ( 0 == rd )
                break; // EOF

            if ( 2 != rd )
                break;

            int len = ( line[ 0 ] << 8 )  + line[ 1 ];

            rd = read_stream( f_B, line, len );

            if ( 0 == rd )
                break; // EOF

            if ( len != rd )
                break;

            // Normalize old and new payload types
            //
            int old_pt = payload_type; if ( old_pt >= 250 ) old_pt = 0;
            int new_pt = line[1];      if ( new_pt >= 250 ) new_pt = 0;

            // On-fly change of payload type
            //
            if ( line[1] != payload_type )
                EndRecording ();

            // Start recording if not already started 
            // and: manual recording is already started or it automatic recording
            //
            if ( fd_Wave < 0 && ( startrec_key < 0 || manually_started_recording ) )
            {
                payload_type = line[1];

                BeginRecording ();
                }

            if ( fd_Wave >= 0 )
            {
                // Skip first 4 octets:
                // - Octet [0] is subchannel#, skip it
                // - Octet [1] is payload type
                // - Octets [2,3] are sequence number
                //
                len -= 4;
                unsigned char* payload = line + 4;

                write( fd_Wave, payload, len );

                switch ( payload_type )
                {
                    case   0:
                    case   1: sample_count += len;         break; // 64 kbps
                    case   2: sample_count += len * 8 / 5; break; // 40 kbps
                    case   3: sample_count += len * 8 / 4; break; // 32 kbps
                    case   4: sample_count += len * 8 / 3; break; // 24 kbps
                    case   5: sample_count += len * 8 / 2; break; // 16 kbps
                    case   6: sample_count += len * 8;     break; //  8 kbps
                    case 250:
                    case 251: sample_count += len;         break; // 64 kbps
                    }

                if ( split_filesize > 0 && sample_count >= split_filesize ) // Max sec per file
                    EndRecording ();
                else if ( sample_count >= max_filesize )
                    EndRecording ();
                }
            }

        ///////////////////////////////////////////////////////////////////////
        // D Channel from PBX
        ///////////////////////////////////////////////////////////////////////
        //
        if ( FD_ISSET( f_DS, &readfds ) )
        {
            int rd = read_stream( f_DS, line, 2 );

            if ( 0 == rd )
                break; // EOF

            if ( 2 != rd )
                break;

            int len = ( line[ 0 ] << 8 ) + line[ 1 ];

            rd = read_stream( f_DS, line, len ); 
            // subchannel goes to line[1] and data from line[2]
            // we will put length in line[0] and line[1]

            if ( 0 == rd )
                break; // EOF

            if ( len != rd )
                break;

            getTimestamp ();

            ParsePDU_FromPBX( line + 1, len - 1 );
            fflush( stdout );

            if ( line[ 2 ] == FNC_DASL_LOOP_SYNC )
            {
                }
            else if ( line[ 2 ] == FNC_XMIT_FLOW_CONTROL )
            {
                }
            else if ( line[ 2 ] == FNC_RESET_REMOTE_DTSS )
            {
                }
            else if ( line[ 2 ] == FNC_KILL_REMOTE_IDODS )
            {
                }
            else
            {
                // Forward from PBX to DTS
                //
                line[ 0 ] = 1; // Channel #1 == f_DM i.e. DTS
                write( f_DM, line, len );
                }
            }

        ///////////////////////////////////////////////////////////////////////
        // D Channel from DTS
        ///////////////////////////////////////////////////////////////////////
        //
        if ( FD_ISSET( f_DM, &readfds ) )
        {
            int rd = read_stream( f_DM, line, 2 );

            if ( 0 == rd )
                break; // EOF

            if ( 2 != rd )
                break;

            int len = ( line[ 0 ] << 8 ) + line[ 1 ];

            rd = read_stream( f_DM, line, len ); 

            if ( 0 == rd )
                break; // EOF

            if ( len != rd )
                break;

            getTimestamp ();

            ParsePDU_FromDTS( line + 1, len - 1 );
            fflush( stdout );
 
            if ( line[ 2 ] == FNC_DASL_LOOP_SYNC )
            {
                }
            else if ( line[ 2 ] == FNC_XMIT_FLOW_CONTROL )
            {
                }
            else if ( line[ 2 ] == FNC_RESET_REMOTE_DTSS )
            {
                }
            else if ( line[ 2 ] == FNC_KILL_REMOTE_IDODS )
            {
                }
            else
            {
                // Forward from DTS to PBX
                //
                line[ 0 ] = 0; // Channel #0 == f_DS i.e. PBX
                write( f_DS, line, len );
                }
            }
            
        ///////////////////////////////////////////////////////////////////////
        // UDP Channel from VR application
        ///////////////////////////////////////////////////////////////////////
        //
        if ( inUdpSocket >= 0 && FD_ISSET( inUdpSocket, &readfds ) )
        {
            memset( line, sizeof(line), 0 );
	        sockaddr_in from;
	        socklen_t fromlen = sizeof( from );
            recvfrom( inUdpSocket, (char*)line, sizeof( line ), 0,
			    (sockaddr*) &from, &fromlen );

            char qDN[ 256 ] = {0};
            char qAddr[ 256 ] = {0};
            unsigned int qPort = 0;
            int parm_count = sscanf( (char*)line, "THREATKEY: %s SRC=%[^:]:%u", qDN, qAddr, &qPort );

            if ( parm_count <= 1 )
            {
                strcpy( qAddr, inet_ntoa( from.sin_addr ) );
                qPort = int( ntohs( from.sin_port ) );
                }

            if ( parm_count >= 1 )
            {
                if ( strcmp( DN, qDN ) == 0 ) // requested DN is our DN
                {
                    if ( (
                        ( ( is_VAD_active || IsOWS ) && is_voice_detected ) 
                        || 
                        ( ! ( is_VAD_active || IsOWS ) && transmission_order > 0  )
                        ) 
                    )
                    {
                        if ( startrec_key >= 0 && ! manually_started_recording ) // Start also recording!
                        {
                            printf( "%s: SYS: ---- %s" EOL, timestamp, "StartRecReq" );
                            manually_started_recording = true;
                            manually_stop_recording = false;
                            FlashLED1( startrec_key ); // Indicate (on DTS) activated recording

                            if ( fd_Wave < 0 )
                                BeginRecording ();
                            }
                
                        if ( mkdir( "../bomb", 0775 ) == 0 ) // Create folder
                            chown( "../bomb", UID_VOICE, GID_VOICE );

                        char newpath[ 256 ]; 
                        sprintf( newpath, "../bomb/%s", filename + 11 ); // skip cur_path prefix from filepath

                        getTimestamp ();

                        struct stat stat_buf;
                        if ( stat( newpath, &stat_buf ) == 0 ) // link to voice file already exists
                        {
                            printf( "%s: SYS: ---- %-19s UDP [%s:%u]" EOL, timestamp, 
                                "ThreatContinued", qAddr, qPort ); fflush( stdout );
                            }
                        else // create link
                        {
                            printf( "%s: SYS: ---- %-19s %s, UDP [%s:%u]" EOL, timestamp, 
                                "ThreatActivated", filename, qAddr, qPort ); fflush( stdout );

                            link( filename, newpath );

                            FlashLED2( bomb_key ); // Indicate (on DTS) activated bomb-threat
                            }

                        // Acknowledge packet: recording
                        sockaddr_in remote;
                        remote.sin_family = AF_INET;
                        remote.sin_addr.s_addr = inet_addr( qAddr );
                        remote.sin_port = htons( qPort );

                        sendto( inUdpSocket, "THREATACK: 1", 12, 0, 
                                (sockaddr*)&remote, sizeof( remote ) );
                        }
                    else
                    {
                        printf( "%s: SYS: ---- %-19s UDP [%s:%u]" EOL, timestamp, 
                            "ThreatOutOfCall", qAddr, qPort ); fflush( stdout );

                        // Acknowledge packet: not recording
                        sockaddr_in remote;
                        remote.sin_family = AF_INET;
                        remote.sin_addr.s_addr = inet_addr( qAddr );
                        remote.sin_port = htons( qPort );

                        sendto( inUdpSocket, "THREATACK: 0", 12, 0, 
                                (sockaddr*)&remote, sizeof( remote ) );
                        }
                    }
                else if ( qDN[ 0 ] ) // DN is not empty and not ours; Forward message to next port...
                {
                    fprintf( stderr, "idodVR: %d: UDP: (%s) Fwd to localhost:%u\n", portID, line, int( localUdpPort + 1 ) );

                    sockaddr_in remote;
                    remote.sin_family = AF_INET;
                    remote.sin_addr.s_addr = inet_addr( "127.0.0.1" );
                    remote.sin_port = htons( localUdpPort + 1 );

                    char msg[ 256 ];
                    sprintf( msg, "THREATKEY: %s SRC=%s:%u", qDN, qAddr, qPort );

                    sendto( inUdpSocket, msg, strlen(msg)+1, 0, 
                            (sockaddr*)&remote, sizeof( remote ) );
                    }
                }
            }
        }

    if ( inUdpSocket >= 0 )
        close( inUdpSocket );

    printf( "%s: <EOF>" EOL, dspboard );
    return 0;
    }

