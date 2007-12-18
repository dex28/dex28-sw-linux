#include "DTS.h"

///////////////////////////////////////////////////////////////////////////////

void DTS::Passthru( void )
{
    bool remoteLoopSyncLost = false;
    int old_equsta = 0;

	f_B = open( dspboard, O_RDWR );
    if ( f_B < 0 )
        return;

    ::ioctl( f_B, IOCTL_HPI_SET_B_CHANNEL, channel );

    unsigned char line[ 4096 ];

    int menuKeyCounter = 0;
    autoReconnect = false;

    bool isMasterDTS = remoteUdpPort != 0;

    fprintf( stderr, "Connected as %s DTS (OWS = %d; CTT10 = %d); D4oIP v%d.%d\n", 
        isMasterDTS ? "Master" : "Slave", IsOWS, IsCTT10,
        ( D4oIPver >> 8 ) & 0xFF, D4oIPver & 0xFF );

    // Reset local DTS. This will force DTS initiation sequence.
    //
    if ( IsOWS )
    {
        write( tcpSocket, "\x00\x06\x40\x01\x80\x02\x01\x02", 8 );
        }
    else if ( IsCTT10 )
    {
        EquipmentActivate ();
/*
        if ( CTT10_userid_pdu_len > 0 )
        {
            fprintf( stderr, "Sending CTT10 userid\n" );
            line[ 0 ] = ( CTT10_userid_pdu_len >> 8 ) & 0xFF;
            line[ 1 ] = CTT10_userid_pdu_len & 0xFF;
            memcpy( line + 2, CTT10_userid_pdu, CTT10_userid_pdu_len );
            write( tcpSocket, line, CTT10_userid_pdu_len + 2 );
            }
*/
        }
    else
    {
        EquipmentTestRequestReset ();
        }

    MonotonicTimer heartBeat;

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
        if ( f_B >= 0 )
        {
            FD_SET( f_B, &readfds );
            if ( f_B > max_fd ) max_fd = f_B;
            }
        if ( tcpSocket >= 0 )
        {
            FD_SET( tcpSocket, & readfds );
            if ( tcpSocket > max_fd ) max_fd = tcpSocket;
            }
        if ( inUdpSocket >= 0 && localUdpPort != 0 )
        {
            FD_SET( inUdpSocket, &readfds );
            if ( inUdpSocket > max_fd ) max_fd = inUdpSocket;
            }
        if ( tau.getFD () >= 0 )
        {
            FD_SET( tau.getFD (), &readfds );
            if ( tau.getFD () > max_fd ) max_fd = tau.getFD ();
            }
        if ( tau.getSock () >= 0 )
        {
            FD_SET( tau.getSock (), &readfds );
            if ( tau.getSock () > max_fd ) max_fd = tau.getSock ();
            }

        timeval tvSelect;
        tvSelect.tv_sec = 0;
        tvSelect.tv_usec = 500000; // 500ms timeout

        int rc = select( max_fd + 1, &readfds, NULL, NULL, &tvSelect );

        if ( rc < 0 ) // error
        {
            goto EXIT_passthru;
            }

        // From D4oIP v1.1 there should be regular heartbeat
        //
        if ( D4oIPver >= 0x0101 )
        {
            long tElapsed = heartBeat.Elapsed ();
            if ( tElapsed > 5000 )
            {
                // Heartbeat is missing more than 5 sec
                //
                Messagef( true, "Heart-beat sense failed" );

                autoReconnect = true;
                autoReconnectCount = connParams.RETRYC;
                if ( autoReconnectCount <= 0 )
                    autoReconnectCount = 5;

                goto EXIT_passthru;
                }
            }

        if ( rc == 0 ) // timeout
            continue;

        ///////////////////////////////////////////////////////////////////////
        // B Channel from Local DTS
        ///////////////////////////////////////////////////////////////////////
        //
        if ( FD_ISSET( f_B, &readfds ) ) // B Channel from Local DTS
        {
		    int rd = read_stream( f_B, line, 2 );

		    if ( 0 == rd || 2 != rd )
            {
                Messagef( true, "Failed to read local B len." );
			    goto EXIT_passthru; // EOF
                }

		    int len = ( line[ 0 ] << 8 )  + line[ 1 ];

		    rd = read_stream( f_B, line + 2, len );

		    if ( 0 == rd || len != rd )
            {
                Messagef( true, "Failed to read local B data." );
			    goto EXIT_passthru; // EOF
                }

            int buflen = len - 1;
            unsigned char buf[ 256 ];
            memcpy( buf, line + 3, buflen );

            if ( remoteUdpPort != 0 )
            {
	            sendto( inUdpSocket, (char*)buf, buflen, 0, (sockaddr*)&remoteUdp, sizeof( remoteUdp ) );
                }
            }

        ///////////////////////////////////////////////////////////////////////
        // B Channel from Remote PBX (GW)
        ///////////////////////////////////////////////////////////////////////
        //
        if ( inUdpSocket >= 0 && FD_ISSET( inUdpSocket, &readfds ) )
        {
	        sockaddr_in from;
	        socklen_t fromlen = sizeof( from );
            int retval = recvfrom( inUdpSocket, (char*)line + 1, sizeof( line ), 0,
			    (sockaddr*) &from, &fromlen );

            if ( retval >= 1 )
            {
                line[ 0 ] = channel;
                write( f_B, line, retval + 1 );

                //printf( "Got B-Ch data len=%d from Remote PBX\n", retval );
                }
            }

        ///////////////////////////////////////////////////////////////////////
        // D Channel from DTS
        ///////////////////////////////////////////////////////////////////////
        //
        if ( FD_ISSET( f_D, &readfds ) )
        {
		    int rd = read_stream( f_D, line, 2 );

		    if ( 0 == rd || 2 != rd )
            {
                Messagef( true, "Failed to read local D len." );
			    goto EXIT_passthru; // EOF
                }

		    int len = ( line[ 0 ] << 8 ) + line[ 1 ];

		    rd = read_stream( f_D, line + 1, len );

		    if ( 0 == rd || len != rd )
            {
                Messagef( true, "Failed to read local D len." );
			    goto EXIT_passthru; // EOF
                }
#if 1
            fprintf( stderr, "DTS -> PBX:" );
            for ( int i = 2; i < len + 1; i++ )
                if ( i == 3 ) 
                    fprintf( stderr, " \033[41;1m %02X \033[40;0m", line[ i ] );
                else
                    fprintf( stderr, " %02X", line[ i ] );
            fprintf( stderr, "\n" );
#endif
            if ( line[ 3 ] == FNC_DASL_LOOP_SYNC ) // Local Loop Sync
            {
                old_equsta = 0;

                if ( line[ 4 ] != 0 && ! IsOWS & ! IsCTT10 ) // SYNC OK
                    write( tcpSocket, "\x00\x03\x00\x51\x00", 5 );
                }
            else if ( line[ 3 ] == FNC_XMIT_FLOW_CONTROL ) 
            {
                f_D_XON = line[ 4 ];

                fprintf( stderr, " \033[41;1m %s \033[40;0m\n", f_D_XON ? "XON" : "XOFF" );
                }
            else if ( ! isMasterDTS && line[ 3 ] == FNC_EQUSTA )
            {
                int cur_equsta = line[ 7 ] & 0x3F; // equipment status

                if ( cur_equsta == 0 ) // if EQUSTA passive, respond with EQUACT
                {
                    EquipmentActivate ();
                    }
                else // if EQUSTA active 
                {
                    // if D4 info, respond with EQUSTAD4REQ
                    if ( line[ 4 ] == 0x0E && ( line[ 12 ] & 0x0F ) == 0x03 )
                    {
                        EquipmentStatusD4Request( 0x00 );
                        }
                    else if ( old_equsta == 0 ) // last EQUSTA(active)
                    {
                        // Reset all connected DTSs. This will force DTS initiation sequence
                        // only on 'master' DTS.
                        unsigned char pdu [] = { 0x00, 0x02, 0x00, FNC_RESET_REMOTE_DTSS };
                        send( tcpSocket, pdu, sizeof( pdu ), 0 );

                        Messagef( true, "Connected as Monitor" );
                        }
                    }

                old_equsta = cur_equsta;
                }
            else if ( ! isMasterDTS && line[ 3 ] == FNC_EQUTESTRES )
            {
                }
            else if ( ! isMasterDTS && line[ 3 ] == FNC_EQULOCALTST )
            {
                if ( line[ 4 ] == 0 ) // back to normal mode
                {
                    }
                }
            else
            {
                len--; // trim channel id

                if ( tau.getFD () < 0 || tau.getMode() != 1 ) // Do not send PBX->DTS in PC Control mode
                {
                    line[ 0 ] = ( len >> 8 ) & 0xFF;
                    line[ 1 ] = len & 0xFF;
                    write( tcpSocket, line, len + 2 );
                    }

                if ( tau.getFD () >= 0  )
                {
                    // We should also copy ELU 2B+D signal from DTS to DTE
                    tau.SendDataFrame( /*addr=*/ 1, line + 2, len );
                    }

                if ( ! disable4C )
                {
                    if ( line[ 3 ] == FNC_FIXFNCACT && line[ 4 ] == KEY_FIXFN_CLEAR ) 
                    {
                        if ( ++menuKeyCounter >= 4 ) // 4x clear -> close connection
                        {
                            Messagef( true, "Ready." );
                            autoReconnect = false;
                            goto EXIT_passthru;
                            }
                        }
                    else
                    {
                        menuKeyCounter = 0; // reset "exit" counter
                        }
                    }
                }
            }

        ///////////////////////////////////////////////////////////////////////
        // D Channel from GW (PBX)
        ///////////////////////////////////////////////////////////////////////
        //
        if ( FD_ISSET( tcpSocket, &readfds ) )
        {
            int rd = read_stream( tcpSocket, line, 2 );

            if ( 0 == rd || 2 != rd )
            {
                // closed socket
                Messagef( true, "Remote party closed link." );
                goto EXIT_passthru; // EOF
                }

            heartBeat.Restart ();

		    int len = ( line[ 0 ] << 8 )  + line[ 1 ];

		    rd = read_stream( tcpSocket, line + 1, len );

		    if ( 0 == rd || len != rd )
            {
                Messagef( true, "Remote party closed link." );
			    goto EXIT_passthru; // EOF
                }
#if 1
            fprintf( stderr, "PBX -> DTS:" );
            for ( int i = 1; i < len + 1; i++ )
                if ( i == 2 )
                    fprintf( stderr, " \033[44;1m %02X \033[40;0m", line[ i ] );
                else
                    fprintf( stderr, " %02X", line[ i ] );
            fprintf( stderr, "\n" );
#endif
            if ( line[ 2 ] == FNC_DASL_LOOP_SYNC ) // Remote Loop Sync
            {
                if ( line[ 3 ] == 0x00 )
                {
                    Messagef( true, "Remote cable is disconnected." );

                    remoteLoopSyncLost = true;
                    }
                else if ( remoteLoopSyncLost )
                {
                    Messagef( false, "Remote cable is reconnected." );

                    if ( isMasterDTS )
                    {
                        // Reset local DTS. This will force DTS initiation sequence.

                        if ( IsOWS )
                            write( tcpSocket, "\x00\x06\x40\x01\x80\x02\x01\x02", 8 );
                        else if ( IsCTT10 )
                            ;//EquipmentActivate ();
                        else
                            EquipmentTestRequestReset ();
                        }

                    remoteLoopSyncLost = false;
                    }
                }
            else if ( line[ 2 ] == FNC_XMIT_FLOW_CONTROL ) 
            {
                f_D_XON = line[ 4 ];

                fprintf( stderr, " \033[44;1m %s \033[40;0m\n", f_D_XON ? "XON" : "XOFF" );
                }
            else if ( line[ 2 ] == FNC_HEARTBEAT )
            {
                }
            else if ( ! isMasterDTS && line[ 2 ] == FNC_EQUACT )
            {
                }
            else if ( ! isMasterDTS && line[ 2 ] == FNC_EQUTESTREQ )
            {
                }
            else if ( ! isMasterDTS && line[ 2 ] == FNC_EQUSTAD4REQ )
            {
                }
            else
            {
                if ( line[ 2 ] == FNC_TRANSLVLUPDATE ) // if PBX switches D3+ DTS mode on, update display info
                    D3_basic = false;

                if ( tau.getFD () < 0 || tau.getMode() != 1 ) // Do not send PBX->DTS in PC Control mode
                {
                    line[ 0 ] = channel; // prefix packet with channel
                    write( f_D, line, len + 1 );
                    }

                if ( tau.getFD () >= 0 )
                {
                    // We should also copy ELU 2B+D signal from PBX to DTE
                    tau.SendDataFrame( /*addr=*/ 0, line + 1, len );
                    }
                }
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
            else if ( tau.OnReceivedOctet( octet ) )
            {
                // Note: TAU-D data contains complete ELU 2B+D signal,
                // i.e. inclusive NBYTES and CS. We have to discard those when
                // sending to DSP fd (DTS) or network socket (remote PBX).
                //
                int pkt_len = tau.getDataLen () - 2; // remove NBYTES & CS from ELU2B+D signal

                if ( tau.getAddr () == 1 ) // Copy to DTS
                {
                    tau.getData () [ 0 ] =  channel; // Prefix packet with channel#
                    write( f_D, tau.getData (), pkt_len + 1 );
                    tau.getData () [ 1 ] &= ~0x0E; // Remove SN from OPC
#if 1
                    fprintf( stderr, "TAU -> DTS:" );
                    for ( int i = 1; i < pkt_len + 1; i++ )
                        if ( i == 2 )
                            fprintf( stderr, " \033[44;1m %02X \033[40;0m", tau.getData()[ i ] );
                        else
                            fprintf( stderr, " %02X", tau.getData()[ i ] );
                    fprintf( stderr, "\n" );
#endif
                    }
                else if ( tau.getAddr () == 0 ) // Copy to PBX
                {
                    unsigned char pkt[ 64 ];
                    pkt[ 0 ] = ( pkt_len >> 8 ) & 0xFF;
                    pkt[ 1 ] = ( pkt_len & 0xFF );
                    memcpy( pkt + 2, tau.getData () + 1, pkt_len ); // Starting from OPC
                    pkt[ 2 ] &= ~0x0E; // Remove SN from OPC

                    if ( pkt[ 2 ] == 0x20 && pkt[ 3 ] == 0x02 && pkt[ 4 ] == 0x60 )
                    {
                        if ( tau.getFD () >= 0 && tau.getMode () == 1 ) // in PC control mode
                        {
                            printf( "-------------- SENDING EQUSTA to PBX ---------------\n" );

                            // Send EQUSTA to PBX
                            write( tcpSocket, "\x00\x06\x40\x01\x80\x02\x01\x02", 8 );
                            }
                        }

                    write( tcpSocket, pkt, pkt_len + 2 );
#if 1
                    fprintf( stderr, "TAU -> PBX:" );
                    for ( int i = 2; i < pkt_len + 2; i++ )
                        if ( i == 3 ) 
                            fprintf( stderr, " \033[41;1m %02X \033[40;0m", pkt[ i ] );
                        else
                            fprintf( stderr, " %02X", pkt[ i ] );
                    fprintf( stderr, "\n" );
#endif
                    }
                }
            }
	    }

EXIT_passthru:

    close( f_B );
    }
