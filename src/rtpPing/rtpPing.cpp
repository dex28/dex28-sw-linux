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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h> // IPTOS_*

#include <math.h>
#include <pthread.h>

using namespace std;

int pkt_count = 200;
int pkt_interval = 20;
int pkt_size = 160;

int outUdpSocket = -1;

sockaddr_in local;
sockaddr_in remote;

volatile int pkts_sent = 0;
volatile int pkts_received = 0;

volatile int lastSentSeqNo = 0;
volatile int lastReceivedSeqNo = 0;

timeval* tv_sent = NULL;

volatile double min_rtt = 1e6;
volatile double max_rtt = 0;
volatile double avg_rtt = 0;
volatile double var_rtt = 0;
volatile double last_rtt = 0;

int histogram[ 1000 ] = { 0 };

void* Packet_Receiver( void* )
{
    unsigned char pkt[ 4096 ];

    int maxfds = outUdpSocket;
    for (;;)
    {
        fd_set readfds;
        FD_ZERO( &readfds );
        FD_SET( outUdpSocket, &readfds );

        if ( ! select( maxfds + 1, &readfds, NULL, NULL, NULL ) )
            continue;

        if ( FD_ISSET( outUdpSocket, &readfds ) )
        {
            sockaddr_in from;
            socklen_t fromlen = sizeof( from );
            int retval = recvfrom( outUdpSocket, (char*)pkt, sizeof( pkt ), 0,
                (sockaddr*) &from, &fromlen );

            if ( retval >= 1 )
            {
                // Got packet
                ++pkts_received;
                int seqno = ( pkt[ 1 ] << 8 ) + pkt[ 2 ];
                timeval tv_rcvd;
                gettimeofday( &tv_rcvd, NULL );

                // printf( "--> %5d %lu.%lu\n", seqno, tv_rcvd.tv_sec, tv_rcvd.tv_usec );

                double rtt // in ms
                    = double( tv_rcvd.tv_sec - tv_sent[ seqno ].tv_sec ) * 1e3
                    + double( tv_rcvd.tv_usec - tv_sent[ seqno ].tv_usec ) / 1e3;

                if ( rtt < 0 ) histogram[ 0 ]++;
                else if ( rtt > 1000 ) histogram[ 999 ]++;
                else histogram[ int( rtt ) ]++;

                last_rtt = rtt;
                if ( rtt < min_rtt ) min_rtt = rtt;
                if ( rtt > max_rtt ) max_rtt = rtt;
                double alpha = 1.0 / pkts_received;
                avg_rtt = avg_rtt * ( 1.0 - alpha ) + rtt * alpha;
                double var = rtt - avg_rtt;
                if ( var < 0 ) var = -var;
                var_rtt = var_rtt * ( 1.0 - alpha ) + var * alpha;

                lastReceivedSeqNo = seqno;
                }
            }
        }

    return NULL;
    }

void* Packet_Sender( void* )
{
/*
    int tos = IPTOS_LOWDELAY;

    setsockopt( outUdpSocket, IPPROTO_IP, IP_TOS, &tos, sizeof( tos ) );
*/
    unsigned char pkt[ 4096 ];
    for ( int i = 0; i < pkt_size; i++ )
        pkt[ 3 + i ] = 0xAA;

    for( int i = 0; i < pkt_count; i++ )
    {
        pkt[ 0 ] = 0xFF; // payload
        pkt[ 1 ] = ( i >> 8 ) & 0xFF; // sequence number
        pkt[ 2 ] = i & 0xFF;

        sendto( outUdpSocket, (char*)pkt, 3 + pkt_size, 0, (sockaddr*)&remote, sizeof( remote ) );

        ++pkts_sent;
        lastSentSeqNo = ( pkt[ 1 ] << 8 ) + pkt[ 2 ];
        gettimeofday( &tv_sent[ i ], NULL );

        // printf( "<-- %5d %lu.%lu\n", i, tv_sent[ i ].tv_sec, tv_sent[ i ].tv_usec );

        usleep( ( pkt_interval - 3 ) * 1000 ); // in ms
        }

    return NULL;
    }

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        printf( "usage: %s ipaddr pktcount pktsize delay\n", argv[0] );
        return 1;
        }

    char* remoteIpAddr = argv[ 1 ];

    if ( argc >= 3 )
        sscanf( argv[ 2 ], "%d", &pkt_count );

    if ( argc >= 4 )
        sscanf( argv[ 3 ], "%d", &pkt_size );

    if ( argc >= 5 )
        sscanf( argv[ 4 ], "%d", &pkt_interval );

    unsigned short localUdpPort = 20001;
    unsigned short remoteUdpPort = 20000;

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY; // OR: inet_addr( "interface" ); 
    local.sin_port = htons( localUdpPort ); // Port MUST be in Network Byte Order
    remote.sin_family = AF_INET;
    inet_aton( remoteIpAddr, &remote.sin_addr );
    remote.sin_port = htons( remoteUdpPort );

     outUdpSocket = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ); // Open inbound UDP socket
    if ( outUdpSocket < 0 )
    {
        return 1;
        }
    else if ( bind( outUdpSocket, (sockaddr*)&local, sizeof( local ) ) != 0 )
    {
        return 1;
        }

    tv_sent = new timeval[ pkt_count ];
    memset( tv_sent, 0, sizeof( timeval ) * pkt_count );

    pthread_t thread_rcv;
    if ( 0 != pthread_create( &thread_rcv, NULL, Packet_Receiver, NULL) )
        return 1;

    pthread_t thread_snd;
    if ( 0 != pthread_create( &thread_snd, NULL, Packet_Sender, NULL) )
        return 1;

    int last_pkts_received = 0;

    while( pkts_sent < pkt_count )
    {
        sleep( 1 );

        printf( "%3d Tx%5d Rx%5d (%4.1lf%%) "
            "Min %5.1lf Max %5.1lf Avg %5.1lf Var %5.1lf\n", 
            pkts_received - last_pkts_received, pkts_sent, pkts_received, 
            double( pkts_received ) / pkts_sent * 100.0,
            min_rtt, max_rtt, avg_rtt, var_rtt
            );

        last_pkts_received = pkts_received;
        }

    pthread_join( thread_snd, NULL );

    sleep( 1 );

    // pthread_join( thread_rcv, NULL );

    int sum_above_var = 0;

    const double Beta = 4.0;
    int max_hist = 0;
    for ( int i = 0; i < 1000; i++ )
        if ( histogram[ i ] > max_hist )
            max_hist = histogram[ i ];

    FILE* f = fopen( "hist.txt", "w" );
    for ( int i = int( min_rtt ); i < int( max_rtt ) + 1 && i < 1000; i++ )
    {
        fprintf( f, "%s%4d%5d ",  
            i == int( avg_rtt ) ? "*" : 
            i == int( avg_rtt + Beta * var_rtt ) ? ">" : " ",
            i, histogram[ i ]
            );
        for ( int j = 0; j < int(histogram[ i ]* 70.0/max_hist); j++ )
            fprintf( f, "*" );
        fprintf( f, "\n" );
        if ( i >= int( avg_rtt + Beta * var_rtt ) )
            sum_above_var += histogram[ i ];
        }

    fprintf( f, "%d packets above variance\n", sum_above_var );
    fclose( f );

    close( outUdpSocket );

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
