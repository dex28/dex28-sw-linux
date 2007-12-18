/*******************************************************/
/* file: ports.c                                       */
/* abstract:  This file contains the routines to       */
/*            output values on the JTAG ports, to read */
/*            the TDO bit, and to read a byte of data  */
/*            from the prom                            */
/*                                                     */
/*******************************************************/
#include "ports.h"
/*#include "prgispx.h"*/

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
  
#include "stdio.h"
extern FILE *in;

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

#define PIOB_BASE 0xfffff600UL

#define PIO_B14	  ((unsigned long) 1 << 14) // TMS, output  JTAG pin 6
#define PIO_B15	  ((unsigned long) 1 << 15) // TDI, output  JTAG pin 5
#define PIO_B16	  ((unsigned long) 1 << 16) // TDO, input   JTAG pin 4
#define PIO_B17	  ((unsigned long) 1 << 17) // TCK, output  JTAG pin 3

#define PIOB_PER  ( PIOB_BASE + 0x0000 )
#define PIOB_OER  ( PIOB_BASE + 0x0010 )
#define PIOB_ODR  ( PIOB_BASE + 0x0014 )
#define PIOB_IFDR ( PIOB_BASE + 0x0024 )
#define PIOB_SODR ( PIOB_BASE + 0x0030 )
#define PIOB_CODR ( PIOB_BASE + 0x0034 )
#define PIOB_PDSR ( PIOB_BASE + 0x003C )
#define PIOB_IDR  ( PIOB_BASE + 0x0044 )


static int mem_fd = -1;
static unsigned char* map_base;

void hw_Setup( void )
{
    mem_fd = open( "/dev/mem", O_RDWR | O_SYNC );
    if( mem_fd  < 0 )
    {
        printf( "Couldn't open /dev/mem.\n" );
        return;
    }

    // Map one page
    map_base = (unsigned char*)mmap( 0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, PIOB_BASE  & ~MAP_MASK );
    if (map_base == (unsigned char*)-1 )
    {
        printf("Couldn't get Map-Address.\n");
        return;
    }
/*
    printf( "%08x\n", *((unsigned long*) (map_base + ( (PIOB_BASE + 0x78) & MAP_MASK))) );
*/
    // Configure the registers
    //
    *((unsigned long*) (map_base + (PIOB_PER & MAP_MASK)))  = PIO_B14 | PIO_B15 | PIO_B16 | PIO_B17; // enable io
    *((unsigned long*) (map_base + (PIOB_IFDR & MAP_MASK))) = PIO_B14 | PIO_B15 | PIO_B16 | PIO_B17; // disable input filter
    *((unsigned long*) (map_base + (PIOB_IDR & MAP_MASK)))  = PIO_B14 | PIO_B15 | PIO_B16 | PIO_B17; // disable interrupt
    *((unsigned long*) (map_base + (PIOB_OER & MAP_MASK)))  = PIO_B14 | PIO_B15 | PIO_B17; // Enable output
    *((unsigned long*) (map_base + (PIOB_ODR & MAP_MASK)))  = PIO_B16; // Disable output, i.e. input
    *((unsigned long*) (map_base + (PIOB_SODR & MAP_MASK))) = PIO_B14 | PIO_B15 | PIO_B17; // Set high

#if 0
    printf( "------------------- %4s %4s %4s %4s\n",
            "TCK", "TMS", "TDI", "TDO" );
#endif
    }

void hw_Cleanup( void )
{
    if ( mem_fd < 0 )
        return;

    munmap( map_base, MAP_SIZE );
    close( mem_fd );
    }

void setPort(short p,short val)
{
    if ( mem_fd < 0 )
        return;

    if (p == TMS)
    {
        if ( val )
		    *((unsigned long*) (map_base + (PIOB_SODR & MAP_MASK))) = PIO_B14;
        else
		    *((unsigned long*) (map_base + (PIOB_CODR & MAP_MASK))) = PIO_B14;
        }
    else if (p == TDI)
    {
        if ( val )
		    *((unsigned long*) (map_base + (PIOB_SODR & MAP_MASK))) = PIO_B15;
        else
		    *((unsigned long*) (map_base + (PIOB_CODR & MAP_MASK))) = PIO_B15;
        }
    else if (p == TCK)
    {
        if ( val )
		    *((unsigned long*) (map_base + (PIOB_SODR & MAP_MASK))) = PIO_B17;
        else
		    *((unsigned long*) (map_base + (PIOB_CODR & MAP_MASK))) = PIO_B17;
        }

#if 0
    printf( "------------------- %4s %4s %4s %4s\n",
            p == 0 ? " |  " : " *| ",
            p == 1 ? " |  " : " *| ",
            p == 2 ? " |  " : " *| ",
            readTDOBit () ? " |  " : " *| "
          );
#endif
}


/* toggle tck LH */
void pulseClock()
{
    setPort(TCK,0);  /* set the TCK port to low  */
    setPort(TCK,1);  /* set the TCK port to high */
}


/* read in a byte of data from the prom */

void readByte(unsigned char *data)
{
    /* pretend reading using a file */
    fscanf(in,"%c",data);
    /**data=*xsvf_data++;*/
}

/* read the TDO bit from port */

unsigned char readTDOBit()
{
    if ( mem_fd < 0 )
        return 0;

    if ( *((unsigned long*) (map_base + (PIOB_PDSR & MAP_MASK))) & PIO_B16 )
        return 1;
    else
        return 0;
}


/* Wait at least the specified number of microsec.                           */
/* Use a timer if possible; otherwise estimate the number of instructions    */
/* necessary to be run based on the microcontroller speed.  For this example */
/* we pulse the TCK port a number of times based on the processor speed.     */
void waitTime(long microsec)
{
    static long tckCyclesPerMicrosec    = 5;
    long        tckCycles   = microsec * tckCyclesPerMicrosec;
    long        i;

    /* For systems with TCK rates >= 1 MHz;  This implementation is fine. */
    for ( i = 0; i < tckCycles; ++i )
    {
        pulseClock();
    }

#if 0
    /* For systems with TCK rates << 1 MHz;  Consider this implementation. */
    if ( microsec >= 50L )
    {
        /* Make sure TCK is low during wait for XC18V00/XCF00 */
        /* Or, a running TCK implementation as shown above is an OK alternate */
        setPort( TCK, 0 );

        /* Use Windows Sleep().  Round up to the nearest millisec */
        _sleep( ( microsec + 999L ) / 1000L );
    }
    else    /* Satisfy Virtex-II TCK cycles */
    {
        for ( i = 0; i < microsec;  ++i )
        {
            pulseClock();
        }
    }
#endif

#if 0
    /* If Virtex-II support is not required, then this implementation is fine */
    /* Make sure TCK is low during wait for XC18V00/XCF00 */
    /* Or, a running TCK implementation as shown above is an OK alternate */
    setPort( TCK, 0 );
    /* Use Windows Sleep().  Round up to the nearest millisec */
    _sleep( ( microsec + 999L ) / 1000L );
#endif
}
