/* Synched up with: FSF 19.31. */

/* s/ file for bsd386 system.  */

#include "bsd4-3.h"

#ifndef __bsdi__
#define __bsdi__ 1
#endif

#define DECLARE_GETPWUID_WITH_UID_T

#define SIGNALS_VIA_CHARACTERS

#define PENDING_OUTPUT_COUNT(FILE) ((FILE)->_p - (FILE)->_bf._base)
#define A_TEXT_OFFSET(x)    (sizeof (struct exec))
#define A_TEXT_SEEK(hdr) (N_TXTOFF(hdr) + A_TEXT_OFFSET(hdr))

#define LIBS_DEBUG
#define LIB_X11_LIB -L/usr/X11/lib -lX11
#define LIBS_SYSTEM -lutil -lcompat

#define HAVE_GETLOADAVG

/* System uses OXTABS instead of the expected TAB3.
   (Copied from netbsd.h.)  */
#define TABDLY OXTABS
#define TAB3 OXTABS

#define NO_TERMIO

/* This silences a few compilation warnings.  */
#ifdef emacs
#undef BSD
#include <sys/param.h> /* To get BSD defined consistently.  */
#endif
  
#undef HAVE_UNION_WAIT

#define GETPGRP_NO_ARG 1
