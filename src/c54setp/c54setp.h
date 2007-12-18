#ifndef _C54SETP_H_INCLUDED
#define _C54SETP_H_INCLUDED

class DSPCFG
{
    char lockname[ 128 ];

public:

    char devname[ 64 ];
    int  channel;

    DSPCFG( void )
    {
        channel = -1;
        strcpy( devname, "" );
        strcpy( lockname, "" );
        }

    bool Find( int abs_channel_no, char* dsp_cfg )
    {
        channel = -1; // -1 means "invalid channel"
        strcpy( devname, "" );
        strcpy( lockname, "" );

        FILE* f = fopen( dsp_cfg, "r" );
        if ( ! f )
        {
            fprintf( stderr, "Failed to open configuration file.\n" );
            return -1;
            }

        for( int i = 0; i < 4; i++ ) // Max 4 DSPs !
        {
            char line[ 256 ];
            if ( ! fgets( line, sizeof( line ), f ) )
                break;

            char coffname[ 128 ];
            int channel_count;

            if ( 4 != sscanf( line, " %s %s %d %s", devname, coffname, &channel_count, lockname ) )
                continue;

            if ( abs_channel_no < channel_count ) // Found available channel
            {
                channel = abs_channel_no;
                break;
                }

            abs_channel_no -= channel_count; // skip channels on this DSP
            }

        fclose( f );

        return channel >= 0; // channel not found if still -1
        }

    bool isRunning( void )
    {
        if ( channel < 0 )
            return false;

        struct stat info;
        return 0 == stat( lockname, &info );
        }

    bool waitToBeRunning( void )
    {
        if ( channel < 0 )
            return false;

        while( ! isRunning () )
            usleep( 300000 );

        return true;
        }
    };

struct PORT_PARAMETERS
{
    char PORT[ 32 ];
    char DN[ 32 ];
    int LOCAL_UDP;
    int USE_DEFAULTS;
    int DSCP_UDP;
    int DSCP_TCP;
    int CODEC;
    int JIT_MINLEN;
    int JIT_MAXLEN;
    int JIT_ALPHA;
    int JIT_BETA;
    int LEC;
    int LEC_LEN;
    int LEC_INIT;
    int LEC_MINERL;
    int NFE_MINNFL;
    int NFE_MAXNFL;
    int NLP;
    int NLP_MINVPL;
    int NLP_HANGT;
    int FMTD;
    int PKT_COMPRTP;
    int PKT_ISRBLKSZ;
    int PKT_BLKSZ;
    int PKT_SIZE;
    int GEN_P0DB;
    int BCH_LOOP;
    int BOMB_KEY;
    char NAS_URL1[ 128 ];
    char NAS_URL2[ 128 ];
    int STARTREC_KEY;
    int STOPREC_KEY;
    int SPLIT_FSIZE;
    int VRVAD_ACTIVE;
    int VRVAD_THRSH;
    int VRVAD_HANGT;
    int VR_MINFSIZE;
    int TX_AMP;

    static bool parse_AV_pair( const char* line, const char* attr, int& value )
    {
        while ( *line == *attr )
        { 
            line++; attr++; 
            }

        if ( *attr != 0 && *line != '=' )
            return false;

        return 1 == sscanf( line + 1, "%d", &value );
        }

    static bool parse_AV_pair( const char* line, const char* attr, char* value )
    {
        while ( *line == *attr )
        { 
            line++; attr++; 
            }

        if ( *attr != 0 && *line != '=' )
            return false;

        return 1 == sscanf( line + 1, " %[^\n]", value );
        }

