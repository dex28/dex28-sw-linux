#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
  
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

#define PIOB_BASE 0xfffff600UL

#define PIO_B0	  ((unsigned long) 1 << 0)

#define PIOB_PER  ( PIOB_BASE + 0x0000 )
#define PIOB_OER  ( PIOB_BASE + 0x0010 )
#define PIOB_IFDR ( PIOB_BASE + 0x0024 )
#define PIOB_SODR ( PIOB_BASE + 0x0030 )
#define PIOB_CODR ( PIOB_BASE + 0x0034 )
#define PIOB_IDR  ( PIOB_BASE + 0x0044 )

volatile bool running = true; // main loop running flag

static int wfd = 0; // Watchdog file descriptor

static void watchdog_shutdown(int unused)
{
    if ( wfd >= 0 )
    {
	    write( wfd, "V", 1 ); // Magic
	    close( wfd );
    }

    running = false;
}


int main( int argc, char** argv )
{
    if ( argc >= 2 &&  strcmp( argv[1], "-d" ) == 0 )
    {
        daemon( 0, 0 );
    }

	signal(SIGHUP, watchdog_shutdown);
	signal(SIGINT, watchdog_shutdown);
	signal(SIGTERM, watchdog_shutdown);

    int fd = open( "/dev/mem", O_RDWR | O_SYNC );
    if( fd  < 0 )
    {
    	printf( "Couldn't open /dev/mem.\n" );
	    return -1;
    }

    // Map one page
    unsigned char* map_base = (unsigned char*)mmap( 0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PIOB_BASE  & ~MAP_MASK );
    if (map_base == (unsigned char*)-1 )
    {
    	printf("Couldn't get Map-Address.\n");
		return -1;
	}
    
    // Configure the registers

    *((unsigned long*) (map_base + (PIOB_PER & MAP_MASK)))  = PIO_B0; // enable io
    *((unsigned long*) (map_base + (PIOB_IFDR & MAP_MASK))) = PIO_B0; // disable input filter
    *((unsigned long*) (map_base + (PIOB_IDR & MAP_MASK)))  = PIO_B0; // disable interrupt
    *((unsigned long*) (map_base + (PIOB_OER & MAP_MASK)))  = PIO_B0; // Enable output
    *((unsigned long*) (map_base + (PIOB_SODR & MAP_MASK))) = PIO_B0; // led Off

    for ( int i = 0; i < 3 && running; i++ )
    {		   
        usleep( 300000 );
		*((unsigned long*) (map_base + (PIOB_CODR & MAP_MASK))) = PIO_B0; // On 
        usleep( 300000 );
		*((unsigned long*) (map_base + (PIOB_SODR & MAP_MASK))) = PIO_B0; // Off
    }

	wfd = open( "/dev/watchdog", O_WRONLY );

    while( running )
    {
        for ( int i = 0; i < 6; i++ )
        {
		    write(wfd, "\0", 1);
            usleep( 500000 );
            }

        if ( ! running ) break;
		*((unsigned long*) (map_base + (PIOB_CODR & MAP_MASK))) = PIO_B0; // On
        usleep( 40000 );
		*((unsigned long*) (map_base + (PIOB_SODR & MAP_MASK))) = PIO_B0; // Off
        usleep( 10000 );
		*((unsigned long*) (map_base + (PIOB_CODR & MAP_MASK))) = PIO_B0; // On
        usleep( 40000 );
		*((unsigned long*) (map_base + (PIOB_SODR & MAP_MASK))) = PIO_B0; // Off
    }

    *((unsigned long*) (map_base + (PIOB_CODR & MAP_MASK))) = PIO_B0; // LED On

    munmap( map_base, MAP_SIZE );

    close( fd );

	watchdog_shutdown(0);

    return 0;
}
