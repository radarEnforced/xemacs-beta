/* Copyright (C) 1985, 1986, 1987, 1988, 1990, 1992
   Free Software Foundation, Inc.

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
along with XEmacs; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Synched up with: FSF 19.31. */


/*
 * unexec.c - Convert a running program into an a.out file.
 *
 * Author:	Spencer W. Thomas
 * 		Computer Science Dept.
 * 		University of Utah
 * Date:	Tue Mar  2 1982
 * Modified heavily since then.
 *
 * Synopsis:
 *	unexec (new_name, a_name, data_start, bss_start, entry_address)
 *	char *new_name, *a_name;
 *	unsigned data_start, bss_start, entry_address;
 *
 * Takes a snapshot of the program and makes an a.out format file in the
 * file named by the string argument new_name.
 * If a_name is non-NULL, the symbol table will be taken from the given file.
 * On some machines, an existing a_name file is required.
 *
 * The boundaries within the a.out file may be adjusted with the data_start
 * and bss_start arguments.  Either or both may be given as 0 for defaults.
 *
 * Data_start gives the boundary between the text segment and the data
 * segment of the program.  The text segment can contain shared, read-only
 * program code and literal data, while the data segment is always unshared
 * and unprotected.  Data_start gives the lowest unprotected address.
 * The value you specify may be rounded down to a suitable boundary
 * as required by the machine you are using.
 *
 * Specifying zero for data_start means the boundary between text and data
 * should not be the same as when the program was loaded.
 * If NO_REMAP is defined, the argument data_start is ignored and the
 * segment boundaries are never changed.
 *
 * Bss_start indicates how much of the data segment is to be saved in the
 * a.out file and restored when the program is executed.  It gives the lowest
 * unsaved address, and is rounded up to a page boundary.  The default when 0
 * is given assumes that the entire data segment is to be stored, including
 * the previous data and bss as well as any additional storage allocated with
 * break (2).
 *
 * The new file is set up to start at entry_address.
 *
 * If you make improvements I'd like to get them too.
 * harpo!utah-cs!thomas, thomas@Utah-20
 *
 */