    PORT_PARAMETERS( void )
    {
        memset( this, 0, sizeof( *this ) );

        // PORT      = ""
        // DN        = ""
        LOCAL_UDP    = 15000;
        USE_DEFAULTS = 1;
        DSCP_UDP     = 0;
        DSCP_TCP     = 0;
        CODEC        = 0;
        JIT_MINLEN   = 20;
        JIT_MAXLEN   = 160;
        JIT_ALPHA    = 4;
        JIT_BETA     = 4;
        LEC          = 1;
        LEC_LEN      = 128;
        LEC_INIT     = 0;
        LEC_MINERL   = 6;
        NFE_MINNFL   = -60;
        NFE_MAXNFL   = -35;
        NLP          = 1;
        NLP_MINVPL   = 0;
        NLP_HANGT    = 100;
        FMTD         = 1;
        PKT_COMPRTP  = 1;
        PKT_ISRBLKSZ = 0;
        PKT_BLKSZ    = 40;
        PKT_SIZE     = 160;
        GEN_P0DB     = 2017;
        BCH_LOOP     = 0;
        BOMB_KEY     = -1;
        // NAS_URL1  = ""
        // NAS_URL2  = ""
        STARTREC_KEY = -1;
        STOPREC_KEY  = -1;
        SPLIT_FSIZE  = 0;
        VRVAD_ACTIVE = 0;
        VRVAD_THRSH  = 50;
        VRVAD_HANGT  = 5000;
        VR_MINFSIZE  = 1100;
        TX_AMP       = 0;
        }

    void Dump( void )
    {
        printf( "LOCAL_UDP    = %d\n", LOCAL_UDP );
        printf( "USE_DEFAULTS = %d\n", USE_DEFAULTS );
        printf( "DSCP_UDP     = %d\n", DSCP_UDP );
        printf( "DSCP_TCP     = %d\n", DSCP_TCP );
        printf( "CODEC        = %d\n", CODEC );
        printf( "JIT_MINLEN   = %d\n", JIT_MINLEN );
        printf( "JIT_MAXLEN   = %d\n", JIT_MAXLEN );
        printf( "JIT_ALPHA    = %d\n", JIT_ALPHA );
        printf( "JIT_BETA     = %d\n", JIT_BETA );
        printf( "LEC          = %d\n", LEC );
        printf( "LEC_LEN      = %d\n", LEC_LEN );
        printf( "LEC_INIT     = %d\n", LEC_INIT );
        printf( "LEC_MINERL   = %d\n", LEC_MINERL );
        printf( "NFE_MINNFL   = %d\n", NFE_MINNFL );
        printf( "NFE_MAXNFL   = %d\n", NFE_MAXNFL );
        printf( "NLP          = %d\n", NLP );
        printf( "NLP_MINVPL   = %d\n", NLP_MINVPL );
        printf( "NLP_HANGT    = %d\n", NLP_HANGT );
        printf( "FMTD         = %d\n", FMTD );
        printf( "PKT_COMPRTP  = %d\n", PKT_COMPRTP );
        printf( "PKT_ISRBLKSZ = %d\n", PKT_ISRBLKSZ );
        printf( "PKT_BLKSZ    = %d\n", PKT_BLKSZ );
        printf( "PKT_SIZE     = %d\n", PKT_SIZE );
        printf( "GEN_P0DB     = %d\n", GEN_P0DB );
        printf( "BCH_LOOP     = %d\n", BCH_LOOP );
        printf( "BOMB_KEY     = %d\n", BOMB_KEY );
        printf( "NAS_URL1     = %s\n", NAS_URL1 );
        printf( "NAS_URL2     = %s\n", NAS_URL2 );
        printf( "STARTREC_KEY = %d\n", STARTREC_KEY );
        printf( "STOPREC_KEY  = %d\n", STOPREC_KEY );
        printf( "SPLIT_FSIZE  = %d\n", SPLIT_FSIZE );
        printf( "VRVAD_ACTIVE = %d\n", VRVAD_ACTIVE );
        printf( "VRVAD_THRSH  = %d\n", VRVAD_THRSH );
        printf( "VRVAD_HANGT  = %d\n", VRVAD_HANGT );
        printf( "VR_MINFSIZE  = %d\n", VR_MINFSIZE );
        printf( "TX_AMP       = %d\n", TX_AMP );
        }

