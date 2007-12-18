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
#include "xpi.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h> // IPTOS_*

int main( int argc, char** argv )
{
    int f_D = open( "/dev/xpi", O_RDWR );
    if ( f_D < 0 )
        return 0;

    ioctl( f_D, IOCTL_XPI_SET_CHANNEL, 1 ); // select channel (pcm)

    ioctl( f_D, IOCTL_XPI_SET_PCM1_SOURCE, argc >= 2 ? atol( argv[ 1 ] ) : 1 ); // select PCM

    int tslot = argc >= 3 ? atol( argv[ 2 ] ) : 0;
    ioctl( f_D, IOCTL_XPI_SET_PCM1_TS, tslot ); // select TS
    int t1 = argc >= 3 ? -1 : 0;

//
    unsigned short localUdpPort = 15000;

    sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY; // OR: inet_addr( "interface" ); 
    local.sin_port = htons( localUdpPort ); // Port MUST be in Network Byte Order

    int inUdpSocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Open inbound UDP socket
    if ( inUdpSocket < 0 )
    {
        localUdpPort = 0;
        }
    else if ( bind( inUdpSocket, (sockaddr*)&local, sizeof( local ) ) != 0 )
    {
        // Inbound UDP port associated to channel is already used.
        // Say that we do not have local udp port, and allocate any free UDP used just as outbound.
        localUdpPort = 0;
        close( inUdpSocket );
        inUdpSocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ); // UDP socket just for sending
        }

    printf( "Local UDP %hu\r\n", localUdpPort ); fflush( stdout );

    char* remoteIpAddr = "10.0.1.20";
    unsigned short remoteUdpPort = 15000;

    sockaddr_in remote;
    remote.sin_family = AF_INET;
    inet_aton( remoteIpAddr, &remote.sin_addr );
    remote.sin_port = htons( remoteUdpPort );
//

    int rtp_seqno = 0;

    for (;;)
    {
        unsigned char pcm[ 163 ];
        int pcm_len = 160;

        while( pcm_len > 0 )
        {
            int rd = read( f_D, pcm + 3, 160 );
            if ( rd <= 0 )
                return -1;
            pcm_len -= rd;
            }

        pcm[ 0 ] = 0x00; // Codec = ALaw
        pcm[ 1 ] = ( rtp_seqno >> 8 ) & 0xFF;
        pcm[ 2 ] = rtp_seqno & 0xFF;
        ++rtp_seqno;

        bool empty = true;
        for ( int i = 0; i < 160; i++ )
        {
            if ( pcm[i+3] != 0xFF && pcm[i+3] != 0x54 )
            {
                empty = false;
                break;
                }
            }

        if ( ! empty )
        {
            sendto( inUdpSocket, pcm, 160 + 3, 0, (sockaddr*)&remote, sizeof( remote ) );

            printf( "%3d ", tslot );
            for ( int i = 0; i < 16; i++ )
                printf( " %02x", (int)pcm[i+3] );
            printf( "\r" );
            }
        else if ( t1 >= 0  )
        {
            printf( "%3d time slot empty", tslot );
            t1 = 100;
            }
        else
        {
            printf( "%3d time slot empty\r", tslot );
            }
        
        if ( t1 >= 0 && ++t1 >= 100 )
        {
            t1 = 0;
            if ( ++tslot == 64 ) tslot = 0;
            ioctl( f_D, IOCTL_XPI_SET_PCM1_TS, tslot ); // select TS
            printf( "\n" );
            }

        fflush( stdout );
        }

    return 0;
    }
