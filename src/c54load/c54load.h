/*
    Project:     Albatros / cload
  
    Module:      cload.h
  
    Description: Albatros c54xx COFF loader
                 CLOAD class declaration
  
    Copyright (c) 2002 By Mikica B Kocic
    Copyright (c) 2002 By IPTC Technology Communications AB
*/

#ifndef _CLOAD_H_INCLUDED
#define _CLOAD_H_INCLUDED

//////////////////////////////////////////////////////////////////////////
// This module contains functions to read in and load a COFF object file.

//////////////////////////////////////////////////////////////////////////
//  Target dependent parameters.
//
typedef unsigned short T_ADDR;          // Type for target address
typedef unsigned short T_DATA;          // Type for target data word
typedef unsigned short T_SIZE;          // Type for cinit size field

#define LOCTOBYTE(x)   ((x)<<1)         // 16-bit word addrs to byte addrs
#define BYTETOLOC(x)   ((x)>>1)         // byte addrs to word addrs
#define BIT_OFFSET(a)  (0)              // Bit offset of addr within byte

#define LOADWORDSIZE  2                 // Minimum divisor of load size

//////////////////////////////////////////////////////////////////////////
//  COFF data structures and related definitions used by the linker
//
//////////////////////////////////////////////////////////////////////////
//  COFF file header
//
struct COFF_HEADER
{   
    unsigned short  f_version;      // Version ID; indicates the version 
                                    // of the COFF file structure
    unsigned short  f_nscns;        // number of section headers
    long            f_timdat;       // time & date stamp; indicates when
                                    // the file was created
    long            f_symptr;       // file pointer; contains the symbol 
                                    // table's starting address
    long            f_nsyms;        // number of entries in the symbol table
    unsigned short  f_opthdr;       // number of bytes in the optional header.
                                    // this field is either 0 or 28; if it is
                                    // 0, there is no optional file header.
    unsigned short  f_flags;        // flags; see bellow
    unsigned short  f_target;       // Target ID; magic number indicates the
                                    // file can be executed in a 'C5000 system
    } __attribute__((packed));

//////////////////////////////////////////////////////////////////////////
//  COFF file header version
//
const unsigned short
    COFF_V1 = 0x00C1,  // Version ID of the COFF1 file
    COFF_V2 = 0x00C2;  // Version ID of the COFF2 file

//////////////////////////////////////////////////////////////////////////
//  COFF file header flags
//
enum
{
    F_RELFLG   = 0x0001,    // relocation info stripped from file
    F_EXEC     = 0x0002,    // file is executable (no unresolved refs)
    F_LNNO     = 0x0004,    // line numbers stripped from file
    F_LSYMS    = 0x0008,    // local symbols stripped from file
    F_GSP10    = 0x0010,    // 34010 version
    F_GSP20    = 0x0020,    // 34020 version
    F_SWABD    = 0x0040,    // bytes swabbed (in names)
    F_AR16WR   = 0x0080,    // byte ordering of an AR16WR (PDP-11)
    F_LITTLE   = 0x0100,    // byte ordering of an AR32WR (vax)
    F_BIG      = 0x0200,    // byte ordering of an AR32W (3B, maxi)
    F_PATCH    = 0x0400,    // contains "patch" list in optional header
    F_NODF     = 0x0400,
    F_SYMMERGE = 0x1000     // Duplicate symbols were removed
    };

#define F_VERSION    (F_GSP10  | F_GSP20)
#define F_BYTE_ORDER (F_LITTLE | F_BIG)

//////////////////////////////////////////////////////////////////////////
//  OPTIONAL FILE HEADER
//
struct AOUT_HEADER 
{
    short   magic;          // Optional file header magic number 
                            // (0108h for SunOS or HP-UX; 0801h for DOS)
    short   vstamp;         // Version stamp
    long    tsize;          // Size in words of executable code
    long    dsize;          // Size in words of initialized data
    long    bsize;          // Size in words of uninitialized data
    long    entrypt;        // Entry point
    long    text_start;     // Beginning address of executable code
    long    data_start;     // Beginning address of initialized data
    } __attribute__((packed));