/* Even more heavily modified by james@bigtex.cactus.org of Dell Computer Co.
 * ELF support added.
 *
 * Basic theory: the data space of the running process needs to be
 * dumped to the output file.  Normally we would just enlarge the size
 * of .data, scooting everything down.  But we can't do that in ELF,
 * because there is often something between the .data space and the
 * .bss space.
 *
 * In the temacs dump below, notice that the Global Offset Table
 * (.got) and the Dynamic link data (.dynamic) come between .data1 and
 * .bss.  It does not work to overlap .data with these fields.
 *
 * The solution is to create a new .data segment.  This segment is
 * filled with data from the current process.  Since the contents of
 * various sections refer to sections by index, the new .data segment
 * is made the last in the table to avoid changing any existing index.

 * This is an example of how the section headers are changed.  "Addr"
 * is a process virtual address.  "Offset" is a file offset.

raid:/nfs/raid/src/dist-18.56/src> dump -h temacs

temacs:

           **** SECTION HEADER TABLE ****
[No]    Type    Flags   Addr         Offset       Size          Name
        Link    Info    Adralgn      Entsize

[1]     1       2       0x80480d4    0xd4         0x13          .interp
        0       0       0x1          0            

[2]     5       2       0x80480e8    0xe8         0x388         .hash
        3       0       0x4          0x4          

[3]     11      2       0x8048470    0x470        0x7f0         .dynsym
        4       1       0x4          0x10         

[4]     3       2       0x8048c60    0xc60        0x3ad         .dynstr
        0       0       0x1          0            

[5]     9       2       0x8049010    0x1010       0x338         .rel.plt
        3       7       0x4          0x8          

[6]     1       6       0x8049348    0x1348       0x3           .init
        0       0       0x4          0            

[7]     1       6       0x804934c    0x134c       0x680         .plt
        0       0       0x4          0x4          

[8]     1       6       0x80499cc    0x19cc       0x3c56f       .text
        0       0       0x4          0            

[9]     1       6       0x8085f3c    0x3df3c      0x3           .fini
        0       0       0x4          0            

[10]    1       2       0x8085f40    0x3df40      0x69c         .rodata
        0       0       0x4          0            

[11]    1       2       0x80865dc    0x3e5dc      0xd51         .rodata1
        0       0       0x4          0            

[12]    1       3       0x8088330    0x3f330      0x20afc       .data
        0       0       0x4          0            

[13]    1       3       0x80a8e2c    0x5fe2c      0x89d         .data1
        0       0       0x4          0            

[14]    1       3       0x80a96cc    0x606cc      0x1a8         .got
        0       0       0x4          0x4          

[15]    6       3       0x80a9874    0x60874      0x80          .dynamic
        4       0       0x4          0x8          

[16]    8       3       0x80a98f4    0x608f4      0x449c        .bss
        0       0       0x4          0            

[17]    2       0       0            0x608f4      0x9b90        .symtab
        18      371     0x4          0x10         

[18]    3       0       0            0x6a484      0x8526        .strtab
        0       0       0x1          0            

[19]    3       0       0            0x729aa      0x93          .shstrtab
        0       0       0x1          0            

[20]    1       0       0            0x72a3d      0x68b7        .comment
        0       0       0x1          0            

raid:/nfs/raid/src/dist-18.56/src> dump -h xemacs

xemacs:

           **** SECTION HEADER TABLE ****
[No]    Type    Flags   Addr         Offset       Size          Name
        Link    Info    Adralgn      Entsize

[1]     1       2       0x80480d4    0xd4         0x13          .interp
        0       0       0x1          0            

[2]     5       2       0x80480e8    0xe8         0x388         .hash
        3       0       0x4          0x4          

[3]     11      2       0x8048470    0x470        0x7f0         .dynsym
        4       1       0x4          0x10         

[4]     3       2       0x8048c60    0xc60        0x3ad         .dynstr
        0       0       0x1          0            

[5]     9       2       0x8049010    0x1010       0x338         .rel.plt
        3       7       0x4          0x8          

[6]     1       6       0x8049348    0x1348       0x3           .init
        0       0       0x4          0            

[7]     1       6       0x804934c    0x134c       0x680         .plt
        0       0       0x4          0x4          

[8]     1       6       0x80499cc    0x19cc       0x3c56f       .text
        0       0       0x4          0            

[9]     1       6       0x8085f3c    0x3df3c      0x3           .fini
        0       0       0x4          0            

[10]    1       2       0x8085f40    0x3df40      0x69c         .rodata
        0       0       0x4          0            

[11]    1       2       0x80865dc    0x3e5dc      0xd51         .rodata1
        0       0       0x4          0            

[12]    1       3       0x8088330    0x3f330      0x20afc       .data
        0       0       0x4          0            

[13]    1       3       0x80a8e2c    0x5fe2c      0x89d         .data1
        0       0       0x4          0            

[14]    1       3       0x80a96cc    0x606cc      0x1a8         .got
        0       0       0x4          0x4          

[15]    6       3       0x80a9874    0x60874      0x80          .dynamic
        4       0       0x4          0x8          

[16]    8       3       0x80c6800    0x7d800      0             .bss
        0       0       0x4          0            

[17]    2       0       0            0x7d800      0x9b90        .symtab
        18      371     0x4          0x10         

[18]    3       0       0            0x87390      0x8526        .strtab
        0       0       0x1          0            

[19]    3       0       0            0x8f8b6      0x93          .shstrtab
        0       0       0x1          0            

[20]    1       0       0            0x8f949      0x68b7        .comment
        0       0       0x1          0            

[21]    1       3       0x80a98f4    0x608f4      0x1cf0c       .data
        0       0       0x4          0            

 * This is an example of how the file header is changed.  "Shoff" is
 * the section header offset within the file.  Since that table is
 * after the new .data section, it is moved.  "Shnum" is the number of
 * sections, which we increment.
 *
 * "Phoff" is the file offset to the program header.  "Phentsize" and
 * "Shentsz" are the program and section header entries sizes respectively.
 * These can be larger than the apparent struct sizes.

raid:/nfs/raid/src/dist-18.56/src> dump -f temacs

temacs:

                    **** ELF HEADER ****
Class        Data       Type         Machine     Version
Entry        Phoff      Shoff        Flags       Ehsize
Phentsize    Phnum      Shentsz      Shnum       Shstrndx

1            1          2            3           1
0x80499cc    0x34       0x792f4      0           0x34
0x20         5          0x28         21          19

raid:/nfs/raid/src/dist-18.56/src> dump -f xemacs

xemacs:

                    **** ELF HEADER ****
Class        Data       Type         Machine     Version
Entry        Phoff      Shoff        Flags       Ehsize
Phentsize    Phnum      Shentsz      Shnum       Shstrndx

1            1          2            3           1
0x80499cc    0x34       0x96200      0           0x34
0x20         5          0x28         22          19

 * These are the program headers.  "Offset" is the file offset to the
 * segment.  "Vaddr" is the memory load address.  "Filesz" is the
 * segment size as it appears in the file, and "Memsz" is the size in
 * memory.  Below, the third segment is the code and the fourth is the
 * data: the difference between Filesz and Memsz is .bss

raid:/nfs/raid/src/dist-18.56/src> dump -o temacs

temacs:
 ***** PROGRAM EXECUTION HEADER *****
Type        Offset      Vaddr       Paddr
Filesz      Memsz       Flags       Align

6           0x34        0x8048034   0           
0xa0        0xa0        5           0           

3           0xd4        0           0           
0x13        0           4           0           

1           0x34        0x8048034   0           
0x3f2f9     0x3f2f9     5           0x1000      

1           0x3f330     0x8088330   0           
0x215c4     0x25a60     7           0x1000      

2           0x60874     0x80a9874   0           
0x80        0           7           0           

raid:/nfs/raid/src/dist-18.56/src> dump -o xemacs

xemacs:
 ***** PROGRAM EXECUTION HEADER *****
Type        Offset      Vaddr       Paddr
Filesz      Memsz       Flags       Align

6           0x34        0x8048034   0           
0xa0        0xa0        5           0           

3           0xd4        0           0           
0x13        0           4           0           

1           0x34        0x8048034   0           
0x3f2f9     0x3f2f9     5           0x1000      

1           0x3f330     0x8088330   0           
0x3e4d0     0x3e4d0     7           0x1000      

2           0x60874     0x80a9874   0           
0x80        0           7           0           


 */

