/* m- file for Mips machines.
   Copyright (C) 1987, 1992 Free Software Foundation, Inc.

This file is part of GNU Emacs.

GNU Emacs is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XEmacs; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/* Synched up with: FSF 19.31. */

/* The following line tells the configuration script what sort of 
   operating system this machine is likely to run.
   USUAL-OPSYS="note"

NOTE-START
Use m-mips4.h for RISCOS version 4; use s-bsd4-3.h with the BSD world.
Note that the proper m- file for the Decstation is m-pmax.h.
NOTE-END  */

/* Define WORD_MACHINE if addresses and such have
 * to be corrected before they can be used as byte counts.  */

#undef WORD_MACHINE

/* Define how to take a char and sign-extend into an int.
   On machines where char is signed, this is a no-op.  */

#define SIGN_EXTEND_CHAR(c) ((signed char)(c))

/* Now define a symbol for the cpu type, if your compiler
   does not define it automatically:
   Ones defined so far include vax, m68000, ns16000, pyramid,
   orion, tahoe, APOLLO and many others */
#ifndef mips
#	define mips
#endif

/* Define EXPLICIT_SIGN_EXTEND if XINT must explicitly sign-extend
   the bit field into an int.  In other words, if bit fields
   are always unsigned.

   If you use NO_UNION_TYPE, this flag does not matter.  */

#define EXPLICIT_SIGN_EXTEND

/* Data type of load average, as read out of kmem.  */

#define LOAD_AVE_TYPE long

/* Convert that into an integer that is 100 for a load average of 1.0  */

#define LOAD_AVE_CVT(x) (int) (((double) (x)) * 100.0 / 256.0)

/* CDC EP/IX 1.4.3 uses /unix */

#undef KERNEL_FILE
#define KERNEL_FILE "/unix"

/* Define CANNOT_DUMP on machines where unexec does not work.
   Then the function dump-emacs will not be defined
   and temacs will do (load "loadup") automatically unless told otherwise.  */

#undef CANNOT_DUMP

/* Define VIRT_ADDR_VARIES if the virtual addresses of
   pure and impure space as loaded can vary, and even their
   relative order cannot be relied on.

   Otherwise Emacs assumes that text space precedes data space,
   numerically.  */

/* #define VIRT_ADDR_VARIES */

/* Define C_ALLOCA if this machine does not support a true alloca
   and the one written in C should be used instead.
   Define HAVE_ALLOCA to say that the system provides a properly
   working alloca function and it should be used.
   Define neither one if an assembler-language alloca
   in the file alloca.s should be used.  */

#ifdef __GNUC__
#define HAVE_ALLOCA
#else
#define C_ALLOCA
#endif

/* Define NO_REMAP if memory segmentation makes it not work well
   to change the boundary between the text section and data section
   when Emacs is dumped.  If you define this, the preloaded Lisp
   code will not be sharable; but that's better than failing completely.  */

#define NO_REMAP

/* This machine requires completely different unexec code
   which lives in a separate file.  Specify the file name.  */

#define UNEXEC "unexmips.o"

/* Describe layout of the address space in an executing process.  */

#define TEXT_START 0x400000
#define DATA_START 0x800000

/* Alter some of the options used when linking.  */

#ifndef NEWSOS5
#ifdef BSD

/* DECstations don't have this library. */
/* #define LIBS_MACHINE "-lmld"  */

#define LD_SWITCH_MACHINE "-D 800000"
#define LIBS_DEBUG

#define LINKER "/bsd43/bin/ld"
  
#else /* not BSD */

#undef LIBS_MACHINE
/* #define LIBS_MACHINE "-lmld" */ /* mrb */
#define LD_SWITCH_MACHINE "-D 800000 -g3"
#define START_FILES "pre-crt0.o /usr/lib/crt1.o"
#define LIB_STANDARD "-lbsd -lc /usr/lib/crtn.o"
#define LIBS_TERMCAP "-lcurses"

#define C_SWITCH_MACHINE "-I/usr/include/bsd"
#define C_DEBUG_SWITCH "-O -g3"

#endif /* not BSD */
#endif /* not NEWSOS5 */

#ifndef NEWSOS5
#ifdef USG

/* Don't try to use SIGIO even though it is defined.  */
#define BROKEN_SIGIO

/* Describe special kernel features.  */

#if defined(emacs) && !defined(INHIBIT_BSD_TIME)
#include <bsd/sys/time.h>
#endif

/* The `select' in the system won't work for pipes, so don't use it.  */
#undef HAVE_SELECT /* override configuration decision */

#define HAVE_PTYS

/* ??? */
#define IRIS

#endif /* USG */

#ifdef BSD
#define COFF
#define TERMINFO
#undef MAIL_USE_FLOCK  /* Someone should check this.  */
#endif /* BSD */
#endif /* not NEWSOS5 */