//////////////////////////////////////////////////////////////////////////
//  When a UNIX aout header is to be built in the optional header,
//  the following magic numbers can appear in that header:
//
//  AOUT1MAGIC : default : readonly sharable text segment
//  AOUT2MAGIC:          : writable text segment
//  PAGEMAGIC  :         : configured for paging
//
enum
{
    AOUT1MAGIC = 0410,
    AOUT2MAGIC = 0407,
    PAGEMAGIC  = 0413
    };

//////////////////////////////////////////////////////////////////////////
//  SECTION HEADER
//

struct SECT_V1_HEADER // format COFF1
{
    char            s_name[8];      // section name; padded with nulls
    long            s_paddr;        // physical address
    long            s_vaddr;        // virtual address
    long            s_size;         // section size in words
    long            s_scnptr;       // file ptr to raw data for section
    long            s_relptr;       // file ptr to relocation
    long            s_lnnoptr;      // file ptr to line numbers
    unsigned short  s_nreloc;       // number of relocation entries
    unsigned short  s_nlnno;        // number of line number entries
    unsigned short  s_flags;        // flags
    char            s_reserved;     // reserved byte
    char            s_page;         // memory page id
    } __attribute__((packed));

struct SECT_V2_HEADER // format COFF2
{
    union 
    {
        char        s_name[8];  // section name,
        long        s_ptr;      // or pointer to the string table 
                                // if the section name is longer than 8 chars
        };
    long            s_paddr;    // physical address
    long            s_vaddr;    // virtual address
    long            s_size;     // section size in words
    long            s_scnptr;   // file ptr to raw data for section
    long            s_relptr;   // file ptr to relocation
    long            s_lnnoptr;  // file ptr to line numbers
    unsigned long   s_nreloc;   // number of relocation entries
    unsigned long   s_nlnno;    // number of line number entries
    unsigned long   s_flags;    // flags
    short           s_reserved; // reserved byte
    unsigned short  s_page;     // memory page id
    } __attribute__((packed));

//////////////////////////////////////////////////////////////////////////
// Define constants for names of "special" sections
//
#define CINIT   ".cinit"   // Name of cinit section
#define _TEXT   ".text"
#define _DATA   ".data"
#define _BSS    ".bss"
#define _CINIT  ".cinit"
#define _TV     ".tv"

//////////////////////////////////////////////////////////////////////////
// The low 4 bits of s_flags is used as a section "type"
//
enum
{
    STYP_REG    = 0x0000, // "regular" : allocated, relocated, loaded
    STYP_DSECT  = 0x0001, // "dummy"   : not allocated, relocated, not loaded
    STYP_NOLOAD = 0x0002, // "noload"  : allocated, relocated, not loaded
    STYP_GROUP  = 0x0004, // "grouped" : formed of input sections
    STYP_PAD    = 0x0008, // "padding" : not allocated, not relocated, loaded
    STYP_COPY   = 0x0010, // "copy"    : used for C init tables -
                          //          not allocated, relocated, loaded; 
                          //          reloc & lineno entries processed normally
    STYP_TEXT   = 0x0020, // section contains text only
    STYP_DATA   = 0x0040, // section contains data only
    STYP_BSS    = 0x0080, // section contains bss only
    STYP_CLINK  = 0x4000, // section that is conditionally linked

    STYP_ALIGN  = 0x0100  // align flag passed by old version assemblers
    };

#define ALIGN_MASK  0x0F00 // part of s_flags that is used for align vals
#define ALIGNSIZE(x) (1 << ((x & ALIGN_MASK) >> 8))

//////////////////////////////////////////////////////////////////////////
//  RELOCATION ENTRIES
//
struct RELOC
{
    long            r_vaddr;        // (virtual) address of reference
    short           r_symndx;       // index into symbol table
    unsigned short  r_disp;         // additional bits for address calculation
    unsigned short  r_type;         // relocation type
    } __attribute__((packed));

