#include "DTS.h"

///////////////////////////////////////////////////////////////////////////////

bool DTS::Open( int id, char* dsp_cfg )    
{
    if ( ! dspCfg.Find( id, dsp_cfg ) )
    {
        printf( "Could not find DSP %d in %s\n", id, dsp_cfg );
        return false;
    }

    dspCfg.waitToBeRunning ();

    ID = id;
    strcpy( dspboard, dspCfg.devname );
    strcat( dspboard, "io" );
    channel = dspCfg.channel;

    connParams.find( ID );

    strcpy( localIpAddr, "" );

    // Determine local IP address
    {
        char buffer[ 128 ];
        FILE* f = popen( "ifconfig eth0", "r" );
        if ( f )
        {
            while( fgets( buffer, sizeof( buffer ), f ) )
            {
                if ( 1 == sscanf( buffer, " inet addr:%s", localIpAddr ) )
                {
                    break;
                    }
                }
            }
            pclose( f );
        }

    strcpy( infoTitle, "IPTC DEX28 VoIP Gateway" );
    if ( ! localIpAddr[0] )
        strcpy( infoMessage, "Ready." );
    else
        sprintf( infoMessage, "Ready.   (Local IP Addr: %s)", localIpAddr );
    strcpy( infoAdvice, "/" );

    f_D = open( dspboard, O_RDWR );

    if ( f_D < 0 )
    {
        fprintf( stderr, "%d: Failed to open %s channel %d\n", ID, dspboard, channel );
        return false;
        }

    // Receive messages from the single channel
    //
    ::ioctl( f_D, IOCTL_HPI_SET_D_CHANNEL, channel );

    // Put PDU that should be automatically written when device is closed/relased.
    // Ioctl param value 1 indicates that next write will contain PDU.
    //
    ::ioctl( f_D, IOCTL_HPI_ON_CLOSE_WRITE, 1 ); 

    // OnCloseWrite PDU:
    //
    static unsigned char PDU [] = { 0, 0x20, FNC_EQUTESTREQ, FNC_TEST_RESET, 0x00, 0x00, 0x00, 0x00 };
    PDU[ 0 ] = channel;
    write( f_D, PDU, sizeof( PDU ) );

    // Hello, world...
    //
    fprintf( stderr, "Connected [%d] to %s channel %d; TAU D option [%s]\n", 
        ID, dspboard, channel, connParams.TAUD );

    // Open default TAU-D
    //
    char taud_method[ 32 ] = "";
    char taud_param[ 32 ] = "";
    if ( sscanf( connParams.TAUD, " %s %s", taud_method, taud_param ) >= 1 )
    {
        if ( strcasecmp( taud_method, "usb" ) == 0 )
        {
            tau.OpenSerial( "/dev/ttyS1" );
            }
        else if ( strcasecmp( taud_method, "tcp" ) == 0 )
        {
            int taud_tcp_port = 0;
            if ( sscanf( taud_param, "%d", &taud_tcp_port ) >= 1 && taud_tcp_port > 0 )
                tau.OpenTCP( taud_tcp_port );
            }
        else if ( strcasecmp( taud_method, "ctt10" ) == 0 )
        {
            IsCTT10 = true;
            disable4C = true;
            }
        else if ( strcasecmp( taud_method, "no4c" ) == 0 )
        {
            disable4C = true;
            }
        }

    return true;
    }

///////////////////////////////////////////////////////////////////////////////

