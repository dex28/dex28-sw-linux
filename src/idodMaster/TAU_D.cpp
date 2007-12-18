
#include "DTS.h"

#define TAU_D_REVID "TAU D R5A   "

///////////////////////////////////////////////////////////////////////////////

TAU_D::TAU_D( void )
{
    sockfd      = -1;
    fd          = -1;

    mode        = MODE_TRANSPARENT;
    state       = WAIT_FLG1;
    sn_from_DTS = 0x7;
    sn_from_PBX = 0x7;
    sn_to_DTE   = 0xF;
    sn_from_DTE = -1;
    }

TAU_D::~TAU_D( void )
{
    CloseConnection ();

    if ( sockfd >= 0 )
        close( sockfd );
    }

///////////////////////////////////////////////////////////////////////////////

bool TAU_D::OpenSerial( const char* port_name )
{
    CloseConnection ();

    printf( "TAU D: Openning serial port %s\n", port_name );

    fd = open( port_name, O_RDWR | O_NONBLOCK | O_NOCTTY ) ;
    if ( fd < 0 )
    {
        perror( "opening port" );
        return false;
        }

    // Save the current port setup
    // tcgetattr( fd, &oldTermio );

    termios Termio = { 0 };

    // Set input mode: ignore breaks, ignore parity errors, do not strip chars,
    // no CR/NL conversion, no case conversion, no XON/XOFF control,
    // no start/stop
    Termio.c_iflag = IGNBRK | IGNPAR;
    // Set output mode: no post process output, 
    Termio.c_oflag = 0;
    // Set line discipline
    Termio.c_lflag = 0;
    // Set control mode: 9600,8,n,1
    Termio.c_cflag = CS8 | CREAD | CLOCAL;
    Termio.c_cflag &= ~CBAUD;
    Termio.c_cflag |= B9600;
    
    // Set the default paramaters
    tcsetattr( fd, TCSANOW, &Termio );

    return true;
    }

bool TAU_D::OpenTCP( int portno )
{
    CloseConnection ();

    if ( sockfd >= 0 )
        close( sockfd );

    sockfd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sockfd < 0 )
    {
        perror( "creating socket" );
        return false;
        }

    struct sockaddr_in serv_addr;
    memset( &serv_addr, 0, sizeof(serv_addr) );
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons( portno );

    int oldopts = fcntl( sockfd, F_GETFL, 0 );
    fcntl( sockfd, F_SETFL, oldopts | O_NONBLOCK );

    int one = 1;
    setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one) ) ;

    printf( "TAU D: Listening on TCP port %d\n", portno );

    if( bind( sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr) ) < 0 )
    {
        perror( "binding socket" ); 
        return false;
        }

    listen( sockfd, 5 );

    fprintf( stderr, "TAU-D: Port %d, open\n", portno );
    return true;
    }

bool TAU_D::AcceptClient( void )
{
    if ( fd >= 0 )
        close( fd );

    socklen_t clilen = sizeof( cli_addr );
    fd = accept( sockfd, (struct sockaddr*)&cli_addr, &clilen );
    if ( fd < 0 )
    {
        perror( "accepting" );
        return false;
        }

    fprintf( stderr, "TAU-D: Accepted TCP Client: %s:%d\n", 
        inet_ntoa( cli_addr.sin_addr ), int( ntohs( cli_addr.sin_port ) ) );

    return true;    
    }

void TAU_D::CloseConnection( void )
{
    // restore the original terminal settings
    // tcsetattr( fd, TCSANOW, &oldTermio );

    if ( fd >= 0 )
    {
        fprintf( stderr, "TAU-D: Connection closed.\n" );
        close( fd );
        }

    fd = -1;
    }

///////////////////////////////////////////////////////////////////////////////

void TAU_D::SendFrame( int ctl, int addr, unsigned char* buf, int len )
{
    if ( fd < 0 )
        return;

    ++sn_to_DTE;

    int cs = 0;

    char line[ 64 ];
    line[ 0 ] = FRM_FLG1;
    line[ 1 ] = FRM_FLG2;

    line[ 2 ] = 4 + len;
    cs += line[ 2 ];

    line[ 3 ] = ( ( sn_to_DTE & FRM_SN_MASK ) << FRM_SN_SHIFT ) 
              | ( ctl & FRM_CTL_MASK );
    cs += line[ 3 ];

    line[ 4 ] = addr;
    cs += line[ 4 ];

    for ( int i = 0; i < len; i++ )
    {
        line[ 5 + i ] = buf[ i ];
        cs += buf[ i ];
        }

    line[ 5 + len ] = ( cs - 1 ) & 0xFF;

    write( fd, line, 6 + len );
#if 1
    printf( "TAU -> DTE:" );
    for ( int i = 2; i < 6 + len; i++ )
    {
        if ( i == 3 )
            printf( " \033[46;1m %02x \033[40;0m", line[ i ] & 0xFF );
        else
            printf( " %02x", line[ i ] & 0xFF );
        }
    switch( ctl )
    {
        case FRM_CTL_DATA: 
            printf( "\t(Data \033[1mfrom %s\033[0m)", addr ? "DTS" : "PBX" ); 
            break;
        case FRM_CTL_DATA_ACK: 
            printf( "\t(Data Ack)" ); 
            break;
        case FRM_CTL_STATUS_RESP: 
            printf( "\t(Status Resp)" ); 
            break;
        case FRM_CTL_ID_RESP: 
            printf( "\t(ID Resp)" ); 
            break;
        case FRM_CTL_COMMAND_ACK: 
            printf( "\t(Command Ack)" ); 
            break;
        case FRM_CTL_TEST_REPORT: 
            printf( "\t(Test Report)" ); 
            break;
        }
    printf( "\n" );
#endif
    }