    bool Load( const char* ID, bool DN_search = false )
    {
        FILE* f = fopen( "/etc/sysconfig/albatross/gw-ports.cfg", "r" );
        if ( ! f )
        {
            fprintf( stderr, "Cannot open gw-ports.cfg\n" );
            return false;
            }

        char line[ 256 ];
        while( fgets( line, sizeof( line ), f ) )
        {
            // fprintf( stderr, "Searching [%s=%s] %s", DN_search ? "DN" : "PORT", ID, line );

            if ( DN_search )
            {
                if ( parse_AV_pair( line, "PORT", PORT ) ) 
                    continue;

                if ( ! parse_AV_pair( line, "DN", DN ) )
                    continue;

                if ( strcmp( DN, ID ) != 0 )
                    continue;
                }
            else // PORT search
            {
                if ( ! parse_AV_pair( line, "PORT", PORT ) )
                    continue;

                if ( strcmp( PORT, ID ) != 0 )
                    continue;
                }

            // fprintf( stderr, "Found [%s=%s]\n", DN_search ? "DN" : "PORT", ID );

            while( fgets( line, sizeof( line ), f ) )
            {
                int row;
                if ( parse_AV_pair( line, "ROW", row ) )
                    break;

                if ( parse_AV_pair( line, "PORT",         PORT ) ) continue;
                if ( parse_AV_pair( line, "DN",           DN ) ) continue;
                if ( parse_AV_pair( line, "LOCAL_UDP",    LOCAL_UDP ) ) continue;
                if ( parse_AV_pair( line, "USE_DEFAULTS", USE_DEFAULTS ) ) continue;
                if ( parse_AV_pair( line, "DSCP_UDP",     DSCP_UDP ) ) continue;
                if ( parse_AV_pair( line, "DSCP_TCP",     DSCP_TCP ) ) continue;
                if ( parse_AV_pair( line, "CODEC",        CODEC ) ) continue;
                if ( parse_AV_pair( line, "JIT_MINLEN",   JIT_MINLEN ) ) continue;
                if ( parse_AV_pair( line, "JIT_MAXLEN",   JIT_MAXLEN ) ) continue;
                if ( parse_AV_pair( line, "JIT_ALPHA",    JIT_ALPHA ) ) continue;
                if ( parse_AV_pair( line, "JIT_BETA",     JIT_BETA ) ) continue;
                if ( parse_AV_pair( line, "LEC",          LEC ) ) continue;
                if ( parse_AV_pair( line, "LEC_LEN",      LEC_LEN ) ) continue;
                if ( parse_AV_pair( line, "LEC_INIT",     LEC_INIT ) ) continue;
                if ( parse_AV_pair( line, "LEC_MINERL",   LEC_MINERL ) ) continue;
                if ( parse_AV_pair( line, "NFE_MINNFL",   NFE_MINNFL ) ) continue;
                if ( parse_AV_pair( line, "NFE_MAXNFL",   NFE_MAXNFL ) ) continue;
                if ( parse_AV_pair( line, "NLP",          NLP ) ) continue;
                if ( parse_AV_pair( line, "NLP_MINVPL",   NLP_MINVPL ) ) continue;
                if ( parse_AV_pair( line, "NLP_HANGT",    NLP_HANGT ) ) continue;
                if ( parse_AV_pair( line, "FMTD",         FMTD ) ) continue;
                if ( parse_AV_pair( line, "PKT_COMPRTP",  PKT_COMPRTP ) ) continue;
                if ( parse_AV_pair( line, "PKT_ISRBLKSZ", PKT_ISRBLKSZ ) ) continue;
                if ( parse_AV_pair( line, "PKT_BLKSZ",    PKT_BLKSZ ) ) continue;
                if ( parse_AV_pair( line, "PKT_SIZE",     PKT_SIZE ) ) continue;
                if ( parse_AV_pair( line, "GEN_P0DB",     GEN_P0DB ) ) continue;
                if ( parse_AV_pair( line, "BCH_LOOP",     BCH_LOOP ) ) continue;
                if ( parse_AV_pair( line, "BOMB_KEY",     BOMB_KEY ) ) continue;
                if ( parse_AV_pair( line, "NAS_URL1",     NAS_URL1 ) ) continue;
                if ( parse_AV_pair( line, "NAS_URL2",     NAS_URL2 ) ) continue;
                if ( parse_AV_pair( line, "STARTREC_KEY", STARTREC_KEY ) ) continue;
                if ( parse_AV_pair( line, "STOPREC_KEY",  STOPREC_KEY ) ) continue;
                if ( parse_AV_pair( line, "SPLIT_FSIZE",  SPLIT_FSIZE ) ) continue;
                if ( parse_AV_pair( line, "VRVAD_ACTIVE", VRVAD_ACTIVE ) ) continue;
                if ( parse_AV_pair( line, "VRVAD_THRSH",  VRVAD_THRSH ) ) continue;
                if ( parse_AV_pair( line, "VRVAD_HANGT",  VRVAD_HANGT ) ) continue;
                if ( parse_AV_pair( line, "VR_MINFSIZE",  VR_MINFSIZE ) ) continue;
                if ( parse_AV_pair( line, "TX_AMP",       TX_AMP ) ) continue;
                }

            fclose( f );
            return true;
            }

        fclose( f );
        return false;
        }

