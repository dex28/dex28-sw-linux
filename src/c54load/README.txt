
--------------------------------------------------------------------------
GENERIC COFF LOADER                                                   v2.1
--------------------------------------------------------------------------

1. OVERVIEW

This spec describes a generic COFF loader that can be used as the basis for
any application that needs to read in and process a COFF object file for 
any of TI's processors.

This loader supports the following features:

   - Relocation (separate relocation amounts by section).
   - Relocation fields on bit boundaries (applies to 340 processors).
   - Byte-swapped object files (file was built on host with opposite byte
       ordering).
   - C initialization using RAM model (load-time initialization).
   - Big or little endian target format.
   - Support for applications using COFF symbolic information. 
   - Relocation to an external (application) symbol table to permit dynamic 
     linking.

The source code for the loader is 100% target independent.  Target specific
parameters are represented as compile-time constants in a header file.   The
source is also intended to be generic, in the sense that it can be built into
a variety of applications with no change.

2. INTERFACING

2.1 Source Files
----------------

The loader consists of one C file (CLOAD.C) and four header files:

     VERSION.H   specifies the target CPU.
     COFF.H      definition of COFF data structures.
     PARAMS.H    target parameters (word sizes, etc.).
     CLOAD.H     declarations of loader variables and functions.

The application should include the header files above (in the order listed)
in modules that interface with the loader, thereby providing access to the
loader variables and functions.

The types, variables, and functions for the loader are defined in the main
source module CLOAD.C.  All objects that are internal to the loader are
declared 'static' to minimize problems with name conflicts.  CLOAD.C
should require NO modification.

2.2 Loader Variables
--------------------

