/*
    Project:     Albatross / c54load
  
    Module:      c54load.cpp
  
    Description: Albatross c54xx COFF loader
                 CLOAD class implementation
  
    Copyright (c) 2002-2005 By Mikica B Kocic
    Copyright (c) 2002-2005 By IPTC Technology Communications AB
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c54load.h"

CLOAD::CLOAD( void )
{
    verbose = 0;

    need_symbols = false; // application needs symbol table
    clear_bss    = true; // clear bss section
    reloc_a = 0; // relocation amount

    fin = NULL;
    byte_swapped = false;
    init_size = 0; // current size of c initialization
    need_reloc = false; // relocation required

    entry_point = 0;

    reloc_tab = NULL;

    mem_write_this = NULL;
    mem_write = NULL;
    lookup_sym = NULL;
    load_syms = NULL;

    sysparm_addr = 0;
    sysparm_size = 0;
    }

CLOAD:: ~CLOAD( void )
{
    if ( reloc_tab )
        delete [] reloc_tab;
    }

// load() - Main driver for COFF loader.
//
bool CLOAD::Load( const char* filename, int pverbose = 0 )
{
    transferred_byte_count = 0;
    verbose = pverbose;

    fin = fopen( filename, "rb" );
    if ( ! fin )
    {
        printf( "Error: Failed to open %s\n", filename );
        return false;
        }

    if ( verbose >= 1 )
        printf( "File '%s' open.\n", filename );

    if ( ! load_headers() )
    {
        printf( "Error: While loading headers\n" );
        return false;
        }

    if ( ! load_symbols() )
    {
        printf( "Error: While loading symbols\n" );
        return false;
        }
   
    if ( ! load_data() )
    {
        printf( "Error: While loading data\n" );
        return false;
        }

    if ( verbose >= 1 )
        printf( "Transferred %ld bytes\n", transferred_byte_count );

    return true;
    }

// load_headers() - Read in the various headers of the COFF file.
//
bool CLOAD::load_headers( void )
{
    if (!fread(&file_hdr, sizeof(COFF_HEADER), 1, fin))
    {
        printf( "Error: While reading COFF header\n" );
        return false;
        }

    // Structure size sanity check
    //
    if ( sizeof(COFF_HEADER) != 22 || sizeof(AOUT_HEADER) != 28
       || sizeof(SECT_V1_HEADER) != 40 || sizeof(SECT_V2_HEADER) != 48
       || sizeof(SYMENT) != 18 // FIXME || sizeof(AUXENT) != 18
       )
    {
        printf( "Error: COFF_HEADER size %d, AOUT_HEADER size %d, "
                "SECT_V1_HEADER size %d, SECT_V2_HEADER size %d, "
                "SYMENT size %d, AUXENT size %d\n",
            sizeof(COFF_HEADER), sizeof(AOUT_HEADER), 
	    sizeof(SECT_V1_HEADER), sizeof(SECT_V2_HEADER),
	    sizeof(SYMENT), sizeof(AUXENT)
	    );
        return false;
        }

    // make sure this is really a coff file. check for swapped files.
    // determine byte ordering of object data.
    //
    if ( file_hdr.f_version != COFF_V2 && file_hdr.f_version != COFF_V1 )
    {
        swap2byte( &file_hdr.f_version );

        if ( file_hdr.f_version != COFF_V2 && file_hdr.f_version != COFF_V1 )
        {
            printf( "Error: COFF Version number is wrong\n" );
            return false;
            }

        byte_swapped = true;

        swap2byte( &file_hdr.f_nscns ); 
        swap4byte( &file_hdr.f_timdat );
        swap4byte( &file_hdr.f_symptr );
        swap4byte( &file_hdr.f_nsyms );
        swap2byte( &file_hdr.f_opthdr );
        swap2byte( &file_hdr.f_flags );
        swap2byte( &file_hdr.f_target );
        }

    big_e_target = file_hdr.f_flags & F_BIG;

    //
    // read in optional header, if there is one, and swap if needed.
    //
    if ( file_hdr.f_opthdr == sizeof(AOUT_HEADER) )
    {
        if ( fread(&o_filehdr, file_hdr.f_opthdr, 1, fin) != 1) 
        {
            printf( "Error: While reading optional header\n" );
            return false;
            }

        if ( byte_swapped)
        {
            swap2byte( &o_filehdr.magic );
            swap2byte( &o_filehdr.vstamp );
            swap4byte( &o_filehdr.tsize );
            swap4byte( &o_filehdr.dsize );
            swap4byte( &o_filehdr.bsize );
            swap4byte( &o_filehdr.entrypt );
            swap4byte( &o_filehdr.text_start );
            swap4byte( &o_filehdr.data_start );
            }

        entry_point = o_filehdr.entrypt;
        }

    else if( file_hdr.f_opthdr && fseek( fin, (long)file_hdr.f_opthdr, SEEK_SET ) == -1 )
    {
        printf( "Error: While reading optional header\n" );
        return false;
        }

   // Read in section headers (be ware of COFF1 / COFF2 section header format!).
   //
   n_sections = file_hdr.f_nscns;

   if ( file_hdr.f_version == COFF_V1 )
   {
       sect_hdrs1 = new SECT_V1_HEADER[ n_sections ];
       if ( sect_hdrs1 == NULL )
       {
           printf( "Error: While allocating section headers\n" );
           return false;
           }

       if ( fread( sect_hdrs1, sizeof(SECT_V1_HEADER), n_sections, fin ) 
            != (unsigned int)n_sections )
       {
           printf( "Error: While reading section headers\n" );
           return false;
           }

        // Swap section headers if required.
        //
        if ( byte_swapped )
        {
            for ( int i = 0; i < n_sections; i++ )
            {
                SECT_V2_HEADER *sptr = &sect_hdrs2[ i ];

                swap4byte(&sptr->s_paddr);
                swap4byte(&sptr->s_vaddr);
                swap4byte(&sptr->s_size);
                swap4byte(&sptr->s_scnptr);
                swap4byte(&sptr->s_relptr);
                swap4byte(&sptr->s_lnnoptr);
                swap2byte(&sptr->s_nreloc);
                swap2byte(&sptr->s_nlnno);
                swap2byte(&sptr->s_flags);
                }
            }
       }
   else if ( file_hdr.f_version == COFF_V2 )
   {
       sect_hdrs2 = new SECT_V2_HEADER[ n_sections ];
       if ( sect_hdrs2 == NULL )
       {
           printf( "Error: While allocating section headers\n" );
           return false;
           }

       if ( fread( sect_hdrs2, sizeof(SECT_V2_HEADER), n_sections, fin )
            != (unsigned int)n_sections )
       {
           printf( "Error: While reading section headers\n" );
           return false;
           }

        // Swap section headers if required.
        //
        if ( byte_swapped )
        {
            for( int i = 0; i < n_sections; i++ )
            {
                SECT_V2_HEADER *sptr = &sect_hdrs2[ i ];

                swap4byte(&sptr->s_paddr);
                swap4byte(&sptr->s_vaddr);
                swap4byte(&sptr->s_size);
                swap4byte(&sptr->s_scnptr);
                swap4byte(&sptr->s_relptr);
                swap4byte(&sptr->s_lnnoptr);
                swap4byte(&sptr->s_nreloc);
                swap4byte(&sptr->s_nlnno);
                swap4byte(&sptr->s_flags);
                swap2byte(&sptr->s_page);
                }
            }
       }



    // Call an external routine to determine the relocation amounts for
    // each section.
    //
    for ( int i = 0; i < n_sections; i++ )
        reloc_amount[i] = 0;

    set_reloc_amount();

    for ( int i = 0; i < n_sections; i++ )
        need_reloc |= (reloc_amount[i] != 0);

    if ( verbose >= 1 )
    {
        printf( "%d sections, target 0x%x, %s format, %s data, %s\n",
            n_sections, file_hdr.f_target,
            byte_swapped ? "swapped" : "unswapped",
            big_e_target ? "big-E" : "little-E",
            need_reloc   ? "Relocatable file" : "No reloc");
        }

    if ( verbose >=2 )
    {
        for ( int i = 0; i < n_sections; i++)
        {
            SECT_V2_HEADER *sptr = &sect_hdrs2[ i ];
            printf( "section %2d: '%-8.8s', paddr 0x%04lx, vaddr 0x%04lx, size %4ld\n",
                i, sptr->s_name, sptr->s_paddr, sptr->s_vaddr, sptr->s_size
                );
            }
        }

    return true;
    }

// load_symbols() - Read in the symbol table.
//
bool CLOAD::load_symbols( void )
{
    if ( ! need_symbols && ! need_reloc )
        return true;

    // Allocate the relocation symbol table.
    //
    if ( need_reloc )
    {
        reloc_tab_size = MIN( RELOC_TAB_START, file_hdr.f_nsyms );
        reloc_tab = new RELOC_TAB[ reloc_tab_size ];
        reloc_sym_index = 0;
        }

    // Seek to the end of the symbol table and read in the size of the string
    // table. then, allocate and read the string table.
    //
    long str_size;
    if( fseek( fin, file_hdr.f_symptr + (file_hdr.f_nsyms * sizeof(SYMENT)), 0) == -1 ||
       fread( &str_size, sizeof(long), 1, fin ) != 1 )
    {
       str_size = 0;
        }
    else
    {
        if( byte_swapped) swap4byte(&str_size);
            str_buf = new char[ str_size + 4 ];

        if( fread( str_buf + 4, (int)str_size - 4, 1, fin) != 1 )
            return false;
        }

    if ( verbose >= 1 )
    {
        printf("%ld symbols.  String table size=%ld\n",
		             file_hdr.f_nsyms, str_size - 4);
        }

    // If the application needs the symbol table, let it read it in.
    // pass need_reloc to the application so that it can call reloc_add()
    //
    if ( need_symbols )
    {
        if ( ! load_syms )
            return true;

        return load_syms( need_reloc );
        }

    //  Read the symbol table and build the relocation symbol table
    //  for symbols that can be used in relcoation, store them in a
    //  special symbol table that can be searched quickly during
    //  relocation.
    //
    for ( int next, first = 0; first < file_hdr.f_nsyms; first = next )
    {
        SYMENT sym;
        AUXENT aux;

        if (!(next = sym_read(first, &sym, &aux)))
            return false;

        if( sym.n_sclass == C_EXT   || sym.n_sclass == C_EXTDEF ||
           sym.n_sclass == C_STAT  || sym.n_sclass == C_LABEL  ||
           sym.n_sclass == C_BLOCK || sym.n_sclass == C_FCN
           )
            reloc_add( first, &sym );
        }

    return true;
    }

// sym_read() - Read and relocate a symbol and its aux entry.  Return the
//              index of the next symbol.
//
int CLOAD::sym_read
(
  unsigned int index,
  SYMENT *sym,
  AUXENT *aux
  )
{
    //
    // Read in a symbol and its aux entry.
    //
    if( fseek( fin, file_hdr.f_symptr + ((long)index * sizeof(SYMENT)), 0 ) == -1 ||
        fread( sym, sizeof(SYMENT), 1, fin ) != 1                                 ||
        (sym->n_numaux && fread(aux, sizeof(SYMENT), 1, fin) != 1)) return false;

    if( byte_swapped)
    {
        //
        // Swap the symbol table entry.
        //
        if( sym->n_zeroes == 0)
            swap4byte(&sym->n_nptr);
        swap4byte(&sym->n_value);
        swap2byte(&sym->n_scnum);
        swap2byte(&sym->n_type);

        //
        // Swap the aux entry, based on the storage class.
        //
        if( sym->n_numaux) switch( sym->n_sclass )
        {
          case C_FILE    : break;

          case C_STRTAG  :
          case C_UNTAG   :
          case C_ENTAG   : swap2byte(&aux->x_sym.x_misc.x_lnsz.x_size);
                           swap4byte(&aux->x_sym.x_fcnary.x_fcn.x_endndx);
                           break;

          case C_FCN     :
          case C_BLOCK   : swap2byte(&aux->x_sym.x_misc.x_lnsz.x_lnno);
                           swap4byte(&aux->x_sym.x_fcnary.x_fcn.x_endndx);
                           break;

          case C_EOS     : swap2byte(&aux->x_sym.x_misc.x_lnsz.x_size);
                           swap4byte(&aux->x_sym.x_tagndx);
                           break;

          default        : //
                           // Handle function definition symbol
                           //
                           if (((sym->n_type >> 4) & 3) == DT_FCN)
                           {
                               swap4byte(&aux->x_sym.x_tagndx);
                               swap4byte(&aux->x_sym.x_misc.x_fsize);
                               swap4byte(&aux->x_sym.x_fcnary.x_fcn.x_lnnoptr);
                               swap4byte(&aux->x_sym.x_fcnary.x_fcn.x_endndx);
                               swap2byte(&aux->x_sym.x_regcount);
                           }

                           //
                           // Handle arrays.
                           //
                           else if (((sym->n_type >> 4) & 3) == DT_ARY)
                           {
                               swap4byte(&aux->x_sym.x_tagndx);
                               swap2byte(&aux->x_sym.x_misc.x_lnsz.x_lnno);
                               swap2byte(&aux->x_sym.x_misc.x_lnsz.x_size);
                               swap2byte(&aux->x_sym.x_fcnary.x_ary.x_dimen[0]);
                               swap2byte(&aux->x_sym.x_fcnary.x_ary.x_dimen[1]);
                               swap2byte(&aux->x_sym.x_fcnary.x_ary.x_dimen[2]);
                               swap2byte(&aux->x_sym.x_fcnary.x_ary.x_dimen[3]);
                           }

                           //
                           // Handle section definitions
                           //
                           else if( sym->n_type == 0)
                           {
                               swap4byte(&aux->x_scn.x_scnlen);
                               swap2byte(&aux->x_scn.x_nreloc);
                               swap2byte(&aux->x_scn.x_nlinno);
                           }

                           //
                           // Handle misc symbol record
                           //
                           else
                           {
                               swap2byte(&aux->x_sym.x_misc.x_lnsz.x_size);
                               swap4byte(&aux->x_sym.x_tagndx);
                           }
        }
    }

    //
    // Relocate the symbol, based on its storage class.
    //
    switch( sym->n_sclass )
    {
       case C_EXT     :
       case C_EXTDEF  :
       case C_STAT    :
       case C_LABEL   :
       case C_BLOCK   :
       case C_FCN     :
	  //
	  // If the symbol is undefined, call an application routine to look
	  // it up in an external symbol table.  if the symbol is defined,
	  // relocate it according to the section it is defined in.
	  //
          if( sym->n_scnum == 0)
          {
              if( lookup_sym )
                  lookup_sym( sym, aux,index );
              }
          else if( sym->n_scnum > 0)
	        sym->n_value += reloc_amount[sym->n_scnum - 1];
    }
    return index + sym->n_numaux + 1;
}

//
// sym_name() - Return a pointer to the name of a symbol in either the symbol
//              entry or the string table.
char* CLOAD::sym_name
(
   SYMENT *symptr
   )
{
    static char temp[9];

    if( symptr->n_zeroes == 0) return str_buf + symptr->n_offset;

    strncpy( temp, symptr->n_name, 8 );
    temp[8] = 0;
    return temp;
}

// load_data() - Read in the raw data and load it into memory.
//
//
bool CLOAD::load_data( void )
{
    bool ok = true;

    // Loop through the sections and load them one at a time.
    //
    for( int s = 0; s < n_sections && ok; s++ )
    {
        SECT_V2_HEADER* sptr = &sect_hdrs2[ s ];
        int  bss_flag;

        // If this is the text section, relocate the entry point.
        //
        if ( ( sptr->s_flags & STYP_TEXT) && !strcmp(sptr->s_name, ".text" ) )
            entry_point += reloc_amount[s];

        // Set a flag indicating a bss section that must be loaded.
        //
        bss_flag = clear_bss 
            && ( sptr->s_flags & STYP_BSS ) != 0
            && ! strcmp( sptr->s_name, ".bss" );

        // Ignore empty sections or sections whose flags indicate the
        // section is not to be loaded.  note that the cinit section,
        // although it has the styp_copy flag set, must be loaded.
        //
        if ( ! sptr->s_scnptr  && ! bss_flag
            || ! sptr->s_size
            || ( sptr->s_flags & STYP_DSECT )
            || ( sptr->s_flags & STYP_COPY ) && strcmp( sptr->s_name, CINIT ) 
            || ( sptr->s_flags & STYP_NOLOAD )
            )
            continue;

        if ( strncmp( sptr->s_name, ".sysparm", 8 ) == 0 )
        {
            sysparm_addr = sptr->s_vaddr;
            sysparm_size = sptr->s_size;
            }

        // Load all other sections.
        //
        if ( verbose >= 1 )
        {
            printf( "Loading %-8.8s, page=%d, addr=0x%04lx..0x%04lx, size=0x%04lx\n",
                sptr->s_name, sptr->s_page, 
                sptr->s_vaddr, sptr->s_vaddr + sptr->s_size - 1, sptr->s_size
                );
            }

        ok = load_sect_data( s );
        }

    return ok;
    }

// load_sect_data() - Read, relocate, and write out the data for one section.
//
//
bool CLOAD::load_sect_data
(
    int s                                  // Index of current section
    )
{
    SECT_V2_HEADER*       sptr    = &sect_hdrs2[ s ];
    T_ADDR        addr    = sptr->s_vaddr; // current address in section
    int           packet_size;             // size of current data buffer
    int           excess  = 0;             // bytes left from previous buffer
    unsigned int  n_reloc = 0;             // counter for relocation entries
    RELOC         reloc;                   // relocation entry
    int           bss_flag;

    long nbytes;                  // byte count within section

    // Read the first relocation entry, if there are any.
    // if this is a bss section, clear the load buffer.
    //
    if( need_reloc && sptr->s_nreloc 
        && ( fseek( fin, sptr->s_relptr, 0 ) == -1 || !reloc_read( &reloc ) ) )
    {
        printf( "Error: Section %d: Failed to seek relocation entry\n", s );
        return false;
        }

   bss_flag = ( sptr->s_flags & STYP_BSS );
   if ( bss_flag && !strcmp(sptr->s_name, ".bss"))
      for ( nbytes = 0; (size_t)nbytes < sizeof(loadbuf); ++nbytes )
          loadbuf[nbytes] = 0;

   //
   // Copy all the data in the section.
   //
   for ( nbytes = 0; nbytes < LOCTOBYTE(sptr->s_size); nbytes += packet_size )
   {
      int j, ok;
      //
      // Read in a buffer of data.  if the previous relocation spanned
      // accross the end of the last buffer, copy the leftover bytes into
      // the beginning of the new buffer.
      //
      for ( j = 0; j < excess; ++j) loadbuf[j] = loadbuf[packet_size + j];

      packet_size = MIN(LOCTOBYTE(sptr->s_size) - nbytes, sizeof(loadbuf));

      if (!bss_flag &&
	  (fseek(fin, sptr->s_scnptr + nbytes + excess, 0) == -1 ||
	   fread(loadbuf + excess, packet_size - excess, 1, fin) != 1))
	 return false;
      excess = 0;

      //
      // Read and process all the relocation entries that affect data
      // currently in the buffer.
      //
      if( need_reloc)
	 while( n_reloc < sptr->s_nreloc &&
		reloc.r_vaddr < addr + BYTETOLOC(packet_size))
	 {
	    int i = LOCTOBYTE(reloc.r_vaddr - addr);   // Byte index of field

	    //
	    // If this relocation spans past the end of the buffer,
	    // set 'excess' to the number of remaining bytes and flush the
	    // buffer.  at the next fill, these bytes will be copied from
	    // the end of the buffer to the beginning and then relocated.
	    //
	    if( i + MAX(WORDSZ, reloc_size(reloc.r_type)) > packet_size)
	    {
	       i           -= i % LOADWORDSIZE;   // Don't break within a word
	       excess      = packet_size - i;
	       packet_size = i;
	       break;
	    }

	    //
	    // Perform the relocation and read in the next relocation entry.
	    //
	    relocate(&reloc, (unsigned char*)loadbuf + i, s);

	    if( n_reloc++ < sptr->s_nreloc                                    &&
	       (fseek(fin, sptr->s_relptr + ((long)n_reloc * sizeof(RELOC)  ), 0) == -1 ||
		 !reloc_read(&reloc))) return false;
	 }

      // Write out the relocated data to the target device. If this is a
      // cinit section, call a special function to handle it.
      //
      ok = (sptr->s_flags & STYP_COPY) && !strcmp(sptr->s_name, CINIT)
          ? load_cinit(sptr, &packet_size, &excess) 
          : ! mem_write
          ? false
          : ! mem_write( mem_write_this, (unsigned char*)loadbuf, sptr->s_page, addr + reloc_amount[s], packet_size )
          ? false
          : ( transferred_byte_count += packet_size, true );

      if ( ! ok ) return false;

      //
      // Keep track of the address within the section.
      //
      addr += BYTETOLOC(packet_size);
   }
   return true;
}

//
//
// load_cinit() - Process one buffer of C initialization records.
//
//
bool CLOAD::load_cinit
(
   SECT_V2_HEADER *sptr,                         // Current section
   int *packet_size,                     // Pointer to buffer size
   int *excess                           // Returned number of unused bytes
   )
{
   int           i;                      // Byte counter
   int           init_packet_size;       // Size of current initialization
   static T_ADDR init_addr;              // Address of current initialization

   //
   //  Process all the initialization records in the buffer.
   //
   for (i = 0; i < *packet_size; i += init_packet_size)
   {
      //
      // If starting a new initialization, read the size and address from
      // the table.
      //
      if( init_size == 0)
      {
	 T_SIZE temp;

	 //
	 // If the size and address are not fully contained in this buffer,
	 // stop here.  set the 'excess' counter to the number of unprocessed
	 // bytes - these will be copied to the head of the next buffer.
	 //
	 if( i + sizeof(T_SIZE) > (size_t) *packet_size)
	    { *excess += *packet_size - i;  *packet_size = i;  break; }

	 //
	 // If the next size field is zero, break.
	 //
	 temp = unpack((unsigned char*)loadbuf + i, sizeof(T_SIZE), sizeof(T_SIZE), 0);
	 if( temp == 0) break;

	 //
	 // Read the address field, if it's all here.
	 //
	 if( i + sizeof(T_SIZE) + sizeof(T_ADDR) > (size_t)*packet_size)
	    { *excess += *packet_size - i;  *packet_size = i;  break; }

	 i         += sizeof(T_SIZE);
	 init_size  = temp;
	 init_addr  = unpack((unsigned char*)loadbuf + i, sizeof(T_ADDR), sizeof(T_ADDR), 0);
	 i         += sizeof(T_ADDR);
      }

      //
      // Write out the current packet, up to the end of the buffer.
      //
      init_packet_size = MIN(*packet_size - i, init_size * WORDSZ);
      if ( init_packet_size )
      {
	    if ( mem_write )
            if ( mem_write( mem_write_this, (unsigned char*)loadbuf + i, sptr->s_page, init_addr, init_packet_size  ))
                transferred_byte_count += init_packet_size;
            else
	            return false;

            init_addr += BYTETOLOC(init_packet_size);
            init_size -= init_packet_size / WORDSZ;
      }
   }
   return true;
}

//
//
// RELOC_ADD() - Add an entry to the relocation symbol table.  This table
//               stores relocation information for each relocatable symbol.
//
//
void CLOAD::reloc_add(
   unsigned int index,
   SYMENT      *sym
   )
{
   if (!need_reloc) return;

   if( reloc_sym_index >= reloc_tab_size)
   {
      RELOC_TAB* rt2 = new RELOC_TAB[ reloc_tab_size + RELOC_GROW_SIZE ];
      memset( rt2, 0, sizeof( RELOC_TAB ) * ( reloc_tab_size + RELOC_GROW_SIZE ) );
      memcpy( rt2, reloc_tab, reloc_tab_size * sizeof( RELOC_TAB ) );
      reloc_tab_size += RELOC_GROW_SIZE;
      delete [] reloc_tab;
      reloc_tab = rt2;
   }
   reloc_tab[reloc_sym_index  ].rt_index = index;
   reloc_tab[reloc_sym_index  ].rt_scnum = sym->n_scnum;
   reloc_tab[reloc_sym_index++].rt_value = sym->n_value;
}

//
//
// RELOCATE() - Perform a single relocation by patching the raw data.
//
//
int CLOAD::relocate(
   RELOC         *rp,                   // relocation entry
   unsigned char *data,                 // data buffer
   int            s                     // index of current section
   )
{
   int fieldsz = reloc_size(rp->r_type);     // size of actual patch value
   int wordsz  = MAX(fieldsz, WORDSZ);       // size of containing field
   long objval;                              // field to be patched
   long reloc_amt;                           // amount of relocation

   //
   // look up the symbol being referenced in the relocation symbol table.
   // use the symbol value to calculate the relocation amount:
   // 1) if the sym index is -1 (internal relocation) use the relocation
   //    amount of the current section.
   // 2) if the symbol was undefined (defined in section 0), use the
   //    symbol's value.
   // 3) if the symbol has a positive section number, use the relocation
   //    amount for the section in which the symbol is defined.
   // 4) otherwise, the symbol is absolute, so the relocation amount is 0.
   //
   if( rp->r_symndx == -1) reloc_amt = reloc_amount[s];
   else
   {
      int rt_index = reloc_sym(rp->r_symndx);       // index in reloc table
      int sect_ref = reloc_tab[rt_index].rt_scnum;  // section where defined
      reloc_amt    = sect_ref == 0 ? reloc_tab[rt_index].rt_value
		   : sect_ref >  0 ? reloc_amount[sect_ref - 1]
				   : 0;
   }

   //
   // Extract the relocatable field from the object data.
   //
   objval = unpack(data, fieldsz, wordsz, BIT_OFFSET(rp->r_vaddr));

   //
   // Modify the field based on the relocation type.
   //
   switch (rp->r_type)
   {
      //
      // Normal relocations: add in the relocation amount.
      //
      case R_RELBYTE:
      case R_RELWORD:
      case R_REL24:
      case R_RELLONG:
      case R_PARTLS16:
	 objval += reloc_amt;
	 break;

      //
      // 34010 ONE'S COMPLEMENT RELOCATION.  Subtract instead of add.
      //
      case R_OCRLONG:
	 objval -= reloc_amt;
	 break;

      //
      // 34020 WORD-SWAPPED RELOCATION.  Swap before relocating.
      //
      case R_GSPOPR32:
          objval  = ((objval >> 16) & 0xFFFF) | (objval << 16);
          objval += reloc_amt;
          objval  = ((objval >> 16) & 0xFFFF) | (objval << 16);
          break;

      //
      // pc-relative relocations.  in this case the relocation amount
      // is adjusted by the pc difference.   if this is an internal
      // relocation to the current section, no adjustment is needed.
      //
      case R_PCRBYTE:
      case R_PCRWORD:
      case R_GSPPCR16:
      case R_PCRLONG:
	 if( rp->r_symndx != -1)                // If not internal relocation
	 {
            int shift  = 8 * (4 - fieldsz);
	    objval     = (long)(objval << shift) >> shift;   // Sign extend
	    reloc_amt -= reloc_amount[s];

            if( rp->r_type == R_GSPPCR16) reloc_amt >>= 4;   // Bits to words

	    objval += reloc_amt;
	 }
         break;

      //
      // 320C30 PAGE-ADDRESSING RELOCATION.  Calculate the address from
      // the 8-bit page value in the field, the 16-bit offset in the reloc
      // entry, and the relocation amount.  then, store the 8-bit page
      // value of the result back in the field.
      //
      case R_PARTMS8:
	  objval = ((objval << 16) + rp->r_disp + reloc_amt) >> 16;
	  break;

      //
      // DSP(320) PAGE-ADDRESSING.  Calculate address from the 16-bit
      // value in the relocation field plus the relocation amount.  or the
      // top 9 bits of this result into the relocation field.
      //
      case R_PARTMS9:
	  objval = (objval & 0xFE00) |
		   (((rp->r_disp + reloc_amt) >> 7) & 0x1FF);
          break;

      //
      // DSP(320) PAGE-ADDRESSING.  Calculate address as above, and or the
      // 7-bit displacement into the field.
      //
      case R_PARTLS7:
	  objval = (objval & 0x80) | ((rp->r_disp + reloc_amt) & 0x7F);
	  break;

      //
      // DSP(320) 13-BIT CONSTANT.  Relocate only the lower 13 bits of the
      // field.
      //
      case R_REL13:
          objval = (objval & 0xE000) | ((objval + reloc_amt) & 0x1FFF);
          break;

   }

   //
   // Pack the relocated field back into the object data.
   //
   repack(objval, data, fieldsz, wordsz, 0);

   return 0;
}

//
//
// RELOC_READ() - Read a single relocation entry.
//
//
int CLOAD::reloc_read(RELOC* rptr)
{

   if( fread(rptr, sizeof(RELOC), 1, fin) != 1) return false;

   if( byte_swapped)
   {
      swap4byte(&rptr->r_vaddr);  swap2byte(&rptr->r_symndx);
      swap2byte(&rptr->r_disp);   swap2byte(&rptr->r_type);
   }
   return true;
}


//
//
// RELOC_SIZE() - Return the field size in bytes of a relocation type.
//
//
int CLOAD::reloc_size(short type)
{
   switch (type)
   {
       case R_RELBYTE:
       case R_PCRBYTE:
       case R_PARTMS8:
       case R_PARTLS7:      return 1;

       case R_RELWORD:
       case R_PCRWORD:
       case R_GSPPCR16:
       case R_PARTLS16:
       case R_PARTMS9:
       case R_REL13:        return 2;

       case R_REL24:        return 3;

       case R_GSPOPR32:
       case R_RELLONG:
       case R_PCRLONG:
       case R_OCRLONG:      return 4;

       default:             return 0;
   }
}

//
//
// RELOC_SYM() - Search the relocation symbol table for a symbol with the
//               specified index.
//
//
int CLOAD::reloc_sym(int index)
{
   int i = 0,
       j = reloc_sym_index - 1;

   //
   // This is a simple binary search (the reloc table is always sorted).
   //
   while( i <= j )
   {
      int m = (i + j) / 2;
      if      (reloc_tab[m].rt_index < index) i = m + 1;
      else if( reloc_tab[m].rt_index > index) j = m - 1;
      else return m;
   }

   return -1;
}

//
//
//  UNPACK() - Extract a relocation field from object bytes and convert into
//             a long so it can be relocated.
//
//
unsigned long CLOAD::unpack(
   unsigned char *data,
   int fieldsz,
   int wordsz,
   int bit_addr                              // Bit address within byte
   )
{
   register int i;
   int r = bit_addr;                          // Right shift value
   int l = 8 - r;                             // Left shift value
   unsigned long objval = 0;

   if (!big_e_target)
   {
       //
       // Take the first 'fieldsz' bytes and convert
       //
       if( r == 0)
          for (i = fieldsz - 1; i >= 0; --i)
	     objval = (objval << 8) | data[i];
       else
       {
          for (i = fieldsz; i > 0; --i)
	     objval = (objval << 8) | (data[i] << l) | (data[i-1] >> r);

          data[0]       = (data[0]       << l) >> l;
          data[fieldsz] = (data[fieldsz] >> r) << r;
       }
   }
   else      // MSBFIRST
   {
       //
       // TAKE THE LAST 'fieldsz' BYTES.
       //
       if( r == 0)
          for (i = wordsz - fieldsz; i < wordsz; ++i)
	     objval = (objval << 8) | data[i];
       else
       {
          int firstbyte = wordsz - fieldsz;

          for (i = firstbyte; i < wordsz; ++i)
	     objval = (objval << 8) | (data[i] << r) | (data[i+1] >> l);

          data[firstbyte] = (data[firstbyte] >> l) << l;
          data[wordsz]    = (data[wordsz]    << r) >> r;
       }
   }
   return objval;
}

//
//
// REPACK() - Encode a binary relocated field back into the object data.
//
//
void CLOAD::repack(
   unsigned long objval,
   unsigned char *data,
   int fieldsz,
   int wordsz,
   int bit_addr
   )
{
   register int i;
   int r = bit_addr;                          // right shift value
   int l = 8 - r;                             // left shift value

   if (!big_e_target)
   {
       //
       // encode lsb of value first
       //
       if( r == 0)
          for (i = 0; i < fieldsz; objval >>= 8) data[i++] = objval;
       else
       {
          data[0] |= (objval << r);
          objval  >>= l;
          for (i = 1; i < fieldsz; objval >>= 8)  data[i++] = objval;
          data[fieldsz] |= objval;
       }
   }
   else   // msbfirst
   {
       //
       // E de msb of value first
       //
       if( r == 0)
          for (i = wordsz - 1; i >= wordsz - fieldsz; objval >>= 8)
	     data[i--] = objval;
       else
       {
          int firstbyte = wordsz - fieldsz;

          data[wordsz] |= (objval << l);
          objval >>= r;
          for (i = wordsz - 1; i > firstbyte; objval >>= 8) data[i--] = objval;
          data[firstbyte] |= objval;
       }
   }
}

// load_lineno() - Read in, swap, and relocate line number entries.  This
//                  function is not called directly by the loader, but by the
//                  application when it needs to read in line number entries.
//
bool CLOAD::load_lineno
(
   long    filptr,                     // where to read from
   int     count,                      // how many to read
   LINENO *lptr,                       // where to put them
   int     scnum                       // section number of these entries
   )
{
    int i;

    //
    // Read in the requested number of line number entries as a block.
    //
    if( fseek(fin, filptr, 0) == -1 ||
        fread(lptr, count, sizeof(LINENO), fin) != sizeof(LINENO)) return false;

    //
    // Swap and relocate each entry, as necessary.
    //
    if( byte_swapped || reloc_amount[scnum - 1])
       for (i = 0; i < count; i++)
       {
	  if( byte_swapped)
	  {
	      swap2byte(&lptr->l_lnno);
	      swap4byte(&lptr->l_paddr);
	  }

	  if( lptr->l_lnno != 0)
	     lptr->l_paddr += reloc_amount[scnum - 1];

	  lptr = (LINENO *) (((char *)lptr) + sizeof(LINENO));
       }

    return true;
}

void CLOAD::set_reloc_amount()
{
    for(int i=0;i<n_sections;++i)
        reloc_amount[i] = reloc_a;
}
