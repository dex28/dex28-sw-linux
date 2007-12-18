#ifndef _DTS_H_INCLUDED
#define _DTS_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <sys/time.h>
#include <signal.h>

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

#include <netdb.h> // hosten

#include <termios.h> // TAU_D termio

#include "MD5.h"

using namespace std;

#include "ELUFNC.h"
#include "../c54setp/c54setp.h"

///////////////////////////////////////////////////////////////////////////////

class MonotonicTimer
{
    timespec start;

    public: MonotonicTimer ()
    {
        Restart ();
        }

    public: void Restart ()
    {
        clock_gettime( CLOCK_MONOTONIC, &start );
        }

    public: long Elapsed ()
    {
        timespec end;
        clock_gettime( CLOCK_MONOTONIC, &end );

        long seconds  = end.tv_sec  - start.tv_sec; 
        long nseconds = end.tv_nsec - start.tv_nsec; 
     
        return long( ( seconds * 1e3 + nseconds / 1e6 ) + 0.5 ); 
        }
    };

///////////////////////////////////////////////////////////////////////////////

class TAU_D
{
    int fd;

    struct sockaddr_in cli_addr;
    int sockfd;

    int mode; // Contains both status & mode

/*
    DTE <-> TAU frame format:

    +---+---+---+---+---+---+---+---+
    | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    +---+---+---+---+---+---+---+---+
    | 0   0   0   1   0   1   0   1 |  FRM_FLG1  (0x15)
    +---+---+---+---+---+---+---+---+
    | 0   0   0   1   0   1   0   1 |  FRM_FLG2  (0x15)
    +---+---+---+---+---+---+---+---+
    | x   x   x   x   x   x   x   x |  FRM_BC    (4-51)
    +---+---+---+---+---+---+---+---+
    |      SN       |     TYPE      |  FRM_CTL
    +---+---+---+---+---+---+---+---+
    | 0   0   0   0   0   0   0   x |  FRM_ADDR  (0 or 1)
    +---+---+---+---+---+---+---+---+
    | x   x   x   x   x   x   x   x |  FRM_DATA  (optional)
    +---+---+---+---+---+---+---+---+
    | x   x   x   x   x   x   x   x |  FRM_CS
    +---+---+---+---+---+---+---+---+

    BC: byte count (including FRM_BC and FRM_CS, excluding FRM_FLG*)

    CS: checksum ( (sum - 1) mod 255, including BC, CTL, ADDR & DATA)

    SN: sequence number mod 16 (0..15)

    TYPE:

        Bit 3:    0 = Request, 1 = Response
        Bit 2..0: Frame type

        DATA          0 0 0 0    0x00
        DATA_ACK      1 0 0 0    0x08
        STATUS_REQ    0 0 0 1    0x01
        STATUS_RESP   1 0 0 1    0x09
        ID_REQ        0 0 1 0    0x02
        ID_RESP       1 0 1 0    0x0A
        COMMAND       0 0 1 1    0x03
        COMMAND_ACK   1 0 1 1    0x0B
        TEST_REQ      0 1 1 1    0x07
        TEST_REPORT   1 1 1 1    0x0F
*/

public:

    enum // TAU D status & modes
    {
        STATUS_PBX_CONNECTED = 0x80,
        STATUS_DTS_CONNECTED = 0x40,
        MODE_MASK            = 0x1C,
        MODE_PRGFNCREL       = 0x10,
        MODE_MULTIMEDIA      = 0x08,
        MODE_PC_CONTROL      = 0x04,
        MODE_TRANSPARENT     = 0x00,
        DATA_ACK_ENABLE      = 0x02,
        DTS_TO_DTE_ENABLE    = 0x01
        };
/*
    TAU D Modes:
    ===========

    PRGFNCREL mode: 1x0
    
        Signals from PBX are copied to DTE.
        Signals from DTS depend on DTS_TO_DTE_ENABLE bit.
        When the DTE has sent PRGFNCACT signal with key 0x0B to PBX,
        all PRGFNCREL2 signals from DTS are filtered towards exchange
        until a PRGFNCREL2 is sent from DTE.

    MULTIMEDA mode: x10

        Signals from PBX are copied to DTE.
        Signals from DTS depend on DTS_TO_DTE_ENABLE bit.
        Multimedia bit in signal EQUSTA towards PBX is set.
        Signals with FNC 0xA0-0xA6 are filtered towards DTS.

    PC CONTROL mode: 001

        Signals from DTS and PBX are sent to DTE regardless of
        DTS_TO_DTE_ENABLE bit.
        DTS->PBX and PBX->DTS signals are intercepted by the DTE
        and not forwared.

    TRANSPARENT mode: x00

        Signals from exchange are copied to DTE.
        Signals from DTS depend on DTS_TO_DTE_ENABLE bit.
*/

    enum // Address Field values
    {
        ADDR_PBX        = 0x00,
        ADDR_DTS        = 0x01
        };

private:

    enum // Frame Header Flags
    {
        FRM_FLG1            = 0x15,
        FRM_FLG2            = 0x15,
        };

