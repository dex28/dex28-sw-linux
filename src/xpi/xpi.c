/*
    Project:     Albatross / XPI
  
    Module:      xpi.c
  
    Description: Albatross FPGA XPU-D Linux device driver
  
    Copyright (c) 2002-2005 By Mikica B Kocic
    Copyright (c) 2002-2005 By IPTC Technology Communications AB
*/

#include <linux/module.h>  
//#include <linux/config.h>
#include <linux/init.h>

//////////////////////////////////////////////////////////////////////////////

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
#include <linux/utsrelease.h>
#endif
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
#include <linux/config.h>
#else
#include <linux/autoconf.h>
#endif
#include <linux/errno.h>
#include <linux/kernel.h>

//////////////////////////////////////////////////////////////////////////////

MODULE_LICENSE( "GPL" );

MODULE_DESCRIPTION( "FPGA XPU-D Device Driver" );
MODULE_AUTHOR( "Mikica B. Kocic" );

//////////////////////////////////////////////////////////////////////////////

//#include <linux/compatmac.h>

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/tty.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/irq.h>

#include <asm/io.h>  
//#include <asm/irq.h> // disable_irq/enable_irq

#include <asm/arch/gpio.h>
#include <asm/arch/at91rm9200_mc.h>

#include "xpi.h"

enum // FPGA memory map
{
    // Read                              Contents
    XPI_R_GLB_STAT      = 0x00, // byte  D1= EIRQ, D0= MCPU
    XPI_R_BOARD_POS     = 0x01, // byte  D5..0= KA
    XPI_R_INT_ENABLE    = 0x02, // byte  D5= CTXE, D4= PCM, D3= EIRQ, D2= CRX, D1= CTX, D0= FC
    XPI_R_INT_REQUEST   = 0x04, // byte  D5= CTXE, D4= PCM, D3= EIRQ, D2= CRX, D1= CTX, D0= FC
    XPI_R_FC_STATUS     = 0x06, // byte  D3= SENSE, D2= FCE, D1= FCD, D0= FCC
    XPI_R_FC_FIFO_IN    = 0x08, // byte  D5= FCA5 .. D2= FCA0, D1= FCD1, D0= FCD0
    XPI_R_FC_SENSE_IN   = 0x09, // byte  D0= SENSE
    XPI_R_CTX_FIFO_IN   = 0x0A, // byte  D7..0= Data
    XPI_R_CRX_FIFO_IN   = 0x0C, // byte  D7..0= Data
    XPI_R_EIRQ_FIFO_IN  = 0x0E, // byte  D0= EIRQ
    XPI_R_RPCM0_FIFO_IN = 0x10, // byte  D7..0= Data
    XPI_R_RPCM1_FIFO_IN = 0x12, // byte  D7..0= Data
    XPI_R_TPCM0_FIFO_IN = 0x14, // byte  D7..0= Data
    XPI_R_TPCM1_FIFO_IN = 0x16, // byte  D7..0= Data
    XPI_R_PCM_IRQ_ACK   = 0x22, // byte  all '0'
    XPI_R_FPGA_MAGIC    = 0x3E, // word  0x11AA
    // Write
    XPI_W_GLB_CTRL      = 0x00, // byte  D0= MCPU
    XPI_W_INT_EN_SET    = 0x02, // byte  D5= CTXE, D4= PCM, D3= EIRQ, D2= CRX, D1= CTX, D0= FC
    XPI_W_INT_EN_CLR    = 0x03, // byte  D5= CTXE, D4= PCM, D3= EIRQ, D2= CRX, D1= CTX, D0= FC
    XPI_W_FC_CONTROL    = 0x06, // byte  D2= FCE, D1= FCD, D0= FCC
    XPI_W_CTX_FIFO_OUT  = 0x08, // byte  D7..0= Data
    XPI_W_LED_SET       = 0x0E, // byte  D2= LED_R, D1= LED_Y, D0= LED_G
    XPI_W_LED_CLR       = 0x0F, // byte  D2= LED_R, D1= LED_Y, D0= LED_G
    XPI_W_RPCM0_TS_SEL  = 0x10, // byte  D5..0= TS
    XPI_W_RPCM1_TS_SEL  = 0x12, // byte  D5..0= TS
    XPI_W_TPCM0_TS_SEL  = 0x14, // byte  D5..0= TS
    XPI_W_TPCM1_TS_SEL  = 0x16, // byte  D5..0= TS
    // bitmasks
    XPI_IRQ_CTXE        = 0x20, // CTX empty
    XPI_IRQ_PCM         = 0x10, // PCM frame sync
    XPI_IRQ_EIRQ        = 0x08, // EIRQ changed event
    XPI_IRQ_CRX         = 0x04, // CRX data received event
    XPI_IRQ_CTX         = 0x02, // CTX data received event
    XPI_IRQ_FC          = 0x01, // FC data received event
    XPI_LED_R           = 0x04, // Red LED
    XPI_LED_Y           = 0x02, // Yellow LED
    XPI_LED_G           = 0x01, // Green LED
    XPI_FC_SENSE        = 0x08, // SENSE bit
    XPI_FC_FCE          = 0x04, // FCE bit
    XPI_FC_FCD          = 0x02, // FCD bit
    XPI_FC_FCC          = 0x01, // FCC bit
    };