    bool SaveTo( char* dspboard, int channel, int warm_restart );

    static void GetConnParams( int id, unsigned short& udp_port, int& udp_DSCP, int& tcp_DSCP )
    {
        udp_port = 15000 + id;
        udp_DSCP = 0;
        tcp_DSCP = 0;

        char port[ 16 ];
        sprintf( port, "%d", id + 1 );
        PORT_PARAMETERS portParams;
        if ( ! portParams.Load( port ) || portParams.USE_DEFAULTS )
        {
            if ( ! portParams.Load( "default" ) )
                return;

            portParams.LOCAL_UDP += id;
            }

        udp_port = portParams.LOCAL_UDP;
        udp_DSCP = portParams.DSCP_UDP;
        tcp_DSCP = portParams.DSCP_TCP;
        }

    static void GetVR( int id, char* DN, int& bomb_key, int& startrec_key, int& stoprec_key, long& split_filesize, char* NAS_URL1, char* NAS_URL2, bool& is_VAD_active, int& vad_threshold, long& vad_hangovertime, long& vr_minfsize )
    {
        DN[0] = 0;
        bomb_key = -1;
        NAS_URL1[0] = 0;
        NAS_URL2[0] = 0;

        char port[ 16 ];
        sprintf( port, "%d", id + 1 );
        PORT_PARAMETERS portParams;
        if ( ! portParams.Load( port ) )
        {
            if ( ! portParams.Load( "default" ) )
                return;
            }
        else
        {
            strcpy( DN, portParams.DN );

            if ( portParams.USE_DEFAULTS )
            {
                if ( ! portParams.Load( "default" ) )
                    return;
                }
            }


        bomb_key = portParams.BOMB_KEY;
        strcpy( NAS_URL1, portParams.NAS_URL1 );
        strcpy( NAS_URL2, portParams.NAS_URL2 );
        startrec_key = portParams.STARTREC_KEY;
        stoprec_key = portParams.STOPREC_KEY;
        split_filesize = portParams.SPLIT_FSIZE;
        is_VAD_active = portParams.VRVAD_ACTIVE;
        vad_threshold = portParams.VRVAD_THRSH;
        vad_hangovertime = portParams.VRVAD_HANGT;
        vr_minfsize = portParams.VR_MINFSIZE;
        }
    };

struct CONN_PARAMETERS
{
    char DN[ 32 ];
    char IPADDR[ 128 ];
    int  TCPPORT;
    char UID[ 32 ];
    char PWD[ 32 ];
    int  AUTH;
    int  AUTOCONNECT;
    int  RECONNECT;
    char TAUD[ 32 ];
    int  RETRYC;

    static bool parse_AV_pair( const char* line, const char* attr, int& value )
    {
        while ( *line == *attr )
        { 
            line++; attr++; 
            }

        if ( *attr != 0 && *line != '=' )
            return false;

        return 1 == sscanf( line + 1, "%d", &value );
        }