//////////////////////////////////////////////////////////////////////////
//   define all relocation types
//
enum
{
    R_ABS          = 0x00,      // absolute address - no relocation
    R_DIR16        = 0x01,      // UNUSED
    R_REL16        = 0x02,      // UNUSED
    R_DIR24        = 0x04,      // UNUSED
    R_REL24        = 0x05,      // 24 bits, direct
    R_DIR32        = 0x06,      // UNUSED
    R_RELBYTE      = 0x0F,      // 8 bits, direct
    R_RELWORD      = 0x20,      // 16 bits, direct
    R_RELLONG      = 0x21,      // 32 bits, direct
    R_PCRBYTE      = 0x22,      // 8 bits, PC-relative
    R_PCRWORD      = 0x23,      // 16 bits, PC-relative
    R_PCRLONG      = 0x24,      // 32 bits, PC-relative
    R_OCRLONG      = 0x30,      // GSP: 32 bits, one's complement direct
    R_GSPPCR16     = 0x31,      // GSP: 16 bits, PC relative (in words)
    R_GSPOPR32     = 0x32,      // GSP: 32 bits, direct big-endian
    R_PARTLS16     = 0x40,      // Brahma: 16 bit offset of 24 bit address*/
    R_PARTMS8      = 0x41,      // Brahma: 8 bit page of 24 bit address
    R_PARTLS7      = 0x28,      // DSP: 7 bit offset of 16 bit address
    R_PARTMS9      = 0x29,      // DSP: 9 bit page of 16 bit address
    R_REL13        = 0x2A       // DSP: 13 bits, direct
    };

//////////////////////////////////////////////////////////////////////////
//  LINE NUMBER ENTRIES
//
struct LINENO
{
    union
    {
        long       l_symndx ;   // sym. table index of function name, 
                                // iff l_lnno == 0
        long       l_paddr ;    // (physical) address of line number
        };
    unsigned short l_lnno ;     // line number
    } __attribute__((packed));

//////////////////////////////////////////////////////////////////////////
//   STORAGE CLASSES
//
enum
{
    C_EFCN        =  -1,    // physical end of function
    C_NULL        =  0,
    C_AUTO        =  1,     // automatic variable
    C_EXT         =  2,     // external symbol
    C_STAT        =  3,     // static
    C_REG         =  4,     // register variable
    C_EXTDEF      =  5,     // external definition
    C_LABEL       =  6,     // label
    C_ULABEL      =  7,     // undefined label
    C_MOS         =  8,     // member of structure
    C_ARG         =  9,     // function argument
    C_STRTAG      =  10,    // structure tag
    C_MOU         =  11,    // member of union
    C_UNTAG       =  12,    // union tag
    C_TPDEF       =  13,    // type definition
    C_USTATIC     =  14,    // undefined static
    C_ENTAG       =  15,    // enumeration tag
    C_MOE         =  16,    // member of enumeration
    C_REGPARM     =  17,    // register parameter
    C_FIELD       =  18,    // bit field
    C_BLOCK       =  100,   // ".bb" or ".eb"
    C_FCN         =  101,   // ".bf" or ".ef"
    C_EOS         =  102,   // end of structure
    C_FILE        =  103,   // file name
    C_LINE        =  104,   // dummy sclass for line number entry
    C_ALIAS       =  105,   // duplicate tag
    C_HIDDEN      =  106    // special storage class for external
                            // symbols in dmert public libraries
    };

//////////////////////////////////////////////////////////////////////////
//  SYMBOL TABLE ENTRIES
//

#define  SYMNMLEN   8      //  Number of characters in a symbol name
#define  FILNMLEN   14     //  Number of characters in a file name
#define  DIMNUM     4      //  Number of array dimensions in auxiliary entry

struct SYMENT
{
    union
    {
        char            _n_name[SYMNMLEN];      // old COFF version
        struct
        {
            long    _n_zeroes;      // new == 0
            long    _n_offset;      // offset into string table
        } _n_n;
        char            *_n_nptr[2];    // allows for overlaying
    } _n;
    long                    n_value;        // value of symbol
    short                   n_scnum;        // section number
    unsigned short          n_type;         // type and derived type
    char                    n_sclass;       // storage class
    char                    n_numaux;       // number of aux. entries
    } __attribute__((packed));

#define n_name          _n._n_name
#define n_nptr          _n._n_nptr[1]
#define n_zeroes        _n._n_n._n_zeroes
#define n_offset        _n._n_n._n_offset

