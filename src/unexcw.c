/* unexec for GNU Emacs on Cygwin32.
   Copyright (C) 1994, 1998 Free Software Foundation, Inc.

This file is part of XEmacs.

XEmacs is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

XEmacs is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with XEmacs; see the file COPYING.  If not, write to the Free
Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.

*/

/* This is a complete rewrite, some code snarfed from unexnt.c and
   unexec.c, Andy Piper (andyp@parallax.co.uk) 13-1-98 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <config.h>
#include <string.h>
#include "sysfile.h"
#define PERROR(arg) perror(arg);exit(-1) 

#ifndef HAVE_A_OUT_H
unexec (char *, char *, void *, void *,	void *)
{
  PERROR("cannot unexec() a.out.h not installed");
}
#else

#undef CONST
#include <windows.h>
#include <a.out.h>

#define ALLOC_UNIT 0xFFFF
#define ALLOC_MASK ~((unsigned long)(ALLOC_UNIT))
#define ALIGN_ALLOC(addr) \
((((unsigned long)addr) + ALLOC_UNIT) & ALLOC_MASK)

/* To prevent zero-initialized variables from being placed into the bss
   section, use non-zero values to represent an uninitialized state.  */
#define UNINIT_PTR ((void *) 0xF0A0F0A0)
#define UNINIT_LONG (0xF0A0F0A0L)

static void get_section_info (int a_out, char* a_name);
static void copy_executable_and_dump_data_section (int a_out, int a_new);
static void dup_file_area(int a_out, int a_new, long size);
#if 0
static void write_int_to_bss(int a_out, int a_new, void* va, void* newval);
#endif

/* Cached info about the .data section in the executable.  */
void* data_start_va = UNINIT_PTR;
unsigned long  data_size = UNINIT_LONG;

/* Cached info about the .bss section in the executable.  */
void* bss_start = UNINIT_PTR;
unsigned long  bss_size = UNINIT_LONG;
FILHDR f_hdr;
PEAOUTHDR f_ohdr;
SCNHDR f_data, f_bss, f_text, f_idata;

#define PERROR(arg) perror(arg);exit(-1) 
#define CHECK_AOUT_POS(a) \
if (lseek(a_out, 0, SEEK_CUR) != a) \
{ \
  printf("we are at %lx, should be at %lx\n", \
	 lseek(a_out, 0, SEEK_CUR), a); \
  exit(-1); \
}

/* Dump out .data and .bss sections into a new executable.  */
void unexec (char *out_name, char *in_name, void *start_data, 
	     void * d1,	void * d2)
{
  /* ugly nt hack - should be in lisp */
  int a_new, a_out = -1;
  char new_name[MAX_PATH], a_name[MAX_PATH];
  char *ptr;
  
  /* Make sure that the input and output filenames have the
     ".exe" extension...patch them up if they don't.  */
  strcpy (a_name, in_name);
  ptr = a_name + strlen (a_name) - 4;
  if (strcmp (ptr, ".exe"))
    strcat (a_name, ".exe");

  strcpy (new_name, out_name);
  ptr = new_name + strlen (new_name) - 4;
  if (strcmp (ptr, ".exe"))
    strcat (new_name, ".exe");

  /* We need to round off our heap to NT's allocation unit (64KB).  */
  /* round_heap (get_allocation_unit ()); */

  if (a_name && (a_out = open (a_name, O_RDONLY | OPEN_BINARY)) < 0)
    {
      PERROR (a_name);
    }

  if ((a_new = open (new_name, O_WRONLY | O_TRUNC | O_CREAT | OPEN_BINARY,
		     CREAT_MODE)) < 0)
    {
      PERROR (new_name);
    }

  /* Get the interesting section info, like start and size of .bss...  */
  get_section_info (a_out, a_name);

  copy_executable_and_dump_data_section (a_out, a_new);

  close(a_out);
  close(a_new);
}