    static bool parse_AV_pair( const char* line, const char* attr, char* value )
    {
        while ( *line == *attr )
        { 
            line++; attr++; 
            }

        if ( *attr != 0 && *line != '=' )
            return false;

        return 1 == sscanf( line + 1, " %[^\n]", value );
        }

    void Clear( void )
    {
        memset( this, 0, sizeof( *this ) );

        // DN          = ""
        // IPADDR      = ""
        TCPPORT        = 0;
        // UID         = ""
        // PWD         = ""
        AUTH           = -1;
        AUTOCONNECT    = false;
        RECONNECT      = false;
        // TAUD        = ""
        RETRYC         = 5;
        }

    CONN_PARAMETERS( void )
    {
        Clear ();
        }

    void Dump( void )
    {
        printf( "DN           = %s\n", DN );
        printf( "IPADDR       = %s\n", IPADDR );
        printf( "TCPPORT      = %d\n", TCPPORT );
        printf( "UID          = %s\n", UID );
        printf( "PWD          = %s\n", PWD );
        printf( "AUTH         = %d\n", AUTH );
        printf( "AUTOCONNECT  = %d\n", AUTOCONNECT );
        printf( "RECONNECT    = %d\n", RECONNECT );
        printf( "TAUD         = %s\n", TAUD );
        printf( "RETRYC       = %d\n", RETRYC );
        }

    bool Load( const char* ID )
    {
        Clear ();

        FILE* f = fopen( "/etc/sysconfig/albatross/ext-ports.cfg", "r" );
        if ( ! f )
        {
            fprintf( stderr, "Cannot open ext-ports.cfg\n" );
            return false;
            }

        char line[ 256 ];
        while( fgets( line, sizeof( line ), f ) )
        {
            char port[ 32 ];
            if ( ! parse_AV_pair( line, "PORT", port ) )
                continue;

            if ( strcmp( port, ID ) != 0 )
                continue;

            while( fgets( line, sizeof( line ), f ) )
            {
                int row;
                if ( parse_AV_pair( line, "ROW", row ) )
                    break;

                if ( parse_AV_pair( line, "DN",           DN ) ) continue;
                if ( parse_AV_pair( line, "IPADDR",       IPADDR ) ) continue;
                if ( parse_AV_pair( line, "TCPPORT",      TCPPORT ) ) continue;
                if ( parse_AV_pair( line, "UID",          UID ) ) continue;
                if ( parse_AV_pair( line, "PWD",          PWD ) ) continue;
                if ( parse_AV_pair( line, "AUTH",         AUTH ) ) continue;
                if ( parse_AV_pair( line, "AUTOCONNECT",  AUTOCONNECT ) ) continue;
                if ( parse_AV_pair( line, "RECONNECT",    RECONNECT ) ) continue;
                if ( parse_AV_pair( line, "TAUD",         TAUD ) ) continue;
                if ( parse_AV_pair( line, "RETRYC",       RETRYC ) ) continue;
                }

            fclose( f );
            return true;
            }

        fclose( f );
        return false;
        }
   
    bool find( int id )
    {
        CONN_PARAMETERS defaults;
        if ( ! defaults.Load( "default" ) )
            return false;

        char port[ 16 ];
        sprintf( port, "%d", id + 1 );
        if ( ! Load( port ) )
            return false;

        if ( ! DN[ 0 ] ) strcpy( DN, defaults.DN );
        if ( ! IPADDR[ 0 ] ) strcpy( IPADDR, defaults.IPADDR );
        if ( TCPPORT <= 0 ) TCPPORT = defaults.TCPPORT;
        if ( ! UID[ 0 ] ) strcpy( UID, defaults.UID );
        if ( ! PWD[ 0 ] ) strcpy( PWD, defaults.PWD );
        if ( AUTH < 0 ) AUTH = defaults.AUTH;
        // Note: no defaults for AUTOCONNECT, RECONNECT, TAUD and RETRYC !

        return true;
        }
    };

#endif // _C54SETP_H_INCLUDED