bool TAU_D::OnReceivedOctet( int octet )
{
    bool has_data = false;

    // printf( "%s: Got Octet %02x\n", VerboseState(), octet );
        
    switch( state )
    {
        case WAIT_FLG1:
            if ( octet != FRM_FLG1 )
                AbortFrame ();
            else
                state = WAIT_FLG2;
            break;

        case WAIT_FLG2:
            if ( octet != FRM_FLG2 )
                AbortFrame ();
            else
                state = WAIT_BC;
            break;

        case WAIT_BC:
            CS = octet;
            if ( octet < 4 || octet > 51 )
                AbortFrame ();
            else
            {
                data_len = octet - 4;
                data_p = 0;
                state = WAIT_CTL;
            }
            break;

        case WAIT_CTL:
            CS += octet;
            CTL = octet;
            state = WAIT_ADDR;
            break;

        case WAIT_ADDR:
            CS += octet;
            ADDR = octet;
            if ( data_len == 0 )
                state = WAIT_CS;
            else
                state = WAIT_DATA;
            break;

        case WAIT_DATA:
            CS += octet;
            data[ data_p++ ] = octet;
            if ( data_p >= data_len )
                state = WAIT_CS;
            break;

        case WAIT_CS:
            if ( ( ( CS - 1 ) & 0xFF ) != octet )
            {
                fprintf( stderr, 
                    "TAU-D: Invalid checksum; Calculated %02x, received %02x.\n", 
                    ( CS - 1 ) & 0xFF, octet );
                AbortFrame ();
                }
            else
            {
#if 1
                printf( "DTE -> TAU: %02x \033[42;1m %02x \033[40;0m %02x", data_len + 4, CTL, ADDR );
                for ( int i = 0; i < data_len; i++ )
                    printf( " %02x", data[ i ] & 0xFF );
                printf( " %02x", octet ); // CS
#endif
                int sn = ( ( CTL >> FRM_SN_SHIFT ) & FRM_SN_MASK ); 

                switch( CTL & FRM_CTL_MASK )
                {
                    case FRM_CTL_DATA:
                        printf( "\t(Data \033[1mto %s\033[0m)\n", ADDR ? "DTS" : "PBX" );

                        if ( mode & DATA_ACK_ENABLE ) // Opt. send ackonwledge
                            SendFrame( FRM_CTL_DATA_ACK );

                        // Forward data to DTS or to PBX, depending on ADDR,
                        // but ignore duplicated frames.
                        //
                        has_data = ( sn != sn_from_DTE )
                            && ( ADDR == ADDR_DTS || ADDR == ADDR_PBX );

                        break;

                    case FRM_CTL_DATA_ACK:
                        printf( "\t(Data Ack)\n" );
                        break;

                    case FRM_CTL_STATUS_REQ:
                        printf( "\t(Status Req)\n" );
                        data[ 0 ] = mode;
                        SendFrame( FRM_CTL_STATUS_RESP, 0, data, 1 );
                        break;

                    case FRM_CTL_ID_REQ:
                        printf( "\t(ID Req)\n" );
                        SendFrame( FRM_CTL_ID_RESP, 0, (unsigned char*)TAU_D_REVID, 12 );
                        break;

                    case FRM_CTL_COMMAND:
                        printf( "\t(Command %02x)\n", data[0] & 0xFF );
                        SetMode( data[0] );
                        SendFrame( FRM_CTL_COMMAND_ACK );
                        break;

                    case FRM_CTL_TEST_REQ:
                        printf( "\t(Test Req %02x)\n", data[0] & 0xFF );
                        break;

                    default:
                        printf( "\n" );
                        fprintf( stderr, "TAU-D: Received frame with invalid control octet.\n" );
                    }
                
                sn_from_DTE = sn;
                }

            state = WAIT_FLG1;
            break;
        }

    // printf( "----> %s\n", VerboseState() );

    return has_data;
    }
