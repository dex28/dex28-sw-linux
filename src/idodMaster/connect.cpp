#include "DTS.h"

///////////////////////////////////////////////////////////////////////////////

bool DTS::Connect( bool forceMaster )
{
    bool RC = false;

    autoReconnect = false;
    connParams.AUTOCONNECT = false;

    const char* hostname = connParams.IPADDR; 
    unsigned int port = connParams.TCPPORT;
    char* DN = connParams.DN;
    char* uid = connParams.UID;
    char* pwd = connParams.PWD;
    char* auth = "-";
    int udp_DSCP;
    int tcp_DSCP;
    PORT_PARAMETERS::GetConnParams( ID, localUdpPort, udp_DSCP, tcp_DSCP );

    // Report status
    //
    fprintf( stderr, "Connect: %s@%s:%u, uid=%s, pwd=%s, master=%d, UDP %hu, DSCP %02x/%02x, RetryC=%d\n", 
        DN, hostname, port, uid, pwd, forceMaster, localUdpPort, udp_DSCP, tcp_DSCP, autoReconnectCount );

    // Open UDP channel
    //
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
        // local UDP port is already used, allocate dummy port used just for sanding
        close( inUdpSocket );
	    inUdpSocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
        localUdpPort = 0;
        }

    if ( inUdpSocket >= 0 && udp_DSCP )
    {
        if ( setsockopt( inUdpSocket, IPPROTO_IP, IP_TOS, &udp_DSCP, sizeof( udp_DSCP ) ) < 0 )
        {
            Messagef( true, "setsock(UdpDSCP): %s", strerror(errno) );

            if ( inUdpSocket >= 0 )
                close( inUdpSocket );

            close( tcpSocket );
            return RC;
            }
        }

    // Open TCP channel
    //
    tcpSocket = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP ); // Initialize socket descriptor

    sockaddr_in sin;
    hostent* host = gethostbyname( hostname );

    // Place data in sockaddr_in struct
    //
    memcpy( &sin.sin_addr.s_addr, host->h_addr, host->h_length );
    sin.sin_family = AF_INET;
    sin.sin_port = htons( port );

    // Place data in remote UDP sockaddr_in struct
    //
    remoteUdp.sin_family = AF_INET;
    memcpy( &remoteUdp.sin_addr.s_addr, host->h_addr, host->h_length );
    remoteUdpPort = 0; // no port at present (will be known after authorization)
    remoteUdp.sin_port = htons( remoteUdpPort );

    // Set non-blocking 
    //
    long tcpFL = fcntl( tcpSocket, F_GETFL, NULL ); 
    if ( tcpFL < 0) 
    { 
        Messagef( true, "fcntl(GETFL): %s", strerror(errno) );
        goto CLEANUP_EXIT;
        }

    if ( fcntl( tcpSocket, F_SETFL,tcpFL | O_NONBLOCK ) < 0) 
    { 
        Messagef( true, "fcntl(SETFL): %s", strerror(errno) );
        goto CLEANUP_EXIT;
        } 

    Messagef( true, "Connecting to %s", hostname );

    // Connect socket to the service described by sockaddr_in struct
    //
    if ( connect( tcpSocket, (sockaddr*)&sin, sizeof(sin) ) < 0 )
    {
        if ( errno != EINPROGRESS )
        {
            switch( errno )
            {
                case ETIMEDOUT:
                case ECONNREFUSED:
                case ENETUNREACH:
                case ENOTCONN:
                case EHOSTUNREACH:
                    autoReconnect = true;
                }

            Messagef( true, "Connect: %s", strerror(errno) );
            goto CLEANUP_EXIT;
            }
        else // EINPROGRESS: async wait to be connected
        {
            for ( ;; )
            { 
                int max_fd = tcpSocket;
                struct timeval tvSelect;
                tvSelect.tv_sec = 8;
                tvSelect.tv_usec = 0;
 
                fd_set writefds; 
                FD_ZERO( &writefds ); 
                FD_SET( tcpSocket, &writefds ); 
                int rc = select( max_fd + 1, NULL, &writefds, NULL, &tvSelect );
                
                if ( rc < 0 && errno != EINTR ) // error
                { 
                    Messagef( true, "select(conn): %s", strerror(errno) );
                    goto CLEANUP_EXIT;
                    } 
                else if ( rc == 0 ) // timeout
                {
                    autoReconnect = true;
                    Messagef( true, "Connect [%s] timeout", hostname );
                    goto CLEANUP_EXIT;
                    }
                    
                if ( FD_ISSET( tcpSocket, &writefds ) ) // Socket selected for write 
                { 
                    int valopt;
                    socklen_t lon = sizeof(valopt);
                    if ( getsockopt( tcpSocket, SOL_SOCKET, SO_ERROR, &valopt, &lon ) < 0 )
                    { 
                        Messagef( true, "getsockopt: %s", strerror(errno) );
                        goto CLEANUP_EXIT;
                        }
                        
                    if ( valopt == 0 ) // connected
                        break;
                        
                    if ( valopt != EINPROGRESS )
                    { 
                        switch( valopt )
                        {
                            case ETIMEDOUT:
                            case ECONNREFUSED:
                            case ENETUNREACH:
                            case ENOTCONN:
                            case EHOSTUNREACH:
                                autoReconnect = true;
                            }
                        Messagef( true, "%s [%s]", strerror(valopt), hostname );
                        goto CLEANUP_EXIT;
                        }
                    } 
                }
            } 
        }

    if ( fcntl( tcpSocket, F_SETFL,tcpFL | O_NONBLOCK ) < 0)
    { 
        Messagef( true, "fcntl(SETFL): %s", strerror(errno) );
        goto CLEANUP_EXIT;
        } 

    // Set Diffserv codepoint
    //
    if ( tcp_DSCP )
    {
        if ( setsockopt( tcpSocket, IPPROTO_IP, IP_TOS, &tcp_DSCP, sizeof( tcp_DSCP ) ) < 0 )
        {
            Messagef( true, "setsock(TcpDSCP): %s", strerror(errno) );
            goto CLEANUP_EXIT;
            }
        }

    for( ;; )
    {
        int max_fd = tcpSocket;
        fd_set readfds;
        FD_ZERO( &readfds );
        FD_SET( tcpSocket, &readfds );

        timeval tvSelect;
        tvSelect.tv_sec = 8; // 8 sec timeout
        tvSelect.tv_usec = 0;

        int rc = select( max_fd + 1, &readfds, NULL, NULL, &tvSelect );

        if ( rc < 0 && errno != EINTR ) // error
        {
            Messagef( true, "select(prot): %s", strerror(errno) );
            goto CLEANUP_EXIT;
            }
        else if ( rc == 0 ) // timeout
        {
            Messagef( true, "Connect [%s] protocol timeout", hostname );
            goto CLEANUP_EXIT;
            }

        if ( FD_ISSET( tcpSocket, &readfds ) )
        {
            char buffer[ 512 ];
            int len = read_stream_CRLF( tcpSocket, buffer, sizeof( buffer ) );
            // fprintf( stderr, "Got %d bytes: %-.*s", len, len, buffer );
    
            if ( len <= 0 )
            {
                goto CLEANUP_EXIT;
                }
    
            ///////////////////////////////////////////////////////////////////////
            // Expect: HELLO <challenge> [ <majorProtVer>.<minorProtVer> ]
            //
            char challenge[ 80 ];
            int majorProtVer = 1; // default remote protocol version is 1.0
            int minorProtVer = 0;
            int fieldc = sscanf( buffer, "HELLO %32s %d.%d", challenge, &majorProtVer, &minorProtVer );
            if ( fieldc >= 1 )
            {
                D4oIPver = ( majorProtVer << 8 ) | minorProtVer;
    
                MD5 md5ctx;
                md5ctx.Init ();
                md5ctx.Update( challenge, 32 );
                md5ctx.Update( pwd, strlen( pwd ) );
                char challengeResp[ 80 ];
                md5ctx.FinalHex( challengeResp );
                challengeResp[ 64 ] = 0;
    
                ///////////////////////////////////////////////////////////////////
                // Respond: HELLO <uid> <resp> <udpPort> <dn> <auth> <majorProtVer>.<minorProtVer>
                //
                char response[ 256 ];
                int responseLen = sprintf( response, "HELLO %s %s %hu %s %s 1.1\r\n", 
                    uid, challengeResp, localUdpPort, DN ? DN : "-", auth
                    );
                send( tcpSocket, response, responseLen, 0 );
    
                Messagef( false, "Sending credentials" );
                }
    
            ///////////////////////////////////////////////////////////////////////
            // Expect: OK <udpPort>
            //
            if ( 1 == sscanf( buffer, "OK %hu", &remoteUdpPort ) )
            {
                Messagef( false, "Connected to %s", hostname );

                if ( remoteUdpPort == 0 && forceMaster ) // not Master DTS, but should be forced
                {
                    // Send kill message to all remote "idod"s
                    unsigned char pdu [] = { 0x00, 0x02, 0x00, FNC_KILL_REMOTE_IDODS };
                    send( tcpSocket, pdu, sizeof( pdu ), 0 );
    
                    Messagef( true, "Killing other DTSs" );
    
                    autoReconnect = true;
    
                    continue; // wait close of the tcp connection
                    }
    
                remoteUdp.sin_port = htons( remoteUdpPort );
    
                Messagef( false, "Starting ELP2B+D passthru" );
    
                ClearCursor ();
    
                // Update HPIDEV connection information
                //
                sprintf( buffer, "%-2d %-14s %5u %-8s %-8s %s V1.1/%d.%d",
                    ID + 1, hostname, port, DN, uid, forceMaster ? "[E]" : "[S]",
                    ( D4oIPver >> 8 ) & 0xFF, D4oIPver & 0xFF 
                    );
    
                ioctl( f_D, IOCTL_HPI_SET_CHANNEL_INFO, 1 );
                write( f_D, buffer, strlen( buffer ) );
    
                // Passthru connection
                //
                tau.Reset ();
                tau.SetStatus_DTS_Connected( true );
                tau.SetStatus_PBX_Connected( true );
                Passthru ();
                tau.SetStatus_PBX_Connected( false );
                tau.SetStatus_DTS_Connected( false );
    
                ioctl( f_D, IOCTL_HPI_SET_CHANNEL_INFO, 0 ); // clear channel info
    
                RC = true;
                goto CLEANUP_EXIT;
                }
    
            ///////////////////////////////////////////////////////////////////////
            // Expect: ERROR <message>
            //
            if ( strncmp( buffer, "ERROR ", 6 ) == 0 )
            {
                Messagef( true, buffer + 6 );
                goto CLEANUP_EXIT;
                }
            }
        }

CLEANUP_EXIT:
    
    if ( inUdpSocket >= 0 )
        close( inUdpSocket );

    close( tcpSocket );
    return RC;
    }