There are several global variables defined in the loader that an application
can access.  These are defined in CLOAD.C, and declared external in CLOAD.H.

   int verbose;

	 If TRUE, the loader prints progress information as it runs via the
	 function 'load_msg()'.  If FALSE, the loader runs silently.

   int need_symbols;

	 If TRUE, the loader calls the application to load the symbol table
	 (via the function 'load_syms()'.  This allows the application to 
         build its own symbol table from the COFF file.  If FALSE, the loader
	 only reads the symbol table as as necessary to perform relocation.

   int clear_bss;

	 If TRUE, the loader fills the .bss section with zeros.  This is 
	 useful for C programs that require global and static variables to 
	 be pre-initialized to zero. 

   FILE *fin;

       This variable is the file pointer to the input COFF file.  The 
       application is responsible for opening the file before calling
       cload and closing it after cload returns.

   FILHDR  file_hdr;             /* FILE HEADER STRUCTURE               */
   AOUTHDR o_filehdr;            /* OPTIONAL (A.OUT) FILE HEADER        */
   T_ADDR  entry_point;          /* ENTRY POINT OF MODULE               */
   char   *sect_hdrs;            /* ARRAY OF SECTION HEADERS            */
   char   *str_buf;              /* STRING TABLE                        */
   int     n_sections;           /* NUMBER OF SECTIONS IN THE FILE      */
   int     big_e_data;           /* TARGET DATA STORED MSB FIRST        */

        The above variables allow access to various parameters of the COFF
	file.

   T_ADDR reloc_amount[];

	This array contains the relocation amounts for each section.  Initially,
	this array contains all zeros.  The application can specify relocation
	for individual sections via the 'set_reloc_amount' function.

2.3 Loader Functions
--------------------

There are 5 functions in the loader that are intended to be called from
from the application.  These are defined in CLOAD.C.

   int cload()

	 This is the main driver of the loader.  The application should open
	 the object file (for reading, binary mode) and assign the file pointer
	 to the variable 'fin'.  The cload function returns TRUE (1) or FALSE 
	 (0) to indicate if the file was sucessfully read and loaded.

   int sym_read(index, sym, aux)
      int index;
      SYMENT *sym;
      AUXENT *aux;

         This function can be called by the application to read in a symbol 
	 from the symbol table.  The symbol is read in, byte-swapped if
	 necessary, relocated, and stored in *sym.  The aux entry, if present,
	 is read into *aux.  This function returns the index of the next 
	 symbol to be read; that is, the index of the symbol following
	 the last aux entry of 'sym'.

   void reloc_add(index, sym)
      int index;
      SYMENT *sym;

	 In some cases the symbol table is read by the application (indicated
	 by the need_symbols flag being TRUE).  In these cases, the application
	 must provide the loader with relocation information from the symbol
	 table if the file is being relocated.  If need_symbols is TRUE, the
	 loader calls the application function 'load_syms()' to read the 
	 symbol table.  The loader passes load_syms a flag called 'need_reloc'.
	 If need_reloc is TRUE, the file is being relocated and load_syms
	 must provide the loader with relocation information via the function
	 'reloc_add()'.

	 The application must call reloc_add() for each relocatable symbol
	 that is read.  The loader uses this function to build the relocation
	 symbol table, which is later used for patching relocatable references
	 in the raw data.  The parameters to this function are the index in
	 the COFF symbol table of the symbol, and a pointer to the symbol
	 itself.  Any symbol read in that has one of the following storage
	 classes is relocatable and should be passed to this function:

	    C_EXT, C_STAT, C_EXTDEF, C_LABEL, C_BLOCK, C_FCN

	 Symbols must be passed to this function IN THE ORDER THEY APPEAR
	 in the COFF file.

   char *sym_name(sym)
      SYMENT *sym;

	 This function returns a pointer to a symbol name.  If the symbol name
	 has 8 characters or less, the name is copied into a static area, with
	 a terminating NULL byte, and the function returns a pointer to this
	 area.  Otherwise, the function returns a pointer to the name in the 
	 string table.   Note that since this function can return a pointer 
	 to a static string, the application must copy it out if the name
	 must be kept.

   int cload_lineno(filptr, count, lptr, scnum)
      long filptr;
      int count;
      LINENO *lptr;
      int scnum;

         This function is for applications that use the line number entries
	 from the COFF file.  (Line number entries are not used by the loader).
	 This function seeks to the file position specified by 'filptr', reads,
	 swaps, and relocates 'count' line number entries, and stores them in
	 the area pointed to by 'lptr'.  The variable 'scnum' identifies the
	 section correspoinding to these line number entries (for relocation).
 
Also, there is a macro defined in CLOAD.H that can be useful:

   SCNHDR *SECT_HDR(scn_num)

         Returns a pointer to the specified section header from the COFF file.


2.4 Application Functions
-------------------------

There are 7 functions that the loader calls to perform application specific
tasks:

   void set_reloc_amount()

	This function is called once to determine the amount of relocation
	for each section of the object file, allowing the application to 
	allocate the program into memory on a section by section basis. 
	The simplest case for this function is to do nothing, in which case
	no relocation is performed. 

	To provide load-time relocation, the application must set the values 
	in the array 'reloc_amount[]'.  The following example would relocate
	the entire program by 100 locations (increase the address of each
	section by 100).

	   set_reloc_amount()
	   {
	      int i;
	      for (i = 0; i < n_sections; ++i) reloc_amount[i] = 100;
	   }

   int mem_write(buffer, size, addr, page)
      unsigned char *buffer;
      int size;
      T_ADDR addr;
      int    page;

	This function is called to load 'size' bytes data into target system 
	memory, at the target address specified by 'addr'.  'Page' specifies
	an overlay page number, from the section header in the file. The data
	is pointed to by 'buffer'.  This function must interpret a string of
	bytes into a target memory image -- this may involve packing, 
	byte-swapping, or other target-dependent operations.  The maximum
	size of the buffer is specified as the LOADBUFSIZE constant in
	PARAMS.H.

   int load_syms(need_reloc)
      int need_reloc;

        This function is called only if 'need_symbols' is TRUE.  This allows
	the COFF symbol table to be read in under control of the application.
	This function should loop through the symbol table, calling 'sym_read()'
	to read and relocate each symbol.  The application can then process
	each symbol as necessary and build an independent symbol table.

	The 'need_reloc' parameter passed to this function indicates whether
	the loader also needs to process the symbol table to perform relocation.
	If this flag is TRUE, load_syms must call 'reloc_add()' for each 
	relocatable symbol in the file.  This allows the loader to enter the
	symbol into the relocation symbol table.

	A simple example of load_syms might be:

	   /*  NOTE: set need_reloc = TRUE before calling cload()  */

	   int load_syms(need_reloc)
	      int need_reloc;
	   {
	      int i, next;

	      for (i = 0; i < file_hdr.f_nsyms; i = next)
	      {
		 SYMENT s;
		 AUXENT a;

		 next = sym_read(i, &s, &a);

		 /* .... enter into application symbol table ....  */

		 if (need_reloc && 
		    (s.n_sclass == C_EXT   || s.n_sclass == C_EXTDEF ||
		     s.n_sclass == C_STAT  || s.n_sclass == C_LABEL  ||
		     s.n_sclass == C_BLOCK || s.n_sclass == C_FCN)) 
		   reloc_add(i, &s);
	       }
	       return TRUE;
	    }

   void lookup_sym(index, sym, aux)
      int index;
      SYMENT *sym;
      AUXENT *aux;

        This function is provided so that an application can dynamically link
	a relocatable load module to an external symbol table at load time.
	Each time the loader encounters an unresolved symbol in the object
	file, this function is called to look up the symbol.  The application
	can "define" the symbol at load time by filling in the 'n_value' field
	of the symbol entry (pointed to by 'sym').  Also, the 'n_scnum' field
	should be set to 0 tyo indicarte to the loader that this is an absolute
	symbol.   References to the symbol will then be correctly relocated.  

	If dynamic linking is not a requirement for the application, this
	function can be defined to do nothing.
 
   char *myalloc(nbytes)
      int nbytes;
      
	This function is called by the loader to allocate memory.  It is
	equivilent to the standard function 'malloc()'.   The application can
	take special action upon allocation failure, such as printing an
	error message and aborting, or trying to recover.

	The loader uses dynamic memory for:
	   - section headers
	   - string table
	   - internal symbol table for processing relocation entries

        After loading, the loader frees the relocation symbol table.  The
	section headers and sting table remain allocated when the loader 
	returns.

   char *mralloc(ptr, nbytes)
      char *ptr;
      int nbytes;

        This is the application version of 'realloc()'.  The loader uses this
	function to expand the size of the relocation symbol table.

   void load_msg(fmt, arg1, arg2, ...)
      char *fmt;

        This function is called to output progress information during loading
	if the 'verbose' flag is TRUE.   This function is never called if
	'verbose' is FALSE.  The arguments are passed in the style of
	'printf()'.  (There can be up to 4 arguments in addition to the format
	string.)  The simplest form of this function (other than doing
	nothing) simply passes its arguments to printf:

            load_msg(fmt, arg1, arg2, arg3, arg4)
	       char *fmt;
	    {
	       printf(fmt, arg1, arg2, arg3, arg4, arg5);
	    } 

2.5 Target Parameters 
---------------------

Various parameters of the target device are defined in PARAMS.H.  These are:

Macros:

   MAGIC           - Magic number identifying a COFF file for this target.
   BYTETOLOC(x)    - Maps between 8-bit bytes and machine addresses.
                     For example, if the target is addressable by 16-bit 
		     words: "#define BYTETOLOC(x) x<<1".
   LOCTOBYTE(x)    - Maps between target machine addresses and 8-bit bytes.
		     For example, if the target is addressable by 32-bit
		     words: "#define LOCTOBYTE(x) x>>2".
   BIT_OFFSET(a)   - Given a target address, returns the offset within a byte
		     of the address.  Applies only to bit addressable targets
		     like the 34010.  For other targets, this should be defined
		     to be 0.
   LOADBUFSIZE     - Size, in bytes, of the buffer used for writing data
		     to the target.
   LOADWORDSIZE    - If the target memory interface must write data in certain
		     multiples of bytes, this constant specifies the multiple. 
   CINIT           - Name of the COFF section containing C initialization 
		     tables (usually ".cinit").
   INIT_ALIGN      - Alignment requirements for init records (in bytes).  NOTE:
		     LOADBUFSIZE must be a multiple of this size.

Typedefs:

   T_ADDR          - Host type representing a target address.  For example,
		     if addresses are 32 bits: "typedef unsigned long T_ADDR".
   T_DATA          - Host type representing a target data word, as stored in
		     the object file.  For example, if object data is stored
		     as 16-bit words: "typedef unsigned short T_DATA."
   T_SIZE          - Host type corresponding to the size field in the C 
		     initialization tables.  For example, if the size field
		     in the tables is a 16-bit value: "typedef unsigned short
		     T_SIZE".

2.6 Interface Summary
---------------------

Target Parmaters (PARAMS.H)

   typedef T_ADDR                /* HOST TYPE FOR TARGET ADDRESS        */
   typedef T_DATA                /* HOST TYPE FOR TARGET MEMORY WORD    */
   typedef T_SIZE                /* HOST TYPE FOR SIZE FIELD IN .CINIT  */

   BYTETOLOC(x)                  /* CONVERT ADDRESSES TO BYTES          */
   LOCTOBYTE(x)                  /* CONVERT BYTES TO ADDRESSES          */
   BIT_OFFSET(a)                 /* BIT OFFSET OF ADDR WITHIN BYTE      */
   LOADBUFSIZE                   /* SIZE OF WRITE BUFFER FOR RAW DATA   */
   LOADWORDSIZE                  /* BYTE MULTIPLE FOR DATA WRITES       */
   CINIT                         /* NAME OF C INITIALIZATION SECTION    */
   INIT_ALIGN                    /* ALIGNMENT OF INIT RECORDS, IN BYTES */


Loader Variables (CLOAD.H, CLOAD.C)

   int     verbose;              /* PRINT PROGRESS INFO                 */
   int     need_symbols;         /* READ IN SYMBOL TABLE                */
   int     clear_bss;            /* FILL .bss SECTION WITH ZEROS        */

   FILE   *fin;                  /* INPUT COFF FILE                     */
   FILHDR  file_hdr;             /* FILE HEADER STRUCTURE               */
   AOUTHDR o_filehdr;            /* OPTIONAL (A.OUT) FILE HEADER        */
   T_ADDR  entry_point;          /* ENTRY POINT OF MODULE               */
   T_ADDR  reloc_amount[];       /* AMOUNT OF RELOCATION PER SECTION    */
   char   *sect_hdrs;            /* ARRAY OF SECTION HEADERS            */
   char   *str_buf;              /* COFF STRING TABLE                   */
   int     n_sections;           /* NUMBER OF SECTIONS IN THE FILE      */


Loader Macros (CLOAD.H)

   SCNHDR *SECT_HDR(i)           /* RETURN POINTER TO SECTION HEADER I  */


Loader Functions (CLOAD.C)

   int     cload()
   int     sym_read(int index, SYMENT *sym, AUXENT *aux)
   int     reloc_add(int index, SYMENT *sym)
   char   *sym_name(SYMENT *sym)
   int     cload_lineno(long filptr, int count, LINENO *lptr, int scnum)


Application Functions
   
   void  set_reloc_amount()
   int   mem_write(unsigned char *buffer, int size, T_ADDR addr, int page)
   void  lookup_sym(int index, SYMENT *sym, AUXENT *aux)
   void  load_syms(int need_reloc)
   char *myalloc(int nbytes)
   char *mralloc(char *ptr, int nbytes)
   void  load_msg(char *fmt, ...)
