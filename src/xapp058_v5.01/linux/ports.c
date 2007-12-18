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

#include "stdio.h"
extern FILE *in;

#define LINUX

#ifdef LINUX

//#include "conio.h"
#include <unistd.h>
#include <sys/io.h>

#define DATA_OFFSET    (unsigned short) 0
#define STATUS_OFFSET  (unsigned short) 1
#define CONTROL_OFFSET (unsigned short) 2

typedef union outPortUnion {
    unsigned char value;
    struct opBitsStr {
        unsigned char tdi:1;
        unsigned char tck:1;
        unsigned char tms:1;
        unsigned char zero:1;
        unsigned char one:1;
        unsigned char bit5:1;
        unsigned char bit6:1;
        unsigned char bit7:1;
    } bits;
} outPortType;

typedef union inPortUnion {
    unsigned char value;
    struct ipBitsStr {
        unsigned char bit0:1;
        unsigned char bit1:1;
        unsigned char bit2:1;
        unsigned char bit3:1;
        unsigned char tdo:1;
        unsigned char bit5:1;
        unsigned char bit6:1;
        unsigned char bit7:1;
    } bits;
} inPortType;

typedef union ctrlPortUnion {
    unsigned char value;
    struct ctrlBitsStr {
        unsigned char bit0:1; // data strobe
        unsigned char one:1; // auto feed
        unsigned char bit2:1; // not reset printer
        unsigned char bit3:1; // select printer
        unsigned char bit4:1; // enable int on ack
        unsigned char bit5:1;
        unsigned char bit6:1;
        unsigned char bit7:1;
    } bits;
} ctrlPortType;

static inPortType in_word;
static outPortType out_word;
static ctrlPortType ctrl_word;
static unsigned short base_port = 0x378;
static int once = 0;

char* szport[] = { "TCK", "TMS", "TDI" };

#endif


/*BYTE *xsvf_data=0;*/


/* if in debugging mode, then just set the variables */
void setPort(short p,short val)
{
#ifdef LINUX
    if (once == 0) 
    {

        if( ioperm( base_port, 3, 1 ) ) 
        {
            fprintf( stderr, "ioperm error.\n" );
            exit( 1 );
        }

        out_word.value = 0;
        ctrl_word.value = 0;

        out_word.bits.one = 1;
        out_word.bits.zero = 0;

        ctrl_word.bits.one = 1;

        outb( ctrl_word.value, base_port + CONTROL_OFFSET );

        printf( "------------------- %4s %4s %4s %4s\n",
                "TCK", "TMS", "TDI", "TDO" );

        once = 1;
    }
    if (p==TMS)
        out_word.bits.tms = (unsigned char) val;
    if (p==TDI)
        out_word.bits.tdi = (unsigned char) val;
    if (p==TCK) {
        out_word.bits.tck = (unsigned char) val;
        outb( out_word.value, base_port + DATA_OFFSET );
    }

#if 0
    printf( "------------------- %4s %4s %4s %4s\n",
            p == 0 ? " |  " : " *| ",
            p == 1 ? " |  " : " *| ",
            p == 2 ? " |  " : " *| ",
            readTDOBit () ? " |  " : " *| "
          );
#endif

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
#ifdef LINUX
    in_word.value = (unsigned char) inb( base_port + STATUS_OFFSET );
    if (in_word.bits.tdo == 0x1) {
        return( (unsigned char) 1 );
    }
    return( (unsigned char) 0 );
#endif
}


/* Wait at least the specified number of microsec.                           */
/* Use a timer if possible; otherwise estimate the number of instructions    */
/* necessary to be run based on the microcontroller speed.  For this example */
/* we pulse the TCK port a number of times based on the processor speed.     */
void waitTime(long microsec)
{
#ifdef LINUX
    usleep( microsec );
#else
    static long tckCyclesPerMicrosec    = 1;
    long        tckCycles   = microsec * tckCyclesPerMicrosec;
    long        i;

    /* For systems with TCK rates >= 1 MHz;  This implementation is fine. */
    for ( i = 0; i < tckCycles; ++i )
    {
        pulseClock();
    }
#endif

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

