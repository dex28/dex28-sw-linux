#ifndef _ALBA_DB_H_INCLUDED
#define _ALBA_DB_H_INCLUDED

#define ALBA_PRODUCT_DIR "/etc/sysconfig/albatross"
#define GW_DB_NAME "gw-ports.cfg"
#define EXT_DB_NAME "ext-ports.cfg"
#define USERS_DB_NAME "users.cfg"

////////////////////////////////////////////////////////////////////////
// Gateway's Ports Table

#define GW_PORTS_TABLE "GWPORTS"

static dbTable_t gatewayPortTable = 
{
	GW_PORTS_TABLE,
    0, // Length; Until EOT
    (dbCol_t[])
    {
        { "PORT",          T_STRING, 0     }, // Port Identifier; KEY
        { "DN",            T_STRING, (int)"" }, // Directory Number
        { "LOCAL_UDP",     T_INT,    15000 }, // Local RTP UDP Port
        { "USE_DEFAULTS",  T_INT,    1     }, // Use default settings instead specified
        { "DSCP_UDP",      T_INT,    0     }, // Diffserv Code Point for UDP packets
        { "DSCP_TCP",      T_INT,    0     }, // Diffserv Code Point for TCP packets
        { "CODEC",         T_INT,    0     }, // CODEC used for encoding voice farmes
        { "JIT_MINLEN",    T_INT,    20    }, // Minimum Jitter Buffer Length
        { "JIT_MAXLEN",    T_INT,    160   }, // Maximum Jitter Buffer Length
        { "JIT_ALPHA",     T_INT,    4     }, // Exponential Average Smoothness Coefficient
        { "JIT_BETA",      T_INT,    4     }, // Variance Multiplier Coefficient
        { "LEC",           T_INT,    1     }, // Line Echo Canceller Enable
        { "LEC_LEN",       T_INT,    128   }, // Maximum Echo Tail Length
        { "LEC_INIT",      T_INT,    0     }, // Keep State Between Calls (LEC Warm Start)
        { "LEC_MINERL",    T_INT,    6     }, // Minimum Echo Return Loss (ERL)
        { "NFE_MINNFL",    T_INT,    -60   }, // Minimum Noise Floor Level
        { "NFE_MAXNFL",    T_INT,    -35   }, // Maximum Noise Floor Level
        { "NLP",           T_INT,    1     }, // Non-Linear Processor Enable
        { "NLP_MINVPL",    T_INT,    0     }, // Minimum Voice Power Level for NLP
        { "NLP_HANGT",     T_INT,    100   }, // NLP Hangover Time
        { "FMTD",          T_INT,    1     }, // Fax/Modem Tone Detection (FMTD)
        { "PKT_COMPRTP",   T_INT,    1     }, // Compressed RTP Enable
        { "PKT_ISRBLKSZ",  T_INT,    0     }, // ISR Block Size
        { "PKT_BLKSZ",     T_INT,    40    }, // EC Block Size
        { "PKT_SIZE",      T_INT,    160   }, // Packet Payload Size
        { "GEN_P0DB",      T_INT,    2017  }, // Reference Level for 0dBm
        { "BCH_LOOP",      T_INT,    0     }, // B-Channel loopback
        { "BOMB_KEY",      T_INT,    -1    }, // Bomb-threat Key
        { "NAS_URL1",      T_STRING, (int)"" }, // NAS FTP URL1
        { "NAS_URL2",      T_STRING, (int)"" }, // NAS FTP URL2
        { "STARTREC_KEY",  T_INT,    -1    }, // Manual start recording key
        { "STOPREC_KEY",   T_INT,    -1    }, // Manual stop recording key
        { "SPLIT_FSIZE",   T_INT,    0     }, // Max recorder file size (in seconds)
        { "VRVAD_ACTIVE",  T_INT,    0     }, // VR use Voice Activiti Detection (VAD) flag
        { "VRVAD_THRSH",   T_INT,    50    }, // VR VAD noise threshold (pcm exp avg level)
        { "VRVAD_HANGT",   T_INT,    5000  }, // VR VAD hangover time (in milliseconds)
        { "VR_MINFSIZE",   T_INT,    1100  }, // VR minimum file size (in milliseconds)
        { "TX_AMP",        T_INT,    0     }, // Transmit voice amplification
        { NULL,            0,        0     }  // EOT
    },
	0,
	NULL
};