/* Flip through the executable and cache the info necessary for dumping.  */
static void get_section_info (int a_out, char* a_name)
{
  extern int my_ebss;
  /* From lastfile.c  */
  extern char my_edata[];

  if (read (a_out, &f_hdr, sizeof (f_hdr)) != sizeof (f_hdr))
    {
      PERROR (a_name);
    }

  if (f_hdr.e_magic != DOSMAGIC) 
    {
      PERROR("unknown exe header");
    }

  /* Check the NT header signature ...  */
  if (f_hdr.nt_signature != NT_SIGNATURE) 
    {
      PERROR("invalid nt header");
    }

  /* Flip through the sections for .data and .bss ...  */
  if (f_hdr.f_opthdr > 0)
    {
      if (read (a_out, &f_ohdr, AOUTSZ) != AOUTSZ)
	{
	  PERROR (a_name);
	}
    }
  /* Loop through .data & .bss section headers, copying them in */
  lseek (a_out, sizeof (f_hdr) + f_hdr.f_opthdr, 0);

  if (read (a_out, &f_text, sizeof (f_text)) != sizeof (f_text)
      &&
      strcmp (f_text.s_name, ".text"))
    {
      PERROR ("no .text section");
    }

  /* The .bss section.  */
  if (read (a_out, &f_bss, sizeof (f_bss)) != sizeof (f_bss)
      &&
      strcmp (f_bss.s_name, ".bss"))
    {
      PERROR ("no .bss section");
    }

  bss_start = (void *) ((char*)f_ohdr.ImageBase + f_bss.s_vaddr);
  bss_size = (unsigned long)((char*)&my_ebss-(char*)bss_start);
  
  /* must keep bss data that we want to be blank as blank */
  printf("found bss - keeping %lx of %lx bytes\n", bss_size, f_ohdr.bsize);

  /* The .data section.  */
  if (read (a_out, &f_data, sizeof (f_data)) != sizeof (f_data)
      &&
      strcmp (f_data.s_name, ".data"))
    {
      PERROR ("no .data section");
    }

  /* The .data section.  */
  data_start_va = (void *) ((char*)f_ohdr.ImageBase + f_data.s_vaddr);

  /* We want to only write Emacs data back to the executable,
     not any of the library data (if library data is included,
     then a dumped Emacs won't run on system versions other
     than the one Emacs was dumped on).  */
  data_size = (unsigned long)my_edata - (unsigned long)data_start_va;

  /* The .idata section.  */
  if (read (a_out, &f_idata, sizeof (f_idata)) != sizeof (f_idata)
      &&
      strcmp (f_idata.s_name, ".rdata"))
    {
      PERROR ("no .idata section");
    }
}

/* The dump routines.  */