void DTS::Initialize( int type, int version, int extraVerInfo )
{
    IsOWS = false;
    equType = ( type << 8 ) | version;

    hasDisplay = NO_DISPLAY; // Default is not to have a display
    reqRemap = false;
    D3_basic = true;
    D4_mode = false;

    char* desc = "Unknown DTS";

    switch( equType )
    {
        case DTS_OPI_II:
        case DTS_OPI_II_B:
            desc = "OPI-II";
            hasDisplay = OPI_II_DISPLAY;
            break;

        case DTS_CTT10:
        case DTS_CTT10_ACTIVE:
            desc = "CTT10";
            hasDisplay = NO_DISPLAY;
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
            hasDisplay = GRAPHICAL_DISPLAY;
            break;

        case DTS_DBC225:
        case DTS_DBC225_1DSS:
        case DTS_DBC225_2DSS:
        case DTS_DBC225_3DSS:
        case DTS_DBC225_4DSS:
            desc = "DBC225";
            hasDisplay = GRAPHICAL_DISPLAY;
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

    fprintf( stderr, 
        "Found \033[1m%s\033[0m\n"
        "    Type: %02X, Version: %02X, ExtraVerInfo: %02X;\n",
        desc, type, version, extraVerInfo
        );

    switch( hasDisplay )
    {
        case NO_DISPLAY:
            fprintf( stderr, "    No Display.\n" );
            break;
        case OPI_II_DISPLAY:
            fprintf( stderr, "    OPI-II Display.\n" );
            break;
        case CHARACTER_DISPLAY:
            fprintf( stderr, "    Character Display: rows %d, cols %d%s\n",
                rowCount, colCount, reqRemap ? ", 203-remap" : "" );
            break;
        case GRAPHICAL_DISPLAY:
            fprintf( stderr, "    Graphical Display: rows %d, cols %d%s\n",
                rowCount, colCount, reqRemap ? ", 203-remap" : "" );
            break;
        }

#if 0
    TransmissionLevelUpdate( 0x05, 0x07 ); // Enter D3+ State
#endif

    ClearDisplay ();
    ClearLEDs ();
    DispSpecCharacter( 1, 0x07, 0x0F, 0x18, 0x11, 0x01, 0x03, 0x03, 0x00 );
    DispSpecCharacter( 2, 0x1F, 0x1F, 0x10, 0x16, 0x16, 0x16, 0x16, 0x00 );
    DispSpecCharacter( 3, 0x1F, 0x1F, 0x00, 0x1E, 0x19, 0x19, 0x1E, 0x00 );
    DispSpecCharacter( 4, 0x03, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 ); 
    DispSpecCharacter( 5, 0x16, 0x16, 0x10, 0x1F, 0x0F, 0x00, 0x00, 0x00 );
    DispSpecCharacter( 6, 0x18, 0x18, 0x02, 0x1C, 0x18, 0x00, 0x00, 0x00 );
    Update ();

    userInputLen = 0;
    }

///////////////////////////////////////////////////////////////////////////////

void DTS::ParseOutputPDU( unsigned char* pdu, int pdu_len )
{
#if 1
    fprintf( stderr, "DTS -> IDM:" );
    for ( int i = 0; i < pdu_len; i++ )
        if ( i == 1 )
            fprintf( stderr, " \033[41;1m %02X \033[40;0m", pdu[ i ] );
        else
            fprintf( stderr, " %02X", pdu[ i ] );
    fprintf( stderr, "\n" );
#endif

    if ( pdu[ 1 ] == FNC_XMIT_FLOW_CONTROL ) // flow control
    {
        f_D_XON = pdu[ 2 ];
        fprintf( stderr, " \033[41;1m %s \033[40;0m\n", f_D_XON ? "XON" : "XOFF" );
        }
    else if ( pdu[ 1 ] == FNC_DASL_LOOP_SYNC ) // Event: LOOP SYNC LOST
    {
        if ( pdu[ 2 ] == 0x00 ) // On Sync Lost
        {
            f_D_XON = false;
            fprintf( stderr, " \033[41;1m %s \033[40;0m\n", f_D_XON ? "XON" : "XOFF" );

            Go_State( DOWN, -1, "Sync LOST" );
            }
        else // On Sync OK
        {
            f_D_XON = true;
            fprintf( stderr, " \033[41;1m %s \033[40;0m\n", f_D_XON ? "XON" : "XOFF" );

            Go_State( HALF_UP, 3000, "Sync OK" );

            if ( IsCTT10 )
                EquipmentActivate ();
            }
        }
    else if ( pdu[ 1 ] == FNC_EQUSTA && ( pdu[ 5 ] & 0x3F ) != 0 ) // On EQUSTA Active
    {
        if ( state == ACTIVE )
        {
            statusRequested = false;
            Go_State( ACTIVE, 20000, "EQUSTA Active" );

            if ( autoReconnect && autoReconnectCount > 0 )
            {
                if ( autoReconnectCount < 100 )
                    --autoReconnectCount;

                Connect( /*forceMaster*/ true );

                // Reinitialize everything
                //
                EquipmentTestRequestReset ();
                Go_State( DOWN, -1, "Connection Closed" );
                }
            }
        else if ( state == PASSIVE )
        {
            statusRequested = false;

            if ( autoReconnect && autoReconnectCount > 0 )
                Go_State( ACTIVE, 5000, "EQUSTA Active" );
            else
                Go_State( ACTIVE, 20000, "EQUSTA Active" );

            memset( EQUSTA_pdu, 0, sizeof( EQUSTA_pdu ) );
            memcpy( EQUSTA_pdu, pdu, pdu_len );

            if ( EQUSTA_pdu[ 2 ] == 0x0E && ( EQUSTA_pdu[ 10 ] & 0x0F ) == 0x03 )
                EquipmentStatusD4Request( 0x00 );
            else
            {
                Initialize( pdu[ 2 ], pdu[ 3 ], pdu_len >= 11 ? pdu[ 10 ] : 0 );

                if ( IsOWS )
                {
                    autoReconnectCount = 100;

                    Connect( /*forceMaster*/ true );

                    // Reinitialize everything
                    //
                    EquipmentTestRequestReset ();
                    Go_State( DOWN, -1, "Connection Closed" );
                    }
                else
                {
                    if ( autoReconnect && autoReconnectCount > 0 )
                    {
                        if ( autoReconnectCount == 1 )
                            sprintf( infoAdvice, "Retrying... (1 more time)" );
                        else if ( autoReconnectCount >= 100 )
                            sprintf( infoAdvice, "Retrying..." );
                        else
                            sprintf( infoAdvice, "Retrying... (%d more times)", autoReconnectCount );
                        }
                    else
                    {
                        if ( hasDisplay == GRAPHICAL_DISPLAY )
                            strcpy( infoAdvice, "Press Line 1 to connect..." );
                        else
                            strcpy( infoAdvice, "Press Access 1 to connect " );
                        }

                    DisplayMenu ();

                    SetLED( 0x09 );
                    SetLED( 0x0A );
                    FlashLED( 0x0B, 2 );

                    if ( autoReconnect )
                    {
                        }
                    else if ( connParams.AUTOCONNECT )
                    {
                        userInput[ userInputLen ] = 0;
                        fprintf( stderr, "User Input: [%s]\n", userInput );

                        if ( connParams.find( ID ) )
                        {
                            if ( userInputLen > 0 )
                                strcpy( connParams.DN, userInput );

                            autoReconnectCount = connParams.RETRYC;

                            if ( autoReconnectCount <= 0 )
                                autoReconnectCount = 5;

                            Connect( /* forceMaster */ true );

                            // Reinitialize everything
                            //
                            EquipmentTestRequestReset ();
                            Go_State( DOWN, -1, "Connection Closed" );
                            }
                        }
                    }
                }

            }
        else // HALF UP, DOWN or FAULTY
        {
            if ( IsCTT10 )
                Go_State( PASSIVE, -1, "Detected CTT10" );
            else
                EquipmentTestRequestReset ();
            }
        }
    else if ( pdu[ 1 ] == FNC_EQUSTA && ( pdu[ 5 ] & 0x3F ) == 0 ) // On EQUSTA Passive
    {
        EquipmentActivate ();
        Go_State( PASSIVE, 1000, "EQUSTA Passive" );
        }
    else if ( pdu[ 1 ] == FNC_EQUTESTRES && pdu[ 2 ] == 0x7F ) // On EQUTESTRES
    {
        bool faulty = ( pdu[ 3 ] != 0 );

        if ( faulty )
        {
            Go_State( FAULTY, 60000, "EQUTESTRES ErrorStatus Faulty" );
            }
        else if ( state == HALF_UP )
        {
            EquipmentTestRequestReset ();
            Go_State( HALF_UP, 3000, "EQUTESTRES ErrorStatus OK" );
            }
        else
        {
            // Stay in the same state
            }
        }
    else if ( state == ACTIVE && pdu[ 1 ] == FNC_FIXFNCACT ) // On FIXFNCACT
    {
        int keyCode = pdu[ 2 ];

        switch( keyCode )
        {
            case KEY_FIXFN_STAR:
                if ( userInputLen < sizeof( userInput ) )
                {
                    userInput[ userInputLen++ ] = '.';
                    userInput[ userInputLen ] = 0;
                    }
                PutCh( '.' );
                break;

            case KEY_FIXFN_HASH:
                userInputLen = 0;
                userInput[ userInputLen ] = 0;
                PutCh( '\n' );
                break;

            case KEY_FIXFN_CLEAR:
                if ( userInputLen == 0 && autoReconnect && autoReconnectCount > 0 )
                {
                    autoReconnect = false;
                    autoReconnectCount = 0;
                    
                    char oldInfo[256]; 
                    strcpy( oldInfo, infoMessage );
                    Messagef( true, "Reconnect cancelled" );
                    strcpy( infoMessage, oldInfo );
                    
                    if ( hasDisplay == GRAPHICAL_DISPLAY )
                        strcpy( infoAdvice, "Press Line 1 to connect..." );
                    else
                        strcpy( infoAdvice, "Press Access 1 to connect " );
                        
                    ClearDisplay ();
                    DisplayMenu ();
                    }
                    
                if ( userInputLen > 0 )
                    userInput[ --userInputLen ] = 0;
                PutCh( '\b' );
                break;

            case KEY_FIXFN_OFF_HOOK:
                Printf( "[Off Hook]" );
                break;

            case KEY_FIXFN_ON_HOOK:
                Printf( "[On Hook]" );
                break;

            case KEY_FIXFN_LINE:
            {
                static bool x = false;
                TransmissionOrder( ( x = ! x ) ? 0x02 : 0x00 );
                break;
                }

            case KEY_FIXFN_SPK_PLUS:
                Printf( "[+]" );
                break;

            case KEY_FIXFN_SPK_MINUS:
                Printf( "[-]" );
                break;

            case KEY_FIXFN_0:
            case KEY_FIXFN_1:
            case KEY_FIXFN_2:
            case KEY_FIXFN_3:
            case KEY_FIXFN_4:
            case KEY_FIXFN_5:
            case KEY_FIXFN_6:
            case KEY_FIXFN_7:
            case KEY_FIXFN_8:
            case KEY_FIXFN_9:
                if ( userInputLen < sizeof( userInput ) )
                {
                    userInput[ userInputLen++ ] = pdu[ 2 ] + '0';
                    userInput[ userInputLen ] = 0;
                    }
                PutCh( pdu[ 2 ] + '0' );
                break;

            default:
                Printf( "[F-%02X]", pdu[ 2 ] );
                break;
            }

        Update ();

        if ( hasDisplay == GRAPHICAL_DISPLAY )
        {
            ClearDisplayGraphics( 180, 12, 239, 22 );
            WriteDisplayGraphics( 180, 12, ( 3 << 5 ) | 2, userInput );
            }
        }
    else if ( state == ACTIVE && pdu[ 1 ] == FNC_PRGFNCACT ) // On PRGFNCACT
    {
        int keyCode = 0x100 | pdu[ 2 ];

        switch( keyCode )
        {
            case KEY_PRGFN_MENU1: // up
                if ( curRow > 0 )
                    --curRow;
                break;

            case KEY_PRGFN_MENU2: // left
                if ( curCol > 0 )
                    --curCol;
                break;

            case KEY_PRGFN_MENU3: // right
                if ( curCol < colCount - 1 )
                    ++curCol;
                break;

            case KEY_PRGFN_MENU4: // down
                if ( curRow < rowCount - 1 )
                    ++curRow;
                break;

            case KEY_PRGFN_MENU:
                DisplayMenu ();
                break;

            case KEY_PRGFN_PROGRAM:
                for ( int i = 0; i <= 9; i++ )
                {
                    SetLED( 0x0F + i );
                    }
                for ( int i = 9; i >= 0; i-- )
                {
                    ClearLED( 0x0F + i );
                    }
                time_t t1, t2;
                t1 = time( NULL );
                for ( long i = 0; i < 30; i++ )
                {
                    Goto( 0, 0 );
                    Printf( "%06ld %06ld\n", i, i );
                    Update ();
                    }
                t2 = time( NULL );
                fprintf( stderr, "Elapsed %lu seconds\n", t2 - t1 );
                break;

            case KEY_PRGFN_ACCESS1:
            case KEY_PRGFN_ACCESS2:
            case KEY_PRGFN_KEY1:
            case KEY_PRGFN_KEY2:

                userInput[ userInputLen ] = 0;
                fprintf( stderr, "User Input: [%s]\n", userInput );

                if ( connParams.find( ID ) )
                {
                    if ( userInputLen > 0 )
                        strcpy( connParams.DN, userInput );

                    autoReconnectCount = connParams.RETRYC;

                    if ( autoReconnectCount <= 0 )
                        autoReconnectCount = 5;

                    Connect( /* forceMaster if */ keyCode == KEY_PRGFN_ACCESS1 || keyCode == KEY_PRGFN_KEY2 );

                    // Reinitialize everything
                    //
                    EquipmentTestRequestReset ();
                    Go_State( DOWN, -1, "Connection Closed" );
                    }
                break;


            default:
                Printf( "[P-%02X]", pdu[ 2 ] );
                break;
            }

        Update ();
        }
    else if ( pdu[ 1 ] == FNC_EQULOCALTST ) 
    {
        if ( pdu[ 2 ] == 0 )
        {
            // Going back from local test mode
            ClearDisplay ();
            ClearLEDs ();

            DisplayMenu ();
            }
        }
    else if ( state != ACTIVE && pdu[ 1 ] == FNC_PRGFNCACT ) // On PRGFNCACT
    {
        if ( ! IsCTT10 )
            IsOWS = true;

        Connect( /*forceMaster*/ true );

        // Reinitialize everything
        //
        if ( IsCTT10 )
            EquipmentActivate ();
        else
            EquipmentTestRequestReset ();
        Go_State( DOWN, -1, "Connection Closed" );
        }
    else if ( IsCTT10 && pdu[ 1 ] == 0x05 ) // CTT10 login sequence entered
    {
        Go_State( HALF_UP, 3000, "EQUSTA Active" );

        // Remember userid
        //
        CTT10_userid_pdu_len = pdu_len;
        memcpy( CTT10_userid_pdu, pdu, pdu_len );

        bool rc = Connect( /*forceMaster*/ true );

        if ( ! rc ) // connection failed
        {
            // Report "System Unavailable"
            //
            static unsigned char PDU [] = { 0, 0x30, 0xA2, 0x02 }; // system unavailable
            PDU[ 0 ] = channel;
            write( f_D, PDU, sizeof( PDU ) );
            }
        else
        {
            // Reinitialize everything
            //
            EquipmentActivate ();
            Go_State( DOWN, -1, "Connection Closed" );
            }
#if 0
        Initialize( 0x01, 0x01, 0 );

        if ( pdu[ 2 ] != 0xFA ) // star
        {
            //static unsigned char PDU [] = { 0, 0x30, 0xA2, 0x01 }; // wrong terminal type
            static unsigned char PDU [] = { 0, 0x30, 0xA2, 0x02 }; // system unavailable
            //static unsigned char PDU [] = { 0, 0x30, 0xA2, 0x03 }; // already logged on
            //static unsigned char PDU [] = { 0, 0x30, 0xA2, 0x04 }; // invalid userid/password
            PDU[ 0 ] = channel;
            write( f_D, PDU, sizeof( PDU ) );
            }
        else
        {
            static unsigned char PDU [] = { 0, 0x30, 0xA1, 0x01, 0x0B, 0x00 };
            PDU[ 0 ] = channel;
            write( f_D, PDU, sizeof( PDU ) );

            usleep( 20000 );
            static unsigned char PDU2 [] = { 0, 0x30, 0x85, 0x00, 0x29, 0x0C,
                'P', 'r', 'e', 's', 's', ' ', 'L', 'i', 'n', 'e', '1', ' ', ' ', ' ', ' ', ' ' };
            PDU2[ 0 ] = channel;
            write( f_D, PDU2, sizeof( PDU2 ) );
            }
#endif
        }
    else
    {
        fprintf( stderr, "*** PDU [FNC=%02X] Ignored ***\n", pdu[ 1 ] );
        }
    }

///////////////////////////////////////////////////////////////////////////////

void DTS::OnTimeoutEvent( void )
{
    switch( state )
    {
        case DOWN:
            Go_State( DOWN, -1, "TIMEOUT" );
            break;

        case HALF_UP:
#if 0 // non CTT10
            EquipmentTestRequestReset ();
#else
            //CTT10_StatusRequest ();
#endif
            Go_State( HALF_UP, 3000, "TIMEOUT" );
            break;

        case PASSIVE:
            EquipmentTestRequestReset ();
            Go_State( HALF_UP, 3000, "TIMEOUT" );
            break;

        case ACTIVE:
            if ( statusRequested )
            {
                EquipmentTestRequestReset ();
                Go_State( HALF_UP, 3000, "TIMEOUT" );
                }
            else
            {
                statusRequested = true;
                EquipmentStatusRequest ();
                Go_State( ACTIVE, 1000, "TIMEOUT" );
                }
            break;

        case FAULTY:
            EquipmentTestRequestReset ();
            Go_State( FAULTY, 60000, "TIMEOUT" );
            break;
        }
    }

///////////////////////////////////////////////////////////////////////////////

void DTS::Main( void )
{
    Go_State( DOWN, 3000, "Initialize()" ); // CTT patch changed -1 to 3000

    if ( IsCTT10 )
        EquipmentActivate ();
    else
        EquipmentTestRequestErrorStatus (); // Get Loop Sync state

    unsigned char line[ 4096 ];

	for (;;)
	{
        int max_fd = -1;
        fd_set readfds;
        FD_ZERO( &readfds );
        if ( f_D >= 0 )
        {
            FD_SET( f_D, &readfds );
            if ( f_D > max_fd ) max_fd = f_D;
            }
        if ( tau.getFD () >= 0 )
        {
            FD_SET( tau.getFD (), &readfds );
            if ( tau.getFD () > max_fd ) max_fd = tau.getFD ();
            }
        if ( tau.getSock () >= 0 )
        {
            FD_SET( tau.getSock(), &readfds );
            if ( tau.getSock () > max_fd ) max_fd = tau.getSock ();
            }
        if ( max_fd < 0 )
        {
            fprintf( stderr, "There are no open file desriptors to poll.\n" );
            break;
            }

        timeval tvSelect;
        tvSelect.tv_sec = 0;
        tvSelect.tv_usec = 100000; // 100ms timeout

        int rc = select( max_fd + 1, &readfds, NULL, NULL, &tvSelect );
        if ( rc < 0 ) // Error
        {
            break;
            }
        else if ( rc == 0 ) // Timeout
        {
            // fprintf( stderr, "%10d\r", timer );

            tau.AbortFrame ();

            if ( timer >= 0 )
            {
                timer -= 100; // 100ms elapsed

                if ( timer < 0 )
                {
                    timer = -1;
                    fprintf( stderr, "---------------------------------\n" );
                    OnTimeoutEvent ();
                    }
                }

            continue;
            }

        ///////////////////////////////////////////////////////////////////////
        // D Channel from DTS
        ///////////////////////////////////////////////////////////////////////
        //
        if ( f_D >= 0 && FD_ISSET( f_D, &readfds ) )
        {
		    int rd = read_stream( f_D, line, 2 );

		    if ( 0 == rd ) // EOF
            {
			    fprintf( stderr, "%s: Got EOF.\n", dspboard );
			    break;
                }

		    if ( 2 != rd )
		    {
			    fprintf( stderr, "%s: Could not read packet length\n", dspboard );
			    break;
		        }

		    int len = ( line[ 0 ] << 8 )  + line[ 1 ];

		    rd = read_stream( f_D, line, len );

		    if ( 0 == rd )
			    break; // EOF

		    if ( len != rd )
		    {
			    fprintf( stderr, "%s: Expected %d, read %d\n", dspboard, len, rd );
			    break;
		        }

            ParseOutputPDU( line + 1, len - 1 );
            }

        ///////////////////////////////////////////////////////////////////////
        // TAU-D Listener TCP socket
        ///////////////////////////////////////////////////////////////////////
        //
        if ( tau.getSock() >= 0 && FD_ISSET( tau.getSock(), &readfds ) )
        {
            tau.AcceptClient ();
            }

        ///////////////////////////////////////////////////////////////////////
        // Octet from DTE
        ///////////////////////////////////////////////////////////////////////
        //
        if ( tau.getFD () >= 0 && FD_ISSET( tau.getFD(), &readfds ) )
        {
            unsigned char octet;
		    int rd = read( tau.getFD(), &octet, 1 );

		    if ( 0 == rd ) // EOF
            {
                tau.CloseConnection ();
                }
		    else if ( rd < 0 )
		    {
			    fprintf( stderr, "Could not read packet length\n" );
                tau.CloseConnection ();
		        }
            else
            {
                tau.OnReceivedOctet( octet );
                }
            }
        }
    }