/* Modified by wtien@urbana.mcd.mot.com of Motorola Inc. 
 * 
 * The above mechanism does not work if the unexeced ELF file is being
 * re-layout by other applications (such as `strip'). All the applications 
 * that re-layout the internal of ELF will layout all sections in ascending
 * order of their file offsets. After the re-layout, the data2 section will 
 * still be the LAST section in the section header vector, but its file offset 
 * is now being pushed far away down, and causes part of it not to be mapped
 * in (ie. not covered by the load segment entry in PHDR vector), therefore 
 * causes the new binary to fail.
 *
 * The solution is to modify the unexec algorithm to insert the new data2
 * section header right before the new bss section header, so their file
 * offsets will be in the ascending order. Since some of the section's (all 
 * sections AFTER the bss section) indexes are now changed, we also need to 
 * modify some fields to make them point to the right sections. This is done 
 * by macro PATCH_INDEX. All the fields that need to be patched are:
 * 
 * 1. ELF header e_shstrndx field.
 * 2. section header sh_link and sh_info field.
 * 3. symbol table entry st_shndx field.
 *
 * The above example now should look like:

           **** SECTION HEADER TABLE ****
[No]    Type    Flags   Addr         Offset       Size          Name
        Link    Info    Adralgn      Entsize

[1]     1       2       0x80480d4    0xd4         0x13          .interp
        0       0       0x1          0            

[2]     5       2       0x80480e8    0xe8         0x388         .hash
        3       0       0x4          0x4          

[3]     11      2       0x8048470    0x470        0x7f0         .dynsym
        4       1       0x4          0x10         

[4]     3       2       0x8048c60    0xc60        0x3ad         .dynstr
        0       0       0x1          0            

[5]     9       2       0x8049010    0x1010       0x338         .rel.plt
        3       7       0x4          0x8          

[6]     1       6       0x8049348    0x1348       0x3           .init
        0       0       0x4          0            

[7]     1       6       0x804934c    0x134c       0x680         .plt
        0       0       0x4          0x4          

[8]     1       6       0x80499cc    0x19cc       0x3c56f       .text
        0       0       0x4          0            

[9]     1       6       0x8085f3c    0x3df3c      0x3           .fini
        0       0       0x4          0            

[10]    1       2       0x8085f40    0x3df40      0x69c         .rodata
        0       0       0x4          0            

[11]    1       2       0x80865dc    0x3e5dc      0xd51         .rodata1
        0       0       0x4          0            

[12]    1       3       0x8088330    0x3f330      0x20afc       .data
        0       0       0x4          0            

[13]    1       3       0x80a8e2c    0x5fe2c      0x89d         .data1
        0       0       0x4          0            

[14]    1       3       0x80a96cc    0x606cc      0x1a8         .got
        0       0       0x4          0x4          

[15]    6       3       0x80a9874    0x60874      0x80          .dynamic
        4       0       0x4          0x8          

[16]    1       3       0x80a98f4    0x608f4      0x1cf0c       .data
        0       0       0x4          0            

[17]    8       3       0x80c6800    0x7d800      0             .bss
        0       0       0x4          0            

[18]    2       0       0            0x7d800      0x9b90        .symtab
        19      371     0x4          0x10         

[19]    3       0       0            0x87390      0x8526        .strtab
        0       0       0x1          0            

[20]    3       0       0            0x8f8b6      0x93          .shstrtab
        0       0       0x1          0            

[21]    1       0       0            0x8f949      0x68b7        .comment
        0       0       0x1          0            

 */

 /* More mods, by Jack Repenning <jackr@sgi.com>, Fri Aug 11 15:45:52 1995

     Same algorithm as immediately above.  However, the detailed
     calculations of the various locations needed significant
     overhaul.

     At the point of the old .bss, the file offsets and the memory
     addresses do distinct, slightly snaky things:

     offset of .bss is meaningless and unpredictable
     addr of .bss is meaningful
     alignment of .bss is important to addr, so there may be a small
     gap in address range before start of bss
     offset of next section is rounded up modulo 0x1000
     the hole so-introduced is zero-filled, so it can be mapped in as
     the first partial-page of bss (the rest of the bss is mapped from
     /dev/zero)
     I suppose you could view this not as a hole, but as the beginning
     of the bss, actually present in the file.  But you should not
     push that worldview too far, as the linker still knows that the
     "offset" claimed for the bss is unused, and seems not always
     careful about setting it.

     We are doing all our tricks at this same rather complicated
     location (isn't life fun?):

     insert a new data section to contain now-initialized old bss and
	heap 
     define a zero-length bss just so there is one

     The offset of the new data section is dictated by its current
     address (which, of course, we want also to be its addr): the
     loader maps in the whole file region containing old data, rodata,
     got, and new data as a single mapped segment, starting at the
     address of the first chunk; the rest have to be laid out in the
     file such that the map into the right spots.  That is:

			  offset(newdata) ==
	      addrInRunningMemory(newdata)-aIRM(olddata)  
			  + offset(oldData)

     This would not necessarily match the oldbss offset, even if it
     were carefully calculated!  We must compute this.

     The linker that built temacs has also already arranged that
     olddata is properly page-aligned (not necessarily beginning on a
     page, but rather that a page's worth of the low bits of addr and
     offset match).  We preserve this.

     addr(bss) is alignment-constrained from the end of the new data.
     Since we base endof(newdata) on sbrk(), we have a page boundary
     (in both offset and addr) and meet any alignment constraint,
     needing no alignment adjustment of this location and no
     mini-hole.  Or, if you like, we've allowed sbrk() to "compute"
     the mini-hole size for us.

     That puts newbss beginning on a page boundary, both in offset and
     addr.  (offset(bss) is still meaningless, but what the heck,
     we'll fix it up.)

     Since newbss has zero length, and its offset (however
     meaningless) is page aligned, we place the next section exactly
     there, with no hole needed to restore page alignment.

     So, the shift for all sections beyond the playing field is:

	     new_bss_addr - roundup(old_bss_addr,0x1000)

     */
  /* Still more mods... Olivier Galibert 19971705
     - support for .sbss section (automagically changed to data without
       name change)
     - support for 64bits ABI (will need a bunch of fixes in the rest
       of the code before it works
     */