//////////////////////////////////////////////////////////////////////////
// Relocatable symbols have a section number of the
// section in which they are defined.  Otherwise, section
// numbers have the following meanings:
//
#define  N_UNDEF  0                     // undefined symbol
#define  N_ABS    -1                    // value of symbol is absolute
#define  N_DEBUG  -2                    // special debugging symbol
#define  N_TV     (unsigned short)-3    // needs transfer vector (preload)
#define  P_TV     (unsigned short)-4    // needs transfer vector (postload)

//////////////////////////////////////////////////////////////////////////
// The fundamental type of a symbol packed into the low
// 4 bits of the word.
//
#define  _EF    ".ef"

enum
{
    T_NULL     =  0,        // no type info
    T_ARG      =  1,        // function argument (only used by compiler)
    T_CHAR     =  2,        // character
    T_SHORT    =  3,        // short integer
    T_INT      =  4,        // integer
    T_LONG     =  5,        // long integer
    T_FLOAT    =  6,        // floating point
    T_DOUBLE   =  7,        // double word
    T_STRUCT   =  8,        // structure
    T_UNION    =  9,        // union
    T_ENUM     = 10,        // enumeration
    T_MOE      = 11,        // member of enumeration
    T_UCHAR    = 12,        // unsigned character
    T_USHORT   = 13,        // unsigned short
    T_UINT     = 14,        // unsigned integer
    T_ULONG    = 15         // unsigned long
    };

//////////////////////////////////////////////////////////////////////////
// derived types are:
//
enum
{
    DT_NON     = 0,         // no derived type
    DT_PTR     = 1,         // pointer
    DT_FCN     = 2,         // function
    DT_ARY     = 3          // array
    };

/*
#define MKTYPE(basic,d1,d2,d3,d4,d5,d6) \
    ((basic) | ((d1) << 4) | ((d2) << 6) | ((d3) << 8) \
    | ((d4) << 10) | ((d5) << 12) | ((d6) << 14))

//////////////////////////////////////////////////////////////////////////
// type packing constants and macros
//
#define  N_BTMASK     017
#define  N_TMASK      060
#define  N_TMASK1     0300
#define  N_TMASK2     0360
#define  N_BTSHFT     4
#define  N_TSHIFT     2

#define  BTYPE(x)  ((x) & N_BTMASK)
#define  ISINT(x)  (((x) >= T_CHAR && (x) <= T_LONG) ||   \
            ((x) >= T_UCHAR && (x) <= T_ULONG) || (x) == T_ENUM)
#define  ISFLT(x)  ((x) == T_DOUBLE || (x) == T_FLOAT)
#define  ISPTR(x)  (((x) & N_TMASK) == (DT_PTR << N_BTSHFT))
#define  ISFCN(x)  (((x) & N_TMASK) == (DT_FCN << N_BTSHFT))
#define  ISARY(x)  (((x) & N_TMASK) == (DT_ARY << N_BTSHFT))
#define  ISTAG(x)  ((x)==C_STRTAG || (x)==C_UNTAG || (x)==C_ENTAG)

#define  INCREF(x) ((((x)&~N_BTMASK)<<N_TSHIFT)|(DT_PTR<<N_BTSHFT)|(x&N_BTMASK))
#define  DECREF(x) ((((x)>>N_TSHIFT)&~N_BTMASK)|((x)&N_BTMASK))
*/

//////////////////////////////////////////////////////////////////////////
//  AUXILIARY SYMBOL ENTRY
//
union AUXENT
{
    struct
    {
        long            x_tagndx;       // str, un, or enum tag indx
        union
        {
            struct
    {
        unsigned short  x_lnno; // declaration line number
        unsigned short  x_size; // str, union, array size
    } x_lnsz;
    long    x_fsize;        // size of function
    } x_misc;
        union
    {
    struct                  // if ISFCN, tag, or .bb
    {
        long    x_lnnoptr;      // ptr to fcn line #
        long    x_endndx;       // entry ndx past block end
    }       x_fcn;
    struct                  // if ISARY, up to 4 dimen.
    {
        unsigned short  x_dimen[DIMNUM];
    }       x_ary;
    }               x_fcnary;
    unsigned short  x_regcount;   // number of registers used by func
    }       x_sym;
    struct
    {
    char    x_fname[FILNMLEN];
    }       x_file;
    struct
    {
        long    x_scnlen;          // section length
        unsigned short  x_nreloc;  // number of relocation entries
        unsigned short  x_nlinno;  // number of line numbers
    }       x_scn;
};