static void
copy_executable_and_dump_data_section (int a_out, int a_new)
{
  long size=0;
  unsigned long new_data_size, new_bss_size, f_data_s_vaddr,
    file_sz_change, f_data_s_scnptr, bss_padding;
  int i;
  void* empty_space;
  extern int static_heap_dumped;
  SCNHDR section;
  /* calculate new sizes f_ohdr.dsize is the total initalized data
     size on disk which is f_data.s_size + f_idata.s_size. 
     f_ohdr.data_start is the base addres of all data and so should 
     not be changed. *.s_vaddr is the virtual address of the start
     of the section normalzed from f_ohdr.ImageBase. *.s_paddr
     appears to be the number of bytes in the section actually used
     (whereas *.s_size is aligned).

     bsize is now 0 since subsumed into .data
     dsize is dsize + (f_data.s_vaddr - f_bss.s_vaddr)
     f_data.s_vaddr is f_bss.s_vaddr
     f_data.s_size is new dsize maybe.
     what about s_paddr & s_scnptr?  */
  /* this is the amount the file increases in size */
  new_bss_size=f_data.s_vaddr - f_bss.s_vaddr;
  file_sz_change=new_bss_size;
  new_data_size=f_ohdr.dsize + new_bss_size;
  f_data_s_scnptr = f_data.s_scnptr;
  f_data_s_vaddr = f_data.s_vaddr;
  f_data.s_vaddr = f_bss.s_vaddr;
  f_data.s_paddr += new_bss_size;
#if 0 
  if (f_data.s_size + f_idata.s_size != f_ohdr.dsize)
    {
      printf("section size doesn't tally with dsize %lx != %lx\n", 
	     f_data.s_size + f_idata.s_size, f_ohdr.dsize);
    }
#endif
  f_data.s_size += new_bss_size;
  lseek (a_new, 0, SEEK_SET);
  /* write file header */
  f_hdr.f_symptr += file_sz_change;
  f_hdr.f_nscns--;
  printf("writing file header\n");
  if (write(a_new, &f_hdr, sizeof(f_hdr)) != sizeof(f_hdr))
    {
      PERROR("failed to write file header");
    }
  /* write optional header fixing dsize & bsize*/
  printf("writing optional header\n");
  printf("new data size is %lx, >= %lx\n", new_data_size,
	 f_ohdr.dsize + f_ohdr.bsize);
  if (new_data_size < f_ohdr.dsize + f_ohdr.bsize )
    {
      PERROR("new data size is < approx");
    }
  f_ohdr.dsize=new_data_size;
  f_ohdr.bsize=0;
  if (write(a_new, &f_ohdr, sizeof(f_ohdr)) != sizeof(f_ohdr))
    {
      PERROR("failed to write optional header");
    }
  /* write text as is */
  printf("writing text header (unchanged)\n");

  if (write(a_new, &f_text, sizeof(f_text)) != sizeof(f_text))
    {
      PERROR("failed to write text header");
    }

  /* write new data header */
  printf("writing .data header\n");

  if (write(a_new, &f_data, sizeof(f_data)) != sizeof(f_data))
    {
      PERROR("failed to write data header");
    }

  printf("writing .idata header\n");
  f_idata.s_scnptr += file_sz_change;
  if (f_idata.s_lnnoptr != 0) f_idata.s_lnnoptr += file_sz_change;
  if (f_idata.s_relptr != 0) f_idata.s_relptr += file_sz_change;
  if (write(a_new, &f_idata, sizeof(f_idata)) != sizeof(f_idata))
    {
      PERROR("failed to write idata header");
    }

  /* copy other section headers adjusting the file offset */
  for (i=0; i<(f_hdr.f_nscns-3); i++)
    {
      if (read (a_out, &section, sizeof (section)) != sizeof (section))
	{
	  PERROR ("no .data section");
	}
      
      section.s_scnptr += file_sz_change;
      if (section.s_lnnoptr != 0) section.s_lnnoptr += file_sz_change;
      if (section.s_relptr != 0) section.s_relptr += file_sz_change;

      if (write(a_new, &section, sizeof(section)) != sizeof(section))
	{
	  PERROR("failed to write data header");
	}
    }

  /* dump bss to maintain offsets */
  memset(&f_bss, 0, sizeof(f_bss));
  if (write(a_new, &f_bss, sizeof(f_bss)) != sizeof(f_bss))
    {
      PERROR("failed to write bss header");
    }
  
  size=lseek(a_new, 0, SEEK_CUR);
  CHECK_AOUT_POS(size);

  /* copy eveything else until start of data */
  size = f_data_s_scnptr - lseek (a_out, 0, SEEK_CUR);

  printf ("copying executable up to data section ... %lx bytes\n", 
	  size);
  dup_file_area(a_out, a_new, size);

  CHECK_AOUT_POS(f_data_s_scnptr);

  /* dump bss + padding between sections */
  printf ("dumping .bss into executable... %lx bytes\n", bss_size);
  if (write(a_new, bss_start, bss_size) != (int)bss_size)
    {
      PERROR("failed to write bss section");
    }

  /* pad, needs to be zero */
  bss_padding = new_bss_size - bss_size;
  printf ("padding .bss ... %lx bytes\n", bss_padding);
  empty_space = malloc(bss_padding);
  memset(empty_space, 0, bss_padding);
  if (write(a_new, empty_space, bss_padding) != (int)bss_padding)
    {
      PERROR("failed to write bss section");
    }
  free(empty_space);

  /* tell dumped version not to free pure heap */
  static_heap_dumped = 1;
  /* Get a pointer to the raw data in our address space.  */
  printf ("dumping .data section... %lx bytes\n", data_size);
  if (write(a_new, data_start_va, data_size) != (int)data_size)
    {
      PERROR("failed to write data section");
    }
  /* were going to use free again ... */
  static_heap_dumped = 0;
  
  size = lseek(a_out, f_data_s_scnptr + data_size, SEEK_SET);
  size = f_idata.s_scnptr - size;
  dup_file_area(a_out, a_new, size);

  //  lseek(a_out, f_idata.s_scnptr, SEEK_CUR);
  CHECK_AOUT_POS(f_idata.s_scnptr);
  /* now dump - idata don't need to do this cygwin ds is in .data! */
  printf ("dumping .idata section... %lx bytes\n", f_idata.s_size);

  dup_file_area(a_out,a_new,f_idata.s_size);

  /* write rest of file */
  printf ("writing rest of file\n");
  size = lseek(a_out, 0, SEEK_END);
  size = size - (f_idata.s_scnptr + f_idata.s_size); /* length remaining in a_out */
  lseek(a_out, f_idata.s_scnptr + f_idata.s_size, SEEK_SET);

  dup_file_area(a_out, a_new, size);
}

/*
 * copy from aout to anew
 */
static void dup_file_area(int a_out, int a_new, long size)
{
  char page[BUFSIZ];
  long n;
  for (; size > 0; size -= sizeof (page))
    {
      n = size > sizeof (page) ? sizeof (page) : size;
      if (read (a_out, page, n) != n || write (a_new, page, n) != n)
	{
	  PERROR ("dump_out()");
	}
    }
}

#if 0
static void write_int_to_bss(int a_out, int a_new, void* va, void* newval)
{
  int cpos;

  cpos = lseek(a_new, 0, SEEK_CUR);
  if (va < bss_start || va > bss_start + f_data.s_size)
    {
      PERROR("address not in data space\n");
    }
  lseek(a_new, f_data.s_scnptr + ((unsigned long)va - 
				  (unsigned long)bss_start), SEEK_SET);
  if (write(a_new, newval, sizeof(int)) != (int)sizeof(int))
    {
      PERROR("failed to write int value");
    }
  lseek(a_new, cpos, SEEK_SET);
}
#endif

#endif /* HAVE_A_OUT_H */