#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <memory.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include <sym.h> /* for HDRR declaration */
#include <sys/mman.h>
#include <config.h>
#include "lisp.h"

/* in 64bits mode, use 64bits elf */
#ifdef _ABI64
typedef Elf64_Shdr l_Elf_Shdr;
typedef Elf64_Phdr l_Elf_Phdr;
typedef Elf64_Ehdr l_Elf_Ehdr;
typedef Elf64_Addr l_Elf_Addr;
typedef Elf64_Word l_Elf_Word;
typedef Elf64_Off  l_Elf_Off;
typedef Elf64_Sym  l_Elf_Sym;
#else
typedef Elf32_Shdr l_Elf_Shdr;
typedef Elf32_Phdr l_Elf_Phdr;
typedef Elf32_Ehdr l_Elf_Ehdr;
typedef Elf32_Addr l_Elf_Addr;
typedef Elf32_Word l_Elf_Word;
typedef Elf32_Off  l_Elf_Off;
typedef Elf32_Sym  l_Elf_Sym;
#endif


/* Get the address of a particular section or program header entry,
 * accounting for the size of the entries.
 */

#define OLD_SECTION_H(n) \
     (*(l_Elf_Shdr *) ((byte *) old_section_h + old_file_h->e_shentsize * (n)))
#define NEW_SECTION_H(n) \
     (*(l_Elf_Shdr *) ((byte *) new_section_h + new_file_h->e_shentsize * (n)))
#define OLD_PROGRAM_H(n) \
     (*(l_Elf_Phdr *) ((byte *) old_program_h + old_file_h->e_phentsize * (n)))
#define NEW_PROGRAM_H(n) \
     (*(l_Elf_Phdr *) ((byte *) new_program_h + new_file_h->e_phentsize * (n)))

#define PATCH_INDEX(n) \
  do { \
	 if ((n) >= old_bss_index) \
	   (n)++; } while (0)
typedef unsigned char byte;

/* Round X up to a multiple of Y.  */

static int
round_up (int x, int y)
{
  int rem = x % y;
  if (rem == 0)
    return x;
  return x - rem + y;
}

/* Return the index of the section named NAME.
   SECTION_NAMES, FILE_NAME and FILE_H give information
   about the file we are looking in.

   If we don't find the section NAME, that is a fatal error
   if NOERROR is 0; we return -1 if NOERROR is nonzero.  */

