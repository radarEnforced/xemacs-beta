/* emodules.h - Declarations and definitions for XEmacs loadable modules.
(C) Copyright 1998, 1999 J. Kean Johnston. All rights reserved.

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

#ifndef EMODULES_HDR

#ifndef EMODULES_GATHER_VERSION
#define EMODULES_HDR
#endif

#define EMODULES_VERSION    "1.0.0"
#define EMODULES_MAJOR      1
#define EMODULES_MINOR      0
#define EMODULES_PATCH      0
#define EMODULES_REVISION   (long)((EMODULES_MAJOR * 1000) + \
                             (EMODULES_MINOR * 10) + \
                             (EMODULES_PATCH))

#ifndef EMODULES_GATHER_VERSION
#include <config.h>
#include "lisp.h"
#include "sysdep.h"
#include "window.h"
#include "buffer.h"
#include "insdel.h"
#include "frame.h"
#include "lstream.h"
#ifdef FILE_CODING
#include "file-coding.h"
#endif

/* Module loading technology version number */
extern Lisp_Object Vmodule_version;

/* Load path */
extern Lisp_Object Vmodule_load_path;

/* XEmacs version Information */
extern Lisp_Object Vemacs_major_version;
extern Lisp_Object Vemacs_minor_version;

/*
 * Load in a C module. The first argument is the name of the .so file, the
 * second is the name of the module, and the third is the module version.
 * If the module name is NULL, we will always reload the .so. If it is not
 * NULL, we check to make sure we haven't loaded it before. If the version
 * is specified, we check to make sure we didnt load the module of the
 * specified version before. We also use these as checks when we open the
 * module to make sure we have the right module.
 */
extern void emodules_load (CONST char *module, CONST char *name, CONST char *version);

/*
 * Because subrs and symbols added by a dynamic module are not part of
 * the make-docfile process, we need a clean way to get the variables
 * and functions documented. Since people dont like the idea of making
 * shared modules use different versions of DEFSUBR() and DEFVAR_LISP()
 * and friends, we need these two functions to insert the documentation
 * into the right place. These functions will be called by the module
 * init code, generated by ellcc during initialization mode.
 */
extern void emodules_doc_subr (CONST char *objname, CONST char *docstr);
extern void emodules_doc_sym (CONST char *objname, CONST char *docstr);

#define CDOCSUBR(Fname, DOC) emodules_doc_subr (Fname, DOC)
#define CDOCSYM(Sname, DOC)  emodules_doc_sym  (Sname, DOC)
#endif /* EMODULES_GATHER_VERSION */

#endif /* EMODULES_HDR */