    enum // Frame Control
    {
        // Bits 7..4: Sequence Number
        //
        FRM_SN_SHIFT        = 4,
        FRM_SN_MASK         = 0x0F,

        // Bits 3..0: Frame Type
        //
        FRM_CTL_MASK        = 0x0F,
        FRM_CTL_DATA        = 0x00,
        FRM_CTL_DATA_ACK    = 0x08,
        FRM_CTL_STATUS_REQ  = 0x01,
        FRM_CTL_STATUS_RESP = 0x09,
        FRM_CTL_ID_REQ      = 0x02,
        FRM_CTL_ID_RESP     = 0x0A,
        FRM_CTL_COMMAND     = 0x03,
        FRM_CTL_COMMAND_ACK = 0x0B,
        FRM_CTL_TEST_REQ    = 0x07,
        FRM_CTL_TEST_REPORT = 0x0F,
        };

    // RECEIVER (to/from DTE) -------------------------------------------------

    enum STATE // Receiver state-machine
    {
        WAIT_FLG1    = 0,
        WAIT_FLG2    = 1,
        WAIT_BC      = 2,
        WAIT_CTL     = 3,
        WAIT_ADDR    = 4,
        WAIT_DATA    = 5,
        WAIT_CS      = 6
        };

    STATE state;
    int CTL;   // Frame control
    int ADDR;  // Frame address
    int CS;    // Frame checksum

    int data_len;
    unsigned char data[ 64 ];
    int data_p;

    int sn_from_DTS;
    int sn_from_PBX;

    // SENDER (to/from DTE)
    //
    int sn_to_DTE;
    int sn_from_DTE;

public:

    TAU_D( void );
    ~TAU_D( void );

    const char* VerboseState( void )
    {
        static const char* verb[] = {
            "WAIT FLG1", "WAIT FLG2", "WAIT BC", "WAIT CTL", 
            "WAIT ADDR", "WAIT_DATA", "WAIT_CS"
            };
        return verb[ state ];
        }

    int getSock( void ) const
    {
        return sockfd;
        }

    int getFD( void ) const
    {
        return fd;
        }

    int getDataLen( void ) const
    {
        return data_len;
        }

    unsigned char* getData( void )
    {
        return data;
        }

    int getAddr( void ) const
    {
        return ADDR;
        }

    int getMode( void ) const
    {
        return ( mode & MODE_MASK ) >> 2;
        }

    void SetMode( int new_mode )
    {
        mode &= ~0x3F;
        mode |= ( new_mode & 0x3F );
        }

    void Reset( void )
    {
        SetMode( MODE_TRANSPARENT );
        }

    void SetStatus_PBX_Connected( bool flag )
    {
        if ( flag ) mode |= STATUS_PBX_CONNECTED;
        else mode &= ~STATUS_PBX_CONNECTED;
        }

    void SetStatus_DTS_Connected( bool flag )
    {
        if ( flag ) mode |= STATUS_DTS_CONNECTED;
        else mode &= ~STATUS_DTS_CONNECTED;
        }

    void AbortFrame( void )
    {
        if ( state == WAIT_FLG1 )
            return;

        state = WAIT_FLG1;
        printf( "TAU_D: Aborting Frame while %s\n", VerboseState () );
        }

    bool OpenSerial( const char* port_name );
    bool OpenTCP( int portno );
    bool AcceptClient( void );
    void CloseConnection( void );

    void SendFrame( int ctl, int addr = 0, unsigned char* buf = NULL, int len = 0 );
    bool OnReceivedOctet( int octet ); // returns true when received valid data frame

    void SendDataFrame( int addr, unsigned char* buf, int len )
    {
        // buf[] contains ELU 2B+D signal without NBYTES and CS
        // This procedure adds NBYTES and generates valid CS, and
        // then sends signal to DTE
        //

        if ( addr == 1 && mode != MODE_PC_CONTROL && ! ( mode & DTS_TO_DTE_ENABLE ) )
            return; // Signals from DTS to DTE are copied only on request

        unsigned char pkt[ 64 ];
        pkt[ 0 ] = len + 2; // NBYTES
        memcpy( pkt + 1, buf, len );

        // Add local SN to signal's OPC.
        //
        // OPC format: 
        //  +---+---+---+---+---+---+---+---+
        //  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        //  +-----------+---+-----------+---+
        //  |    OPC    | x |    SN     |IND|
        //  +-----------+---+-----------+---+
        //
        pkt[ 1 ] &= ~0x0E; // Remove old SN (bitmask 00001110)
        //
        if ( addr == 0 )
            pkt[ 1 ] |= ( ( ++sn_from_PBX & 0x07 ) << 1 );
        else
            pkt[ 1 ] |= ( ( ++sn_from_DTS & 0x07 ) << 1 );
    
        // Generate CS for ELU 2b+d signal
        //
        int cs = 0;
        for ( int i = 0; i < len + 1; i++ )
            cs += pkt[ i ];
        pkt[ 1 + len ] = ( cs - 1 ) & 0xFF;

        SendFrame( FRM_CTL_DATA, addr, pkt, len + 2 );
        }
    };