//////////////////////////////////////////////////////////////////////////////

#define dbg_spin_lock(x) \
    do { printk( KERN_INFO "%s:%-4d lock   " #x "\n", __FILE__, __LINE__ ); spin_lock(x); } \
    while(0)

#define dbg_spin_unlock(x) \
    do { printk( KERN_INFO "%s:%-4d unlock " #x "\n", __FILE__, __LINE__ ); spin_unlock(x); } \
    while(0);

#define dbg_spin_lock_bh(x) \
    do { printk( KERN_INFO "%s:%-4d lock   bh " #x "\n", __FILE__, __LINE__ ); spin_lock_bh(x); } \
    while(0)

#define dbg_spin_unlock_bh(x) \
    do { printk( KERN_INFO "%s:%-4d unlock bh " #x "\n", __FILE__, __LINE__ ); spin_unlock_bh(x); } \
    while(0)

//////////////////////////////////////////////////////////////////////////////
// Various compatibility macros and declarations
//
#define INODE_FROM_F(filp)((filp)->f_dentry->d_inode)

//typedef enum { false = 0, true = 1 } bool;

//////////////////////////////////////////////////////////////////////////////

typedef struct XPI XPI;
typedef struct xpidev xpidev;
typedef struct chBuf  chBuf;
typedef struct MsgBuf MsgBuf;
typedef struct dblBuf dblBuf;

//////////////////////////////////////////////////////////////////////////////
// DoubleBuffer class

struct dblBuf
{
    int size;
    int count;
    unsigned char pkt1[ 256 ];
    unsigned char pkt2[ 256 ];
    volatile unsigned char* getp;
    unsigned char* putp;
    };

static inline void dblBuf_Construct( dblBuf* this, int sz )
{
    this->size = sz;
    this->count = 0;
    this->getp = NULL;
    this->putp = this->pkt1;
    }

static inline bool dblBuf_Put( dblBuf* this, int data )
{
    this->putp[ this->count++ ] = data;
    if ( this->count >= this->size )
    {
        this->count = 0;
        this->getp = this->putp;
        this->putp = this->putp == this->pkt1 ? this->pkt2 : this->pkt1;
        return true; // Ready
        }

    return false;
    }

static inline unsigned char* dblBuf_Get( dblBuf* this )
{
    unsigned char* pkt = (unsigned char*)this->getp;
    this->getp = NULL;
    return pkt;
    }

///////////////////////////////////////////////////////////////////////////////
// MsgBuf: an circular message buffer class
//
struct MsgBuf
{
    unsigned char* buffer;         // Circular character buffer
    volatile long size;            // character buffer size; 0 means killed chBuf
    volatile unsigned long writep; // end (new chars coming here; head)
    volatile unsigned long readp;  // begin (oldest chars are here; tail)
    };

static inline void MsgBuf_Construct( MsgBuf* this )
{
    this->buffer = kmalloc( 8192, GFP_KERNEL );

    this->writep = 0;
    this->readp = 0;
    this->size = 8192;
    }

static inline void MsgBuf_Destruct( MsgBuf* this )
{
    this->size = 0;

    if ( this->buffer )
        kfree( this->buffer );
    }

static inline void MsgBuf_Put( MsgBuf* this, int data )
{
    int bufsize = this->size;
    int wp = this->writep;

    // Check free space
    if ( ( this->readp + bufsize - 1 - wp ) % bufsize < 7 )
        return; // insufficient free space. drop data.

    this->buffer[ wp ] = data;
    if ( ++wp >= bufsize )
        wp = 0;

    this->writep = wp;
    }

static inline int MsgBuf_Get( MsgBuf* this, unsigned char* data, int maxlen )
{
    int bufsize = this->size;
    int rp = this->readp;
    int len = 0;

    while( len < maxlen && rp != this->writep )
    {
        data[ len++ ] = this->buffer[ rp ];
        if ( ++rp >= bufsize )
            rp = 0;
        }

    this->readp = rp;
    return len;
    }

static inline int MsgBuf_GetCh( MsgBuf* this )
{
    int bufsize = this->size;
    int rp = this->readp;
    int ch;

    if ( rp == this->writep )
        return -1;

    ch =  this->buffer[ rp ];
    if ( ++rp >= bufsize )
        rp = 0;

    this->readp = rp;
    return ch;
    }

///////////////////////////////////////////////////////////////////////////////
// chBuf: an circular character buffer class
//
struct chBuf
{
    unsigned char* buffer;         // Circular character buffer
    volatile long size;            // character buffer size; 0 means killed chBuf
    volatile unsigned long writep; // end (new chars coming here; head)
    volatile unsigned long readp;  // begin (oldest chars are here; tail)
    wait_queue_head_t queue;       // event notification queue
    spinlock_t userctx_lock;       // user context read MUTEX
    // NOTE: XPI's mutex is used as tasklet write MUTEX
    };

struct XPI
{
    void* mem_base;              // FPGA Register Space
    int irq;                     // irq number associated to XPI interface
    struct tasklet_struct irq_tasklet; // bottom half of irq handler
    spinlock_t ebi_lock;           // MUTEX of the structure 
    MsgBuf msgBuf;
    MsgBuf ctxBuf;
    xpidev* dev_first;
    bool MCPU;
    dblBuf pcmBuf;
    };

struct xpidev
{
    xpidev* next;                // next in xpidev list
    xpidev* prev;                // previous in xpidev list
    int channel;                 // selected channel
    chBuf inBuf;                 // output (our input character buffer)
    };


//////////////////////////////////////////////////////////////////////////////
// Static data declarations
//
static struct file_operations xpi_file_operations;
static XPI xpi;

///////////////////////////////////////////////////////////////////////////////
// chBuf class methods

void chBuf_Construct( chBuf* this )
{
    this->buffer = kmalloc( 8192, GFP_KERNEL );

    this->writep = 0;
    this->readp = 0;
    this->size = 8192;

    spin_lock_init( &this->userctx_lock );

    init_waitqueue_head( &this->queue );
    }

void chBuf_Destruct( chBuf* this )
{
    this->size = 0;
    wake_up_interruptible( &this->queue ); // make all listeners to kill themselfs

    if ( this->buffer )
        kfree( this->buffer );
    }

void chBuf_TaskletsWrite( chBuf* this, const char* data, int len )
{
    int i;
    int wp; // write-out pointer
    int wp_bound; // upper (inclusive) bound for write-out pointer
    int bufsize;

    // Writing a NULL pointer or negative length signals chBuf readers
    // to report EOF, i.e. to release xpidev devices
    //
    if ( data == NULL || len < 0 )
    {
        this->size = 0; // mark EOF
        wake_up_interruptible( &this->queue );
        return;
        }

    if ( this->size == 0 )
        return;

    // Write message to circular buffer
    // Note.This segment is protected by XPI's spin lock.
    //
    bufsize = this->size;
    wp = this->writep;
    wp_bound = ( this->readp + bufsize - 1 ) % bufsize; // one position left of readp
    // wp_bound: Drop data if there is no space left in output buffer

    for ( i = 0; i < len && wp != wp_bound; i++ )
    {
        this->buffer[ wp ] = data[ i ];
        if ( ++wp >= bufsize )
            wp = 0;
        }

    this->writep = wp;

    // Enqueue message into queue, and awake any reading process.
    wake_up_interruptible( &this->queue );
    }

int chBuf_UsersRead( chBuf* this, char* data, int maxlen, bool non_block )
{
    int wp; // write-out pointer
    int rp; // read-in pointer
    int rp_bound; // inclusive upper bound for read-in pointer
    int count; // number of bytes read
    int bufsize;

    if ( non_block )
    {
        if ( this->size == 0 )
            return 0; // EOF; chBuf is not initialized or it has been killed
        else if ( this->writep == this->readp )
            return -EAGAIN; // device is unreadable
        }

    if ( this->writep == this->readp )
    {
        int ret = 0;
        wait_queue_t wait;

        init_waitqueue_entry( &wait, current );
        add_wait_queue( &this->queue, &wait );

        for ( ;; )
        {
            set_current_state( TASK_INTERRUPTIBLE );

            if ( this->size == 0 || this->writep != this->readp )
                break; // EOF (killed chBuf) or something arrived

            if ( signal_pending( current ) ) // a signal arrived
            {
                ret = -ERESTARTSYS; // we should tell the FS layer to handle it
                break;
                }

            schedule ();
            }

        set_current_state( TASK_RUNNING );
        remove_wait_queue( &this->queue, &wait );

        if ( ret )
            return ret;
        }

    if ( this->size == 0 )
        return 0; // EOF; chBuf has been killed

    // Read message from circular buffer

    spin_lock( &this->userctx_lock );

    count = 0;
    bufsize = this->size;
    rp = this->readp;
    rp_bound = this->writep;

    for ( wp = 0; wp < maxlen && rp != rp_bound; wp++, count++ )
    {
        data[ wp ] = this->buffer[ rp ];
        if ( ++rp >= bufsize )
            rp = 0;
        }

    this->readp = rp;

    spin_unlock( &this->userctx_lock );

    return count;
    }

//////////////////////////////////////////////////////////////////////////////
// XPI class methods

static inline void xpi_SetRESET( bool on )
{
    if ( on )
        at91_set_gpio_value( AT91_PIN_PC15, 1 ); // RESET <= '1'
    else
        at91_set_gpio_value( AT91_PIN_PC15, 0 ); // RESET <= '0'
    }

//////////////////////////////////////////////////////////////////////////////
// XPI class methods


// Top half of the common XPI interrupt service routine
// 
static irqreturn_t xpi_irq_handler
(
    int irq, // interrupt that has occured
    void* dev_id // pointer gives as argument for request_irq
    )
{
    bool perform_tasklet = false;

    int irq_list = readb( xpi.mem_base + XPI_R_INT_REQUEST ); // get irq list

    //AT91_SYS->AIC_ICCR = AT91C_ID_IRQ0; //  acknowledge interrupt in PIO

    perform_tasklet = false;

    if ( irq_list & XPI_IRQ_FC )
    {
        MsgBuf_Put( &xpi.msgBuf, 0xF0 );
        MsgBuf_Put( &xpi.msgBuf, readb( xpi.mem_base + XPI_R_FC_FIFO_IN ) );
        MsgBuf_Put( &xpi.msgBuf, readb( xpi.mem_base + XPI_R_FC_SENSE_IN ) );
        perform_tasklet = true;
        }
    if ( irq_list & XPI_IRQ_CTX )
    {
        MsgBuf_Put( &xpi.msgBuf, 0xF1 );
        MsgBuf_Put( &xpi.msgBuf, readb( xpi.mem_base + XPI_R_CTX_FIFO_IN ) );
        perform_tasklet = true;
        }
    if ( irq_list & XPI_IRQ_CRX )
    {
        MsgBuf_Put( &xpi.msgBuf, 0xF2 );
        MsgBuf_Put( &xpi.msgBuf, readb( xpi.mem_base + XPI_R_CRX_FIFO_IN ) );
        perform_tasklet = true;
        }
    if ( irq_list & XPI_IRQ_EIRQ )
    {
        MsgBuf_Put( &xpi.msgBuf, 0xF3 );
        MsgBuf_Put( &xpi.msgBuf, readb( xpi.mem_base + XPI_R_EIRQ_FIFO_IN ) );
        perform_tasklet = true;
        }
    if ( irq_list & XPI_IRQ_PCM )
    {
        bool ready = dblBuf_Put( &xpi.pcmBuf, readb( xpi.mem_base + XPI_R_RPCM1_FIFO_IN ) );
        if ( ready )
            perform_tasklet = true;
        readb( xpi.mem_base + XPI_R_PCM_IRQ_ACK ); // acknowledge PCM frame
        }
    if ( irq_list & XPI_IRQ_CTXE )
    {
        {
            int ch = MsgBuf_GetCh( &xpi.ctxBuf );
            if ( ch < 0 )
            {
                writeb( XPI_IRQ_CTXE, xpi.mem_base + XPI_W_INT_EN_CLR ); // disable CTXE irq
                }
            else
            {
                writeb( ch, xpi.mem_base + XPI_W_CTX_FIFO_OUT ); // write MCTX
                }
            }

        perform_tasklet = true;
        }

    // schedule tasklet only if it has something to do
    //
    if ( perform_tasklet )
        tasklet_schedule( &xpi.irq_tasklet );

    return IRQ_HANDLED;
    }

// Bottom half of the common XPI interrupt service routine
// 
static void xpi_irq_tasklet( unsigned long data )
{
    for( ;; )
    {
        xpidev* dev;
        unsigned char pkt[ 256 ];
        int len = MsgBuf_Get( &xpi.msgBuf, pkt, sizeof(pkt) );
        if ( ! len )
            break;

        spin_lock( &xpi.ebi_lock );

        dev = xpi.dev_first;

        while( dev )
        {
            if ( dev->channel == 0 )
                chBuf_TaskletsWrite( &dev->inBuf, pkt, len ); 

            dev = dev->next;
            }

        spin_unlock( &xpi.ebi_lock );
        }

    if ( xpi.pcmBuf.getp )
    {
        unsigned char* pkt = dblBuf_Get( &xpi.pcmBuf );
        int pktsz = xpi.pcmBuf.size;
        xpidev* dev;

        spin_lock( &xpi.ebi_lock );

        dev = xpi.dev_first;

        while( dev )
        {
            if ( dev->channel == 1 )
                chBuf_TaskletsWrite( &dev->inBuf, pkt, pktsz ); 

            dev = dev->next;
            }

        spin_unlock( &xpi.ebi_lock );
        }
    }

static int xpi_FC_Command( int cmd )
{
    int i;
    void* fcctrl = xpi.mem_base + XPI_W_FC_CONTROL;

    writeb( 0x00, fcctrl );       // FCC, FCD, FCE = 0

    for ( i = 0; i < 8; i++ )
    {
        int FCE_FCD = ( cmd & 0x80 ) ? XPI_FC_FCD : 0;
        writeb( FCE_FCD,              fcctrl ); // FCC = 0
        writeb( FCE_FCD | XPI_FC_FCC, fcctrl ); // FCC = 1
        cmd <<= 1;
        }

    writeb( XPI_FC_FCE, fcctrl ); // FCE = 1

    for ( i = 0; i < 3; i++ )
        cmd = readb( fcctrl );    // Read SENSE

    writeb( 0x00, fcctrl );       // FCE = 0

    return ( cmd & XPI_FC_SENSE ) != 0; // Return SENSE
    }

static void xpi_Destruct( XPI* this )
{
    writeb( XPI_LED_G, xpi.mem_base + XPI_W_LED_CLR ); // Clear green LED

    xpi_SetRESET( true );

    if ( this->irq >= 0 )
    {
        free_irq( this->irq, this );
        }

    MsgBuf_Destruct( &this->msgBuf );
    MsgBuf_Destruct( &this->ctxBuf );
    }

static bool xpi_Construct( XPI* this )
{
    int rc = 0;

    // Initialize members
    //
    this->mem_base = NULL;
    this->irq = -1;
    this->dev_first = NULL;
    this->MCPU = false;

    spin_lock_init( &this->ebi_lock );
    MsgBuf_Construct( &this->msgBuf );
    MsgBuf_Construct( &this->ctxBuf );
    dblBuf_Construct( &this->pcmBuf, 160 );

    tasklet_init( &this->irq_tasklet, xpi_irq_tasklet, (unsigned long)this );

    // Configure PC15 to be output, initially set high
    // 
    at91_set_gpio_output( AT91_PIN_PC15, 1 ); // RESET <= '1'

    // Configure CS2 to access FPGA; 8-bit bus with wait states and
    //
    at91_sys_write( AT91_SMC_CSR(2)
       , AT91_SMC_RWHOLD_(1)  // Read and Write Signal Hold: 1 cycles (16 ns)
       | AT91_SMC_RWSETUP_(0) // Read and Write Signal Setup: 0 cycles
       | AT91_SMC_ACSS_STD    // Address Chip Select Setup: Standard
                              // Data Read Protocol: Standard (not early)
       | AT91_SMC_DBW_8       // Data Bus Width: 8-bit
                              // Byte Access Type: CS connected to two 8-bit wide devices
       | AT91_SMC_TDF_(0)     // Data Float Time: 0 cycles
       | AT91_SMC_WSEN        // Wait State Enable: Enabled NWS
       | AT91_SMC_NWS_(0)     // Number of Wait States: 1; One wait state is 16 ns
       );

    printk( "CSR NEW VALUE: %08x\n", at91_sys_read( AT91_SMC_CSR(2) ) );

    // Remap memory NCS2
    //
    this->mem_base = ioremap( AT91_CHIPSELECT_2, 1 << 16 );

    {
        int magic = readw( xpi.mem_base + XPI_R_FPGA_MAGIC );

        if ( magic != 0x11AA )
        {
            printk( "xpi: Bad FPGA Magic %04x\n", magic );
            return false;
            }
        }

    // Configure and allocate interrupt
    //
    at91_set_A_periph( AT91_PIN_PB29, 1 ); // Disable PB29, use it as IRQ0
    this->irq = AT91RM9200_ID_IRQ0;
    set_irq_type( this->irq, AT91_AIC_SRCTYPE_LOW );

    rc = request_irq( this->irq, xpi_irq_handler, IRQF_SHARED, XPI_DEVICE_NAME, this );
    if ( rc ) // error
    {
        printk( KERN_ERR "xpi: Can't get assigned irq %i\n", this->irq );
        return false;
        }

    {
        int slot = readb( xpi.mem_base + XPI_R_BOARD_POS );

        xpi_SetRESET( false ); // Clear RESET

        if ( ( slot & 0x30 ) != 0x30 ) // CPU-D board found
        {
            this->MCPU = false;
            printk( "xpi: Acting as Device Board; Board position: %d\n", slot );
            }
        else // No CPU-D_ boards
        {
            writeb( 0x01, xpi.mem_base + XPI_W_GLB_CTRL ); // Set MCPU <= '1';

            slot = readb( xpi.mem_base + XPI_R_BOARD_POS ); // Update slot#

            if ( ( slot & 0x30 ) != 0 )
            {
                printk( "xpi: Becoming CPU-D...\n" );
                printk( "xpi: Somehing wrong! KA5..4 are not low!\n" );
                xpi_SetRESET( true ); // Reset FPGA
                xpi_Destruct( this );
                return false;
                }
            else
            {
                this->MCPU = true;
                printk( "xpi: Acting as CPU-D; Board position: %d\n", slot );
                }
            }
        }

    writeb( XPI_LED_G, xpi.mem_base + XPI_W_LED_SET ); // Set green LED

    printk( KERN_INFO "xpi: Constructed\n" );

    return true;
    }

// This function is called whenever a process attempts 
// to open the device file. This is common for all c/d/io types
// of device nodes.
//
static int xpi_open
(
    struct inode* inode, 
    struct file* file
    )
{

    xpidev* dev = kmalloc( sizeof( xpidev ), GFP_KERNEL );
    memset( dev, 0, sizeof( xpidev ) );
    file->private_data = dev;

    dev->channel = -1;

    // Initialize data buffers
    chBuf_Construct( &dev->inBuf );

    // Link device into hpidev_io list of DSP
    //
    spin_lock_bh( &xpi.ebi_lock );

    dev->prev = NULL;
    dev->next = xpi.dev_first;
    if ( xpi.dev_first )
        xpi.dev_first->prev = dev;
    xpi.dev_first = dev;

    spin_unlock_bh( &xpi.ebi_lock );


#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,4,0xFF)
    MOD_INC_USE_COUNT;
#endif

    return 0;
    }

static int xpi_release
(
    struct inode* inode, 
    struct file* file
    )
{
    xpidev* dev = (xpidev*)file->private_data;

    // Unlink device from dev_stdio list of XPI
    //
    spin_lock_bh( &xpi.ebi_lock );

    if ( dev->prev )
        dev->prev->next = dev->next;
    else
        xpi.dev_first = dev->next;

    if ( dev->next )
        dev->next->prev = dev->prev;

    dev->next = NULL;
    dev->prev = NULL;

    spin_unlock_bh( &xpi.ebi_lock );

    // Destruct members
    //
    chBuf_Destruct( &dev->inBuf );

    if ( dev )
        kfree( dev );

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,4,0xFF)
    MOD_DEC_USE_COUNT;
#endif

    return 0;
    }

static ssize_t xpi_read
(
    struct file* file,
    char* buffer,      // The buffer to fill with the data 
    size_t length,     // The length of the buffer 
    loff_t* offset     // offset to the file 
    )
{
    xpidev* dev = (xpidev*)file->private_data;

    int non_block = ( file->f_flags & O_NONBLOCK ) != 0;

    return chBuf_UsersRead( &dev->inBuf, buffer, length, non_block );
    }

static ssize_t xpi_write
(
    struct file* file,
    const char* buffer,
    size_t length,
    loff_t* offset
    )
{
    xpidev* dev = (xpidev*)file->private_data;
    unsigned char* data = (unsigned char*) buffer;
    int i;

    if ( dev->channel == 1 )
        return 0;

    spin_lock_bh( &xpi.ebi_lock );

    for ( i = 0; i < length; i++ )
    {
        MsgBuf_Put( &xpi.ctxBuf, *data++ );
        }

    writeb( XPI_IRQ_CTXE, xpi.mem_base + XPI_W_INT_EN_SET ); // enable CTXE irq

    spin_unlock_bh( &xpi.ebi_lock );

    // Write functions are supposed to return the number 
    // of bytes actually inserted into the buffer
    //
    return length;
    }

static int xpi_ioctl
(
    struct inode* inode,
    struct file* file,
    unsigned int ioctl_num, // The number of the ioctl
    unsigned long ioctl_param // The parameter to it 
    )
{
    xpidev* dev = (xpidev*)file->private_data;
    int result = 0;

    // Switch according to the ioctl called 
    //
    switch( ioctl_num ) 
    {
        case IOCTL_XPI_SET_CHANNEL:
            dev->channel =  ioctl_param;
            break;

        case IOCTL_XPI_SET_PCM1_SOURCE:
            break;

        case IOCTL_XPI_SET_PCM1_TS:
            writeb( ioctl_param, xpi.mem_base + XPI_W_RPCM1_TS_SEL ); // Select time-slot
            break;

        case IOCTL_XPI_FC_COMMAND:
            result = xpi_FC_Command( ioctl_param );
            break;

        case IOCTL_XPI_GET_MCPU_FLAG:
            result = xpi.MCPU;
            break;
        };

    return result;
    }

unsigned int xpi_poll
(
    struct file* file,
    struct poll_table_struct* wait
    )
{
    xpidev* dev = (xpidev*)file->private_data;
    unsigned int mask = 0;

    // The buffer is circular; it is considered full if 'writep' is right
    // behind 'readp'.
    //
    //int left = ( xpi.inBuf.readp + xpi.inBuf.size - xpi.inBuf.writep ) % xpi.inBuf.size;

    poll_wait( file, &dev->inBuf.queue, wait );

    if ( dev->inBuf.size == 0 || dev->inBuf.readp != dev->inBuf.writep )
        mask |= POLLIN | POLLRDNORM; // readable

    //if( left != 1 )
    //    mask |= POLLOUT | POLLWRNORM; // writable

    return mask;
    }

///////////////////////////////////////////////////////////////////////////////
// File operations tables
//
static struct file_operations xpi_file_operations = 
{
    owner   : THIS_MODULE,
    open    : xpi_open,
    release : xpi_release,
    read    : xpi_read,
    write   : xpi_write,
    ioctl   : xpi_ioctl,
    poll    : xpi_poll
    };

///////////////////////////////////////////////////////////////////////////////
// Module initialization and cleanup

// Initialize the module - register the xpidev device
//
static int __init xpi_ModuleInit( void )
{
    int rc = 0; // Negative values signify an error

    // Register the character device (at least try) 
    //
    rc = register_chrdev( XPI_MAJOR_NUM, XPI_DEVICE_NAME, 
                               &xpi_file_operations );
    if ( rc ) // error
    {
        printk( KERN_ERR "xpidev: registering character device failed\n" );
        return rc;
        }

    memset( &xpi, 0, sizeof( xpi ) );

    if ( ! xpi_Construct( &xpi ) )
    {
        rc = unregister_chrdev( XPI_MAJOR_NUM, XPI_DEVICE_NAME );
        if ( rc ) // error
           printk( KERN_ERR "xpidev: unregister character device failed\n" );

        return -EBUSY;
        }

    printk( KERN_INFO "XPI module inserted.\n" );

    // If we return a non zero value, it means that module initialization
    // is failed and the kernel module can't be loaded 
    //
    return 0;
    }

// Cleanup - unregister the xpidev device
//
static void __exit xpi_ModuleCleanup( void )
{
    int rc;

    xpi_Destruct( &xpi );

    // Unregister the device
    //
    rc = unregister_chrdev( XPI_MAJOR_NUM, XPI_DEVICE_NAME );
    if ( rc ) // error
       printk( KERN_ERR "xpidev: unregister character device failed\n" );

    printk( KERN_INFO "XPI module removed.\n" );
    }

// Module entry & exit declarations
//
module_init( xpi_ModuleInit );
module_exit( xpi_ModuleCleanup );