//////////////////////////////////////////////////////////////////////////
//  NAMES OF "SPECIAL" SYMBOLS
//
#define _STEXT          ".text"
#define _ETEXT          "etext"
#define _SDATA          ".data"
#define _EDATA          "edata"
#define _SBSS           ".bss"
#define _END            "end"
#define _CINITPTR       "cinit"

//////////////////////////////////////////////////////////////////////////
//  ENTRY POINT SYMBOLS
//
#define _CSTART         "_c_int00"
#define _START          "_start"
#define _MAIN           "_main"
#define _TVORIG         "_tvorig"
#define _TORIGIN        "_torigin"
#define _DORIGIN        "_dorigin"
#define _SORIGIN        "_sorigin"

//////////////////////////////////////////////////////////////////////////
//
// class CLOAD
// 
//    load              - Main driver for COFF loader.
//    load_headers      - Read in the various headers of the COFF file.
//    load_data         - Read in the raw data and load it into memory.
//    load_sect_data    - Read, relocate, and write out one section.
//    load_cinit        - Process one buffer of C initialization records.
//    load_symbols      - Read in the symbol table.
//    sym_read          - Read and relocate a symbol and its aux entry.
//    sym_name          - Return a pointer to the name of a symbol.
//    reloc_add         - Add a symbol to the relocation symbol table.
//    relocate          - Perform a single relocation.
//    reloc_read        - Read in and swap one relocation entry.
//    reloc_size        - Return the field size of a relocation type.
//    reloc_sym         - Search the relocation symbol table for a symbol.
//    unpack            - Extract a relocation field from object bytes.
//    repack            - Encode a relocated field into object bytes.
//    load_lineno       - Read in & swap line number entries.
//    swap4byte         - Swap the order of bytes in a long.
//    swap2byte         - Swap the order of bytes in a short.
//    set_reloc_amount  - Define relocation amounts for each section.
//
// The loader calls the following external functions to perform application
// specific tasks:
//
//   mem_write          - Load a buffer of data into memory.
//   lookup_sym         - Look up a symbol in an external symbol table.
//   load_syms          - Build the symbol table for the application.
//

// constants, macros, variables, and structures for the loader.
//
inline int MIN( int a, int b )
    { return a < b ? a : b; }

inline int MAX( int a, int b )
    { return a > b ? a : b; }

#define WORDSZ sizeof(T_DATA)           // size of data units in obj file

//
// The relocation symbol table is allocated dynamically, and reallocated
// as more space is needed.
//
#define RELOC_TAB_START 128             // starting size of table
#define RELOC_GROW_SIZE 128             // realloc amount for table

class CLOAD
{
    // this structure is used to store the relocation amounts for symbols.
    // each relocatable symbol has a corresponding entry in this table.
    // the table is sorted by symbol index; lookup uses a binary search.
    //
    struct RELOC_TAB
    {
        short         rt_index;         // index of symbol in symbol table
        short         rt_scnum;         // section number symbol defined in
        unsigned long rt_value;         // relocated value of symbol
        };

    long transferred_byte_count;  // mem_write statistics

    int     verbose;              // verbose mode/debug leve; 
                                  // show what you are doing

    bool    need_symbols;         // read in symbol table
    bool    clear_bss;            // clear bss section
    unsigned long reloc_a;        // Relocation amount

    FILE*   fin;                  // coff input file

    bool    byte_swapped;         // byte ordering opposite of host
    bool    need_reloc;           // relocation required
    bool    big_e_target;         // object data is stored msb first
    int     n_sections;           // number of sections in the file

    COFF_HEADER file_hdr;         // file header structure
    AOUT_HEADER o_filehdr;        // optional (a.out) file header
    T_ADDR  entry_point;          // entry point of module