///////////////////////////////////////////////////////////////////////////////

class DTS
{
    int tcpSocket;
    int D4oIPver; // 0xAABB: AA = major, BB = minor
    int inUdpSocket;
    sockaddr_in remoteUdp;
    unsigned short remoteUdpPort;
    unsigned short localUdpPort;
    bool autoReconnect;
    int autoReconnectCount;
    bool IsOWS;
    bool IsCTT10;

    int CTT10_userid_pdu_len;
    unsigned char CTT10_userid_pdu[ 32 ];

    bool disable4C;

    ///////////////////////////////////////////////////////////////////////////

    TAU_D tau;

    ///////////////////////////////////////////////////////////////////////////

    int ID;
    DSPCFG dspCfg;
    char dspboard[ 32 ];
    int channel;
    int f_D; // D channel device handle
    int f_B; // B channel device handle

    ///////////////////////////////////////////////////////////////////////////

    bool f_D_XON;

    ///////////////////////////////////////////////////////////////////////////

    enum STATE
    {
        DOWN,
        HALF_UP,
        PASSIVE,
        ACTIVE,
        FAULTY
        };

    volatile int timer;
    STATE state;
    bool statusRequested;

    ///////////////////////////////////////////////////////////////////////////
    // Equpiment Type

    enum EQU_TYPE
    {
        DTS_UNKNOWN      = -1,

        DTS_OPI_II_TYPE  = 0x0000, // OPI-II instrument
                                   // ----------------
        DTS_OPI_II       = 0x0000, // OPI-II console only 
        DTS_OPI_II_B     = 0x0001, // OPI-II with Braille unit 

        DTS_CTT10_TYPE   = 0x0100, // CTT10 Family
        DTS_CTT10        = 0x0101, // CTT10 - not logged on
        DTS_CTT10_ACTIVE = 0x0103, // CTT10 - active

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
        GRAPHICAL_DISPLAY
        };

    // Extra version information contains:
    // B0..B3: 0 = No extra information
    //         1 = DBC210 (Equipment version = DBC201)
    //         2 = DBC211, -212, -213, -214
    //         3 = DBC220, -222, -223, -224, -225

    unsigned char EQUSTA_pdu[ 12 ];
    int equType;

    DISPLAY_TYPE hasDisplay;
    bool reqRemap; // DBC203/213 display with additional fields in D3 basic mode
    bool D3_basic; // D3 basic or D3+ mode
    bool D4_mode; // D4 or non-D4 mode

    ///////////////////////////////////////////////////////////////////////////
    // DISPLAY
    //
    // Display fields: (Max 6 rows) x (Max 4 columns) x (10 characters)
    char field[ 6 ][ 4 ][ 10 ];
    int rowCount, colCount; // display dimensions in characters
    int curRow, curCol; // current row/column cursor position (in characters)

    // Previous saved state (used by Update())
    //
    char oldField[ 6 ][ 4 ][ 10 ];
    int oldCurRow, oldCurCol;

    CONN_PARAMETERS connParams;

    // User Input
    //
    char userInput[ 128 ];
    unsigned int userInputLen;

    // Info Title & Message
    //
    char infoTitle[ 128 ];
    char infoMessage[ 128 ];
    char infoAdvice[ 128 ];
    char localIpAddr[ 64 ];

    ///////////////////////////////////////////////////////////////////////////
    
    int TransformRowCol( int row, int col )
    {
        static const struct { int row, col; } MAP[ 3 ][ 4 ] =
        {
            { { 16, 0 }, { 0, 0 }, { 0, 1 }, { 16, 3 } },
            { {  1, 0 }, { 1, 1 }, { 2, 0 }, {  2, 1 } },
            { { 18, 0 }, { 3, 0 }, { 3, 1 }, { 18, 3 } }
            };

        // Note that rowCount must be 3 for D3 basic mode
        return D3_basic && reqRemap
            ? ( MAP[ row % 3 ][ col %= 4 ].col << 5 ) 
               + MAP[ row % 3 ][ col %= 4 ].row
            : ( col << 5 ) + row;
        }

public:

    ///////////////////////////////////////////////////////////////////////////

    DTS( void )
    {
        ID = -1;
        channel = 0;

        timer = -1;
        state = DOWN;

        equType = DTS_UNKNOWN; // unknown
        IsOWS = false;
        IsCTT10 = false;
        CTT10_userid_pdu_len = 0;

        disable4C = false;

        hasDisplay = NO_DISPLAY;
        D4_mode = false;
        D3_basic = true;
        reqRemap = false;
        rowCount = 2;
        colCount = 20;
        curRow = curCol = 0;
        oldCurRow = oldCurCol = -1;

        userInputLen = 0;

        f_D = -1;
        f_D_XON = true; // by default we assume XON on D channel

        memset( field, ' ', sizeof( field ) );
        memset( oldField, ' ', sizeof( field ) );

        autoReconnect = false;
        autoReconnectCount = 5;
        }

