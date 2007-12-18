/*
    Project:     Albatross / HPI
  
    Module:      hpi.c
  
    Description: Albatross DSP HPI Linux device driver

    Platform          Supported Kernel
    at91rm9200/arm    >= 2.6.19
    i386              >= 2.6.11
  
    Copyright (c) 2002-2006 By Mikica B Kocic
    Copyright (c) 2002-2006 By IPTC Technology Communications AB
*/

//////////////////////////////////////////////////////////////////////////////

#include <linux/version.h>
#include <linux/kernel.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18)
#include <linux/utsrelease.h>
#endif
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
#include <linux/config.h>
#else
#include <linux/autoconf.h>
#endif
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>

//////////////////////////////////////////////////////////////////////////////

MODULE_LICENSE( "GPL" );

MODULE_DESCRIPTION( "TI c54x Device Driver" );
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

#ifdef CONFIG_PCI // ---------------------------------------------------------
#include <linux/pci.h>
#endif // CONFIG_PCI ---------------------------------------------------------

#include <asm/io.h>  
#include <asm/irq.h> // disable_irq/enable_irq

//#include <linux/sched.h>
//#include <linux/sys.h>
//#include <linux/fs.h>
//#include <linux/tqueue.h>
//#include <asm/system.h>
//#include <asm/segment.h>
//#include <asm/msr.h>
//#include <asm/uaccess.h>

#ifdef CONFIG_ARM // ---------------------------------------------------------

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
#include <asm/arch/gpio.h>
#include <asm/arch/at91rm9200_mc.h>
#else
#include <asm/arch/AT91RM9200_EMAC.h>
#include <asm/arch/pio.h>
#endif

#define ARM_USE_TIMER
#endif // CONFIG_ARM ---------------------------------------------------------

#include "hpi.h"

//////////////////////////////////////////////////////////////////////////////

#undef KERN_INFO
#define KERN_INFO

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

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
typedef enum { false = 0, true = 1 } bool;
#endif

enum MSGTYPE
{
	MSGTYPE_STDIO    = 0,
	MSGTYPE_IO_D     = 1,
	MSGTYPE_IO_B     = 2,
	MSGTYPE_WATCHDOG = 3,
	//
	// undefined message types should be skipped and not handled
	//
	MSGTYPE_CTRL     = 7
	};

enum SUBTYPE
{
    SUBTYPE_NONE   = 0,
    SUBTYPE_D_CHANNEL = 1,
    SUBTYPE_B_CHANNEL = 2
    };

//////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_PCI // ---------------------------------------------------------

enum
{
    PCI_DEVICE_ID_TI_PCI2040 = 0xAC60
    };

enum
{
    MAX_HPI_COUNT = 4
    };

enum // CSR offsets
{
    PCI2040_CSR_IRQ_EVENT_SET         = 0x00, // 32-bit
    PCI2040_CSR_IRQ_EVENT_CLEAR       = 0x04, // 32-bit
    PCI2040_CSR_IRQ_MASK_SET          = 0x08, // 32-bit
    PCI2040_CSR_IRQ_MASK_CLEAR        = 0x0C, // 32-bit
    PCI2040_CSR_HPI_ERROR_REPORT      = 0x10, // 16-bit
    PCI2040_CSR_HPI_RESET             = 0x14, // 16-bit
    PCI2040_CSR_HPI_DSP_EXIST         = 0x16, // 16-bit
    PCI2040_CSR_HPI_DATA_WIDTH        = 0x18  // 16-bit
    };

enum // DSP offsets (PCI AD14 & AD13 -> HCS0..3 )
{
    DSP_ADDR_OFFSET_0                 = 0x0000,
    DSP_ADDR_OFFSET_1                 = 0x2000,
    DSP_ADDR_OFFSET_2                 = 0x4000,
    DSP_ADDR_OFFSET_3                 = 0x6000
    };

enum // DSP HPI registers (PCI AD12 -> HCNTL1, AD11 -> HCNTL0 )
{
    DSP_HPIC_ADDR                     = 0x0000,
    DSP_HPIDpp_ADDR                   = 0x0800,
    DSP_HPIA_ADDR                     = 0x1000,
    DSP_HPID_ADDR                     = 0x1800
    };

#endif // CONFIG_PCI ---------------------------------------------------------

#ifdef CONFIG_ARM // ---------------------------------------------------------

// ebi2hpi_cpld flag indicates presence of EBI2HPI-CPLD, meaning that
// multiple DSPs are connected to EBI over CPLD. Otherwise, one and only one
// DSP is connected to EBI and with HRESET directly tied to PC15.
// 
static bool ebi2hpi_cpld = false;

enum
{
    MAX_HPI_COUNT = 1
    };

enum // DSP offsets  (EBI A5 & A4 decoded as HCS0..3)
{
    DSP_ADDR_OFFSET_0                 = 0x0000,
    DSP_ADDR_OFFSET_1                 = 0x0010,
    DSP_ADDR_OFFSET_2                 = 0x0020,
    DSP_ADDR_OFFSET_3                 = 0x0030
    };

enum // DSP HPI registers (EBI A2 -> HCNTL1, A1 -> HCNTL0 )
{
    DSP_HPIC_ADDR                     = 0x0000,
    DSP_HPIDpp_ADDR                   = 0x0002,
    DSP_HPIA_ADDR                     = 0x0004,
    DSP_HPID_ADDR                     = 0x0006
    };

#endif // CONFIG_ARM // ---------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// DSP & HPI classes

enum // HPI address map of the Scratch memory
{
    HPIA_INP_MSGQUE   = 0x00007A, // DSP's Inbound  Message Buffer pointer
    HPIA_OUT_MSGQUE   = 0x00007B, // DSP's Outbound Message Buffer pointer
    //                  0x00007C  // Not used
    //                  0x00007D  // Not used
    HPIA_BOOT_HIADDR  = 0x00007E, // Used by DSP ROM boot loader
    HPIA_BOOT_LOADDR  = 0x00007F  // Used by DSP ROM boot loader
    };

//////////////////////////////////////////////////////////////////////////////

typedef struct BANKDESC BANKDESC;
typedef struct DSP DSP;
typedef struct HPI HPI;
typedef struct chBuf chBuf;
typedef struct hpidev_c hpidev_c;
typedef struct hpidev_stdio hpidev_stdio;
typedef struct hpidev_io hpidev_io;
typedef enum MSGTYPE MSGTYPE;


// DSP: class that represents single DSP processor
//
// Methods:
//     inline void dsp_Set_HRESET( DSP* this, bool value );
//     inline void dsp_Pulse_HRESET( DSP* this );
//     inline unsigned int dsp_Get_HPIC( DSP* this );
//     inline unsigned int dsp_Get_HPIA( DSP* this );
//     inline unsigned int dsp_Get_HPID( DSP* this );
//     inline unsigned int dsp_Get_HPIDpp( DSP* this );
//     inline void dsp_Put_HPIC( DSP* this, unsigned short value );
//     inline void dsp_Put_HPIA( DSP* this, unsigned short value );
//     inline void dsp_Put_HPID( DSP* this, unsigned short value );
//     inline void dsp_Put_ppHPID( DSP* this, unsigned short value );
//     inline void dsp_Put_XHPIA( DSP* this, unsigned long addr );
//     bool dsp_VerifyExistence( DSP* this );
//     inline void dsp_Initialize( DSP* this );
//     inline void dsp_RunProgram( DSP* this, unsigned long addr );
//     int dsp_DispatchIncomingMessages( DSP* this );
//     int dsp_WriteToMsgBuf( DSP* this, MSGTYPE msg_type, unsigned char* data, int len );
//     inline bool dsp_Construct( DSP* this, HPI* hpi, int id, void* baseaddr );
//     inline void dsp_Destruct( DSP* this );
//
struct DSP
{
    HPI* hpi;                    // Parent / owner
    int ID;                      // DSP identifier (address)
    volatile bool exist;         // true if this DSP is found
    int ChipID;                  // DSP Chip Identifier
    int ChipRevision;            // DSP Chip Revision
    void* mem_base;              // Memory mapped base address to access DSP

    bool running;                // false if Reset and BOOT Loader phase; true if running

    volatile bool hint_arrived;  // true if HINT has arrived from the DSP
    long tasklet_count_tot;      // irq tasklet statistics

    spinlock_t dsp_lock;         // MUTEX of the structure and HPIA/D/C DSP regs

    volatile long ORIGIN_INP_MSGQUE; // HPIA origin of the DSP's Output MsgBuf header
    volatile long ORIGIN_OUT_MSGQUE; // HPIA origin of the DSP's Input MsgBuf header

    unsigned char temp_buf[ 3840 ]; // temporary buffer (to store messages)

    hpidev_stdio* dev_stdio_first; // list of owning hpidev_stdio devices; first in the list
    hpidev_io* dev_ioC_first;    // list of No channel hpidev_io devices; first in the list
    hpidev_io* dev_ioD_first;    // list of D channel hpidev_io devices; first in the list
    hpidev_io* dev_ioB_first;    // list of B channel hpidev_io devices; first in the list

    long dev_stdio_count;        // current hpidev_stdio devices count
    long dev_ioC_count;          // current hpidev_io devices count
    long dev_ioD_count;          // current hpidev_io devices count
    long dev_ioB_count;          // current hpidev_io devices count

    unsigned char dspwd_report[ 256 ]; // last DSP watchdog report
    };

// HPI: class that represents HPI interface PC-card
//
// Methods:
//     int hpi_DetectIRQ( HPI* this );
//     static void hpi_irq_handler( int irq, void* dev_id, struct pt_regs* regs );
//     static void hpi_irq_tasklet( unsigned long data );
//     bool hpi_Construct( HPI* this, int hpi_id, pci_dev* pci );
//     void hpi_Destruct( HPI* this );
//
struct HPI
{
    int ID;                      // HPI interface identifier (address)
    volatile bool exist;         // true if this HPI has at least one DSP

#ifdef CONFIG_PCI // ---------------------------------------------------------
    struct pci_dev* pcidev;      // associated PCI2040
    void* csr;                   // HPI Control and Status Registers (CSR)
#endif

    void* mem_base;              // DSP Control Space
    int irq;                     // irq number associated to HPI interface

    long irq_count_tot;          // irq handler statistics
    long tasklet_count_tot;      // irq tasklet statistics

    int dsp_count;               // Number of DSPs
    DSP dsp[ 4 ];                // DSPs found on this HPI; Max 4 DSPs

    struct tasklet_struct irq_tasklet; // bottom half of irq handler
    };

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
    // NOTE: DSP's mutex is used as tasklet write MUTEX
    };

