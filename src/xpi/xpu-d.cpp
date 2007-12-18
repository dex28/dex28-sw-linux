
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

#define MAP_SIZE 0x1000UL
#define MAP_MASK (MAP_SIZE - 1)


static int mem_fd = -1;
static unsigned char* map_base;

struct __attribute__ ((__packed__)) FPGA_R_REG
{
    unsigned short magic;
    unsigned short led;
    unsigned short fc;
    unsigned short ctx;
    };

struct __attribute__ ((__packed__)) FPGA_W_REG
{
    unsigned short _ignored;
    unsigned short led;
    unsigned short fc;
    };


volatile FPGA_R_REG* fpga_R;
volatile FPGA_W_REG* fpga_W;

void hw_Setup( void )
{
    mem_fd = open( "/dev/mem", O_RDWR | O_SYNC );
    if( mem_fd  < 0 )
    {
        printf( "Couldn't open /dev/mem.\n" );
        return;
        }

    unsigned long FPGA_BASE = 0x30000000; // NCS2

    // Map one page
    map_base = (unsigned char*)mmap( 0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, FPGA_BASE  & ~MAP_MASK );
    if (map_base == (unsigned char*)-1 )
    {
        printf("Couldn't get Map-Address.\n");
        return;
        }

    fpga_R = (FPGA_R_REG*)map_base;
    fpga_W = (FPGA_W_REG*)map_base;
    }

void hw_Cleanup( void )
{
    if ( mem_fd < 0 )
        return;

    munmap( map_base, MAP_SIZE );
    close( mem_fd );
    }

int main ()
{
    hw_Setup ();

    int col = 0;
    for ( ;; )
    {
        int x = fpga_R->ctx;
        for ( int i = 7; i >= 0; i-- )
        {
            fprintf( stderr, x & ( 1 << i ) ? "1" : "0" );
            if ( ++ col == 80 ) { col = 0; fprintf( stderr, "\n" ); }
            }
        }

    hw_Cleanup ();
    }