    ~DTS ()
    {
        if ( f_D >= 0 )
            close( f_D );
        }

    ///////////////////////////////////////////////////////////////////////////

    void Go_State( STATE newState, int timeout, const char* eventDesc )
    {
        static const char* stateDesc [] =
        {
            "DOWN", "HALF UP", "PASSIVE", "ACTIVE", "FAULTY"
            };

        if ( timeout >= 0 )
            fprintf( stderr, "    On %s: %s -> %s; Timeout in %d ms.\n",
                eventDesc, stateDesc[ state ], stateDesc[ newState ], timeout );
        else
            fprintf( stderr, "    On %s: %s -> %s; No timeout.\n",
                eventDesc, stateDesc[ state ], stateDesc[ newState ] );

        state = newState;

        timer = timeout;
        }

    ///////////////////////////////////////////////////////////////////////////

    void ClearDisplay( void )
    {
        if ( state != ACTIVE ) // || hasDisplay != CHARACTER_DISPLAY )
            return;

        oldCurRow = oldCurCol = -1;
        memset( oldField, ' ', sizeof( field ) );

        if ( hasDisplay != NO_DISPLAY )
        {
            static unsigned char PDU [] = { 0, 0x20, FNC_CLEARDISPLAY, 0xFF, 0xFF, 0xFF };
            PDU[ 0 ] = channel;
            write( f_D, PDU, sizeof( PDU ) );
            fprintf( stderr, "IDM -> DTS: CLEARDISPLAY\n" );
            }
        }

    void ClearDisplayField( int row, int col )
    {
        if ( state != ACTIVE || hasDisplay != CHARACTER_DISPLAY )
            return;

        static unsigned char PDU [] = { 0, 0x20, FNC_CLEARDISPLAYFIELD, 0x00 };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = TransformRowCol( row, col );

        write( f_D, PDU, sizeof( PDU ) );
        fprintf( stderr, "IDM -> DTS: CLEARDISPLAYFIELD: R=%d, C=%d\n", row, col );
        }

    void WriteDisplayField( int row, int col, char* str )
    {
        if ( state != ACTIVE || hasDisplay != CHARACTER_DISPLAY )
            return;

        static unsigned char PDU[ 4 + 10 ] = { 0, 0x20, FNC_WRITEDISPLAYFIELD, 0x00 };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = TransformRowCol( row, col );

        int i;
        for ( i = 0; i < 10 && *str; i++, str++ )
            PDU[ 4 + i ] = (unsigned char)*str;
        for ( ; i < 10; i++ )
            PDU[ 4 + i ] = ' ';

        write( f_D, PDU, sizeof( PDU ) );
        fprintf( stderr, "IDM -> DTS: WRITEDISPLAYFIELD: R=%d, C=%d [", row, col );
        for ( int i = 0; i < 10; i++ )
            fprintf( stderr, "%c", isprint( PDU[ 4 + i ] ) ? PDU[ 4 + i ] : '.' );
        fprintf( stderr, "]\n" );
        }

    void ActivateCursor( int row, int col, int pos )
    {
        if ( state != ACTIVE || hasDisplay != CHARACTER_DISPLAY )
            return;

        static unsigned char PDU [] = { 0, 0x20, FNC_ACTCURSOR, 0x00, 0x00 };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = TransformRowCol( row, col );
        PDU[ 4 ] = pos;

        write( f_D, PDU, sizeof( PDU ) );
        fprintf( stderr, "IDM -> DTS: ACTCURSOR: R=%d, C=%d, P=%d\n", row, col, pos );
        }

    void ClearCursor( void )
    {
        if ( state != ACTIVE || hasDisplay != CHARACTER_DISPLAY )
            return;

        static unsigned char PDU [] = { 0, 0x20, FNC_CLEARCURSOR };
        PDU[ 0 ] = channel;

        write( f_D, PDU, sizeof( PDU ) );
        fprintf( stderr, "IDM -> DTS: CLEARCURSOR\n" );
        }

    void DispSpecCharacter( int id, int c1, int c2, int c3, int c4, int c5, int c6, int c7, int c8 )
    {
        if ( state != ACTIVE || hasDisplay != CHARACTER_DISPLAY )
            return;

        static unsigned char PDU[ 12 ] = { 0, 0x20, FNC_DISSPECCHR };

        PDU[ 0 ] = channel;
        PDU[ 3 ] = id;
        PDU[ 4 ] = c1; PDU[ 5 ] = c2; PDU[ 6 ] = c3; PDU[ 7 ] = c4;
        PDU[ 8 ] = c5; PDU[ 9 ] = c6; PDU[ 10 ] = c7; PDU[ 11 ] = c8;

        write( f_D, PDU, sizeof( PDU ) );
        fprintf( stderr, "IDM -> DTS: DISSPECCHR: %d, %02X %02X %02X %02X %02X %02X %02X %02X\n", 
            PDU[ 3 ], PDU[ 4 ], PDU[ 5 ], PDU[ 6 ], PDU[ 7 ], PDU[ 8 ], PDU[ 9 ], PDU[ 10 ], PDU[ 11 ]
            );
        }