static int
find_section (char *name,
	      char *section_names,
	      char *file_name,
	      l_Elf_Ehdr *old_file_h,
	      l_Elf_Shdr *old_section_h,
	      int noerror)
{
  int idx;

  for (idx = 1; idx < old_file_h->e_shnum; idx++)
    {
#ifdef DEBUG
      fprintf (stderr, "Looking for %s - found %s\n", name,
	       section_names + OLD_SECTION_H (idx).sh_name);
#endif
      if (!strcmp (section_names + OLD_SECTION_H (idx).sh_name,
		   name))
	break;
    }
  if (idx == old_file_h->e_shnum)
    {
      if (noerror)
	return -1;
      else
	fatal ("Can't find .bss in %s.\n", file_name);
    }

  return idx;
}

/* ****************************************************************
 * unexec
 *
 * driving logic.
 *
 * In ELF, this works by replacing the old .bss section with a new
 * .data section, and inserting an empty .bss immediately afterwards.
 *
 */
int
unexec (char *new_name,
	char *old_name,
	uintptr_t data_start,
	uintptr_t bss_start,
	uintptr_t entry_address)
{
  extern uintptr_t bss_end;
  int new_file, old_file, new_file_size;

  /* Pointers to the base of the image of the two files.  */
  caddr_t old_base, new_base;

  /* Pointers to the file, program and section headers for the old and new
     files.  */
  l_Elf_Ehdr *old_file_h, *new_file_h;
  l_Elf_Phdr *old_program_h, *new_program_h;
  l_Elf_Shdr *old_section_h, *new_section_h;
  l_Elf_Shdr *oldbss;

  /* Point to the section name table in the old file.  */
  char *old_section_names;

  l_Elf_Addr old_bss_addr, new_bss_addr;
  l_Elf_Addr old_base_addr;
  l_Elf_Word old_bss_size, new_data2_size;
  l_Elf_Off  new_data2_offset, new_base_offset;
  l_Elf_Addr new_data2_addr;
  l_Elf_Addr new_offsets_shift;

  int n, nn, old_bss_index, old_data_index;
  int old_mdebug_index, old_sbss_index;
  struct stat stat_buf;

  /* Open the old file & map it into the address space.  */

  old_file = open (old_name, O_RDONLY);

  if (old_file < 0)
    fatal ("Can't open %s for reading: errno %d\n", old_name, errno);

  if (fstat (old_file, &stat_buf) == -1)
    fatal ("Can't fstat(%s): errno %d\n", old_name, errno);

  old_base = mmap (0, stat_buf.st_size, PROT_READ, MAP_SHARED, old_file, 0);

  if (old_base == (caddr_t) -1)
    fatal ("Can't mmap(%s): errno %d\n", old_name, errno);

#ifdef DEBUG
  fprintf (stderr, "mmap(%s, %x) -> %x\n", old_name, stat_buf.st_size,
	   old_base);
#endif

  /* Get pointers to headers & section names.  */

  old_file_h = (l_Elf_Ehdr *) old_base;
  old_program_h = (l_Elf_Phdr *) ((byte *) old_base + old_file_h->e_phoff);
  old_section_h = (l_Elf_Shdr *) ((byte *) old_base + old_file_h->e_shoff);
  old_section_names
    = (char *) old_base + OLD_SECTION_H (old_file_h->e_shstrndx).sh_offset;

  /* Find the mdebug section, if any.  */

  old_mdebug_index = find_section (".mdebug", old_section_names,
				   old_name, old_file_h, old_section_h, 1);

  /* Find the .sbss section, if any.  */

  old_sbss_index = find_section (".sbss", old_section_names,
				 old_name, old_file_h, old_section_h, 1);

  if (old_sbss_index != -1 && (OLD_SECTION_H (old_sbss_index).sh_type == SHT_PROGBITS))
    old_sbss_index = -1;

  /* Find the old .bss section. */

  old_bss_index = find_section (".bss", old_section_names,
				old_name, old_file_h, old_section_h, 0);

  /* Find the old .data section.  Figure out parameters of
     the new data2 and bss sections.  */

  old_data_index = find_section (".data", old_section_names,
				 old_name, old_file_h, old_section_h, 0);

  old_bss_addr	    = OLD_SECTION_H (old_bss_index).sh_addr;
  old_bss_size	    = OLD_SECTION_H (old_bss_index).sh_size;
  old_base_addr     = old_sbss_index == -1 ? old_bss_addr : OLD_SECTION_H (old_sbss_index).sh_addr;
#if defined(emacs) || !defined(DEBUG)
  bss_end	    = (uintptr_t) sbrk (0);
  new_bss_addr	    = (l_Elf_Addr) bss_end;
#else
  new_bss_addr	    = old_bss_addr + old_bss_size + 0x1234;
#endif
  new_data2_addr    = old_bss_addr;
  new_data2_size    = new_bss_addr - old_bss_addr;
  new_data2_offset  = OLD_SECTION_H (old_data_index).sh_offset +
    (new_data2_addr - OLD_SECTION_H (old_data_index).sh_addr);
  new_base_offset  = OLD_SECTION_H (old_data_index).sh_offset +
    (old_base_addr - OLD_SECTION_H (old_data_index).sh_addr);
  new_offsets_shift = new_bss_addr - (old_base_addr & ~0xfff) + 
    ((old_base_addr & 0xfff) ? 0x1000 : 0);

#ifdef DEBUG
  fprintf (stderr, "old_bss_index %d\n", old_bss_index);
  fprintf (stderr, "old_bss_addr %x\n", old_bss_addr);
  fprintf (stderr, "old_bss_size %x\n", old_bss_size);
  fprintf (stderr, "old_base_addr %x\n", old_base_addr);
  fprintf (stderr, "new_bss_addr %x\n", new_bss_addr);
  fprintf (stderr, "new_data2_addr %x\n", new_data2_addr);
  fprintf (stderr, "new_data2_size %x\n", new_data2_size);
  fprintf (stderr, "new_data2_offset %x\n", new_data2_offset);
  fprintf (stderr, "new_offsets_shift %x\n", new_offsets_shift);
#endif

  if ((unsigned) new_bss_addr < (unsigned) old_bss_addr + old_bss_size)
    fatal (".bss shrank when undumping???\n");

  /* Set the output file to the right size and mmap it.  Set
     pointers to various interesting objects.  stat_buf still has
     old_file data.  */

  new_file = open (new_name, O_RDWR | O_CREAT, 0666);
  if (new_file < 0)
    fatal ("Can't creat (%s): errno %d\n", new_name, errno);

  new_file_size = stat_buf.st_size /* old file size */
    + old_file_h->e_shentsize	   /* one new section header */
    + new_offsets_shift;	   /* trailing section shift */

  if (ftruncate (new_file, new_file_size))
    fatal ("Can't ftruncate (%s): errno %d\n", new_name, errno);

  new_base = mmap (0, new_file_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   new_file, 0);

  if (new_base == (caddr_t) -1)
    fatal ("Can't mmap (%s): errno %d\n", new_name, errno);

  new_file_h = (l_Elf_Ehdr *) new_base;
  new_program_h = (l_Elf_Phdr *) ((byte *) new_base + old_file_h->e_phoff);
  new_section_h
    = (l_Elf_Shdr *) ((byte *) new_base + old_file_h->e_shoff
		      + new_offsets_shift);

  /* Make our new file, program and section headers as copies of the
     originals.  */

  memcpy (new_file_h, old_file_h, old_file_h->e_ehsize);
  memcpy (new_program_h, old_program_h,
	  old_file_h->e_phnum * old_file_h->e_phentsize);

  /* Modify the e_shstrndx if necessary. */
  PATCH_INDEX (new_file_h->e_shstrndx);

  /* Fix up file header.  We'll add one section.  Section header is
     further away now.  */

  new_file_h->e_shoff += new_offsets_shift;
  new_file_h->e_shnum += 1;


#ifdef DEBUG
  fprintf (stderr, "Old section offset %x\n", old_file_h->e_shoff);
  fprintf (stderr, "Old section count %d\n", old_file_h->e_shnum);
  fprintf (stderr, "New section offset %x\n", new_file_h->e_shoff);
  fprintf (stderr, "New section count %d\n", new_file_h->e_shnum);
#endif

  /* Fix up a new program header.  Extend the writable data segment so
     that the bss area is covered too. Find that segment by looking
     for one that starts before and ends after the .bss and it PT_LOADable.
     Put a loop at the end to adjust the offset and address of any segment
     that is above data2, just in case we decide to allow this later.  */

  oldbss = &OLD_SECTION_H(old_bss_index);
  for (n = new_file_h->e_phnum - 1; n >= 0; n--)
    {
      /* Compute maximum of all requirements for alignment of section.  */
      l_Elf_Phdr * ph =  (l_Elf_Phdr *)((byte *) new_program_h + 
                                                 new_file_h->e_phentsize*(n));
#ifdef DEBUG
      printf ("%d @ %0x + %0x against %0x + %0x",
              n, ph->p_vaddr, ph->p_memsz,
              oldbss->sh_addr, oldbss->sh_size);
#endif
      if ((ph->p_type == PT_LOAD) && 
          (ph->p_vaddr <= oldbss->sh_addr) &&
          ((ph->p_vaddr + ph->p_memsz)>=(oldbss->sh_addr + oldbss->sh_size))) {
        ph->p_filesz += new_offsets_shift;
        ph->p_memsz = ph->p_filesz;
#ifdef DEBUG
        puts (" That's the one!");
        fflush (stdout);
#endif
        break;
      }
#ifdef DEBUG
      putchar ('\n');
      fflush (stdout);
#endif
    }
  if (n < 0)
    fatal ("Couldn't find segment next to %s in %s\n",
	   old_sbss_index == -1 ? ".sbss" : ".bss", old_name);


#if 1				/* Maybe allow section after data2 - does this ever happen?  */
  for (n = new_file_h->e_phnum - 1; n >= 0; n--)
    {
      if (NEW_PROGRAM_H (n).p_vaddr
	  && NEW_PROGRAM_H (n).p_vaddr >= new_data2_addr)
	NEW_PROGRAM_H (n).p_vaddr += new_offsets_shift - old_bss_size;

      if (NEW_PROGRAM_H (n).p_offset >= new_data2_offset)
	NEW_PROGRAM_H (n).p_offset += new_offsets_shift;
    }
#endif

  /* Fix up section headers based on new .data2 section.  Any section
     whose offset or virtual address is after the new .data2 section
     gets its value adjusted.  .bss size becomes zero and new address
     is set.  data2 section header gets added by copying the existing
     .data header and modifying the offset, address and size.  */
  for (old_data_index = 1; old_data_index < old_file_h->e_shnum;
       old_data_index++)
    if (!strcmp (old_section_names + OLD_SECTION_H (old_data_index).sh_name,
		 ".data"))
      break;
  if (old_data_index == old_file_h->e_shnum)
    fatal ("Can't find .data in %s.\n", old_name);

  /* Walk through all section headers, insert the new data2 section right 
     before the new bss section.  */
  for (n = 1, nn = 1; n < old_file_h->e_shnum; n++, nn++)
    {
      caddr_t src;

      /* XEmacs change: */
      if (n < old_bss_index)
	{
	  memcpy (&NEW_SECTION_H (nn), &OLD_SECTION_H (n), 
		  old_file_h->e_shentsize);
	  
	}
      else if (n == old_bss_index)
	{
	  
	  /* If it is bss section, insert the new data2 section before it.  */
	  /* Steal the data section header for this data2 section.  */
	  memcpy (&NEW_SECTION_H (nn), &OLD_SECTION_H (old_data_index),
		  new_file_h->e_shentsize);
	  
	  NEW_SECTION_H (nn).sh_addr = new_data2_addr;
	  NEW_SECTION_H (nn).sh_offset = new_data2_offset;
	  NEW_SECTION_H (nn).sh_size = new_data2_size;
	  /* Use the bss section's alignment. This will assure that the
	     new data2 section always be placed in the same spot as the old
	     bss section by any other application.  */
	  NEW_SECTION_H (nn).sh_addralign = OLD_SECTION_H (n).sh_addralign;

	  /* Now copy over what we have in the memory now.  */
	  memcpy (NEW_SECTION_H (nn).sh_offset + new_base, 
		  (caddr_t) OLD_SECTION_H (n).sh_addr, 
		  new_data2_size);
	  nn++;
	  memcpy (&NEW_SECTION_H (nn), &OLD_SECTION_H (n), 
		  old_file_h->e_shentsize);
      
	  /* The new bss section's size is zero, and its file offset and virtual
	     address should be off by NEW_OFFSETS_SHIFT.  */
	  NEW_SECTION_H (nn).sh_offset += new_offsets_shift;
	  NEW_SECTION_H (nn).sh_addr	= new_bss_addr;
	  /* Let the new bss section address alignment be the same as the
	     section address alignment followed the old bss section, so 
	     this section will be placed in exactly the same place.  */
	  NEW_SECTION_H (nn).sh_addralign = OLD_SECTION_H (n).sh_addralign;
	  NEW_SECTION_H (nn).sh_size = 0;
	}
      else			/* n > old_bss_index */
	memcpy (&NEW_SECTION_H (nn), &OLD_SECTION_H (n), 
		old_file_h->e_shentsize);
      
      /* Any section that was original placed AFTER the bss
	 section must now be adjusted by NEW_OFFSETS_SHIFT.  */

      if (NEW_SECTION_H (nn).sh_offset >= new_base_offset)
	NEW_SECTION_H (nn).sh_offset += new_offsets_shift;
      
      /* If any section hdr refers to the section after the new .data
	 section, make it refer to next one because we have inserted 
	 a new section in between.  */
      
      PATCH_INDEX (NEW_SECTION_H (nn).sh_link);
      /* For symbol tables, info is a symbol table index,
	 so don't change it.  */
      if (NEW_SECTION_H (nn).sh_type != SHT_SYMTAB
	  && NEW_SECTION_H (nn).sh_type != SHT_DYNSYM)
	PATCH_INDEX (NEW_SECTION_H (nn).sh_info);
      
      /* Fix the type and alignment for the .sbss section */
      if ((old_sbss_index != -1) && !strcmp (old_section_names + NEW_SECTION_H (nn).sh_name, ".sbss"))
	{
	  NEW_SECTION_H (nn).sh_type = SHT_PROGBITS;
	  NEW_SECTION_H (nn).sh_offset = round_up (NEW_SECTION_H (nn).sh_offset,
						   NEW_SECTION_H (nn).sh_addralign);
	}

      /* Now, start to copy the content of sections. */
      if (NEW_SECTION_H (nn).sh_type == SHT_NULL
	  || NEW_SECTION_H (nn).sh_type == SHT_NOBITS)
	continue;
      
      /* Write out the sections. .data, .data1 and .sbss (and data2, called
	 ".data" in the strings table) get copied from the current process
	 instead of the old file.  */
      if (!strcmp (old_section_names + NEW_SECTION_H (nn).sh_name, ".data")
	  || !strcmp (old_section_names + NEW_SECTION_H (nn).sh_name, ".data1")
	  || !strcmp (old_section_names + NEW_SECTION_H (nn).sh_name, ".got")
	  || !strcmp (old_section_names + NEW_SECTION_H (nn).sh_name, ".sbss"))
	src = (caddr_t) OLD_SECTION_H (n).sh_addr;
      else
	src = old_base + OLD_SECTION_H (n).sh_offset;
      
      memcpy (NEW_SECTION_H (nn).sh_offset + new_base, src,
	      NEW_SECTION_H (nn).sh_size);

      /* Adjust  the HDRR offsets in .mdebug and copy the 
	 line data if it's in its usual 'hole' in the object.
	 Makes the new file debuggable with dbx.
	 patches up two problems: the absolute file offsets
	 in the HDRR record of .mdebug (see /usr/include/syms.h), and
	 the ld bug that gets the line table in a hole in the
	 elf file rather than in the .mdebug section proper.
	 David Anderson. davea@sgi.com  Jan 16,1994.  */
      if (n == old_mdebug_index)
	{
#define MDEBUGADJUST(__ct,__fileaddr)		\
  if (n_phdrr->__ct > 0)			\
    {						\
      n_phdrr->__fileaddr += movement;		\
    }

	  HDRR * o_phdrr = (HDRR *)((byte *)old_base + OLD_SECTION_H (n).sh_offset);
	  HDRR * n_phdrr = (HDRR *)((byte *)new_base + NEW_SECTION_H (nn).sh_offset);
	  unsigned movement = new_offsets_shift;

	  MDEBUGADJUST (idnMax, cbDnOffset);
	  MDEBUGADJUST (ipdMax, cbPdOffset);
	  MDEBUGADJUST (isymMax, cbSymOffset);
	  MDEBUGADJUST (ioptMax, cbOptOffset);
	  MDEBUGADJUST (iauxMax, cbAuxOffset);
	  MDEBUGADJUST (issMax, cbSsOffset);
	  MDEBUGADJUST (issExtMax, cbSsExtOffset);
	  MDEBUGADJUST (ifdMax, cbFdOffset);
	  MDEBUGADJUST (crfd, cbRfdOffset);
	  MDEBUGADJUST (iextMax, cbExtOffset);
	  /* The Line Section, being possible off in a hole of the object,
	     requires special handling.  */
	  if (n_phdrr->cbLine > 0)
	    {
	      if (o_phdrr->cbLineOffset > (OLD_SECTION_H (n).sh_offset
					   + OLD_SECTION_H (n).sh_size))
		{
		  /* line data is in a hole in elf. do special copy and adjust
		     for this ld mistake.
		     */
		  n_phdrr->cbLineOffset += movement;

		  memcpy (n_phdrr->cbLineOffset + new_base,
			  o_phdrr->cbLineOffset + old_base, n_phdrr->cbLine);
		}
	      else
		{
		  /* somehow line data is in .mdebug as it is supposed to be.  */
		  MDEBUGADJUST (cbLine, cbLineOffset);
		}
	    }
	}

      /* If it is the symbol table, its st_shndx field needs to be patched. */
      if (NEW_SECTION_H (nn).sh_type == SHT_SYMTAB
	  || NEW_SECTION_H (nn).sh_type == SHT_DYNSYM)
	{
	  l_Elf_Shdr *spt = &NEW_SECTION_H (nn);
	  unsigned int num = spt->sh_size / spt->sh_entsize;
	  l_Elf_Sym * sym = (l_Elf_Sym *) (NEW_SECTION_H (nn).sh_offset
					   + new_base);
	  for (; num--; sym++)
	    {
	      if (sym->st_shndx == SHN_UNDEF
		  || sym->st_shndx == SHN_ABS
		  || sym->st_shndx == SHN_COMMON)
		continue;
	
	      PATCH_INDEX (sym->st_shndx);
	    }
	}
    }

  /* Close the files and make the new file executable.  */

  if (close (old_file))
    fatal ("Can't close (%s): errno %d\n", old_name, errno);

  if (close (new_file))
    fatal ("Can't close (%s): errno %d\n", new_name, errno);

  if (stat (new_name, &stat_buf) == -1)
    fatal ("Can't stat (%s): errno %d\n", new_name, errno);

  n = umask (777);
  umask (n);
  stat_buf.st_mode |= 0111 & ~n;
  if (chmod (new_name, stat_buf.st_mode) == -1)
    fatal ("Can't chmod (%s): errno %d\n", new_name, errno);

  return 0;
}