// hpidev_c:  device class with 'control' capability
//
// Methods:
//     int hpidev_open( struct inode* inode, struct file* file );
//     int hpidev_c_release( struct inode* inode, struct file* file );
//     ssize_t hpidev_c_read( struct file* file, char* buffer, size_t length, loff_t* offset );
//     ssize_t hpidev_c_write( struct file* file, const char* buffer, size_t length,  loff_t* offset );
//     loff_t hpidev_c_llseek( struct file* file, loff_t offset, int whence );
//
struct hpidev_c
{
    HPI* hpi;                   // referred HPI
    DSP* dsp;                   // referred DSP
    loff_t offset;              // file offset used by read/write,
                                // i.e. local XHPIA address
    spinlock_t mutex;           // MUTEX protecting offset
    };

// hpidev_stdio: device class with 'debug' (or file i/o) capability
//
// Methods:
//     int hpidev_stdio_release( struct inode* inode, struct file* file );
//     ssize_t hpidev_stdio_read( struct file* file, char* buffer, size_t length, loff_t* offset );
//     ssize_t hpidev_stdio_write( struct file* file, const char* buffer, size_t length,  loff_t* offset );
//     unsigned int hpidev_d_poll( struct file* file, struct poll_table_struct* wait );
//
struct hpidev_stdio
{
    HPI* hpi;                    // referred HPI
    DSP* dsp;                    // referred DSP
    hpidev_stdio* next;          // next in hpidev_d list
    hpidev_stdio* prev;          // previous in hpidev_d list
    chBuf inBuf;                 // DSP output / our input character buffer
    };

// hpidev_io: device class with 'i/o' (i.e. communication) capability
//
// Methods:
//     int hpidev_io_release( struct inode* inode, struct file* file );
//     int hpidev_io_ioctl( struct inode* inode, struct file* file, unsigned int ioctl_num, unsigned long ioctl_param );
//     ssize_t hpidev_io_read( struct file* file, char* buffer, size_t length, loff_t* offset );
//     ssize_t hpidev_io_write( struct file* file, const char* buffer, size_t length,  loff_t* offset );
//     unsigned int hpidev_io_poll( struct file* file, struct poll_table_struct* wait );
//
struct hpidev_io
{
    HPI* hpi;                    // referred HPI
    DSP* dsp;                    // referred DSP
    hpidev_io* next;             // next in hpidev_io list
    hpidev_io* prev;             // previous in hpidev_io list
    chBuf inBuf;                 // input data buffer
    int subtype;                 // one of: SUBTYPE_*
    int subchannel;              // destination address filter; 0xFF == promiscuous mode, -1 == disabled
    int onCloseWriteLen;         // data that will be automatically written on close()
    unsigned char onCloseWrite[ 128 ];
    int channelInfoLen;
    char channelInfo[ 128 ];     // descriptive information about the I/O channel
    };

//////////////////////////////////////////////////////////////////////////////
// Static data declarations
//
static struct file_operations hpidev_c_file_operations;
static struct file_operations hpidev_stdio_file_operations;
static struct file_operations hpidev_io_file_operations;

static int hpiCount = 0;
static HPI system_hpi[ MAX_HPI_COUNT ]; // hpi interfaces known to linux

//
// System (Module) methods:
//
// int hpidev_proc_read
//    ( char* buf, char **start, off_t offset, int count, int* eof, void* data );
//
// static int hpidev_ModuleInit( void );
// static void hpidev_ModuleCleanup( void );
//

///////////////////////////////////////////////////////////////////////////////
// chBuf class methods