    void WriteDisplayGraphics( int x, int y, int chDesc, const char* str )
    {
        if ( state != ACTIVE || hasDisplay != GRAPHICAL_DISPLAY )
            return;

        int strLen = strlen( str );

        static unsigned char PDU[ 512 ] = { 0, 0x20, FNC_WRITEDISPLAYGR };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = x & 0xFF; // LSB
        PDU[ 4 ] = x >> 8; // MSB
        PDU[ 5 ] = y & 0xFF;
        PDU[ 6 ] = 0x00;
        PDU[ 7 ] = chDesc;
        PDU[ 8 ] = strLen;
        strcpy( (char*)PDU + 9, str );

        write( f_D, PDU, 9 + strLen );

        fprintf( stderr, "IDM -> DTS: WRITEDISPLAYGR: X=%d, Y=%d [", x, y );
        for ( int i = 0; i < strLen; i++ )
            fprintf( stderr, "%c", isprint( PDU[ 9 + i ] ) ? PDU[ 9 + i ] : '.' );
        fprintf( stderr, "]\n" );
        }

    void DrawGraphics( int xStart, int yStart, int xEnd, int yEnd, int drawDesc, int chDesc )
    {
        if ( state != ACTIVE || hasDisplay != GRAPHICAL_DISPLAY )
            return;

        static unsigned char PDU[ 12 ] = { 0, 0x20, FNC_DRAWGR };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = drawDesc;
        PDU[ 4 ] = xStart & 0xFF; // LSB
        PDU[ 5 ] = xStart >> 8; // MSB
        PDU[ 6 ] = yStart & 0xFF;
        PDU[ 7 ] = 0x00;
        PDU[ 8 ] = xEnd & 0xFF; // LSB
        PDU[ 9 ] = xEnd >> 8; // MSB
        PDU[ 10 ] = yEnd & 0xFF;
        PDU[ 11 ] = chDesc;

        write( f_D, PDU, sizeof( PDU ) );

        fprintf( stderr, "IDM -> DTS: DRAWGR\n" );
#if 0
        int len = sizeof( PDU );

        fprintf( stderr, "IDM -> DTS:" );
        for ( int i = 0; i < len; i++ )
            if ( i == 2 )
                fprintf( stderr, " \033[44;1m %02X \033[40;0m", PDU[ i ] );
            else
                fprintf( stderr, " %02X", PDU[ i ] );
        fprintf( stderr, "\n" );
#endif
        }

    void ClearDisplayGraphics( int xStart, int yStart, int xEnd, int yEnd )
    {
        if ( state != ACTIVE || hasDisplay != GRAPHICAL_DISPLAY )
            return;

        static unsigned char PDU[ 9 ] = { 0, 0x20, FNC_CLEARDISPLAYGR };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = xStart & 0xFF; // LSB
        PDU[ 4 ] = xStart >> 8; // MSB
        PDU[ 5 ] = yStart & 0xFF;
        PDU[ 6 ] = xEnd & 0xFF; // LSB
        PDU[ 7 ] = xEnd >> 8; // MSB
        PDU[ 8 ] = yEnd & 0xFF;

        write( f_D, PDU, sizeof( PDU ) );

        fprintf( stderr, "IDM -> DTS: CLEARDISPLAYGR\n" );
#if 0
        int len = sizeof( PDU );

        fprintf( stderr, "IDM -> DTS:" );
        for ( int i = 0; i < len; i++ )
            if ( i == 2 )
                fprintf( stderr, " \033[44;1m %02X \033[40;0m", PDU[ i ] );
            else
                fprintf( stderr, " %02X", PDU[ i ] );
        fprintf( stderr, "\n" );
#endif
        }

    void HwIconsGraphics( int xStart, int yStart, int present )
    {
        if ( state != ACTIVE || hasDisplay != GRAPHICAL_DISPLAY )
            return;

        static unsigned char PDU[ 7 ] = { 0, 0x20, FNC_HWICONSGR };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = xStart & 0xFF; // LSB
        PDU[ 4 ] = xStart >> 8; // MSB
        PDU[ 5 ] = yStart & 0xFF;
        PDU[ 6 ] = present;

        write( f_D, PDU, sizeof( PDU ) );

        fprintf( stderr, "IDM -> DTS: HWICONSGR\n" );
#if 0
        int len = sizeof( PDU );

        fprintf( stderr, "IDM -> DTS:" );
        for ( int i = 0; i < len; i++ )
            if ( i == 2 )
                fprintf( stderr, " \033[44;1m %02X \033[40;0m", PDU[ i ] );
            else
                fprintf( stderr, " %02X", PDU[ i ] );
        fprintf( stderr, "\n" );
#endif
        }

    void ClearLEDs( void )
    {
        if ( state != ACTIVE )
            return;

        static unsigned char PDU [] = { 0, 0x20, FNC_CLEARLEDS };
        PDU[ 0 ] = channel;
        write( f_D, PDU, sizeof( PDU ) );

        // fprintf( stderr, "IDM -> DTS: CLEARLEDS\n" );
        }

