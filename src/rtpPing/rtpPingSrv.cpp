#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef WINDOWS

#include <unistd.h>
#include <sys/select.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h> // IPTOS_*

#define INVALID_SOCKET -1
#define SOCKET int

#else

#include <winsock2.h>

#define close closesocket
#define socklen_t int

#endif

int main( int argc, char** argv )
{
#ifdef WINDOWS
    WORD wVersionRequested;
    wVersionRequested = MAKEWORD( 1, 1 );

    WSADATA wsaData;
    int err = WSAStartup( wVersionRequested, &wsaData );
    if ( err != 0 ) {
        printf( "no usable winsock.dll" );
        return 1;
    }
#endif

    unsigned short localUdpPort = 20000;

    sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY; // OR: inet_addr( "interface" ); 
    local.sin_port = htons( localUdpPort ); // Port MUST be in Network Byte Order

    SOCKET inUdpSocket = socket( PF_INET, SOCK_DGRAM, 0 ); // Open inbound UDP socket
    if ( inUdpSocket == INVALID_SOCKET  )
    {
        printf( "Failed to open socket\n" );
        return 1;
        }
    else if ( bind( inUdpSocket, (sockaddr*)&local, sizeof( local ) ) != 0 )
    {
        printf( "Failed to bind socket\n" );
        return 1;
        }

    printf( "OK %hu\r\n", localUdpPort ); fflush( stdout );
/*
    int tos = IPTOS_LOWDELAY;

    setsockopt( inUdpSocket, IPPROTO_IP, IP_TOS, &tos, sizeof( tos ) );
*/
    unsigned char pkt[ 4096 ];

    int maxfds = inUdpSocket;

    int pkt_count = 0;

    for (;;)
    {
        fd_set readfds;
        FD_ZERO( &readfds );
        FD_SET( inUdpSocket, &readfds );

        if ( ! select( maxfds + 1, &readfds, NULL, NULL, NULL ) )
            continue;

        ///////////////////////////////////////////////////////////////////////
        // B Channel from remote DTS
        ///////////////////////////////////////////////////////////////////////
        //
        if ( FD_ISSET( inUdpSocket, &readfds ) )
        {
            sockaddr_in from;
            socklen_t fromlen = sizeof( from );
            int retval = recvfrom( inUdpSocket, (char*)pkt, sizeof( pkt ), 0,
                (sockaddr*) &from, &fromlen );

            // printf( "SRV Got %d bytes\n", retval );

            if ( retval >= 1 )
            {
                sendto( inUdpSocket, (char*)pkt, retval, 0, (sockaddr*)&from, sizeof( from ) );
                if ( ( pkt_count % 50 ) == 0 )
                    fprintf( stderr, "." );
                }
            }
        }

    close( inUdpSocket );

    return 0;
    }

void operator delete [] (void *p)
{
    free( p );
    }

void operator delete (void *p)
{
    free( p );
    }

void* operator new (size_t t)
{
    void* p = calloc( 1, t );
    return p;
    }

void* operator new[] (size_t t)
{
    void* p = calloc( 1, t );
    return p;
    }
