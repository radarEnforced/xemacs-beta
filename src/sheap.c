/* Static Heap management routines for XEmacs.
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
02111-1307, USA.*/

#include "config.h"
#include <stdio.h>
#include <lisp.h>
#include <stddef.h>
#include "sheap-adjust.h"

#ifdef MULE
#define STATIC_HEAP_BASE	0x500000
#define STATIC_HEAP_SLOP	0x30000
#else
#define STATIC_HEAP_BASE	0x400000
#define STATIC_HEAP_SLOP	0x30000
#endif
#define STATIC_HEAP_SIZE \
(STATIC_HEAP_BASE + SHEAP_ADJUSTMENT + STATIC_HEAP_SLOP)
#define BLOCKSIZE	(1<<12)
#define ALLOC_UNIT (BLOCKSIZE-1)
#define ALLOC_MASK ~((unsigned long)(ALLOC_UNIT))
#define ALIGN_ALLOC(addr) ((((unsigned long)addr) + ALLOC_UNIT) & ALLOC_MASK)

char	static_heap_buffer[STATIC_HEAP_SIZE]={0};
char*	static_heap_base=static_heap_buffer;
char*	static_heap_ptr=static_heap_buffer;
unsigned long	static_heap_size=STATIC_HEAP_SIZE;
int 	static_heap_initialized=0;
int 	static_heap_dumped=0;

void* more_static_core ( ptrdiff_t increment )
{
  int size = (int) increment;
  void *result;
  
  if (!static_heap_initialized)
    {
      if (((unsigned long) static_heap_base & ~VALMASK) != 0)
	{
	  printf ("error: The heap was allocated in upper memory.\n");
	  exit (-1);
	}
      static_heap_base=(char*)ALIGN_ALLOC(static_heap_buffer);
      static_heap_ptr=static_heap_base;
      static_heap_size=STATIC_HEAP_SIZE - 
	(static_heap_base-static_heap_buffer);
#ifdef __CYGWIN32__
      sbrk(BLOCKSIZE);		/* force space for fork to work */
#endif
      static_heap_initialized=1;
    }
  
  result = static_heap_ptr;

  /* we don't need to align - handled by gmalloc.  */

  if (size < 0) 
    {
      if (static_heap_ptr + size < static_heap_base)
	{
	  return 0;
	}
    }
  else 
    {
      if (static_heap_ptr + size >= static_heap_base + static_heap_size)
	{
	  printf(

"\nRequested %d bytes, static heap exhausted!  base is %p, current ptr
is %p. You have exhausted the static heap, if you want to run temacs,
adjust sheap-adjust.h to 0 or a +ve number.  If you are dumping then
STATIC_HEAP_SLOP is too small.  Generally you should *not* try to run
temacs with a static heap, you should dump first.\n", size,
static_heap_base, static_heap_ptr);

	  exit(-1);
	  return 0;
	}
    }
  static_heap_ptr += size;
  
  return result;
}

void
sheap_adjust_h ()
{
  FILE *stream = fopen ("sheap-adjust.h", "w");

  if (stream == NULL)
    report_file_error ("Opening sheap adjustment file",
		       Fcons (build_string ("sheap-adjust.h"), Qnil));

  fprintf (stream,
	   "/*\tDo not edit this file!\n"
	   "\tAutomatically generated by XEmacs */\n"
	   "# define SHEAP_ADJUSTMENT (%d)\n",
	   ((static_heap_ptr - static_heap_buffer) - STATIC_HEAP_BASE));
  fclose (stream);
}