    void ClearLED( int id )
    {
        if ( state != ACTIVE )
            return;

        static unsigned char PDU [] = { 0, 0x20, FNC_CLEARLED, 0 };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = id;
        write( f_D, PDU, sizeof( PDU ) );

        // fprintf( stderr, "IDM -> DTS: CLEARLED: %02X\n", id );
        }

    void SetLED( int id )
    {
        if ( state != ACTIVE )
            return;

        static unsigned char PDU [] = { 0, 0x20, FNC_SETLED, 0 };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = id;
        write( f_D, PDU, sizeof( PDU ) );

        // fprintf( stderr, "IDM -> DTS: SETLED: %02X\n", id );
        }

    void FlashLED( int id, int cadence = 0 )
    {
        if ( state != ACTIVE )
            return;

        static unsigned char PDU [] = { 0, 0x20, FNC_FLASHLEDCAD0, 0 };
        PDU[ 0 ] = channel;
        PDU[ 2 ] = cadence == 2 ? FNC_FLASHLEDCAD2 
                 : cadence == 1 ? FNC_FLASHLEDCAD1
                 : FNC_FLASHLEDCAD0;
        PDU[ 3 ] = id;
        write( f_D, PDU, sizeof( PDU ) );

        //f printf( stderr, "IDM -> DTS: FLASHLEDCAD0: %02X\n", id );
        }

    void TransmissionOrder( int type )
    {
        if ( state != ACTIVE )
            return;

        static unsigned char PDU [] = { 0, 0x20, FNC_TRANSMISSION, 0 };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = type;
        write( f_D, PDU, sizeof( PDU ) );

        fprintf( stderr, "IDM -> DTS: TRANSMISSION: %02X\n", type );
        }

    void TransmissionLevelUpdate( int loudspeakerLevel, int earpieceLevel )
    {
        if ( state != ACTIVE )
            return;

        static unsigned char PDU [] = { 0, 0x20, FNC_TRANSLVLUPDATE, 0x00, 0x00 };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = loudspeakerLevel;
        PDU[ 4 ] = earpieceLevel;
        write( f_D, PDU, sizeof( PDU ) );

        fprintf( stderr, "IDM -> DTS: TRANSLVLUPDATE: %02x %02x\n", loudspeakerLevel, earpieceLevel );

        D3_basic = false; // Go D3+ mode
        }

    void EquipmentActivate( void )
    {
        if ( IsCTT10 )
        {
            static unsigned char PDU [] =
            {
                0, // channel
                /* D01 */ 0x20, // OPC: 010b
                /* D02 */ 0x80, // FNC: EQUACT
                /* D03 */ 0x0B, // 
                /* D04 */ 0x00, // 
                /* D05 */ 0x00, // 
                /* D06 */ 0x0F, // 
                /* D07 */ 0x5A, // 
                /* D08 */ 0x0C, // 
                /* D09 */ 0x04, // 
                /* D10 */ 0x04, // 
                /* D11 */ 0x50, // 
                /* D12 */ 0x14, // 
                /* D13 */ 0x50, // 
                /* D14 */ 0x00, // 
                /* D15 */ 0x01, // 
                /* D16 */ 0x04, // 
                /* D17 */ 0x04, // 
                /* D18 */ 0x04, // 
                /* D19 */ 0x04  // 
                };
            PDU[ 0 ] = channel;
            write( f_D, PDU, sizeof( PDU ) );
            fprintf( stderr, "IDM -> DTS: EQUACT CTT10\n" );
            }
        else
        {
            static unsigned char PDU [] =
            {
                0, // channel
                /* D01 */ 0x20, // OPC: 010b
                /* D02 */ 0x80, // FNC: EQUACT
                /* D03 */ 0x14, // Internal ring signal, 1:st on interval
                /* D04 */ 0x64, // Internal ring signal, 1:st off interval
                /* D05 */ 0x00, // Internal ring signal, 2:nd on interval
                /* D06 */ 0x01, // Internal ring signal, 2:nd off interval
                /* D07 */ 0x07, // External ring signal, 1:st on interval
                /* D08 */ 0x06, // External ring signal, 1:st off interval
                /* D09 */ 0x07, // External ring signal, 2:nd on interval
                /* D10 */ 0x64, // External ring signal, 2:nd off interval
                /* D11 */ 0x06, // Call back ring signal, 1:st on interval
                /* D12 */ 0x08, // Call back ring signal, 1:st off interval
                /* D13 */ 0x06, // Call back ring signal, 2:nd on interval
                /* D14 */ 0x08, // Call back ring signal, 2:nd off interval
                /* D15 */ 0xB9, // LED indicator cadence 0, 1:st on interval
                /* D16 */ 0x05, // LED indicator cadence 0, 1:st off interval
                /* D17 */ 0x05, // LED indicator cadence 0, 2:nd on interval
                /* D18 */ 0x05, // LED indicator cadence 0, 2:nd off interval
                /* D19 */ 0x32, // LED indicator cadence 1, On interval
                /* D20 */ 0x32, // LED indicator cadence 1, Off interval
                /* D21 */ 0x1E, // LED indicator cadence 2, On interval
                /* D22 */ 0x1E  // LED indicator cadence 2, Off interval
                };
            PDU[ 0 ] = channel;
            write( f_D, PDU, sizeof( PDU ) );
            fprintf( stderr, "IDM -> DTS: EQUACT\n" );
            }
        }

