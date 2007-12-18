#include <cstdio>
#include <cctype>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <ctime>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/select.h>

#include <sys/ioctl.h>
#include "../hpi/hpi.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/time.h>

extern "C" {
#include "libpri.h"
    };


using namespace std;


static int offset = 0;

#define MAX_CHAN		32
#define DCHANNEL_TIMESLOT  16

struct pri_chan {
	pid_t pid;
	int needhangup;
	int alreadyhungup;
	q931_call *call;
} chans[MAX_CHAN];

static int str2node(char *node)
{
	if (!strcasecmp(node, "cpe"))
		return PRI_CPE;
	if (!strcasecmp(node, "network"))
		return PRI_NETWORK;
	return -1;
}



static void hangup_channel(int channo)
{
	if (chans[channo].pid) {

#if 0
		printf("Killing channel %d (pid = %d)\n", channo, chans[channo].pid);
#endif
		chans[channo].alreadyhungup = 1;
		kill(chans[channo].pid, SIGTERM);
	} else if (chans[channo].needhangup)
		chans[channo].needhangup = 0;
}


static void launch_channel(int channo)
{
	pid_t pid;
	char ch[80];

	/* Make sure hangup state is reset */
	chans[channo].needhangup = 0;
	chans[channo].alreadyhungup = 0;

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "--!! Unable to fork\n");
		chans[channo].needhangup = 1;
	}
	if (pid) {
		printf("-- Launching process %d to handle channel %d\n", pid, channo);
		chans[channo].pid = pid;
	} else {
		sprintf(ch, "%d", channo + offset);
	}

}


static int get_free_channel(int channo)
{
	channo--;
	if((channo>MAX_CHAN)||(channo<0)) {
		fprintf(stderr, "Invalid Bchannel RANGE <%d", channo);
		return 0;
	};
	
	while(chans[channo].pid) {
		channo--;
	}

	return channo;
}

/* place here criteria for completion of destination number */
static int number_incommplete(char *number)
{
  return strlen(number) < 3;
}

static void start_channel(struct pri *pri, pri_event *e)
{
	int channo = e->ring.channel;
	int		flag = 1;
	pri_event_ring	*ring = &e->ring;

	if(channo == -1) {
		channo = e->ring.channel = get_free_channel(MAX_CHAN);

		if(channo == DCHANNEL_TIMESLOT)
			channo = e->ring.channel = get_free_channel(MAX_CHAN);
		  
		
		fprintf(stdout, "Any channel selected: %d\n", channo);

		if(!channo) {
		  pri_release(pri, ring->call, PRI_CAUSE_REQUESTED_CHAN_UNAVAIL);
		  fprintf(stdout, "Abort call due to no avl B channels\n");
		  return;
		}

		flag = 0;
	}
	/* Make sure it's a valid number */
	if ((channo >= MAX_CHAN) || (channo < 0)) { 
		fprintf(stderr, "--!! Channel %d is out of range\n", channo);
		return;
	}

	/* Make sure nothing is there */
	if (chans[channo].pid) {
		fprintf(stderr, "--!! Channel %d still has a call on it, ending it...\n", channo);
		hangup_channel(channo);
		/* Wait for it to die */
		while(chans[channo].pid)
			usleep(100);
	}

	/* Record call number */
	chans[channo].call = e->ring.call;

	/* Answer the line */
	if(flag) {
		pri_answer(pri, chans[channo].call, channo, 1);
	} else {
		pri_need_more_info(pri, chans[channo].call, channo, 1);
	}

	/* Launch a process to handle it */
	launch_channel(channo);

}



static void handle_pri_event(struct pri *pri, pri_event *e)
{
	switch(e->e) {
	case PRI_EVENT_DCHAN_UP:
		printf("-- D-Channel is now up!  :-)\n");
		break;
	case PRI_EVENT_DCHAN_DOWN:
		printf("-- D-Channel is now down! :-(\n");
		break;
	case PRI_EVENT_RESTART:
		printf("-- Restarting channel %d\n", e->restart.channel);
		hangup_channel(e->restart.channel);
		break;
	case PRI_EVENT_CONFIG_ERR:
		printf("-- Configuration error detected: %s\n", e->err.err);
		break;
	case PRI_EVENT_RING:
		printf("-- Ring on channel %d (from %s to %s), answering...\n", e->ring.channel, e->ring.callingnum, e->ring.callednum);
		start_channel(pri, e);
		break;
	case PRI_EVENT_HANGUP:
		printf("-- Hanging up channel %d\n", e->hangup.channel);
		hangup_channel(e->hangup.channel);
		break;
	case PRI_EVENT_RINGING:
	case PRI_EVENT_ANSWER:
		fprintf(stderr, "--!! What?  We shouldn't be making any calls...\n");
		break;
	case PRI_EVENT_HANGUP_ACK:
		/* Ignore */
		break;
    case PRI_EVENT_HANGUP_REQ:
		/* Ignore */
        break;
	case PRI_EVENT_INFO_RECEIVED:
		fprintf(stdout, "number is: %s\n", e->ring.callednum);
		if(!number_incommplete(e->ring.callednum)) {
			fprintf(stdout, "final number is: %s\n", e->ring.callednum);
			pri_answer(pri, e->ring.call, 0, 1);
		}
		
		break;
	default:
		fprintf(stderr, "--!! Unknown PRI event %d\n", e->e);
	}
}