    SECT_V1_HEADER* sect_hdrs1;    // array of section headers
    SECT_V2_HEADER* sect_hdrs2;    // array of section headers

    char*   str_buf;              // string table

    T_SIZE  init_size;            // current size of c initialization
    char    loadbuf[ 4096 ];      // load buffer

    T_ADDR  reloc_amount[ 256 ];  // amount of relocation per section; 
                                  // max 256 sections
    RELOC_TAB* reloc_tab;         // relocation symbol table
    int     reloc_tab_size;       // current allocated amount
    int     reloc_sym_index;      // current size of table

    T_ADDR  sysparm_addr;          // pointer to .sysparm section
    T_SIZE  sysparm_size;         // size of .sysparm section

private:

    // loader functions.
    //
    bool load_headers( void );         // load file and section headers

    bool load_data( void );            // load raw data from file

    bool load_sect_data( int s );      // load data from one section

    bool load_cinit                    // process buffer of c initialization
    (
        SECT_V2_HEADER* sptr,          // current section
        int* packet_size,              // pointer to buffer size
        int* excess                    // returned number of unused bytes
        );

    bool load_symbols( void );         // load and process coff symbol table

    int sym_read                       // read, relocate, and swap a symbol
    (
        unsigned int index,
        SYMENT* sym,
        AUXENT* aux
        );

    char* sym_name                     // return real name of a symbol
    (
        SYMENT* symptr
        );

    void reloc_add                     // add symbol to reloc symbol table
    (
        unsigned int index,
        SYMENT* sym
        );

    int relocate                       // perform a relocation patch
    (
        RELOC* rp,                     // relocation entry
        unsigned char* data,           // data buffer
        int s                          // index of current section
       );

    int reloc_read( RELOC* rptr );     // read and swap a reloc entry

    int reloc_size( short type );      // return size in bytes of reloc field

    int reloc_sym( int index );        // lookup a symbol in the reloc symtab

    unsigned long unpack               // extract reloc field from data
    (
        unsigned char *data,
        int fieldsz,
        int wordsz,
        int bit_addr                   // bit address within byte
        );

    void repack                        // insert relocated field into data
    (
        unsigned long objval,
        unsigned char *data,
        int fieldsz,
        int wordsz,
        int bit_addr
        );

    bool load_lineno                   // read and relocate lineno entries
    (
        long filptr,                   // where to read from
        int count,                     // how many to read
        LINENO* lptr,                  // where to put them
        int scnum                      // section number of these entries
        );

    void set_reloc_amount( void );

public:

    // Constructors & destructors
    //
    CLOAD( void );
    ~CLOAD( void );

    // Methods
    //
    bool Load( const char* filename, int pverbose );   // main driver for coff loader

    T_ADDR GetEntryPoint( void ) const
    {
        return entry_point;
        }

    T_ADDR GetSysParmAddr( void ) const
    {
        return sysparm_addr;
        }

    T_SIZE GetSysParmSize( void ) const
    {
        return sysparm_size;
        }

    // External functions (defined by user application).
    //
    void* mem_write_this;

    bool (*mem_write) // write to target memory
        ( void* thisp, void* buffer, int page, unsigned long addr, int nbytes );

    void (*lookup_sym) // lookup an undefined symbol and def it
        ( SYMENT* sym, AUXENT* aux, short indx );

    bool (*load_syms)( int need_reloc ); // build the symbol table
    };

//  swap4byte() - Swap the order of bytes in a long.
//
inline void swap4byte( void* addr )
{
    unsigned char t, *tp = (unsigned char*)addr;

    t = tp[ 0 ]; tp[ 0 ] = tp[ 3 ]; tp[ 3 ] = t;
    t = tp[ 1 ]; tp[ 1 ] = tp[ 2 ]; tp[ 2 ] = t;
    }

//  swap2byte() - Swap the order of bytes in a short.
//
inline void swap2byte( void* addr )
{
    unsigned char t, *tp = (unsigned char*)addr;

    t = tp[ 0 ]; tp[ 0 ] = tp[ 1 ]; tp[ 1 ] = t;
    }

#endif // _CLOAD_H_INCLUDED