    void EquipmentStatusRequest( void )
    {
        if ( IsCTT10 )
        {
            static unsigned char PDU [] = { 0, 0x30, FNC_EQUSTAREQ, 0x20, 0x06, 0x07, 0x01, 0x07, 0x00, 0x00 };
            PDU[ 0 ] = channel;
            write( f_D, PDU, sizeof( PDU ) );
            fprintf( stderr, "IDM -> DTS: EQUSTAREQ (CTT10)\n" );
            }
        else
        {
            static unsigned char PDU [] = { 0, 0x20, FNC_EQUSTAREQ };
            PDU[ 0 ] = channel;
            write( f_D, PDU, sizeof( PDU ) );
            fprintf( stderr, "IDM -> DTS: EQUSTAREQ\n" );
            }
        }

    void EquipmentStatusD4Request( int exchangeType )
    {
        if ( state != ACTIVE )
            return;

        static unsigned char PDU [] = { 0, 0x20, FNC_EQUSTAD4REQ, 0x00 };
        PDU[ 0 ] = channel;
        PDU[ 3 ] = exchangeType;
        write( f_D, PDU, sizeof( PDU ) );
        fprintf( stderr, "IDM -> DTS: EQUSTAD4REQ\n" );
        }

    void EquipmentTestRequestErrorStatus( void )
    {
        static unsigned char PDU [] = { 0, 0x20, FNC_EQUTESTREQ, FNC_TEST_ERROR_STATUS };
        PDU[ 0 ] = channel;
        write( f_D, PDU, sizeof( PDU ) );
        fprintf( stderr, "IDM -> DTS: EQUTESTREQ: Text Code = %02X\n", PDU[ 3 ] );
        }

    void EquipmentTestRequestReset( void )
    {
        static unsigned char PDU [] = { 0, 0x20, FNC_EQUTESTREQ, FNC_TEST_RESET, 0x00, 0x00, 0x00, 0x00 };
        PDU[ 0 ] = channel;
        write( f_D, PDU, sizeof( PDU ) );
        fprintf( stderr, "IDM -> DTS: EQUTESTREQ: Test Code = %02X\n", PDU[ 3 ] );
        }

    void CTT10_StatusRequest( void )
    {
        static unsigned char PDU [] = { 0, 0x30, FNC_EQUSTAREQ, 0x20, 0x06, 0x06, 0x12, 0x01, 0x42, 0x48 };
        PDU[ 0 ] = channel;
        write( f_D, PDU, sizeof( PDU ) );
        fprintf( stderr, "IDM -> DTS: EQUSTAREQ: CTT10\n" );
        }

    ///////////////////////////////////////////////////////////////////////////

    void Update( void )
    {
        if ( state != ACTIVE || hasDisplay != CHARACTER_DISPLAY )
            return;
#if 0
        timeval tvSelect;
        tvSelect.tv_sec = 0;
        tvSelect.tv_usec = 0; // 100ms timeout

        do
        {
            fd_set readfds;
            FD_ZERO( &readfds );
            FD_SET( f_D, &readfds );

            if ( ! select( f_D + 1, &readfds, NULL, NULL, &tvSelect ) )
                continue;

            unsigned char pdu[ 512 ];
		    int rd = read_stream( f_D, pdu, 2 );

		    if ( 0 == rd )
			    return; // EOF

		    if ( 2 != rd )
		    {
			    fprintf( stderr, "%s: Could not read packet length\n", dspboard );
			    return;
		        }

		    int len = ( pdu[ 0 ] << 8 )  + pdu[ 1 ];

		    rd = read_stream( f_D, pdu, len );

		    if ( 0 == rd )
			    return; // EOF

		    if ( len != rd )
		    {
			    fprintf( stderr, "%s: Expected %d, read %d\n", dspboard, len, rd );
			    return;
		        }

            if ( pdu[ 1 ] == 0x00 && pdu[ 2 ] == FNC_XMIT_FLOW_CONTROL )
            {
                f_D_XON = pdu[ 3 ];
                fprintf( stderr, " \033[41;1m %s \033[40;0m\n", f_D_XON ? "XON" : "XOFF" );
                }
            else
            {
                fprintf( stderr, "*** PDU [FNC=%02X] Ignored ***\n", pdu[ 2 ] );
                }

            tvSelect.tv_usec = 25000; // 20ms wait next time

            } while( ! f_D_XON );
#endif
        for ( int row = 0; row < rowCount; row++ )
        {
            for ( int col = 0; col < colCount / 10; col++ )
            {
                if ( memcmp( field[ row ][ col ], oldField[ row][ col ], 10 ) != 0 )
                {
                    memcpy( oldField[ row ][ col ], field[ row ][ col ], 10 );
                    if ( memcmp( field[ row ][ col ], "           ", 10 ) == 0 )
                        ClearDisplayField( row, col );
                    else
                        WriteDisplayField( row, col, field[ row ][ col ] );

                    usleep( 15000 );
                    }
                }
            }

        if ( curRow != oldCurRow || curCol != oldCurCol )
        {
            ActivateCursor( curRow, curCol / 10, curCol % 10 );
            oldCurRow = curRow;
            oldCurCol = curCol;
            usleep( 30000 );
            }
        }