void chBuf_Construct( chBuf* this )
{
    this->buffer = kmalloc( 16384, GFP_KERNEL );

    this->writep = 0;
    this->readp = 0;
    this->size = 16384;

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

void chBuf_TaskletWriteD( chBuf* this, const char* data, int len )
{
    int i;
    int wp; // write-out pointer
    int wp_bound; // upper (inclusive) bound for write-out pointer
    int bufsize;

    // Writing a NULL pointer or negative length signals chBuf readers
    // to report EOF, i.e. to release hpidev_stdio devices
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
    // Note.This segment is protected by DSP's spin lock.
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

void chBuf_TaskletWriteIO( chBuf* this, const unsigned char* data, int len )
{
    int i;
    int wp; // write-out pointer
    int left; // left space in buffer
    int maxsize;

    // Writing a NULL pointer or negative length signals chBuf readers
    // to report EOF, i.e. to release hpidev_stdio devices
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
    // Note.This segment is protected by DSP's spin lock.
    //
    wp = this->writep;
    maxsize = this->size;
    left = maxsize - 1 - ( wp + maxsize - this->readp ) % maxsize; // left space in buffer

    if ( len + 4 > left )
    {
        // printk( KERN_WARNING "I/O Packet dropped %d %d!\n", len, left );
        return; // Drop the packet if there is no space left in buffer
        }

    // Store packet length; MSB first
    this->buffer[ wp ] = ( len >> 8 ) & 0xFF;
    if ( ++wp >= this->size )
        wp = 0;
    this->buffer[ wp ] = len & 0xFF;
    if ( ++wp >= this->size )
        wp = 0;

    // Store packet contents
    for ( i = 0; i < len; i++ )
    {
        this->buffer[ wp ] = data[ i ];
        if ( ++wp >= this->size )
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
// DSP class methods

static inline void dsp_Set_ENABLE( DSP* this, bool enable )
{
#ifdef CONFIG_PCI // ---------------------------------------------------------

    unsigned short bitmask = readw( this->hpi->csr + PCI2040_CSR_HPI_DSP_EXIST );

    if ( enable )
    {
        bitmask |= ( 1 << this->ID ); // turn on bit
        }
    else
    {
        bitmask &= ~( 1 << this->ID ); // turn off bit
        }

    writew( bitmask, this->hpi->csr + PCI2040_CSR_HPI_DSP_EXIST );

#endif // CONFIG_PCI ---------------------------------------------------------
    }

static inline void dsp_Set_HRESET( DSP* this, bool hreset )
{
#ifdef CONFIG_PCI // ---------------------------------------------------------

    unsigned short bitmask = readw( this->hpi->csr + PCI2040_CSR_HPI_RESET );

    if ( hreset )
    {
        this->running = false;
        bitmask |= ( 1 << this->ID ); // turn on bit
        }
    else
    {
        bitmask &= ~( 1 << this->ID ); // turn off bit
        }

    writew( bitmask, this->hpi->csr + PCI2040_CSR_HPI_RESET );

#endif // CONFIG_PCI ---------------------------------------------------------

#ifdef CONFIG_ARM // ---------------------------------------------------------

    if ( ebi2hpi_cpld )
    {
        // With CPLD: individual reset through CPLD
        //
        if ( hreset )
            writeb( 0x01, this->mem_base + 0x0008 ); // Set HRST[]
        else
            writeb( 0x00, this->mem_base + 0x0008 ); // Clear HRST[]
        }
    else
    {
        // Without CPLD: PC15 tied to HRESET
        //
        if ( hreset )
            at91_set_gpio_value( AT91_PIN_PC15, 1 ); // RESET <= '1'
        else
            at91_set_gpio_value( AT91_PIN_PC15, 0 ); // RESET <= '0'
        }

#endif // CONFIG_ARM ---------------------------------------------------------
    }

static inline void dsp_Pulse_HRESET( DSP* this )
{
    dsp_Set_HRESET( this, true );

    mdelay( 1 ); // hold HRESET for 1ms

    dsp_Set_HRESET( this, false );

    mdelay( 1 ); // wait 1ms
    }

static inline unsigned int dsp_Get_HPIC( DSP* this )
{
    return readw( this->mem_base + DSP_HPIC_ADDR );
    }

static inline unsigned int dsp_Get_HPIA( DSP* this )
{
    return readw( this->mem_base + DSP_HPIA_ADDR );
    }

static inline unsigned int dsp_Get_HPID( DSP* this )
{
    return readw( this->mem_base + DSP_HPID_ADDR );
    }

static inline unsigned int dsp_Get_HPIDpp( DSP* this )
{
    return readw( this->mem_base + DSP_HPIDpp_ADDR );
    }

static inline void dsp_Put_HPIC( DSP* this, unsigned short value )
{
    value |= 0x01; // always set BOB to be 1 (LSB first; intel platform)
    value &= 0xFF; // truncate to 8 bits
    value = ( value << 8 ) | value; // upper byte should be repeated lower byte
    writew( value, this->mem_base + DSP_HPIC_ADDR );
    }

static inline void dsp_Put_HPIA( DSP* this, unsigned short value )
{
    writew( value, this->mem_base + DSP_HPIA_ADDR );
    }

static inline void dsp_Put_HPID( DSP* this, unsigned short value )
{
    writew( value, this->mem_base + DSP_HPID_ADDR );
    }

static inline void dsp_Put_ppHPID( DSP* this, unsigned short value )
{
    writew( value, this->mem_base + DSP_HPIDpp_ADDR );
    }

static inline void dsp_Put_XHPIA( DSP* this, unsigned long addr )
{
    int xaddr = ( addr >> 16 ) & 0xFF;

    dsp_Put_HPIC( this, 0x10 ); // turn on XHPIA bit in HPIC

    // XHPIA contains bits 23-16 of 'addr'. During the write access,
    // the first and second byte XHPIA values are both written to 
    // the same register!
    //
    dsp_Put_HPIA( this, ( xaddr << 8 ) | xaddr );

    dsp_Put_HPIC( this, 0x00 ); // turn off XHPIA bit in HPIC

    dsp_Put_HPIA( this, addr & 0xFFFF );
    }

static bool dsp_VerifyExistence( DSP* this, bool silent )
{
    int test_pattern = 0x3141 + this->ID;

    dsp_Set_ENABLE( this, true ); // Enable DSP on the bus

    this->exist = false;

    // Reset DSP. If DSP is present, this will make
    // DSP bootloader to generate HINT.
    //
    dsp_Pulse_HRESET( this );

    // Set XHPIA location 00:1000h; Writing to XHPIA will
    // also initialize BLOB in HPIC
    //
    dsp_Put_XHPIA( this, 0x001000 );

    // Verify that HINT and BLOB are corretly set in HPIC.
    //
    if ( dsp_Get_HPIC( this ) != 0x0909 )
    {
        if ( ! silent )
        {
            printk( KERN_WARNING "hpi%d:dsp%d: (base=%p) Failed HPIC sanity check: %04x, expected %04x\n",
                this->hpi->ID, this->ID, this->mem_base, dsp_Get_HPIC( this ), 0x0909 );
            }

        dsp_Set_ENABLE( this, false ); // Disable DSP on the bus
        dsp_Set_HRESET( this, true ); // Leave DSP in reset state

        return false; // Failed HPIC sanity check
        }

    // Acknowledge interrupt (irq handler will not acknowledge interrupt
    // because we did not set DSP's 'exist flag' to be true)
    //
    dsp_Put_HPIC( this, 0x08 ); // acknowledge HINT

    // Verify that HPIA is set to correct value
    //
    if ( dsp_Get_HPIA( this ) != 0x1000 )
    {
        if ( ! silent )
        {
            printk( KERN_WARNING "hpi%d:dsp%d: Failed HPIA sanity check: %04x, expected %04x\n",
                this->hpi->ID, this->ID, dsp_Get_HPIA( this ), 0x1000 );
            }

        dsp_Set_ENABLE( this, false ); // Disable DSP on the bus
        dsp_Set_HRESET( this, true ); // Leave DSP in reset state

        return false; // Failed HPIA sanity check
        }

    // Fill location 00:1000h with test pattern
    //
    dsp_Put_HPIA  ( this, 0x1000 );
    dsp_Put_HPID  ( this, 0xAAAA );
    dsp_Put_ppHPID( this, 0xAAAA );

    dsp_Put_HPIA  ( this, 0x1000 );
    dsp_Put_HPID  ( this, 0x5555 );
    dsp_Put_ppHPID( this, 0x5555 );

    dsp_Put_HPIA  ( this, 0x1000 );
    dsp_Put_HPID  ( this, test_pattern );
    dsp_Put_ppHPID( this, 0x0000 );

    dsp_Put_HPIA( this, 0x1000 );

    // Verify that location 00:1000h was filled with the correct pattern
    //
    if ( dsp_Get_HPID( this ) != test_pattern )
    {
        if ( ! silent )
        {
            printk( KERN_WARNING "hpi%d:dsp%d: Failed HPID sanity check: %04x, expected %04x\n",
               this->hpi->ID, this->ID, dsp_Get_HPID( this ), test_pattern );
            }

        dsp_Set_ENABLE( this, false ); // Disable DSP on the bus
        dsp_Set_HRESET( this, true ); // Leave DSP in reset state

        return false; // Failed HPID sanity check
        }

    // Load DSP code to get DSP ID at address 0x1000.
    // Code should store Chip ID at address 0x1100.
    //
    dsp_Put_HPIA( this, 0x1000 ); // Start Address

    dsp_Put_ppHPID( this, 0x10F8 ); // LD  *(0x003E), A; Load ACC with Chip ID & Revision
    dsp_Put_ppHPID( this, 0x003E );
    dsp_Put_ppHPID( this, 0x80F8 ); // STL A, *(0x1100); Store ACC to location 1100h
    dsp_Put_ppHPID( this, 0x1100 );
    dsp_Put_ppHPID( this, 0xF073 ); // L1: B L1        ; Freeze
    dsp_Put_ppHPID( this, 0x0004 );

    // Clear data at address 0x1100 (it will contain Chip ID).
    //
    dsp_Put_HPIA( this, 0x1100 );
    dsp_Put_HPID( this, 0x0000 );

    // Run Program at address 0x1000.
    //
    dsp_Put_HPIA( this, HPIA_BOOT_HIADDR );
    dsp_Put_HPID( this, 0x0000 );
    dsp_Put_HPIA( this, HPIA_BOOT_LOADDR );
    dsp_Put_HPID( this, 0x1000 );

    // Wait for data at 0x1100 to become != 0, maximum 10 times x 1 ms.
    //
    dsp_Put_HPIA( this, 0x1100 );
    {
        int i;
        for ( i = 0; i < 10; i++ )
        {
            mdelay( 1 ); // wait a while

            if ( dsp_Get_HPID( this ) != 0 )
                break;
            }

        if ( i >= 10 && ! silent )
        {
            printk( KERN_WARNING "hpi%d:dsp%d: Failed to get DSP Chip ID.\n",
                   this->hpi->ID, this->ID );
            }
        }

    // Get Chip ID & Revision. Note that result will be 0, 
    // if the DSP program failed to run.
    // 
    this->ChipID = ( ( dsp_Get_HPID( this ) >> 8 ) & 0xFF );
    this->ChipRevision = ( ( dsp_Get_HPID( this ) >> 4 ) & 0xF );

    // Reset DSP again
    //
    dsp_Pulse_HRESET( this );

    dsp_Put_XHPIA( this, 0x001100 ); // Point to Chip ID

    dsp_Put_HPIC( this, 0x08 ); // acknowledge HINT

    // Report DSP data.
    //
    if ( ! silent )
    {
        printk( KERN_INFO "hpi%d:dsp%d: TMS320VC54%02xR%01x, "
            "Base 0x%p; HPIC: %04x, HPIA: %04x, HPID: %04x\n",
            this->hpi->ID, this->ID, this->ChipID, this->ChipRevision,
            this->mem_base,
            dsp_Get_HPIC( this ), dsp_Get_HPIA( this ), dsp_Get_HPID( this )
            );
        }

    // Be paranoid and clear HINT interrupt request, again.
    //
    dsp_Put_HPIC( this, 0x08 ); // acknowledge HINT

    this->exist = true;

    return true;
    }

static inline void dsp_Initialize( DSP* this )
{
    if ( this->ORIGIN_INP_MSGQUE )
    {
        long addr = this->ORIGIN_INP_MSGQUE;
        this->ORIGIN_INP_MSGQUE = 0;

        // Initialize input MsgQueue
        //
        dsp_Put_XHPIA( this, addr ); // message queue origin
        dsp_Put_HPID( this, 0x0000 ); // maxlen
        dsp_Put_ppHPID( this, 0x0000 ); // begin
        dsp_Put_ppHPID( this, 0x0000 ); // end

        dsp_Put_XHPIA( this, HPIA_INP_MSGQUE );
        dsp_Put_HPID( this, 0x0000 );
        }

    if ( this->ORIGIN_OUT_MSGQUE )
    {
        long addr = this->ORIGIN_OUT_MSGQUE;
        this->ORIGIN_OUT_MSGQUE = 0;

        // Initialize output MsgQueue
        //
        dsp_Put_XHPIA( this, addr ); // message queue origin
        dsp_Put_HPID( this, 0x0000 ); // maxlen
        dsp_Put_ppHPID( this, 0x0000 ); // begin
        dsp_Put_ppHPID( this, 0x0000 ); // end

        dsp_Put_XHPIA( this, HPIA_OUT_MSGQUE );
        dsp_Put_HPID( this, 0x0000 );
        }
    }

static inline void dsp_RunProgram( DSP* this, unsigned long addr )
{
    memset( this->dspwd_report, 0, sizeof( this->dspwd_report ) );

    spin_lock_bh( &this->dsp_lock );

    this->running = true;

    // Load upper half of starting address to 00:007Eh location.
    //
    dsp_Put_HPIA( this, HPIA_BOOT_HIADDR );
    dsp_Put_HPID( this, ( addr >> 16 ) & 0xFFFF );

    // Load lower half of starting address to 00:007Fh location.
    // This will trigger DSP bootloader to branch to that address.
    //
    dsp_Put_HPIA( this, HPIA_BOOT_LOADDR );
    dsp_Put_HPID( this, addr & 0xFFFFF );

    spin_unlock_bh( &this->dsp_lock );
    }

static void dsp_Reset( DSP* this, bool send_kill )
{
    hpidev_stdio* dev_stdio;
    hpidev_io* dev_io;

    spin_lock_bh( &this->dsp_lock );

    dsp_Pulse_HRESET( this );

    dsp_Initialize( this );

    // Send KILL to all associated hpidev devices
    //
    dev_stdio = this->dev_stdio_first;
    while( dev_stdio )
    {
        chBuf_TaskletWriteD( &dev_stdio->inBuf, NULL, 0 );
        dev_stdio = dev_stdio->next;
        }

    dev_io = this->dev_ioC_first;
    while( dev_io )
    {
        chBuf_TaskletWriteIO( &dev_io->inBuf, NULL, 0 );
        dev_io = dev_io->next;
        }

    dev_io = this->dev_ioD_first;
    while( dev_io )
    {
        chBuf_TaskletWriteIO( &dev_io->inBuf, NULL, 0 );
        dev_io = dev_io->next;
        }

    dev_io = this->dev_ioB_first;
    while( dev_io )
    {
        chBuf_TaskletWriteIO( &dev_io->inBuf, NULL, 0 );
        dev_io = dev_io->next;
        }

    spin_unlock_bh( &this->dsp_lock );
    }

// Check DSP message buffer for new messages.
// Return true if we have found (and processed) a message.
// This routing should be called with spin locked dsp_lock
// and under irq tasklet only. Return number of processed messages.
//
static int dsp_DispatchIncomingMessages( DSP* this )
{
    unsigned char* data;
    int i;
    int maxlen, begin, end, curp, buf_cont_size;
    int len, wordcount;
    int msg_type;
    int msg_count = 0;

    if ( ! this->ORIGIN_INP_MSGQUE )
    {
        dsp_Put_XHPIA( this, HPIA_INP_MSGQUE );
        this->ORIGIN_INP_MSGQUE  = dsp_Get_HPID( this );
        }

    if ( ! this->ORIGIN_INP_MSGQUE )
        return 0;

DISPATCH_LOOP:

    // Read msgbuf header from DSP shared memory

    dsp_Put_XHPIA( this, this->ORIGIN_INP_MSGQUE );
    maxlen  = dsp_Get_HPIDpp( this );

    if ( maxlen == 0 ) // If buffer is not initialized yet
        return msg_count;

    begin = dsp_Get_HPIDpp( this );
    end = dsp_Get_HPIDpp( this );

    if ( begin == end ) // There was no message!!!
        return msg_count;

    if ( begin < 0 || begin >= maxlen || end < 0 || end >= maxlen )
    {
        // Invalid buffer header.
        // We should reinitialize shared memory (i.e. put begin==end)
        // FIXME: what to do when end is invalid?
        //
        dsp_Put_HPIA( this, this->ORIGIN_INP_MSGQUE + 1 );
        dsp_Put_HPID( this, end );
        return msg_count;
        }

    // Calculate contents size of the buffer
    //
    buf_cont_size = ( end + maxlen - begin ) % maxlen;

    // Read msgbuf packet from DSP shared memory
    //
    dsp_Put_HPIA( this, this->ORIGIN_INP_MSGQUE + 3 + begin );
    curp = begin;

    // First word is packet length with encoded type as MSBit
    //
    len = dsp_Get_HPIDpp( this );
    if ( ++curp >= maxlen )
    {
        curp = 0;
        dsp_Put_HPIA( this, this->ORIGIN_INP_MSGQUE + 3 + curp );
        };
    msg_type = ( len >> 12 ) & 0x7; // msg_type: bits 14..12
    len &= ~0xF800; // length: bits 10..0

    // len is always a number of octets
    // wordcount will be number of 16-bit words
    //
    wordcount = ( len + 1 ) / 2;

    if ( wordcount + 1 > buf_cont_size )
    {
        // Invalid packet size; it should be less then actual size
        // of the msgbuf contents.
        // We should reinitialize shared memory (i.e. put begin==end)
        //
        dsp_Put_HPIA( this, this->ORIGIN_INP_MSGQUE + 1 );
        dsp_Put_HPID( this, end );
        return msg_count;
        }

    // We are reading new message; count it.
    //
    msg_count++;

    // Read data from shared memory
    // We will store data in temporary buffer
    data = this->temp_buf;
    for ( i = 0; i < len; i += 2 )
    {
        int hpid = dsp_Get_HPIDpp( this );
        if ( ++curp >= maxlen )
        {
            curp = 0;
            dsp_Put_HPIA( this, this->ORIGIN_INP_MSGQUE + 3 + curp );
            };

        *data++ = ( hpid >> 8 ) & 0xFF; // MSB

        if ( i + 1 < len )
            *data++ = hpid & 0xFF; // LSB
        }

    *data = 0x00; // '\0'-terminate data; in case we expect ASCIIZ

    // data will point again to the beginning of parsed packet 
    data = this->temp_buf;

    // Update msgbuf read pointer in shared memory
    dsp_Put_HPIA( this, this->ORIGIN_INP_MSGQUE + 1 );
    dsp_Put_HPID( this, ( begin + 1 + wordcount ) % maxlen );

    if ( msg_type == MSGTYPE_WATCHDOG ) // handle watchdog type of message
    {
        hpidev_stdio* dev_stdio = this->dev_stdio_first;
#if 0
        printk( "hpi%d:dsp%d: WD-MSG: len=%d, wordcount=%d, begin=%d, end=%d\n",
            this->hpi->ID, this->ID, len, wordcount, begin, end );
#endif
        this->dspwd_report[ 0 ] = len; // The len is in bytes !
        memcpy( this->dspwd_report + 1, data, len );

        // Distribute Watchdog message
        //
        while( dev_stdio )
        {
            chBuf_TaskletWriteD( &dev_stdio->inBuf, "WD\n", 3 );
            dev_stdio = dev_stdio->next;
            }
        }
    else if ( msg_type == MSGTYPE_IO_B ) // handle I/O B-Channel messsage type
    {
        hpidev_io* dev_io = this->dev_ioB_first;
#if 0
        static char temp[ 512 ];
        for ( i = 0; i < len; i++ )
            sprintf( temp + i*3, " %02x", (int)data[ i ] );
        printk( "hpi%d:dsp%d: IO-B-MSG: len=%d: %s\n",
            this->hpi->ID, this->ID, len, temp );
#endif
        while( dev_io )
        {
            if ( dev_io->subchannel == data[ 0 ] )
                chBuf_TaskletWriteIO( &dev_io->inBuf, data, len );

            dev_io = dev_io->next;
            }
        }
    else if ( msg_type == MSGTYPE_IO_D ) // handle I/O D-Channel messsage type
    {
        hpidev_io* dev_io = this->dev_ioD_first;
#if 0
        static char temp[ 512 ];
        for ( i = 0; i < len; i++ )
            sprintf( temp + i*3, " %02x", (int)data[ i ] );
        printk( "hpi%d:dsp%d: IO-D-MSG: len=%d: %s\n",
            this->hpi->ID, this->ID, len, temp );
#endif
        while( dev_io )
        {
            if ( dev_io->subchannel == data[ 0 ] )
                chBuf_TaskletWriteIO( &dev_io->inBuf, data, len );

            dev_io = dev_io->next;
            }
        }
    else if ( msg_type == MSGTYPE_STDIO ) // handle debug type of message
    {
        hpidev_stdio* dev_stdio = this->dev_stdio_first;
#if 0
        printk( "hpi%d:dsp%d: D-MSG (%d): %d %d %d %d : %s%s", this->hpi->ID, this->ID, len, 
            (int)data[ 0 ], (int)data[ 1 ], (int)data[ 2 ], (int)data[ 3 ],
            data, data[ len - 1 ] != '\n' ? "<noLF>\n" : "" );
#endif
        while( dev_stdio )
        {
            chBuf_TaskletWriteD( &dev_stdio->inBuf, data, len );
            dev_stdio = dev_stdio->next;
            }
        }

    goto DISPATCH_LOOP;
    // return msg_count;
    }

int dsp_WriteToMsgBuf( DSP* this, MSGTYPE msg_type, unsigned char* data, int len )
{
    int i;
    int maxlen, begin, end, curp, buf_free_size;
    int wordcount;

    if ( len >= 0x400 )
        return 0;

    if ( ! this->ORIGIN_OUT_MSGQUE )
    {
        dsp_Put_XHPIA( this, HPIA_OUT_MSGQUE );
        this->ORIGIN_OUT_MSGQUE  = dsp_Get_HPID( this );
        }

    if ( ! this->ORIGIN_OUT_MSGQUE )
        return 0;

    // Read msgbuf header from DSP shared memory
    //
    dsp_Put_XHPIA( this, this->ORIGIN_OUT_MSGQUE );
    maxlen  = dsp_Get_HPIDpp( this );

    if ( maxlen == 0 ) // If buffer is not initialized yet
        return 0;

    begin = dsp_Get_HPIDpp( this );
    end = dsp_Get_HPIDpp( this );

    // Calculate contents size of the buffer
    //
    buf_free_size = maxlen - ( end + maxlen - begin ) % maxlen;

    // len is always a number of octets
    // wordcount will be number of 16-bit words
    wordcount = ( len + 1 ) / 2;

    // Check if what we want to write can fit the msgbuf?
    if ( wordcount + 2 > buf_free_size )
    {
        printk( KERN_INFO "wordcount %d, free size %d\n", wordcount, buf_free_size );
        return 0;
        }

    // Write msgbuf packet to DSP shared memory
    //
    dsp_Put_HPIA( this, this->ORIGIN_OUT_MSGQUE + 3 + end );
    curp = end;

    // First word is packet length with encoded type as 2 MSBits
    //
    dsp_Put_HPID( this, ( ( msg_type & 0x07 ) << 12 ) | len );

    // Write data to shared memory
    //
    for ( i = 0; i < len; i += 2 )
    {
        int x = ( data[ i ] << 8 );
        if ( i + 1 < len )
            x |= data[ i + 1 ];

        if ( ++curp >= maxlen )
        {
            curp = 0;
            dsp_Put_HPIA( this, this->ORIGIN_OUT_MSGQUE + 3 + curp );
            dsp_Put_HPID( this, x );
            }
        else
        {
            dsp_Put_ppHPID( this, x );
            }
        }

    // Update msgbuf write pointer in shared memory
    dsp_Put_HPIA( this, this->ORIGIN_OUT_MSGQUE + 2 );
    dsp_Put_HPID( this, ( end + 1 + wordcount ) % maxlen );

    dsp_Put_HPIC( this, 0x04 ); // generate DSP_INT

    return len;
    }

static inline bool dsp_Construct( DSP* this, HPI* hpi, int id )
{
    // Initialize members
    //
    this->hpi = hpi;
    this->ID = id;

    this->exist = false;
    this->running = false;

    this->mem_base = NULL;

    switch ( id )
    {
        case 0: this->mem_base = hpi->mem_base + DSP_ADDR_OFFSET_0; break;
        case 1: this->mem_base = hpi->mem_base + DSP_ADDR_OFFSET_1; break;
        case 2: this->mem_base = hpi->mem_base + DSP_ADDR_OFFSET_2; break;
        case 3: this->mem_base = hpi->mem_base + DSP_ADDR_OFFSET_3; break;
        default:
            return false;
        }

    this->hint_arrived = false;
    this->tasklet_count_tot = 0;

    this->ORIGIN_INP_MSGQUE = 0;
    this->ORIGIN_OUT_MSGQUE = 0;

    this->dev_stdio_first = NULL;
    this->dev_ioC_first = NULL;
    this->dev_ioD_first = NULL;
    this->dev_ioB_first = NULL;

    this->dev_stdio_count = 0;
    this->dev_ioC_count = 0;
    this->dev_ioD_count = 0;
    this->dev_ioB_count = 0;

    memset( this->dspwd_report, 0, sizeof( this->dspwd_report ) );

    spin_lock_init( &this->dsp_lock );

    return true;
    }

static inline void dsp_Destruct( DSP* this )
{
    if ( ! this->exist )
        return;

    dsp_Reset( this, /* send kill */ true );

    spin_lock_bh( &this->dsp_lock );

    this->exist = false;

    dsp_Set_ENABLE( this, false ); // Disable DSP from the bus
    dsp_Set_HRESET( this, true ); // Leave DSP in reset state

    spin_unlock_bh( &this->dsp_lock );
    }

//////////////////////////////////////////////////////////////////////////////
// HPI class methods

// Top half of the common HPI interrupt service routine
// 
static irqreturn_t hpi_irq_handler
(
    int irq, // interrupt that has occured
    void* dev_id, // pointer gives as argument for request_irq
    struct pt_regs* regs // holds a snapshot of cpu registers before interrupt
    )
{
    HPI* hpi = (HPI*)dev_id;
    bool perform_tasklet = false;
    int i;

    // BUGF printk( "I" );

#ifdef CONFIG_PCI // ---------------------------------------------------------

    unsigned long event = readl( hpi->csr + PCI2040_CSR_IRQ_EVENT_SET );

    hpi->irq_count_tot ++;

    for ( i = 0; i < hpi->dsp_count; i++ )
    {
        DSP* dsp = &hpi->dsp[i];

        if ( dsp->exist 
            && 
              (
                ( dsp_Get_HPIC( dsp ) & 0x0808 ) ||
                ( ( 1 << i ) & event )
              )
            )
        {
            dsp_Put_HPIC( dsp, 0x08 ); // acknowledge HINT
            writel( 1 << i, hpi->csr + PCI2040_CSR_IRQ_EVENT_CLEAR ); // clear event
            dsp->hint_arrived = true;
            perform_tasklet = true;
            }
        }

#endif // CONFIG_PCI ---------------------------------------------------------

#ifdef CONFIG_ARM // ---------------------------------------------------------

    hpi->irq_count_tot ++;

    for ( i = 0; i < hpi->dsp_count; i++ )
    {
        DSP* dsp = &hpi->dsp[i];

        if ( dsp->exist 
            && ( dsp_Get_HPIC( dsp ) & 0x0808 )
            )
        {
            dsp_Put_HPIC( dsp, 0x08 ); // acknowledge HINT
            // AT91_SYS->AIC_ICCR = AT91C_ID_IRQ0; //  acknowledge interrupt in PIO
            dsp->hint_arrived = true;
            perform_tasklet = true;
            }
        }

#endif // CONFIG_ARM ---------------------------------------------------------

    // schedule tasklet only if it has something to do
    //
    if ( perform_tasklet )
        tasklet_schedule( &hpi->irq_tasklet );

    return IRQ_HANDLED;
    }

#ifdef ARM_USE_TIMER // -------------------------------------------------------

static struct timer_list timer_1kHz;

static void hpi_timer( unsigned long param )
{
    HPI* hpi = &system_hpi[ 0 ];
    bool perform_tasklet = false;
    int i;

    // BUGF printk( "T" );

    hpi->irq_count_tot ++;

    for ( i = 0; i < hpi->dsp_count; i++ )
    {
        DSP* dsp = &hpi->dsp[i];

        if ( dsp->exist 
            && ( dsp_Get_HPIC( dsp ) & 0x0808 )
            )
        {
            // BUGF printk( "%c", i + '5' );
            dsp_Put_HPIC( dsp, 0x08 ); // acknowledge HINT
            dsp->hint_arrived = true;
            perform_tasklet = true;
            }
        }

    // schedule tasklet only if it has something to do
    //
    if ( perform_tasklet )
        tasklet_schedule( &hpi->irq_tasklet );

    timer_1kHz.expires = jiffies + 1;
    add_timer( &timer_1kHz );
    }

#endif // ARM_USE_TIMER ------------------------------------------------------

// Bottom half of the common HPI interrupt service routine
// 
static void hpi_irq_tasklet( unsigned long data )
{
    HPI* hpi = (HPI*)data;
    int dsp_id;

    hpi->tasklet_count_tot ++;

    for ( dsp_id = 0; dsp_id < hpi->dsp_count; dsp_id++ )
    {
        DSP* dsp = &hpi->dsp[ dsp_id ];

        if ( ! dsp->exist )
            continue;

        if ( ! dsp->hint_arrived )
            continue;

        dsp->hint_arrived = false;

        dsp->tasklet_count_tot++;

        // BUGF printk( "%c", 'A' + dsp_id );

        spin_lock( &dsp->dsp_lock );

        // Get all arrived messages, and dispatch them
        // until message queue is empty
        //
        dsp_DispatchIncomingMessages( dsp );

        spin_unlock( &dsp->dsp_lock );
        }
    }

static bool hpi_Construct( HPI* this, int hpi_id )
{
    int i;
    int rc = 0;

    // Initialize members
    //
    this->ID = hpi_id;
    this->exist = true;

    this->mem_base = NULL;
    this->irq = -1;
    this->irq_count_tot = 0;
    this->tasklet_count_tot = 0;

    this->dsp_count = 4;

    tasklet_init( &this->irq_tasklet, hpi_irq_tasklet, (unsigned long)this );

#ifdef CONFIG_PCI // ---------------------------------------------------------

    if ( this->pcidev == NULL )
	return false; // pcidev should be set by the caller !!!

    this->csr = NULL;

    // HPI Control and Status Registers
    printk( "BAR0: %08lx - %08lx, len = %ld\n",
        pci_resource_start( this->pcidev, 0 ),
        pci_resource_end( this->pcidev, 0 ),
        pci_resource_len( this->pcidev, 0 )
        );

    // DSP Control Space
    printk( "BAR1: %08lx - %08lx, len = %ld\n",
        pci_resource_start( this->pcidev, 1 ),
        pci_resource_end( this->pcidev, 1 ),
        pci_resource_len( this->pcidev, 1 )
        );

    rc = pci_enable_device( this->pcidev );
    if ( rc ) // error
        ;

    rc = pci_request_regions( this->pcidev, HPI_DEVICE_NAME );
    if ( rc ) // error
        ;

    // Remap IO Memory

    this->csr = ioremap( pci_resource_start( this->pcidev, 0 ), pci_resource_len( this->pcidev, 0 ) );

    this->mem_base = ioremap( pci_resource_start( this->pcidev, 1 ), pci_resource_len( this->pcidev, 1 ) );

    // Read IRQ from PCI configuration space
    //
    this->irq = this->pcidev->irq;

#endif // CONFIG_PCI ---------------------------------------------------------

#ifdef CONFIG_ARM // ---------------------------------------------------------

    // Configure PC15 to be output, initially set high
    //
    at91_set_gpio_output( AT91_PIN_PC15, 1 ); // RESET <= '1'

    // Configure CS2 to access DSP as LCD-like interface
    // i.e. ACCS set to 1 cycle less at the beginning and the end of the access.
    // Default is to have 7 wait states, to accomodate CPLD latency.
    // WS settings will be lowered later, if CPLD is not detected.
    //
    // CSR2 settings and speed measurements:
    //
    // CSR2 = 00014087, Speed = 37.751 Mbps  (must be set when used EBI2HPI CPLD)
    // CSR2 = 00014085, Speed = 44.887 Mbps  (optional, when EBI2HPI CPLD is not used)
    //
    at91_sys_write( AT91_SMC_CSR(2)
       , AT91_SMC_RWHOLD_(0)  // Read and Write Signal Hold: 0 cycles
       | AT91_SMC_RWSETUP_(0) // Read and Write Signal Setup: 0 cycles
       | AT91_SMC_ACSS_1      // Address Chip Select Setup: 1 cycle
                              // Data Read Protocol: Standard (not early)
       | AT91_SMC_DBW_8       // Data Bus Width: 8-bit
                              // Byte Access Type: CS connected to two 8-bit wide devices
       | AT91_SMC_TDF_(0)     // Data Float Time: 0 cycles
       | AT91_SMC_WSEN        // Wait State Enable: Enabled NWS
       | AT91_SMC_NWS_(7)     // Number of Wait States: 1; One wait state is 16 ns
                              // 7 -> HDS wdith = 5 x TMCK ~= 80ns
       );

    // Remap memory; CS2 is connected to DSP
    //
    this->mem_base = ioremap( AT91_CHIPSELECT_2, 1 << 16 );

#ifndef ARM_USE_TIMER

    // Configure and allocate interrupt
    //
    at91_set_A_periph( AT91_PIN_PB29, 1 ); // Disable PB29, use it as IRQ0
    this->irq = AT91RM9200_ID_IRQ0;
    set_irq_type( this->irq, AT91_AIC_SRCTYPE_LOW );

#endif

#endif // CONFIG_ARM ---------------------------------------------------------

    // Initialize DSP structures
    //
    rc = 0;
    for ( i = 0; i < this->dsp_count; i++ )
    {
        if ( ! dsp_Construct( &this->dsp[ i ], this, i ) )
            rc = -ENODEV;
        }
    if ( rc ) // error
        ;

    // Confirm that we are alive and healthy.
    //
#ifdef CONFIG_PCI // ---------------------------------------------------------
    printk( KERN_INFO 
        "hpi%d:      PC2040; "
        "Mem Base %p, IRQ %d\n",
        this->ID, this->mem_base, this->irq );
#endif
#ifdef CONFIG_ARM // ---------------------------------------------------------

    // EBI2HPI-CPLD cannot exit if we can find DSP #0
    //
    ebi2hpi_cpld = ! dsp_VerifyExistence( &this->dsp[ 0 ], /*silent=*/ true );

    if ( ebi2hpi_cpld )
    {
        // Release common DSP reset
        mdelay( 1 );
        at91_set_gpio_value( AT91_PIN_PC15, 0 ); // RESET <= '0'
        }
    else // EBI2HPI-CPLD not found
    {
        // Limit number of DSPs to 1
        this->dsp_count = 1;
        }

    printk( KERN_INFO "hpi: CSR2 = %08x\n", at91_sys_read( AT91_SMC_CSR(2) ) );

    printk( KERN_INFO 
        "hpi%d:      ARM %s; "
        "Mem Base %p, IRQ %d\n",
        this->ID, ebi2hpi_cpld ? "EBI2HPI-CPLD" : "EBI",
        this->mem_base, this->irq );

#endif

    // Detect and initialize all DSPs attached to HPI
    //
    for ( i = 0; i < this->dsp_count; i++ )
    {
        if ( dsp_VerifyExistence( &this->dsp[ i ], /*silent=*/ false ) )
            dsp_Initialize( &this->dsp[ i ] );
        }

    rc = 0;
    for ( i = 0; i < this->dsp_count; i++ )
    {
        rc += this->dsp[ i ].exist;
        }
    if ( ! rc )
    {
        printk( KERN_INFO "hpi%d:      No DSPs found.\n", this->ID );
        return false; // no DSPs found on this HPI
        }

#ifdef CONFIG_PCI // ---------------------------------------------------------

    // Clear all DSP pending interrupts
    //
    writel( 0xF, this->csr + PCI2040_CSR_IRQ_MASK_CLEAR );
    writel( 0xF, this->csr + PCI2040_CSR_IRQ_EVENT_CLEAR );

    // Enable interrupts
    //
    for ( i = 0; i < hpi->dsp_count; i++ )
    {
        if ( this->dsp[ i ].exist ) 
            writel( 1 << i, this->csr + PCI2040_CSR_IRQ_MASK_SET );
        }

    // Master interrupt enable
    //
    writel( 1 << 31, this->csr + PCI2040_CSR_IRQ_MASK_SET );

    // printk( "Interrupt mask  : %08x\n", readl( this->csr + PCI2040_CSR_IRQ_MASK_SET ) );
    // printk( "Interrupt events: %08x\n", readl( this->csr + PCI2040_CSR_IRQ_EVENT_SET ) );

#endif // CONFIG_PCI ---------------------------------------------------------

#ifdef CONFIG_ARM // ---------------------------------------------------------

    // AT91_SYS->AIC_ICCR = AT91C_ID_IRQ0; //  acknowledge interrupt in PIO

#endif // CONFIG_ARM ---------------------------------------------------------

#ifndef ARM_USE_TIMER

    rc = request_irq( this->irq, hpi_irq_handler, SA_SHIRQ, HPI_DEVICE_NAME, this );
    if ( rc ) // error
    {
        this->exist = false;

        printk( KERN_ERR "hpi%d: Can't get assigned irq %i\n",
                this->ID, this->irq );
        return false;
        }

#endif

    // printk( KERN_INFO "hpi%d: Constructed\n", this->ID );

    return true;
    }

static void hpi_Destruct( HPI* this )
{
    int i;

    if ( ! this->exist )
        return;

    // Destruct all DSPs
    //
    for ( i = 0; i < this->dsp_count; i++ )
    {
        dsp_Destruct( &this->dsp[ i ] ); // this also sets 'exist' to false!
        }

#ifdef CONFIG_ARM // ---------------------------------------------------------

#ifdef ARM_USE_TIMER
    del_timer( &timer_1kHz );
#endif

    at91_set_gpio_value( AT91_PIN_PC15, 1 ); // RESET <= '1', i.e. reset DSP or CPLD

#endif // CONFIG_ARM

#ifdef CONFIG_PCI // ---------------------------------------------------------

    // Disable all interrupts
    //
    writel( 0xFFFFFFFF, this->csr + PCI2040_CSR_IRQ_MASK_CLEAR );
    writel( 0xFFFFFFFF, this->csr + PCI2040_CSR_IRQ_EVENT_CLEAR );

    // printk( "Interrupt mask  : %08x\n", readl( this->csr + PCI2040_CSR_IRQ_MASK_SET ) );
    // printk( "Interrupt events: %08x\n", readl( this->csr + PCI2040_CSR_IRQ_EVENT_SET ) );

#endif // CONFIG_PCI ---------------------------------------------------------

    if ( this->irq >= 0 )
    {
        free_irq( this->irq, this );
        }

#ifdef CONFIG_PCI // ---------------------------------------------------------

    iounmap( this->csr );
    iounmap( this->mem_base );

    pci_release_regions( this->pcidev );

    pci_disable_device( this->pcidev );

#endif // CONFIG_PCI ---------------------------------------------------------

    // printk( KERN_INFO "hpi%d: Destructed\n", this->ID );
    }

// This function is called whenever a process attempts 
// to open the device file. This is common for all c/d/io types
// of device nodes.
//
static int hpidev_open
(
    struct inode* inode, 
    struct file* file
    )
{
    int minor = MINOR( inode->i_rdev );

    // Parse device minor number. Format:
    //
    //  xxx------ : device type: 0= Control, 1= Debug, 2= I/O, 3= Voice
    //  ---xxx--- : HPI ID: 0..3
    //  ------xxx : DSP ID: 0..3
    //
    int type   = ( minor >> 6 ) & 0x03;
    int hpi_id = ( minor >> 3 ) & 0x03;
    int dsp_id = ( minor >> 0 ) & 0x03;
    HPI* hpi = NULL;
    DSP* dsp = NULL;

    if ( hpi_id > hpiCount )
	    return -ENODEV;

    hpi = &system_hpi[ hpi_id ];
    dsp = &system_hpi[ hpi_id ].dsp[ dsp_id ];

    if ( ! hpi->exist || ! dsp->exist )
        return -ENODEV;

    switch( type )
    {
        case 1: // debug
        {
            hpidev_stdio* dev = kmalloc( sizeof( hpidev_stdio ), GFP_KERNEL );
            memset( dev, 0, sizeof( hpidev_stdio ) );

            // Initialize private data
            dev->hpi = hpi;
            dev->dsp = dsp;

            // Initialize character circular buffer
            chBuf_Construct( &dev->inBuf );

            // Adjust file operations table
            file->f_op = &hpidev_stdio_file_operations;
            file->private_data = dev;

            spin_lock_bh( &dsp->dsp_lock );

            // Link device into hpidev_stdio list of DSP
            dev->prev = NULL;
            dev->next = dsp->dev_stdio_first;
            if ( dsp->dev_stdio_first )
                dsp->dev_stdio_first->prev = dev;
            dsp->dev_stdio_first = dev;
            dsp->dev_stdio_count++;

            spin_unlock_bh( &dsp->dsp_lock );

            break;
            }

        case 2: // I/O
        {
            hpidev_io* dev = kmalloc( sizeof( hpidev_io ), GFP_KERNEL );
            memset( dev, 0, sizeof( hpidev_io ) );

            // Initialize private data
            dev->hpi = hpi;
            dev->dsp = dsp;
            dev->subtype = SUBTYPE_NONE; // "IO Control" channel
            dev->onCloseWriteLen = 0; // == empty
            dev->channelInfoLen = 0; // == empty
            dev->subchannel = -1; // == disabled

            // Initialize data buffers
            chBuf_Construct( &dev->inBuf );
            //chBuf_Construct( &dev->outBuf );

            // Adjust file operations table
            file->f_op = &hpidev_io_file_operations;
            file->private_data = dev;

            // Link device into hpidev_io list of DSP
            //
            spin_lock_bh( &dsp->dsp_lock );

            dev->prev = NULL;
            dev->next = dsp->dev_ioC_first;
            if ( dsp->dev_ioC_first )
                dsp->dev_ioC_first->prev = dev;
            dsp->dev_ioC_first = dev;
            dsp->dev_ioC_count++;

            spin_unlock_bh( &dsp->dsp_lock );

            break;
            }

        case 0: // control
        default:
        {
            hpidev_c* dev = kmalloc( sizeof( hpidev_c ), GFP_KERNEL );
            memset( dev, 0, sizeof( hpidev_c ) );

            // Initialize private data
            dev->hpi = hpi;
            dev->dsp = dsp;
            dev->offset = 0;

            spin_lock_init( &dev->mutex );

            // Adjust file operations table
            file->f_op = &hpidev_c_file_operations;
            file->private_data = dev;

            break;
            }
        };

    return 0;
    }

// This function is called when a process closes the 
// device file. It doesn't have a return value because 
// it cannot fail. Regardless of what else happens, you 
// should always be able to close a device (in 2.0, a 2.2
// device file could be impossible to close).
//
static int hpidev_c_release
(
    struct inode* inode, 
    struct file* file
    )
{
    hpidev_c* dev = (hpidev_c*)file->private_data;

    if ( dev )
        kfree( dev );

    return 0;
    }

static int hpidev_stdio_release
(
    struct inode* inode, 
    struct file* file
    )
{
    hpidev_stdio* dev = (hpidev_stdio*)file->private_data;
    DSP* dsp = dev->dsp;

    // Unlink device from dev_stdio list of DSP
    //
    spin_lock_bh( &dsp->dsp_lock );

    if ( dev->prev )
        dev->prev->next = dev->next;
    else
        dsp->dev_stdio_first = dev->next;

    if ( dev->next )
        dev->next->prev = dev->prev;

    dev->next = NULL;
    dev->prev = NULL;

    dsp->dev_stdio_count--;

    spin_unlock_bh( &dsp->dsp_lock );

    // Destruct members
    //
    chBuf_Destruct( &dev->inBuf );

    if ( dev )
        kfree( dev );

    return 0;
    }

static void hpidev_io_Unlink( hpidev_io* dev )
{
    DSP* dsp = dev->dsp;

    if ( dev->prev )
    {
        dev->prev->next = dev->next;
        }
    else if ( dsp->dev_ioC_first == dev && dev->subtype == SUBTYPE_NONE )
    {
        dsp->dev_ioC_first = dev->next;
        }
    else if ( dsp->dev_ioB_first == dev && dev->subtype == SUBTYPE_B_CHANNEL )
    {
        dsp->dev_ioB_first = dev->next;
        }
    else if ( dsp->dev_ioD_first == dev && dev->subtype == SUBTYPE_D_CHANNEL )
    {
        dsp->dev_ioD_first = dev->next;
        }
    else
    {
        panic( "Corrupted hpidev_io device list.\n" );
        }

    if ( dev->next )
        dev->next->prev = dev->prev;

    dev->next = NULL;
    dev->prev = NULL;

    if ( dev->subtype == SUBTYPE_NONE )
        dsp->dev_ioC_count--;
    else if ( dev->subtype == SUBTYPE_B_CHANNEL )
        dsp->dev_ioB_count--;
    else if ( dev->subtype == SUBTYPE_D_CHANNEL )
        dsp->dev_ioD_count--;
    else
        panic( "Corrupted device subtype.\n" );
    }

static int hpidev_io_release
(
    struct inode* inode, 
    struct file* file
    )
{
    hpidev_io* dev = (hpidev_io*)file->private_data;
    DSP* dsp = dev->dsp;

    if ( dev->onCloseWriteLen > 0 ) // onCloseWrite ?
    {
        // printk( KERN_INFO "On Close Write: %d bytes\n", dev->onCloseWriteLen );

        spin_lock_bh( &dsp->dsp_lock );

        if ( dev->subtype == SUBTYPE_B_CHANNEL )
            dsp_WriteToMsgBuf( dsp, MSGTYPE_IO_B, dev->onCloseWrite, dev->onCloseWriteLen );
        else if ( dev->subtype == SUBTYPE_D_CHANNEL )
            dsp_WriteToMsgBuf( dsp, MSGTYPE_IO_D, dev->onCloseWrite, dev->onCloseWriteLen );

        spin_unlock_bh( &dsp->dsp_lock );
        }

    // Unlink device from dev_io list of DSP
    //
    spin_lock_bh( &dsp->dsp_lock );

    hpidev_io_Unlink( dev );

    spin_unlock_bh( &dsp->dsp_lock );

    // Destruct members
    //
    //chBuf_Destruct( &dev->outBuf );
    chBuf_Destruct( &dev->inBuf );

    if ( dev )
        kfree( dev );

    return 0;
    }

// This function is called whenever a process tries to 
// do an ioctl on our device file. We get two extra 
// parameters (additional to the inode and file 
// structures, which all device functions get): the number
// of the ioctl called and the parameter given to the 
// ioctl function.
//
// If the ioctl is write or read/write (meaning output 
// is returned to the calling process), the ioctl call 
// returns the output of this function.
//
static int hpidev_c_ioctl
(
    struct inode* inode,
    struct file* file,
    unsigned int ioctl_num, // The number of the ioctl
    unsigned long ioctl_param // The parameter to it 
    )
{
    int result = 0;

    hpidev_c* dev = (hpidev_c*)file->private_data;
    //HPI* hpi = dev->hpi;
    DSP* dsp = dev->dsp;

    // Switch according to the ioctl called 
    //
    switch( ioctl_num ) 
    {
        case IOCTL_HPI_RESET_DSP:

            dsp_Reset( dsp, ioctl_param );

            break;

        case IOCTL_HPI_GET_DSP_CHIP_ID:

            result = dsp->ChipID;

            break;

        case IOCTL_HPI_RUN_PROGRAM:
        {
            dsp_RunProgram( dsp, ioctl_param );

            break;
            }

        case IOCTL_HPI_GET_HPIC:

            disable_irq( dsp->hpi->irq );
            hpi_irq_handler( dsp->hpi->irq, dsp->hpi, NULL );
            enable_irq( dsp->hpi->irq );

            spin_lock_bh( &dsp->dsp_lock );
            result = dsp_Get_HPIC( dsp );
            spin_unlock_bh( &dsp->dsp_lock );

            break;

        case IOCTL_HPI_GET_DSPWD_REPORT:
        {
            int dspwd_report_totlen = dsp->dspwd_report[ 0 ] + 1;

            // if ( _IOC_DIR(ioctl_num) & _IOC_READ )
            if ( ! access_ok ( VERIFY_WRITE, (void*)ioctl_param, dspwd_report_totlen ) )
                return -EFAULT;

            if ( copy_to_user( (void*)ioctl_param, dsp->dspwd_report, dspwd_report_totlen ) )
                return -EFAULT;

            break;
            }

        case IOCTL_HPI_GET_IODEV_COUNT:
        {
            spin_lock_bh( &dsp->dsp_lock );
            result = dsp->dev_stdio_count + dsp->dev_ioC_count
                   + dsp->dev_ioD_count + dsp->dev_ioB_count;
            spin_unlock_bh( &dsp->dsp_lock );
            break;
            }

        default:
            return -ENOTTY;
        }

    return result;
    }

// This function is called whenever a process tries to 
// do an ioctl on our device file. We get two extra 
// parameters (additional to the inode and file 
// structures, which all device functions get): the number
// of the ioctl called and the parameter given to the 
// ioctl function.
//
// If the ioctl is write or read/write (meaning output 
// is returned to the calling process), the ioctl call 
// returns the output of this function.
//
static int hpidev_io_ioctl
(
    struct inode* inode,
    struct file* file,
    unsigned int ioctl_num, // The number of the ioctl
    unsigned long ioctl_param // The parameter to it 
    )
{
    int result = 0;

    hpidev_io* dev = (hpidev_io*)file->private_data;
    DSP* dsp = dev->dsp;

    // Switch according to the ioctl called 
    //
    switch( ioctl_num ) 
    {
        case IOCTL_HPI_SET_D_CHANNEL:
            spin_lock_bh( &dev->dsp->dsp_lock );

            // Unlink from previous subtype & subchannel
            //
            hpidev_io_Unlink( dev );

            // Set new subtype & subchannel
            //
            dev->subtype = SUBTYPE_D_CHANNEL;
            dev->subchannel = ioctl_param & 0xFF;

            // Link device into new IO belonging list
            //
            dev->prev = NULL;
            dev->next = dsp->dev_ioD_first;
            if ( dsp->dev_ioD_first )
                dsp->dev_ioD_first->prev = dev;
            dsp->dev_ioD_first = dev;
            dsp->dev_ioD_count++;

            spin_unlock_bh( &dev->dsp->dsp_lock );
            break;

        case IOCTL_HPI_SET_B_CHANNEL:
            spin_lock_bh( &dev->dsp->dsp_lock );

            // Unlink from previous subtype & subchannel
            //
            hpidev_io_Unlink( dev );

            // Set new subtype & subchannel
            //
            dev->subtype = SUBTYPE_B_CHANNEL;
            dev->subchannel = ioctl_param & 0xFF;

            // Link device into new IO belonging list
            //
            dev->prev = NULL;
            dev->next = dsp->dev_ioB_first;
            if ( dsp->dev_ioB_first )
                dsp->dev_ioB_first->prev = dev;
            dsp->dev_ioB_first = dev;
            dsp->dev_ioB_count++;

            spin_unlock_bh( &dev->dsp->dsp_lock );
            break;

        case IOCTL_HPI_ON_CLOSE_WRITE:
            // param <> 0: signal next write to collect onCloseWrite[]
            // param == 0: remove onCloseWrite[]
            dev->onCloseWriteLen = ioctl_param != 0 ? -1 : 0; 
            break;
        
        case IOCTL_HPI_SET_CHANNEL_INFO:
            // param <> 0: signal next write to collect channelInfo[]
            // param == 0: remove channelInfo[]
            dev->channelInfoLen = ioctl_param != 0 ? -1 : 0; 
            if ( dev->channelInfoLen == 0 )
                dev->channelInfo[ 0 ] = 0;
            break;
        
        default:
            return -ENOTTY;
        }

    return result;
    }

// This function is called whenever a process which 
// has already opened the device file attempts to 
// read from it.
//
static ssize_t hpidev_c_read
(
    struct file* file,
    char* buffer,      // The buffer to fill with the data 
    size_t length,     // The length of the buffer 
    loff_t* offset     // offset to the file 
    )
{
    hpidev_c* dev = (hpidev_c*)file->private_data;
    DSP* dsp = dev->dsp;
    int count = length / 2; // Number of 16-bit words to read
    int i;

    unsigned short* data = (unsigned short*) buffer;

    spin_lock( &dev->mutex );
    spin_lock_bh( &dsp->dsp_lock );

    dsp_Put_XHPIA( dsp, dev->offset );

    for ( i = 0; i < count; i++ )
        *data++ = dsp_Get_HPIDpp( dsp );

    dev->offset += count;
    *offset += dev->offset;

    spin_unlock_bh( &dsp->dsp_lock );
    spin_unlock( &dev->mutex );

    // Read functions are supposed to return the number 
    // of bytes actually inserted into the buffer
    //
    return count * 2; // Number of bytes actually transferred
    }

static ssize_t hpidev_stdio_read
(
    struct file* file,
    char* buffer,      // The buffer to fill with the data 
    size_t length,     // The length of the buffer 
    loff_t* offset     // offset to the file 
    )
{
    hpidev_stdio* dev = (hpidev_stdio*)file->private_data;

    int non_block = ( file->f_flags & O_NONBLOCK ) != 0;

    return chBuf_UsersRead( &dev->inBuf, buffer, length, non_block );
    }

static ssize_t hpidev_io_read
(
    struct file* file,
    char* buffer,      // The buffer to fill with the data 
    size_t length,     // The length of the buffer 
    loff_t* offset     // offset to the file 
    )
{
    hpidev_io* dev = (hpidev_io*)file->private_data;

    int non_block = ( file->f_flags & O_NONBLOCK ) != 0;

    return chBuf_UsersRead( &dev->inBuf, buffer, length, non_block );
    }

// This function is called when somebody tries to 
// write into our device file. 
//
static ssize_t hpidev_c_write
(
    struct file* file,
    const char* buffer,
    size_t length,
    loff_t* offset
    )
{
    hpidev_c* dev = (hpidev_c*)file->private_data;
    DSP* dsp = dev->dsp;
    int count = length / 2; // Number of 16-bit words to read
    int i;

    unsigned short* data = (unsigned short*) buffer;

    spin_lock( &dev->mutex );
    spin_lock_bh( &dsp->dsp_lock );

    dsp_Put_XHPIA( dsp, dev->offset );

    dsp_Put_HPID( dsp, *data++ );

    for ( i = 1; i < count; i++ )
        dsp_Put_ppHPID( dsp, *data++ );

    dev->offset += count;
    *offset += dev->offset;

    spin_unlock_bh( &dsp->dsp_lock );
    spin_unlock( &dev->mutex );

    // Write functions are supposed to return the number 
    // of bytes actually inserted into the buffer
    //
    return count * 2; // Number of bytes actually transferred
    }

static ssize_t hpidev_stdio_write
(
    struct file* file,
    const char* buffer,
    size_t length,
    loff_t* offset
    )
{
    //hpidev_stdio* dev = (hpidev_stdio*)file->private_data;
    return 0;
    }

static ssize_t hpidev_io_write
(
    struct file* file,
    const char* buffer,
    size_t length,
    loff_t* offset
    )
{
    hpidev_io* dev = (hpidev_io*)file->private_data;
    DSP* dsp = dev->dsp;
    int written;

    if ( dev->onCloseWriteLen < 0 ) // write to onCloseWrite buffer
    {
        if ( length > sizeof( dev->onCloseWrite ) )
            length = 0;

        dev->onCloseWriteLen = length;
        memcpy( dev->onCloseWrite, buffer, length );
        return length;
        }
    else if ( dev->channelInfoLen < 0 ) // write to channelInfo buffer
    {
        if ( length > sizeof( dev->channelInfo ) - 1 ) // leave space for '\0'
            length = 0;

        dev->channelInfoLen = length;
        memcpy( dev->channelInfo, buffer, length );
        dev->channelInfo[ length ] = 0; // ASCIIZ
        return length;
        }

    // printk( KERN_INFO "Beginning to write\n" );

    spin_lock_bh( &dsp->dsp_lock );

    if ( dev->subtype == SUBTYPE_B_CHANNEL )
        written = dsp_WriteToMsgBuf( dsp, MSGTYPE_IO_B, (unsigned char*)buffer, length );
    else if ( dev->subtype == SUBTYPE_D_CHANNEL )
        written = dsp_WriteToMsgBuf( dsp, MSGTYPE_IO_D, (unsigned char*)buffer, length );
    else
        written = dsp_WriteToMsgBuf( dsp, MSGTYPE_CTRL, (unsigned char*)buffer, length );

    spin_unlock_bh( &dsp->dsp_lock );

    // printk( KERN_INFO "IO write written %d\n", written );

    return written > 0 ? written : ENOTTY;
    }

// The llseek method is used to change the current read/write position 
// in a file, and the new position is returned as a (positive) return value.
// The loff_t is a "long offset" and is at least 64 bits wide even on 32-bit 
// platforms. Errors are signaled by a negative return value. If the function
// is not specified for the driver, a seek relative to end-of-file fails, while
// other seeks succeed by modifying the position counter in the file structure.
//
loff_t hpidev_c_llseek
(
    struct file* file,
    loff_t offset,
    int whence
    )
{
    hpidev_c* dev = (hpidev_c*)file->private_data;
    loff_t newpos;

    spin_lock( &dev->mutex );

    switch( whence )
    {
        case 0: // SEEK_SET
            newpos = offset;
            break;
        case 1: // SEEK_CUR
            newpos = dev->offset + offset;
            break;
        case 2: // SEEK_END; not supported
        default:
            spin_unlock( &dev->mutex );
            return -EINVAL;
        }

    if ( newpos < 0 )
    {
        spin_unlock( &dev->mutex );
        return -EINVAL;
        }

    dev->offset = newpos;
    file->f_pos = newpos;

    spin_unlock( &dev->mutex );

    return newpos;
    }

// The poll / select driver's method. It will be called whenever the user-space
// program performs a poll or select system call involving a file descriptor
// associated with the driver.
//
unsigned int hpidev_stdio_poll
(
    struct file* file,
    struct poll_table_struct* wait
    )
{
    hpidev_stdio* dev = (hpidev_stdio*)file->private_data;
    unsigned int mask = 0;

    // The buffer is circular; it is considered full if 'writep' is right
    // behind 'readp'.
    //
    //int left = ( dev->inBuf.readp + dev->inBuf.size - dev->inBuf.writep ) % dev->inBuf.size;

    poll_wait( file, &dev->inBuf.queue, wait );

    if ( dev->inBuf.size == 0 || dev->inBuf.readp != dev->inBuf.writep )
        mask |= POLLIN | POLLRDNORM; // readable

    //if( left != 1 )
    //    mask |= POLLOUT | POLLWRNORM; // writable

    return mask;
    }

unsigned int hpidev_io_poll
(
    struct file* file,
    struct poll_table_struct* wait
    )
{
    hpidev_io* dev = (hpidev_io*)file->private_data;
    unsigned int mask = 0;

    // The buffer is circular; it is considered full if 'writep' is right
    // behind 'readp'.
    //
    //int left = ( dev->inBuf.readp + dev->inBuf.size - dev->inBuf.writep ) % dev->inBuf.size;

    poll_wait( file, &dev->inBuf.queue, wait );

    if ( dev->inBuf.size == 0 || dev->inBuf.readp != dev->inBuf.writep )
        mask |= POLLIN | POLLRDNORM; // readable

    return mask;
    }

///////////////////////////////////////////////////////////////////////////////
// File operations tables
//
// This structure will hold the functions to be called 
// when a process does something to the device we 
// created. Since a pointer to this structure is kept in 
// the devices table, it can't be local to
// init_module. NULL is for unimplemented functions.
//
static struct file_operations hpidev_file_operations = 
{
    owner   : THIS_MODULE,
    open    : hpidev_open
    };

static struct file_operations hpidev_c_file_operations = 
{
    owner   : THIS_MODULE,
    open    : hpidev_open,
    release : hpidev_c_release,
    read    : hpidev_c_read,
    write   : hpidev_c_write,
    ioctl   : hpidev_c_ioctl,
    llseek  : hpidev_c_llseek
    };

static struct file_operations hpidev_stdio_file_operations = 
{
    owner   : THIS_MODULE,
    open    : hpidev_open,
    release : hpidev_stdio_release,
    read    : hpidev_stdio_read,
    write   : hpidev_stdio_write,
    poll    : hpidev_stdio_poll
    };

static struct file_operations hpidev_io_file_operations = 
{
    owner   : THIS_MODULE,
    open    : hpidev_open,
    release : hpidev_io_release,
    read    : hpidev_io_read,
    write   : hpidev_io_write,
    ioctl   : hpidev_io_ioctl,
    poll    : hpidev_io_poll
    };

///////////////////////////////////////////////////////////////////////////////
// /proc/hpidev device read method
//
static int hpidev_proc_read
(
    char* buf,
    char **start,
    off_t offset,
    int count,
    int* eof,
    void* data
    )
{
    int i, j, len = 0;
    int limit = count - 80; // Don't print more than this

    for ( i = 0; i < hpiCount && len <= limit; i++ )
    {
        HPI* hpi = &system_hpi[ i ];
        if ( ! hpi->exist )
            continue;

        len += sprintf( buf + len, "hpi%d: irq count %ld, tasklet count %ld\n",
            hpi->ID, hpi->irq_count_tot, hpi->tasklet_count_tot );

        for ( j = 0; j < hpi->dsp_count && len <= limit; j++ )
        {
            DSP* dsp = &hpi->dsp[ j ];
            if ( ! dsp->exist )
                continue;

            len += sprintf( buf + len, "hpi%d:dsp%d: "
                "TMS320VC54%02x_R%01x; "
                "Base 0x%p, %s\n",
                hpi->ID, dsp->ID, dsp->ChipID, dsp->ChipRevision,
                dsp->mem_base,
                dsp->running ? "Running" : "Stopped"
                );

            len += sprintf( buf + len, "    Tasklet %ld, Dev: StdIO %ld, IO-C=%ld, IO-D=%ld, IO-B=%ld\n",
                dsp->tasklet_count_tot, dsp->dev_stdio_count, 
                dsp->dev_ioC_count, dsp->dev_ioD_count, dsp->dev_ioB_count
                );

            len += sprintf( buf + len, "    Last WD Report: Len=%d; %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n\n",
                (int)dsp->dspwd_report[ 0 ], 
                (int)dsp->dspwd_report[ 1 ], (int)dsp->dspwd_report[ 2 ],
                (int)dsp->dspwd_report[ 3 ], (int)dsp->dspwd_report[ 4 ],
                (int)dsp->dspwd_report[ 5 ], (int)dsp->dspwd_report[ 6 ],
                (int)dsp->dspwd_report[ 7 ], (int)dsp->dspwd_report[ 8 ],
                (int)dsp->dspwd_report[ 9 ], (int)dsp->dspwd_report[ 10 ]
                );
            }
        }

    *eof = 1;
    return len;
    }

///////////////////////////////////////////////////////////////////////////////
// /proc/hpidevInfo device read method
//
static int hpidevInfo_proc_read
(
    char* buf,
    char **start,
    off_t offset,
    int count,
    int* eof,
    void* data
    )
{
    int i, j, len = 0;
    int limit = count - 80; // Don't print more than this

    for ( i = 0; i < hpiCount && len <= limit; i++ )
    {
        HPI* hpi = &system_hpi[ i ];
        if ( ! hpi->exist )
            continue;

        for ( j = 0; j < hpi->dsp_count && len <= limit; j++ )
        {
            hpidev_io* dev_io;
            DSP* dsp = &hpi->dsp[ j ];
            if ( ! dsp->exist )
                continue;

            // go through all D channel descriptions

            spin_lock_bh( &dsp->dsp_lock );

            dev_io = dsp->dev_ioD_first;
            while( dev_io )
            {
                if ( dev_io->channelInfoLen > 0 )
                    len += sprintf( buf + len, "%s\n", dev_io->channelInfo );
                dev_io = dev_io->next;
                }

            spin_unlock_bh( &dsp->dsp_lock );
            }
        }

    *eof = 1;
    return len;
    }

///////////////////////////////////////////////////////////////////////////////
// Module initialization and cleanup

// Initialize the module - register the hpidev device
//
static int __init hpidev_ModuleInit( void )
{
    int rc = 0; // Negative values signify an error

    // Register the character device (at least try) 
    //
    rc = register_chrdev( HPI_MAJOR_NUM, HPI_DEVICE_NAME, 
                               &hpidev_file_operations );
    if ( rc ) // error
    {
        printk( KERN_ERR "hpidev: registering character device failed\n" );
        return rc;
        }

    hpiCount = 0;
    memset( system_hpi, 0, sizeof( system_hpi ) );

#ifdef CONFIG_PCI // ---------------------------------------------------------

    {
        // Scan PCI bus for PCI2040 PCI to DSP bridges.
        //
        int i;
        struct pci_dev* pcidev = NULL;
        for ( i = 0; i < MAX_HPI_COUNT; i++ )
        {
            pcidev = pci_find_device( PCI_VENDOR_ID_TI, 
                                      PCI_DEVICE_ID_TI_PCI2040, 
                                      pcidev );
            if ( ! pcidev )
                break;

            system_hpi[ hpiCount ].pcidev = pcidev;

            // PCI2040 found; Construct HPI object on it
            //
            if ( hpi_Construct( &system_hpi[ hpiCount ], hpiCount ) )
                hpiCount++;
            }
        }

#endif // CONFIG_PCI ---------------------------------------------------------

#ifdef CONFIG_ARM // ---------------------------------------------------------

    // ARM has just one HPI
    //
    if ( hpi_Construct( &system_hpi[ hpiCount ], hpiCount ) )
        hpiCount++;

#ifdef ARM_USE_TIMER
    if ( hpiCount == 1 )
    {
        init_timer( &timer_1kHz );
    
        timer_1kHz.function = hpi_timer;
        timer_1kHz.expires = jiffies + 1;
        add_timer( &timer_1kHz );
        }
#endif // ARM_USE_TIMER ------------------------------------------------------

#endif // CONFIG_ARM ---------------------------------------------------------

    if ( hpiCount <= 0 ) // no DSPs found
    {
        rc = unregister_chrdev( HPI_MAJOR_NUM, HPI_DEVICE_NAME );
        if ( rc ) // error
           printk( KERN_ERR "hpidev: unregister character device failed\n" );

        return -EBUSY;
        }

    // Add /proc/hpidev entry
    //
    create_proc_read_entry (
        HPI_DEVICE_NAME,
        0, // default mode
        NULL, // parent dir
        hpidev_proc_read, // info routine
        NULL // client data
        );

    // Add /proc/hpidevInfo entry
    //
    create_proc_read_entry (
        HPI_DEVICE_NAME "Info",
        0, // default mode
        NULL, // parent dir
        hpidevInfo_proc_read, // info routine
        NULL // client data
        );

    printk( KERN_INFO "HPI-DSP module inserted.\n" );

    // If we return a non zero value, it means that module initialization
    // is failed and the kernel module can't be loaded 
    //
    return 0;
    }

// Cleanup - unregister the hpidev device
//
static void __exit hpidev_ModuleCleanup( void )
{
    int i;
    int rc;

    // Remove /proc/hpidev* entries
    //
    remove_proc_entry( HPI_DEVICE_NAME, NULL /* parent dir */); // no problem if it was not registered
    remove_proc_entry( HPI_DEVICE_NAME "Info", NULL /* parent dir */); // no problem if it was not registered

    // Destruct all HPI's
    //
    for ( i = 0; i < hpiCount; i++ )
        hpi_Destruct( &system_hpi[ i ] );

    // Unregister the device
    //
    rc = unregister_chrdev( HPI_MAJOR_NUM, HPI_DEVICE_NAME );
    if ( rc ) // error
       printk( KERN_ERR "hpidev: unregister character device failed\n" );

    printk( KERN_INFO "HPI-DSP module removed.\n" );
    }

// Module entry & exit declarations
//
module_init( hpidev_ModuleInit );
module_exit( hpidev_ModuleCleanup );