typedef struct GW_PORT
{
    char_t* PORT;
    char_t* DN;
    int     LOCAL_UDP;
    int     USE_DEFAULTS;
    int     DSCP_UDP;
    int     DSCP_TCP;
    int     CODEC;
    int     JIT_MINLEN;
    int     JIT_MAXLEN;
    int     JIT_ALPHA;
    int     JIT_BETA;
    int     LEC;
    int     LEC_LEN;
    int     LEC_INIT;
    int     LEC_MINERL;
    int     NFE_MINNFL;
    int     NFE_MAXNFL;
    int     NLP;
    int     NLP_MINVPL;
    int     NLP_HANGT;
    int     FMTD;
    int     PKT_COMPRTP;
    int     PKT_ISRBLKSZ;
    int     PKT_BLKSZ;
    int     PKT_SIZE;
    int     GEN_P0DB;
    int     BCH_LOOP;
    int     BOMB_KEY;
    char_t* NAS_URL1;
    char_t* NAS_URL2;
    int     STARTREC_KEY;
    int     STOPREC_KEY;
    int     SPLIT_FSIZE;
    int     VRVAD_ACTIVE;
    int     VRVAD_THRSH;
    int     VRVAD_HANGT;
    int     VR_MINFSIZE;
    int     TX_AMP;
} GW_PORT;

////////////////////////////////////////////////////////////////////////
// Extender's Ports Table

#define EXT_PORTS_TABLE "EXTPORTS"

static dbTable_t extenderPortTable = 
{
	EXT_PORTS_TABLE,
    0, // Length; Until EOT
    (dbCol_t[])
    {
        { "PORT",          T_STRING, 0       }, // KEY
        { "DN",            T_STRING, (int)"" },
        { "IPADDR",        T_STRING, (int)"" },
        { "TCPPORT",       T_INT,    0       },
        { "UID",           T_STRING, (int)"" },
        { "PWD",           T_STRING, (int)"" },
        { "AUTH",          T_INT,    -1      },
        { "AUTOCONNECT",   T_INT,    0       },
        { "RECONNECT",     T_INT,    0       },
        { "TAUD",          T_STRING, (int)"" },
        { "RETRYC",        T_INT,    5,      },
        { NULL,            0,        0       } // EOT
    },
	0,
	NULL
};

typedef struct EXT_PORT
{
    char_t* PORT;
    char_t* DN;
    char_t* IPADDR;
    int     TCPPORT;
    char_t* UID;
    char_t* PWD;
    int     AUTH;
    int     AUTOCONNECT;
    int     RECONNECT;
    char_t* TAUD;
    int     RETRYC;
} EXT_PORT;

////////////////////////////////////////////////////////////////////////
// User Table

#define USERS_TABLE "USERS"

static dbTable_t userTable = 
{
	USERS_TABLE,
    0, // Length; Until EOT
    (dbCol_t[])
    {
        { "NAME",     T_STRING, 0       }, // KEY
        { "PASS",     T_STRING, (int)"" },
        { "GROUP",    T_STRING, (int)"" },
        { "DISABLED", T_INT,    1       },
        { NULL,       0,        0       } // EOT
    },
	0,
	NULL
};

typedef struct USER
{
    char_t* NAME;
    char_t* PASS;
    char_t* GROUP;
    int     DISABLED;
} USER;

////////////////////////////////////////////////////////////////////////
// User's Authorizations Table

#define USERAUTHS_TABLE "USERAUTHS"

static dbTable_t userAuthTable = 
{
	USERAUTHS_TABLE,
    0, // Length; Until EOT
    (dbCol_t[])
    {
        { "USERNAME", T_STRING, 0          }, // KEY
        { "DN",       T_STRING, (int)"any" },
        { "MODE",     T_INT,    0          },
        { NULL,       0,        0          } // EOT
    },
	0,
	NULL
};


typedef struct USERAUTH
{
    char_t* USERNAME;
    char_t* DN;
    int     MODE;
} USERAUTH;

#endif // _ALBA_DB_H_INCLUDED