    void Clear( void )
    {
        curRow = curCol = 0;
        memset( field, ' ', sizeof( field ) );
        }

    void PutCh( int c )
    {
        if ( c == '\b' )
        {
            if ( curCol > 0 )
            {
                --curCol;
                field[ curRow ][ curCol / 10 ][ curCol % 10 ] = ' ';
                }
            }
        else if ( c == '\r' )
        {
            curCol = 0;
            }
        else if ( c == '\n' )
        {
            curCol = 0;
            if ( ++curRow >= rowCount )
            {
                curRow = rowCount - 1;

                // Scroll up
                for ( int i = 0; i < rowCount - 1; i++ )
                    memcpy( field[ i ], field[ i + 1 ], sizeof( field[ 0 ] ) );
                memset( field[ rowCount - 1 ], ' ', sizeof( field[ rowCount - 1 ] ) );
                }
            }
        else
        {
            field[ curRow ][ curCol / 10 ][ curCol % 10 ] = c;

            if ( ++curCol >= colCount )
            {
                curCol = 0;
                if ( ++curRow >= rowCount )
                {
                    curRow = rowCount - 1;

                    // Scroll up
                    for ( int i = 0; i < rowCount - 1; i++ )
                        memcpy( field[ i ], field[ i + 1 ], sizeof( field[ i ] ) );
                    memset( field[ rowCount - 1 ], ' ', sizeof( field[ rowCount - 1 ] ) );
                    }
                }
            }
        }

    void Goto( int row, int col )
    {
        curRow = row % rowCount;
        curCol = col % colCount;
        }

    void Printf( const char* format... )
    {
        va_list marker;
        va_start( marker, format );

        char line[ 256 ];
        vsprintf( line, format, marker );

        for ( char* str = line; *str; str++ )
            PutCh( *str );
        }

    void Messagef( bool clear, char* format... )
    {
        va_list marker;
        va_start( marker, format );

        vsprintf( infoMessage, format, marker );

        if ( hasDisplay == GRAPHICAL_DISPLAY )
        {
            ClearDisplay ();
            WriteDisplayGraphics( 20,  15, ( 3 << 5 ) | 2, infoMessage );
            }
        else
        {
            if ( ! clear )
                PutCh( '\n' );
            else
            {
                ClearDisplay ();
                Clear ();
                Update ();
                }

            for ( char* str = infoMessage; *str; str++ )
                PutCh( *str );

            Update ();
            }

        if ( ! clear )
            usleep( 500000 );
        else
            usleep( 1000000 );
        }

    void DisplayMenu( void )
    {
        if ( hasDisplay == GRAPHICAL_DISPLAY )
        {
            WriteDisplayGraphics( 20,  0, ( 3 << 5 ) | 2, infoTitle );
            WriteDisplayGraphics( 20, 12, ( 3 << 5 ) | 1, infoMessage );
            WriteDisplayGraphics( 20, 24, ( 3 << 5 ) | 3, infoAdvice );
            //DrawGraphics( 53, 22, 92, 37, 3, ( 3 << 5 ) | 1 );
            }
        else
        {
            Clear ();
            Printf( "%s\n", infoTitle );
            Printf( "%s\n", infoMessage );
            Printf( "%s", infoAdvice );
            Update ();
            }
        }

    ///////////////////////////////////////////////////////////////////////////

    bool Open( int id, char* dsp_cfg );
    void Initialize( int type, int version, int extraVerInfo );

    bool Connect( bool forceMaster = true );
    void Passthru( void );
    void Main( void );
    void OnTimeoutEvent( void );
    void ParseOutputPDU( unsigned char* pdu, int pdu_len );
    };

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

static inline int read_stream_CRLF( int fd, char* buf, int len )
{
    int readc = 0;

    while( readc < len )
    {
        int rc = read( fd, buf + readc, 1 );
        if ( rc < 0 )
        {
            buf[ readc ] = 0; // terminate string
            return rc;
            }
        if ( rc == 0 )
            break;
        // asert( rc == 1 );
        ++readc;

        if ( buf[ readc - 1 ] == '\n' ) // if current is LF
            if ( readc >= 2 && buf[ readc - 2 ] == '\r' ) // if CR is before
            {
                // found CR LF
                buf[ readc - 2 ] = 0; // relpace CR LF with '\0';
                readc -= 2;
                break;
                }
        }

    buf[ readc ] = 0; // terminate string
    return readc;
    }

#endif // _DTS_H_INCLUDED