static int run_pri(int dfd, int swtype, int node)
{
	struct pri *pri;
	pri_event *e;
	struct timeval tv = {0,0}, *next;
	fd_set rfds, efds;
	int res,x;

	pri = pri_new(dfd, node, swtype);
	if (!pri) {
		fprintf(stderr, "Unable to create PRI\n");
		return -1;
	}
	pri_set_debug(pri, -1);
	for (;;) {
		
		/* Run the D-Channel */
		FD_ZERO(&rfds);
		FD_ZERO(&efds);
		FD_SET(dfd, &rfds);
		FD_SET(dfd, &efds);

		if ((next = pri_schedule_next(pri))) {
			gettimeofday(&tv, NULL);
			tv.tv_sec = next->tv_sec - tv.tv_sec;
			tv.tv_usec = next->tv_usec - tv.tv_usec;
			if (tv.tv_usec < 0) {
				tv.tv_usec += 1000000;
				tv.tv_sec -= 1;
			}
			if (tv.tv_sec < 0) {
				tv.tv_sec = 0;
				tv.tv_usec = 0;
			}
		}
		res = select(dfd + 1, &rfds, NULL, &efds, next ? &tv : NULL);
		e = NULL;

		if (!res) {
			e = pri_schedule_run(pri);
		} else if (res > 0) {
			e = pri_check_event(pri);
		}

		if (e) {
			handle_pri_event(pri, e);
		}
/*
		res = ioctl(dfd, ZT_GETEVENT, &x);

		if (!res && x) {
			fprintf(stderr, "Got event on PRI interface: %d\n", x);
		}
*/
		/* Check for lines that need hangups */
		for (x=0;x<MAX_CHAN;x++)
			if (chans[x].needhangup) {
				chans[x].needhangup = 0;
				pri_release(pri, chans[x].call, PRI_CAUSE_NORMAL_CLEARING);
			}

	}
	return 0;
}



int main( int argc, char** argv )
{
    if ( argc < 2 )
        return 0;

    char* dspboard = argv[ 1 ];
    int channel = 0;

	int f_D = open( dspboard, O_RDWR );
    if ( f_D < 0 )
        return 0;

    unsigned char line[ 4096 ];
    printf( "Listening %s : %d\n", dspboard, channel );

    ::ioctl( f_D, IOCTL_HPI_SET_CHANNEL, 0x00 + channel );

	run_pri( f_D, PRI_SWITCH_EUROISDN_E1, PRI_NETWORK );
/*
	for (;;)
	{
		int rd = read( f_D, line, 2 );

		if ( 0 == rd )
			break; // EOF

		if ( 2 != rd )
		{
			printf( "%s: Could not read packet length\n", dspboard );
			break;
		    }

		int len = ( line[ 0 ] << 8 )  + line[ 1 ];

		rd = read( f_D, line, len );

		if ( 0 == rd )
			break; // EOF

		if ( len != rd )
		{
			printf( "%s: Expected %d, read %d\n", dspboard, len, rd );
			break;
		    }

		printf( "%s:%d: ", dspboard, channel );

	    for ( int i = 0; i < len; i++ )
			printf( " %02x", int( line[ i ] ) );

        printf( "\n" );

        if ( line[ 0 ] == 0 ) // D channel
        {
            unsigned char* lapd = line + 1;
            len--;

            if ( lapd[ 0 ] == 0x02 && lapd[ 1 ] == 0x01 && lapd[ 2 ] == 0x7F )
            {
                unsigned char pdu [] = { 0x00, 0x02, 0x01, 0x73 };
                write( f_D, pdu, sizeof( pdu ) );
                }
            }

        }

    printf( "%s: <EOF>\n", dspboard );
*/
    return 0;
    }
