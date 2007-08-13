/* File IO for XEmacs.
   Copyright (C) 1985-1988, 1992-1995 Free Software Foundation, Inc.
   Copyright (C) 1996 Ben Wing.

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

/* Synched up with: Mule 2.0, FSF 19.30. */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "events.h"
#include "frame.h"
#include "insdel.h"
#include "lstream.h"
#include "redisplay.h"
#include "sysdep.h"
#include "window.h"             /* minibuf_level */
#ifdef MULE
#include "mule-coding.h"
#endif

#ifdef HAVE_LIBGEN_H            /* Must come before sysfile.h */
#include <libgen.h>
#endif
#include "sysfile.h"
#include "sysproc.h"
#include "syspwd.h"
#include "systime.h"
#include "sysdir.h"

#ifdef HPUX
#include <netio.h>
#ifdef HPUX_PRE_8_0
#include <errnet.h>
#endif
#endif

/* Nonzero during writing of auto-save files */
static int auto_saving;

/* Set by auto_save_1 to mode of original file so Fwrite_region_internal
   will create a new file with the same mode as the original */
static int auto_save_mode_bits;

/* Alist of elements (REGEXP . HANDLER) for file names 
   whose I/O is done with a special handler.  */
Lisp_Object Vfile_name_handler_alist;

/* Format for auto-save files */
Lisp_Object Vauto_save_file_format;

/* Lisp functions for translating file formats */
Lisp_Object Qformat_decode, Qformat_annotate_function;

/* Functions to be called to process text properties in inserted file.  */
Lisp_Object Vafter_insert_file_functions;

/* Functions to be called to create text property annotations for file.  */
Lisp_Object Vwrite_region_annotate_functions;

/* During build_annotations, each time an annotation function is called,
   this holds the annotations made by the previous functions.  */
Lisp_Object Vwrite_region_annotations_so_far;

/* File name in which we write a list of all our auto save files.  */
Lisp_Object Vauto_save_list_file_name;

/* On VMS, nonzero means write new files with record format stmlf.
   Zero means use var format.  */
int vms_stmlf_recfm;

int disable_auto_save_when_buffer_shrinks;

Lisp_Object Qfile_name_handler_alist;

/* These variables describe handlers that have "already" had a chance
   to handle the current operation.

   Vinhibit_file_name_handlers is a list of file name handlers.
   Vinhibit_file_name_operation is the operation being handled.
   If we try to handle that operation, we ignore those handlers.  */

static Lisp_Object Vinhibit_file_name_handlers;
static Lisp_Object Vinhibit_file_name_operation;

Lisp_Object Qfile_error, Qfile_already_exists;

Lisp_Object Qauto_save_hook;
Lisp_Object Qauto_save_error;
Lisp_Object Qauto_saving;

Lisp_Object Qcar_less_than_car;

Lisp_Object Qcompute_buffer_file_truename;

/* signal a file error when errno contains a meaningful value. */

DOESNT_RETURN
report_file_error (CONST char *string, Lisp_Object data)
{
  /* mrb: #### Needs to be fixed at a lower level; errstring needs to
     be MULEized.  The following at least prevents a crash... */
  Lisp_Object errstring = build_ext_string (strerror (errno), FORMAT_BINARY);

  /* System error messages are capitalized.  Downcase the initial
     unless it is followed by a slash.  */
  if (string_char (XSTRING (errstring), 1) != '/')
    set_string_char (XSTRING (errstring), 0,
		     DOWNCASE (current_buffer,
			       string_char (XSTRING (errstring), 0)));

  signal_error (Qfile_error,
                Fcons (build_translated_string (string),
		       Fcons (errstring, data)));
}

void
maybe_report_file_error (CONST char *string, Lisp_Object data,
			 Lisp_Object class, Error_behavior errb)
{
  Lisp_Object errstring;

  /* Optimization: */
  if (ERRB_EQ (errb, ERROR_ME_NOT))
    return;

  errstring = build_string (strerror (errno));

  /* System error messages are capitalized.  Downcase the initial
     unless it is followed by a slash.  */
  if (string_char (XSTRING (errstring), 1) != '/')
    set_string_char (XSTRING (errstring), 0,
		     DOWNCASE (current_buffer,
			       string_char (XSTRING (errstring), 0)));

  maybe_signal_error (Qfile_error,
		      Fcons (build_translated_string (string),
			     Fcons (errstring, data)),
		      class, errb);
}

/* signal a file error when errno does not contain a meaningful value. */

DOESNT_RETURN
signal_file_error (CONST char *string, Lisp_Object data)
{
  signal_error (Qfile_error,
                list2 (build_translated_string (string), data));
}

void
maybe_signal_file_error (CONST char *string, Lisp_Object data,
			 Lisp_Object class, Error_behavior errb)
{
  /* Optimization: */
  if (ERRB_EQ (errb, ERROR_ME_NOT))
    return;
  maybe_signal_error (Qfile_error,
		      list2 (build_translated_string (string), data),
		      class, errb);
}

DOESNT_RETURN
signal_double_file_error (CONST char *string1, CONST char *string2,
			  Lisp_Object data)
{
  signal_error (Qfile_error,
                list3 (build_translated_string (string1),
		       build_translated_string (string2),
		       data));
}

void
maybe_signal_double_file_error (CONST char *string1, CONST char *string2,
				Lisp_Object data, Lisp_Object class,
				Error_behavior errb)
{
  /* Optimization: */
  if (ERRB_EQ (errb, ERROR_ME_NOT))
    return;
  maybe_signal_error (Qfile_error,
		      list3 (build_translated_string (string1),
			     build_translated_string (string2),
			     data),
		      class, errb);
}

DOESNT_RETURN
signal_double_file_error_2 (CONST char *string1, CONST char *string2,
			    Lisp_Object data1, Lisp_Object data2)
{
  signal_error (Qfile_error,
                list4 (build_translated_string (string1), 
		       build_translated_string (string2),
		       data1, data2));
}

void
maybe_signal_double_file_error_2 (CONST char *string1, CONST char *string2,
				  Lisp_Object data1, Lisp_Object data2,
				  Lisp_Object class, Error_behavior errb)
{
  /* Optimization: */
  if (ERRB_EQ (errb, ERROR_ME_NOT))
    return;
  maybe_signal_error (Qfile_error,
		      list4 (build_translated_string (string1), 
			     build_translated_string (string2),
			     data1, data2),
		      class, errb);
}

static Lisp_Object
close_file_unwind (Lisp_Object fd)
{
  if (CONSP (fd))
    {
      if (INTP (XCAR (fd)))
	close (XINT (XCAR (fd)));

      free_cons (XCONS (fd));
    }
  else
    close (XINT (fd));

  return Qnil;
}

static Lisp_Object
close_stream_unwind (Lisp_Object stream)
{
  Lstream_close (XLSTREAM (stream));
  return Qnil;
}

/* Restore point, having saved it as a marker.  */

static Lisp_Object
restore_point_unwind (Lisp_Object point_marker)
{
  BUF_SET_PT (current_buffer, marker_position (point_marker));
  return (Fset_marker (point_marker, Qnil, Qnil));
}

/* Versions of read() and write() that allow quitting out of the actual
   I/O.  We don't use immediate_quit (i.e. direct longjmp() out of the
   signal handler) because that's way too losing.

   (#### Actually, longjmp()ing out of the signal handler may not be
   as losing as I thought.  See sys_do_signal() in sysdep.c.)

   Solaris include files declare the return value as ssize_t.
   Is that standard? */
int
read_allowing_quit (int fildes, void *buf, unsigned int nbyte)
{
  int nread;
  QUIT;

  nread = sys_read_1 (fildes, buf, nbyte, 1);
  return nread;
}

int
write_allowing_quit (int fildes, CONST void *buf, unsigned int nbyte)
{
  int nread;

  QUIT;
  nread = sys_write_1 (fildes, buf, nbyte, 1);
  return nread;
}


Lisp_Object Qexpand_file_name;
Lisp_Object Qfile_truename;
Lisp_Object Qsubstitute_in_file_name;
Lisp_Object Qdirectory_file_name;
Lisp_Object Qfile_name_directory;
Lisp_Object Qfile_name_nondirectory;
Lisp_Object Qunhandled_file_name_directory;
Lisp_Object Qfile_name_as_directory;
Lisp_Object Qcopy_file;
Lisp_Object Qmake_directory_internal;
Lisp_Object Qdelete_directory;
Lisp_Object Qdelete_file;
Lisp_Object Qrename_file;
Lisp_Object Qadd_name_to_file;
Lisp_Object Qmake_symbolic_link;
Lisp_Object Qfile_exists_p;
Lisp_Object Qfile_executable_p;
Lisp_Object Qfile_readable_p;
Lisp_Object Qfile_symlink_p;
Lisp_Object Qfile_writable_p;
Lisp_Object Qfile_directory_p;
Lisp_Object Qfile_regular_p;
Lisp_Object Qfile_accessible_directory_p;
Lisp_Object Qfile_modes;
Lisp_Object Qset_file_modes;
Lisp_Object Qfile_newer_than_file_p;
Lisp_Object Qinsert_file_contents;
Lisp_Object Qwrite_region;
Lisp_Object Qverify_visited_file_modtime;
Lisp_Object Qset_visited_file_modtime;
Lisp_Object Qset_buffer_modtime;

/* If FILENAME is handled specially on account of its syntax,
   return its handler function.  Otherwise, return nil.  */

DEFUN ("find-file-name-handler", Ffind_file_name_handler, 1, 2, 0, /*
Return FILENAME's handler function for OPERATION, if it has one.
Otherwise, return nil.
A file name is handled if one of the regular expressions in
`file-name-handler-alist' matches it.

If OPERATION equals `inhibit-file-name-operation', then we ignore
any handlers that are members of `inhibit-file-name-handlers',
but we still do run any other handlers.  This lets handlers
use the standard functions without calling themselves recursively.
*/
       (filename, operation))
{
  /* This function must not munge the match data.  */
  Lisp_Object chain, inhibited_handlers;

  CHECK_STRING (filename);

  if (EQ (operation, Vinhibit_file_name_operation))
    inhibited_handlers = Vinhibit_file_name_handlers;
  else
    inhibited_handlers = Qnil;

  for (chain = Vfile_name_handler_alist; CONSP (chain);
       chain = XCDR (chain))
    {
      Lisp_Object elt = XCAR (chain);
      if (CONSP (elt))
	{
	  Lisp_Object string;
	  string = XCAR (elt);
	  if (STRINGP (string)
	      && (fast_lisp_string_match (string, filename) >= 0))
	    {
	      Lisp_Object handler = XCDR (elt);
	      if (NILP (Fmemq (handler, inhibited_handlers)))
		return (handler);
	    }
	}
      QUIT;
    }
  return Qnil;
}

static Lisp_Object
call2_check_string (Lisp_Object fn, Lisp_Object arg0, Lisp_Object arg1)
{
  /* This function can GC */
  Lisp_Object result = call2 (fn, arg0, arg1);
  CHECK_STRING (result);
  return (result);
}

static Lisp_Object
call3_check_string (Lisp_Object fn, Lisp_Object arg0, 
                    Lisp_Object arg1, Lisp_Object arg2)
{
  /* This function can GC */
  Lisp_Object result = call3 (fn, arg0, arg1, arg2);
  CHECK_STRING (result);
  return (result);
}


DEFUN ("file-name-directory", Ffile_name_directory, 1, 1, 0, /*
Return the directory component in file name NAME.
Return nil if NAME does not include a directory.
Otherwise return a directory spec.
Given a Unix syntax file name, returns a string ending in slash;
on VMS, perhaps instead a string ending in `:', `]' or `>'.
*/
       (file))
{
  /* This function can GC */
  Bufbyte *beg;
  Bufbyte *p;
  Lisp_Object handler;

  CHECK_STRING (file);

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (file, Qfile_name_directory);
  if (!NILP (handler))
    {
      Lisp_Object retval = call2 (handler, Qfile_name_directory,
				  file);

      if (!NILP (retval))
	CHECK_STRING (retval);
      return retval;
    }

#ifdef FILE_SYSTEM_CASE
  file = FILE_SYSTEM_CASE (file);
#endif
  beg = XSTRING_DATA (file);
  p = beg + XSTRING_LENGTH (file);

  while (p != beg && !IS_ANY_SEP (p[-1])
#ifdef VMS
	 && p[-1] != ':' && p[-1] != ']' && p[-1] != '>'
#endif /* VMS */
	 ) p--;

  if (p == beg)
    return Qnil;
#ifdef DOS_NT
  /* Expansion of "c:" to drive and default directory.  */
  /* (NT does the right thing.)  */
  if (p == beg + 2 && beg[1] == ':')
    {
      int drive = (*beg) - 'a';
      /* MAXPATHLEN+1 is guaranteed to be enough space for getdefdir.  */
      Bufbyte *res = (Bufbyte *) alloca (MAXPATHLEN + 5);
      unsigned char *res1;
#ifdef WINDOWSNT
      res1 = res;
      /* The NT version places the drive letter at the beginning already.  */
#else /* not WINDOWSNT */
      /* On MSDOG we must put the drive letter in by hand.  */
      res1 = res + 2;
#endif /* not WINDOWSNT */
      if (getdefdir (drive + 1, res)) 
	{
#ifdef MSDOS
	  res[0] = drive + 'a';
	  res[1] = ':';
#endif /* MSDOS */
	  if (IS_DIRECTORY_SEP (res[strlen ((char *) res) - 1]))
	    strcat ((char *) res, "/");
	  beg = res;
	  p = beg + strlen ((char *) beg);
	}
    }
#endif /* DOS_NT */
  return make_string (beg, p - beg);
}

DEFUN ("file-name-nondirectory", Ffile_name_nondirectory, 1, 1, 0, /*
Return file name NAME sans its directory.
For example, in a Unix-syntax file name,
this is everything after the last slash,
or the entire name if it contains no slash.
*/
       (file))
{
  /* This function can GC */
  Bufbyte *beg, *p, *end;
  Lisp_Object handler;

  CHECK_STRING (file);

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (file, Qfile_name_nondirectory);
  if (!NILP (handler))
    return (call2_check_string (handler, Qfile_name_nondirectory,
				file));

  beg = XSTRING_DATA (file);
  end = p = beg + XSTRING_LENGTH (file);

  while (p != beg && !IS_ANY_SEP (p[-1])
#ifdef VMS
	 && p[-1] != ':' && p[-1] != ']' && p[-1] != '>'
#endif /* VMS */
	 ) p--;

  return make_string (p, end - p);
}

DEFUN ("unhandled-file-name-directory", Funhandled_file_name_directory, 1, 1, 0, /*
Return a directly usable directory name somehow associated with FILENAME.
A `directly usable' directory name is one that may be used without the
intervention of any file handler.
If FILENAME is a directly usable file itself, return
(file-name-directory FILENAME).
The `call-process' and `start-process' functions use this function to
get a current directory to run processes in.
*/
  (filename))
{
  /* This function can GC */
  Lisp_Object handler;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (filename, Qunhandled_file_name_directory);
  if (!NILP (handler))
    return call2 (handler, Qunhandled_file_name_directory,
		  filename);

  return Ffile_name_directory (filename);
}


static char *
file_name_as_directory (char *out, char *in)
{
  int size = strlen (in) - 1;

  strcpy (out, in);

#ifdef VMS
  /* Is it already a directory string? */
  if (in[size] == ':' || in[size] == ']' || in[size] == '>')
    return out;
  /* Is it a VMS directory file name?  If so, hack VMS syntax.  */
  else if (! strchr (in, '/')
	   && ((size > 3 && ! strcmp (&in[size - 3], ".DIR"))
	       || (size > 3 && ! strcmp (&in[size - 3], ".dir"))
	       || (size > 5 && (! strncmp (&in[size - 5], ".DIR", 4)
				|| ! strncmp (&in[size - 5], ".dir", 4))
		   && (in[size - 1] == '.' || in[size - 1] == ';')
		   && in[size] == '1')))
    {
      char *p, *dot;
      char brack;

      /* x.dir -> [.x]
	 dir:x.dir --> dir:[x]
	 dir:[x]y.dir --> dir:[x.y] */
      p = in + size;
      while (p != in && *p != ':' && *p != '>' && *p != ']') p--;
      if (p != in)
	{
	  strncpy (out, in, p - in);
	  out[p - in] = '\0';
	  if (*p == ':')
	    {
	      brack = ']';
	      strcat (out, ":[");
	    }
	  else
	    {
	      brack = *p;
	      strcat (out, ".");
	    }
	  p++;
	}
      else
	{
	  brack = ']';
	  strcpy (out, "[.");
	}
      dot = strchr (p, '.');
      if (dot)
	{
	  /* blindly remove any extension */
	  size = strlen (out) + (dot - p);
	  strncat (out, p, dot - p);
	}
      else
	{
	  strcat (out, p);
	  size = strlen (out);
	}
      out[size++] = brack;
      out[size] = '\0';
    }
#else /* not VMS */
  /* For Unix syntax, Append a slash if necessary */
  if (!IS_ANY_SEP (out[size]))
    {
      out[size + 1] = DIRECTORY_SEP;
      out[size + 2] = '\0';
    }
#endif /* not VMS */
  return out;
}

DEFUN ("file-name-as-directory", Ffile_name_as_directory, 1, 1, 0, /*
Return a string representing file FILENAME interpreted as a directory.
This operation exists because a directory is also a file, but its name as
a directory is different from its name as a file.
The result can be used as the value of `default-directory'
or passed as second argument to `expand-file-name'.
For a Unix-syntax file name, just appends a slash.
On VMS, converts \"[X]FOO.DIR\" to \"[X.FOO]\", etc.
*/
       (file))
{
  /* This function can GC */
  char *buf;
  Lisp_Object handler;

  CHECK_STRING (file);

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (file, Qfile_name_as_directory);
  if (!NILP (handler))
    return (call2_check_string (handler, Qfile_name_as_directory,
				file));

  buf = (char *) alloca (XSTRING_LENGTH (file) + 10);
  return build_string (file_name_as_directory
		       (buf, (char *) XSTRING_DATA (file)));
}

/*
 * Convert from directory name to filename.
 * On VMS:
 *       xyzzy:[mukesh.emacs] => xyzzy:[mukesh]emacs.dir.1
 *       xyzzy:[mukesh] => xyzzy:[000000]mukesh.dir.1
 * On UNIX, it's simple: just make sure there is a terminating /

 * Value is nonzero if the string output is different from the input.
 */

static int
directory_file_name (CONST char *src, char *dst)
{
  long slen;
#ifdef VMS
  long rlen;
  char * ptr, * rptr;
  char bracket;
  struct FAB fab = cc$rms_fab;
  struct NAM nam = cc$rms_nam;
  char esa[NAM$C_MAXRSS];
#endif /* VMS */

  slen = strlen (src);
#ifdef VMS
  if (! strchr (src, '/')
      && (src[slen - 1] == ']'
	  || src[slen - 1] == ':'
	  || src[slen - 1] == '>'))
    {
      /* VMS style - convert [x.y.z] to [x.y]z, [x] to [000000]x */
      fab.fab$l_fna = src;
      fab.fab$b_fns = slen;
      fab.fab$l_nam = &nam;
      fab.fab$l_fop = FAB$M_NAM;

      nam.nam$l_esa = esa;
      nam.nam$b_ess = sizeof esa;
      nam.nam$b_nop |= NAM$M_SYNCHK;

      /* We call SYS$PARSE to handle such things as [--] for us. */
      if (SYS$PARSE(&fab, 0, 0) == RMS$_NORMAL)
	{
	  slen = nam.nam$b_esl;
	  if (esa[slen - 1] == ';' && esa[slen - 2] == '.')
	    slen -= 2;
	  esa[slen] = '\0';
	  src = esa;
	}
      if (src[slen - 1] != ']' && src[slen - 1] != '>')
	{
	  /* what about when we have logical_name:???? */
	  if (src[slen - 1] == ':')
	    {                   /* Xlate logical name and see what we get */
	      ptr = strcpy (dst, src); /* upper case for getenv */
	      while (*ptr)
		{
		  *ptr = toupper ((unsigned char) *ptr);
		  ptr++;
		}
	      dst[slen - 1] = 0;        /* remove colon */
	      if (!(src = egetenv (dst)))
		return 0;
	      /* should we jump to the beginning of this procedure?
		 Good points: allows us to use logical names that xlate
		 to Unix names,
		 Bad points: can be a problem if we just translated to a device
		 name...
		 For now, I'll punt and always expect VMS names, and hope for
		 the best! */
	      slen = strlen (src);
	      if (src[slen - 1] != ']' && src[slen - 1] != '>')
		{ /* no recursion here! */
		  strcpy (dst, src);
		  return 0;
		}
	    }
	  else
	    {		/* not a directory spec */
	      strcpy (dst, src);
	      return 0;
	    }
	}
      bracket = src[slen - 1];

      /* If bracket is ']' or '>', bracket - 2 is the corresponding
	 opening bracket.  */
      ptr = strchr (src, bracket - 2);
      if (ptr == 0)
	{ /* no opening bracket */
	  strcpy (dst, src);
	  return 0;
	}
      if (!(rptr = strrchr (src, '.')))
	rptr = ptr;
      slen = rptr - src;
      strncpy (dst, src, slen);
      dst[slen] = '\0';
      if (*rptr == '.')
	{
	  dst[slen++] = bracket;
	  dst[slen] = '\0';
	}
      else
	{
	  /* If we have the top-level of a rooted directory (i.e. xx:[000000]),
	     then translate the device and recurse. */
	  if (dst[slen - 1] == ':'
	      && dst[slen - 2] != ':'	/* skip decnet nodes */
	      && strcmp(src + slen, "[000000]") == 0)
	    {
	      dst[slen - 1] = '\0';
	      if ((ptr = egetenv (dst))
		  && (rlen = strlen (ptr) - 1) > 0
		  && (ptr[rlen] == ']' || ptr[rlen] == '>')
		  && ptr[rlen - 1] == '.')
		{
		  char * buf = (char *) alloca (strlen (ptr) + 1);
		  strcpy (buf, ptr);
		  buf[rlen - 1] = ']';
		  buf[rlen] = '\0';
		  return directory_file_name (buf, dst);
		}
	      else
		dst[slen - 1] = ':';
	    }
	  strcat (dst, "[000000]");
	  slen += 8;
	}
      rptr++;
      rlen = strlen (rptr) - 1;
      strncat (dst, rptr, rlen);
      dst[slen + rlen] = '\0';
      strcat (dst, ".DIR.1");
      return 1;
    }
#endif /* VMS */
  /* Process as Unix format: just remove any final slash.
     But leave "/" unchanged; do not change it to "".  */
  strcpy (dst, src);
#ifdef APOLLO
  /* Handle // as root for apollo's.  */
  if ((slen > 2 && dst[slen - 1] == '/')
      || (slen > 1 && dst[0] != '/' && dst[slen - 1] == '/'))
    dst[slen - 1] = 0;
#else
  if (slen > 1 
      && IS_DIRECTORY_SEP (dst[slen - 1])
#ifdef DOS_NT
      && !IS_ANY_SEP (dst[slen - 2])
#endif
      )
    dst[slen - 1] = 0;
#endif
  return 1;
}

DEFUN ("directory-file-name", Fdirectory_file_name, 1, 1, 0, /*
Return the file name of the directory named DIR.
This is the name of the file that holds the data for the directory DIR.
This operation exists because a directory is also a file, but its name as
a directory is different from its name as a file.
In Unix-syntax, this function just removes the final slash.
On VMS, given a VMS-syntax directory name such as \"[X.Y]\",
it returns a file name such as \"[X]Y.DIR.1\".
*/
       (directory))
{
  /* This function can GC */
  char *buf;
  Lisp_Object handler;

  CHECK_STRING (directory);

#if 0 /* #### WTF? */
  if (NILP (directory))
    return Qnil;
#endif

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (directory, Qdirectory_file_name);
  if (!NILP (handler))
    return (call2_check_string (handler, Qdirectory_file_name,
				directory));
#ifdef VMS
  /* 20 extra chars is insufficient for VMS, since we might perform a
     logical name translation. an equivalence string can be up to 255
     chars long, so grab that much extra space...  - sss */
  buf = (char *) alloca (XSTRING_LENGTH (directory) + 20 + 255);
#else
  buf = (char *) alloca (XSTRING_LENGTH (directory) + 20);
#endif
  directory_file_name ((char *) XSTRING_DATA (directory), buf);
  return build_string (buf);
}

DEFUN ("make-temp-name", Fmake_temp_name, 1, 1, 0, /*
Generate temporary file name (string) starting with PREFIX (a string).
The Emacs process number forms part of the result,
so there is no danger of generating a name being used by another process.
*/
       (prefix))
{
  CONST char suffix[] = "XXXXXX";
  Bufbyte *data;
  Bytecount len;
  Lisp_Object val;

  CHECK_STRING (prefix);
  len = XSTRING_LENGTH (prefix);
  val = make_uninit_string (len + countof (suffix) - 1);
  data = XSTRING_DATA (val);
  memcpy (data, XSTRING_DATA (prefix), len);
  memcpy (data + len, suffix, countof (suffix));
  /* !!#### does mktemp() Mule-encapsulate? */
  mktemp ((char *) data);

  return val;
}

DEFUN ("expand-file-name", Fexpand_file_name, 1, 2, 0, /*
Convert FILENAME to absolute, and canonicalize it.
Second arg DEFAULT is directory to start with if FILENAME is relative
 (does not start with slash); if DEFAULT is nil or missing,
the current buffer's value of default-directory is used.
Path components that are `.' are removed, and 
path components followed by `..' are removed, along with the `..' itself;
note that these simplifications are done without checking the resulting
paths in the file system.
An initial `~/' expands to your home directory.
An initial `~USER/' expands to USER's home directory.
See also the function `substitute-in-file-name'.
*/
       (name, defalt))
{
  /* This function can GC */
  Bufbyte *nm;
  
  Bufbyte *newdir, *p, *o;
  int tlen;
  Bufbyte *target;
  struct passwd *pw;
#ifdef VMS
  Bufbyte * colon = 0;
  Bufbyte * close = 0;
  Bufbyte * slash = 0;
  Bufbyte * brack = 0;
  int lbrack = 0, rbrack = 0;
  int dots = 0;
#endif /* VMS */
#ifdef DOS_NT
  /* Demacs 1.1.2 91/10/20 Manabu Higashida */
  int drive = -1;
  int relpath = 0;
  Bufbyte *tmp, *defdir;
#endif /* DOS_NT */
  Lisp_Object handler;
  
  CHECK_STRING (name);

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (name, Qexpand_file_name);
  if (!NILP (handler))
    return (call3_check_string (handler, Qexpand_file_name, name,
				defalt));

  /* Use the buffer's default-directory if DEFALT is omitted.  */
  if (NILP (defalt))
    defalt = current_buffer->directory;
  if (NILP (defalt))		/* this should be a meaningful error */
    {
      /* #### If we had a minibuffer-only frame up then current_buffer
	 is likely to not have a directory setting.  We should
	 probably redo things to make sure that current_buffer stays
	 set to something sensible. */
      if (!preparing_for_armageddon)
	signal_simple_error ("default-directory is not set",
			     make_buffer (current_buffer));
    }
  else
    CHECK_STRING (defalt);

  if (!NILP (defalt))
    {
      handler = Ffind_file_name_handler (defalt, Qexpand_file_name);
      if (!NILP (handler))
	return call3 (handler, Qexpand_file_name, name, defalt);
    }

  /* Make sure DEFALT is properly expanded.
     It would be better to do this down below where we actually use
     defalt.  Unfortunately, calling Fexpand_file_name recursively
     could invoke GC, and the strings might be relocated.  This would
     be annoying because we have pointers into strings lying around
     that would need adjusting, and people would add new pointers to
     the code and forget to adjust them, resulting in intermittent bugs.
     Putting this call here avoids all that crud.

     The EQ test avoids infinite recursion.  */
  if (! NILP (defalt) && !EQ (defalt, name)
      /* This saves time in a common case.  */
      && ! (XSTRING_LENGTH (defalt) >= 3
	    && IS_DIRECTORY_SEP (XSTRING_BYTE (defalt, 0))
	    && IS_DEVICE_SEP (XSTRING_BYTE (defalt, 1))))
    {
      struct gcpro gcpro1;

      GCPRO1 (name);
      defalt = Fexpand_file_name (defalt, Qnil);
      UNGCPRO;
    }

#ifdef VMS
  /* Filenames on VMS are always upper case.  */
  name = Fupcase (name, Fcurrent_buffer ());
#endif
#ifdef FILE_SYSTEM_CASE
  name = FILE_SYSTEM_CASE (name);
#endif

  nm = XSTRING_DATA (name);
  
#ifdef MSDOS
  /* First map all backslashes to slashes.  */
  dostounix_filename (nm = strcpy (alloca (strlen (nm) + 1), nm));
#endif

#ifdef DOS_NT
  /* Now strip drive name. */
  {
    Bufbyte *colon = strrchr (nm, ':');
    if (colon)
      if (nm == colon)
	nm++;
      else
	{
	  drive = colon[-1];
	  nm = colon + 1;
	  if (!IS_DIRECTORY_SEP (*nm))
	    {
	      defdir = alloca (MAXPATHLEN + 1);
	      relpath = getdefdir (tolower (drive) - 'a' + 1, defdir);
	    }
	}	
  }
#endif /* DOS_NT */

  /* Handle // and /~ in middle of file name
     by discarding everything through the first / of that sequence.  */
  p = nm;
  while (*p)
    {
      /* Since we know the path is absolute, we can assume that each
	 element starts with a "/".  */

      /* "//" anywhere isn't necessarily hairy; we just start afresh
	 with the second slash.  */
      if (IS_DIRECTORY_SEP (p[0]) && IS_DIRECTORY_SEP (p[1])
#if defined (APOLLO) || defined (WINDOWSNT)
	  /* // at start of filename is meaningful on Apollo 
	     and WindowsNT systems */
	  && nm != p
#endif /* APOLLO || WINDOWSNT */
	  )
	nm = p + 1;

      /* "~" is hairy as the start of any path element.  */
      if (IS_DIRECTORY_SEP (p[0]) && p[1] == '~')
	nm = p + 1;

      p++;
    }

  /* If nm is absolute, flush ...// and detect /./ and /../.
     If no /./ or /../ we can return right away. */
  if (
      IS_DIRECTORY_SEP (nm[0])
#ifdef VMS
      || strchr (nm, ':')
#endif /* VMS */
      )
    {
      /* If it turns out that the filename we want to return is just a
	 suffix of FILENAME, we don't need to go through and edit
	 things; we just need to construct a new string using data
	 starting at the middle of FILENAME.  If we set lose to a
	 non-zero value, that means we've discovered that we can't do
	 that cool trick.  */
      int lose = 0;

      p = nm;
      while (*p)
	{
	  /* Since we know the path is absolute, we can assume that each
	     element starts with a "/".  */

	  /* "." and ".." are hairy.  */
	  if (IS_DIRECTORY_SEP (p[0])
	      && p[1] == '.'
	      && (IS_DIRECTORY_SEP (p[2])
		  || p[2] == 0
		  || (p[2] == '.' && (IS_DIRECTORY_SEP (p[3])
				      || p[3] == 0))))
	    lose = 1;
#ifdef VMS
	  if (p[0] == '\\')
	    lose = 1;
	  if (p[0] == '/') {
	    /* if dev:[dir]/, move nm to / */
	    if (!slash && p > nm && (brack || colon)) {
	      nm = (brack ? brack + 1 : colon + 1);
	      lbrack = rbrack = 0;
	      brack = 0;
	      colon = 0;
	    }
	    slash = p;
	  }
	  if (p[0] == '-')
#ifndef VMS4_4
	    /* VMS pre V4.4,convert '-'s in filenames. */
	    if (lbrack == rbrack)
	      {
		if (dots < 2)   /* this is to allow negative version numbers */
		  p[0] = '_';
	      }
	    else
#endif /* VMS4_4 */
	      if (lbrack > rbrack &&
		  ((p[-1] == '.' || p[-1] == '[' || p[-1] == '<') &&
		   (p[1] == '.' || p[1] == ']' || p[1] == '>')))
		lose = 1;
#ifndef VMS4_4
	      else
		p[0] = '_';
#endif /* VMS4_4 */
	  /* count open brackets, reset close bracket pointer */
	  if (p[0] == '[' || p[0] == '<')
	    lbrack++, brack = 0;
	  /* count close brackets, set close bracket pointer */
	  if (p[0] == ']' || p[0] == '>')
	    rbrack++, brack = p;
	  /* detect ][ or >< */
	  if ((p[0] == ']' || p[0] == '>') && (p[1] == '[' || p[1] == '<'))
	    lose = 1;
	  if ((p[0] == ':' || p[0] == ']' || p[0] == '>') && p[1] == '~')
	    nm = p + 1, lose = 1;
	  if (p[0] == ':' && (colon || slash))
	    /* if dev1:[dir]dev2:, move nm to dev2: */
	    if (brack)
	      {
		nm = brack + 1;
		brack = 0;
	      }
	    /* if /pathname/dev:, move nm to dev: */
	    else if (slash)
	      nm = slash + 1;
	    /* if node::dev:, move colon following dev */
	    else if (colon && colon[-1] == ':')
	      colon = p;
	    /* if dev1:dev2:, move nm to dev2: */
	    else if (colon && colon[-1] != ':')
	      {
		nm = colon + 1;
		colon = 0;
	      }
	  if (p[0] == ':' && !colon)
	    {
	      if (p[1] == ':')
		p++;
	      colon = p;
	    }
	  if (lbrack == rbrack)
	    if (p[0] == ';')
	      dots = 2;
	    else if (p[0] == '.')
	      dots++;
#endif /* VMS */
	  p++;
	}
      if (!lose)
	{
#ifdef VMS
	  if (strchr (nm, '/'))
	    return build_string (sys_translate_unix (nm));
#endif /* VMS */
#ifndef DOS_NT
	  if (nm == XSTRING_DATA (name))
	    return name;
	  return build_string ((char *) nm);
#endif /* not DOS_NT */
	}
    }

  /* Now determine directory to start with and put it in newdir */

  newdir = 0;

  if (nm[0] == '~')		/* prefix ~ */
    {
      if (IS_DIRECTORY_SEP (nm[1])
#ifdef VMS
	  || nm[1] == ':'
#endif /* VMS */
	  || nm[1] == 0)		/* ~ by itself */
	{
	  if (!(newdir = (Bufbyte *) egetenv ("HOME")))
	    newdir = (Bufbyte *) "";
#ifdef DOS_NT
 	  /* Problem when expanding "~\" if HOME is not on current drive.
 	     Ulrich Leodolter, Wed Jan 11 10:20:35 1995 */
 	  if (newdir[1] == ':')
 	    drive = newdir[0];
	  dostounix_filename (newdir);
#endif
	  nm++;
#ifdef VMS
	  nm++;			/* Don't leave the slash in nm.  */
#endif /* VMS */
	}
      else			/* ~user/filename */
	{
	  for (p = nm; *p && (!IS_DIRECTORY_SEP (*p)
#ifdef VMS
			      && *p != ':'
#endif /* VMS */
			      ); p++);
	  o = (Bufbyte *) alloca (p - nm + 1);
	  memcpy (o, (char *) nm, p - nm);
	  o [p - nm] = 0;

#ifdef  WINDOWSNT
	  newdir = (unsigned char *) egetenv ("HOME");
	  dostounix_filename (newdir);
#else  /* not WINDOWSNT */
	  /* Jamie reports that getpwnam() can get wedged by SIGIO/SIGALARM
	     occurring in it. (It can call select()). */
	  slow_down_interrupts ();
	  pw = (struct passwd *) getpwnam ((char *) o + 1);
	  speed_up_interrupts ();
	  if (pw)
	    {
	      newdir = (Bufbyte *) pw -> pw_dir;
#ifdef VMS
	      nm = p + 1;		/* skip the terminator */
#else
	      nm = p;
#endif /* VMS */
	    }
#endif /* not WINDOWSNT */

	  /* If we don't find a user of that name, leave the name
	     unchanged; don't move nm forward to p.  */
	}
    }

  if (!IS_ANY_SEP (nm[0])
#ifdef VMS
      && !strchr (nm, ':')
#endif /* not VMS */
#ifdef DOS_NT
      && drive == -1
#endif /* DOS_NT */
      && !newdir
      && STRINGP (defalt))
    {
      newdir = XSTRING_DATA (defalt);
    }

#ifdef DOS_NT
  if (newdir == 0 && relpath)
    newdir = defdir; 
#endif /* DOS_NT */
  if (newdir != 0)
    {
      /* Get rid of any slash at the end of newdir.  */
      int length = strlen ((char *) newdir);
      /* Adding `length > 1 &&' makes ~ expand into / when homedir
	 is the root dir.  People disagree about whether that is right.
	 Anyway, we can't take the risk of this change now.  */
#ifdef DOS_NT
      if (newdir[1] != ':' && length > 1)
#endif
      if (IS_DIRECTORY_SEP (newdir[length - 1]))
	{
	  Bufbyte *temp = (Bufbyte *) alloca (length);
	  memcpy (temp, newdir, length - 1);
	  temp[length - 1] = 0;
	  newdir = temp;
	}
      tlen = length + 1;
    }
  else
    tlen = 0;

  /* Now concatenate the directory and name to new space in the stack frame */
  tlen += strlen ((char *) nm) + 1;
#ifdef DOS_NT
  /* Add reserved space for drive name.  (The Microsoft x86 compiler 
     produces incorrect code if the following two lines are combined.)  */
  target = (Bufbyte *) alloca (tlen + 2);
  target += 2;
#else  /* not DOS_NT */
  target = (Bufbyte *) alloca (tlen);
#endif /* not DOS_NT */
  *target = 0;

  if (newdir)
    {
#ifndef VMS
      if (nm[0] == 0 || IS_DIRECTORY_SEP (nm[0]))
	strcpy ((char *) target, (char *) newdir);
      else
#endif
      file_name_as_directory ((char *) target, (char *) newdir);
    }

  strcat ((char *) target, (char *) nm);
#ifdef VMS
  if (strchr (target, '/'))
    strcpy (target, sys_translate_unix (target));
#endif /* VMS */

  /* Now canonicalize by removing /. and /foo/.. if they appear.  */

  p = target;
  o = target;

  while (*p)
    {
#ifdef VMS
      if (*p != ']' && *p != '>' && *p != '-')
	{
	  if (*p == '\\')
	    p++;
	  *o++ = *p++;
	}
      else if ((p[0] == ']' || p[0] == '>') && p[0] == p[1] + 2)
	/* brackets are offset from each other by 2 */
	{
	  p += 2;
	  if (*p != '.' && *p != '-' && o[-1] != '.')
	    /* convert [foo][bar] to [bar] */
	    while (o[-1] != '[' && o[-1] != '<')
	      o--;
	  else if (*p == '-' && *o != '.')
	    *--p = '.';
	}
      else if (p[0] == '-' && o[-1] == '.' &&
	       (p[1] == '.' || p[1] == ']' || p[1] == '>'))
	/* flush .foo.- ; leave - if stopped by '[' or '<' */
	{
	  do
	    o--;
	  while (o[-1] != '.' && o[-1] != '[' && o[-1] != '<');
	  if (p[1] == '.')      /* foo.-.bar ==> bar.  */
	    p += 2;
	  else if (o[-1] == '.') /* '.foo.-]' ==> ']' */
	    p++, o--;
	  /* else [foo.-] ==> [-] */
	}
      else
	{
#ifndef VMS4_4
	  if (*p == '-' &&
	      o[-1] != '[' && o[-1] != '<' && o[-1] != '.' &&
	      p[1] != ']' && p[1] != '>' && p[1] != '.')
	    *p = '_';
#endif /* VMS4_4 */
	  *o++ = *p++;
	}
#else /* not VMS */
      if (!IS_DIRECTORY_SEP (*p))
	{
	  *o++ = *p++;
	}
      else if (IS_DIRECTORY_SEP (p[0]) && IS_DIRECTORY_SEP (p[1])
#if defined (APOLLO) || defined (WINDOWSNT)
	       /* // at start of filename is meaningful in Apollo 
		  and WindowsNT systems */
	       && o != target
#endif /* APOLLO */
	       )
	{
	  o = target;
	  p++;
	}
      else if (IS_DIRECTORY_SEP (p[0])
               && p[1] == '.'
	       && (IS_DIRECTORY_SEP (p[2])
		   || p[2] == 0))
	{
	  /* If "/." is the entire filename, keep the "/".  Otherwise,
	     just delete the whole "/.".  */
	  if (o == target && p[2] == '\0')
	    *o++ = *p;
	  p += 2;
	}
      else if (IS_DIRECTORY_SEP (p[0]) && p[1] == '.' && p[2] == '.'
	       /* `/../' is the "superroot" on certain file systems.  */
	       && o != target
	       && (IS_DIRECTORY_SEP (p[3]) || p[3] == 0))
	{
	  while (o != target && (--o) && !IS_DIRECTORY_SEP (*o))
	    ;
#if defined (APOLLO) || defined (WINDOWSNT)
	  if (o == target + 1 
	      && IS_DIRECTORY_SEP (o[-1]) && IS_DIRECTORY_SEP (o[0]))
	    ++o;
	  else
#endif /* APOLLO || WINDOWSNT */
	  if (o == target && IS_ANY_SEP (*o))
	    ++o;
	  p += 3;
	}
      else
	{
	  *o++ = *p++;
	}
#endif /* not VMS */
    }

#ifdef DOS_NT
  /* at last, set drive name. */
  if (target[1] != ':'
#ifdef WINDOWSNT
      /* Allow network paths that look like "\\foo" */
      && !(IS_DIRECTORY_SEP (target[0]) && IS_DIRECTORY_SEP (target[1]))
#endif /* WINDOWSNT */
      )
    {
      target -= 2;
      target[0] = (drive < 0 ? getdisk () + 'A' : drive);
      target[1] = ':';
    }
#endif /* DOS_NT */

  return make_string (target, o - target);
}

#if 0 /* FSFmacs */
another older version of expand-file-name;
#endif

/* not a full declaration because realpath() is typed differently
   on different systems */
extern char *realpath ();

DEFUN ("file-truename", Ffile_truename, 1, 2, 0, /*
Return the canonical name of the given FILE.
Second arg DEFAULT is directory to start with if FILE is relative
 (does not start with slash); if DEFAULT is nil or missing,
 the current buffer's value of default-directory is used.
No component of the resulting pathname will be a symbolic link, as
 in the realpath() function.
*/
       (filename, defalt))
{
  /* This function can GC */
  struct gcpro gcpro1;
  Lisp_Object expanded_name;
  Lisp_Object handler;

  CHECK_STRING (filename);

  GCPRO1 (filename);
  expanded_name = Fexpand_file_name (filename, defalt);
  UNGCPRO;

  if (!STRINGP (expanded_name))
    return Qnil;

  GCPRO1 (expanded_name);
  handler = Ffind_file_name_handler (expanded_name, Qfile_truename);
  UNGCPRO;

  if (!NILP (handler))
    return (call2_check_string (handler, Qfile_truename,
				expanded_name));

#ifdef VMS
  return (expanded_name);
#else
  {
    char resolved_path[MAXPATHLEN];
    char path[MAXPATHLEN];
    char *p = path;
    int elen = XSTRING_LENGTH (expanded_name);
    
    if (elen >= countof (path))
      goto toolong;
    
    memcpy (path, XSTRING_DATA (expanded_name), elen + 1);
    /* memset (resolved_path, 0, sizeof (resolved_path)); */

    /* Try doing it all at once. */
    /* !!#### Does realpath() Mule-encapsulate? */
    if (!realpath (path, resolved_path))
      {
	/* Didn't resolve it -- have to do it one component at a time. */
	/* "realpath" is a typically useless, stupid un*x piece of crap.
	   It claims to return a useful value in the "error" case, but since
	   there is no indication provided of how far along the pathname
	   the function went before erring, there is no way to use the
	   partial result returned.  What a piece of junk. */
	for (;;)
	  {
	    p = (char *) memchr (p + 1, '/', elen - (p + 1 - path));
	    if (p)
	      *p = 0;

	    /* memset (resolved_path, 0, sizeof (resolved_path)); */
	    if (realpath (path, resolved_path))
	      {
		if (p)
		  *p = '/';
		else
		  break;

	      }
	    else if (errno == ENOENT)
	      {
		/* Failed on this component.  Just tack on the rest of
		   the string and we are done. */
		int rlen = strlen (resolved_path);

		/* "On failure, it returns NULL, sets errno to indicate
		   the error, and places in resolved_path the absolute pathname
		   of the path component which could not be resolved." */
		if (p)
		  {
		    int plen = elen - (p - path);
            
		    if (rlen > 1 && resolved_path[rlen - 1] == '/')
		      rlen = rlen - 1;

		    if (plen + rlen + 1 > countof (resolved_path))
		      goto toolong;

		    resolved_path[rlen] = '/';
		    memcpy (resolved_path + rlen + 1, p + 1, plen + 1 - 1);
		  }
		break;
	      }
	    else
	      goto lose;
	  }
      }

    {
      int rlen = strlen (resolved_path);
      if (elen > 0 && XSTRING_BYTE (expanded_name, elen - 1) == '/'
          && !(rlen > 0 && resolved_path[rlen - 1] == '/'))
	{
	  if (rlen + 1 > countof (resolved_path))
	    goto toolong;
	  resolved_path[rlen] = '/';
	  resolved_path[rlen + 1] = 0;
	  rlen = rlen + 1;
	}
      return make_string ((Bufbyte *) resolved_path, rlen);
    }

  toolong:
    errno = ENAMETOOLONG;
    goto lose;
  lose:
    report_file_error ("Finding truename", list1 (expanded_name));
  }
  return Qnil;	/* suppress compiler warning */
#endif /* not VMS */
}


DEFUN ("substitute-in-file-name", Fsubstitute_in_file_name, 1, 1, 0, /*
Substitute environment variables referred to in FILENAME.
`$FOO' where FOO is an environment variable name means to substitute
the value of that variable.  The variable name should be terminated
with a character not a letter, digit or underscore; otherwise, enclose
the entire variable name in braces.
If `/~' appears, all of FILENAME through that `/' is discarded.

On VMS, `$' substitution is not done; this function does little and only
duplicates what `expand-file-name' does.
*/
       (string))
{
  Bufbyte *nm;

  Bufbyte *s, *p, *o, *x, *endp;
  Bufbyte *target = 0;
  int total = 0;
  int substituted = 0;
  Bufbyte *xnm;
  Lisp_Object handler;

  CHECK_STRING (string);

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (string, Qsubstitute_in_file_name);
  if (!NILP (handler))
    {
      Lisp_Object retval = call2 (handler, Qsubstitute_in_file_name,
				  string);

      if (!NILP (retval))
	CHECK_STRING (retval);
      return retval;
    }

  nm = XSTRING_DATA (string);
#ifdef MSDOS
  dostounix_filename (nm = strcpy (alloca (strlen (nm) + 1), nm));
  substituted = !strcmp (nm, XSTRING_DATA (string));
#endif
  endp = nm + XSTRING_LENGTH (string);

  /* If /~ or // appears, discard everything through first slash. */

  for (p = nm; p != endp; p++)
    {
      if ((p[0] == '~' ||
#ifdef APOLLO
	   /* // at start of file name is meaningful in Apollo system */
	   (p[0] == '/' && p - 1 != nm)
#else /* not APOLLO */
#ifdef WINDOWSNT
	   (IS_DIRECTORY_SEP (p[0]) && p - 1 != nm)
#else /* not WINDOWSNT */
	   p[0] == '/'
#endif /* not WINDOWSNT */
#endif /* not APOLLO */
	   )
	  && p != nm
	  && (0
#ifdef VMS
	      || p[-1] == ':' || p[-1] == ']' || p[-1] == '>'
#endif /* VMS */
	      || IS_DIRECTORY_SEP (p[-1])))
	{
	  nm = p;
	  substituted = 1;
	}
#ifdef DOS_NT
      if (p[0] && p[1] == ':')
	{
	  nm = p;
	  substituted = 1;
	}
#endif /* DOS_NT */
    }

#ifdef VMS
  return build_string (nm);
#else

  /* See if any variables are substituted into the string
     and find the total length of their values in `total' */

  for (p = nm; p != endp;)
    if (*p != '$')
      p++;
    else
      {
	p++;
	if (p == endp)
	  goto badsubst;
	else if (*p == '$')
	  {
	    /* "$$" means a single "$" */
	    p++;
	    total -= 1;
	    substituted = 1;
	    continue;
	  }
	else if (*p == '{')
	  {
	    o = ++p;
	    while (p != endp && *p != '}') p++;
	    if (*p != '}') goto missingclose;
	    s = p;
	  }
	else
	  {
	    o = p;
	    while (p != endp && (isalnum (*p) || *p == '_')) p++;
	    s = p;
	  }

	/* Copy out the variable name */
	target = (Bufbyte *) alloca (s - o + 1);
	strncpy ((char *) target, (char *) o, s - o);
	target[s - o] = 0;
#ifdef DOS_NT
	strupr (target); /* $home == $HOME etc.  */
#endif /* DOS_NT */

	/* Get variable value */
	o = (Bufbyte *) egetenv ((char *) target);
	if (!o) goto badvar;
	total += strlen ((char *) o);
	substituted = 1;
      }

  if (!substituted)
    return string;

  /* If substitution required, recopy the string and do it */
  /* Make space in stack frame for the new copy */
  xnm = (Bufbyte *) alloca (XSTRING_LENGTH (string) + total + 1);
  x = xnm;

  /* Copy the rest of the name through, replacing $ constructs with values */
  for (p = nm; *p;)
    if (*p != '$')
      *x++ = *p++;
    else
      {
	p++;
	if (p == endp)
	  goto badsubst;
	else if (*p == '$')
	  {
	    *x++ = *p++;
	    continue;
	  }
	else if (*p == '{')
	  {
	    o = ++p;
	    while (p != endp && *p != '}') p++;
	    if (*p != '}') goto missingclose;
	    s = p++;
	  }
	else
	  {
	    o = p;
	    while (p != endp && (isalnum (*p) || *p == '_')) p++;
	    s = p;
	  }

	/* Copy out the variable name */
	target = (Bufbyte *) alloca (s - o + 1);
	strncpy ((char *) target, (char *) o, s - o);
	target[s - o] = 0;
#ifdef DOS_NT
	strupr (target); /* $home == $HOME etc.  */
#endif /* DOS_NT */

	/* Get variable value */
	o = (Bufbyte *) egetenv ((char *) target);
	if (!o)
	  goto badvar;

	strcpy ((char *) x, (char *) o);
	x += strlen ((char *) o);
      }

  *x = 0;

  /* If /~ or // appears, discard everything through first slash. */

  for (p = xnm; p != x; p++)
    if ((p[0] == '~'
#ifdef APOLLO
	 /* // at start of file name is meaningful in Apollo system */
	 || (p[0] == '/' && p - 1 != xnm)
#else /* not APOLLO */
#ifdef WINDOWSNT
	 || (IS_DIRECTORY_SEP (p[0]) && p - 1 != xnm)
#else /* not WINDOWSNT */
	 || p[0] == '/'
#endif /* not WINDOWSNT */
#endif /* not APOLLO */
	 )
	/* don't do p[-1] if that would go off the beginning --jwz */
	&& p != nm && p > xnm && IS_DIRECTORY_SEP (p[-1]))
      xnm = p;
#ifdef DOS_NT
    else if (p[0] && p[1] == ':')
	xnm = p;
#endif

  return make_string (xnm, x - xnm);

 badsubst:
  error ("Bad format environment-variable substitution");
 missingclose:
  error ("Missing \"}\" in environment-variable substitution");
 badvar:
  error ("Substituting nonexistent environment variable \"%s\"",
	 target);

  /* NOTREACHED */
  return Qnil;	/* suppress compiler warning */
#endif /* not VMS */
}

/* (directory-file-name (expand-file-name FOO)) */

Lisp_Object
expand_and_dir_to_file (Lisp_Object filename, Lisp_Object defdir)
{
  /* This function can GC */
  Lisp_Object abspath;
  struct gcpro gcpro1;

  GCPRO1 (filename);
  abspath = Fexpand_file_name (filename, defdir);
#ifdef VMS
  {
    Bufbyte c =
      XSTRING_BYTE (abspath, XSTRING_LENGTH (abspath) - 1);
    if (c == ':' || c == ']' || c == '>')
      abspath = Fdirectory_file_name (abspath);
  }
#else
  /* Remove final slash, if any (unless path is root).
     stat behaves differently depending!  */
  if (XSTRING_LENGTH (abspath) > 1
      && IS_DIRECTORY_SEP (XSTRING_BYTE (abspath, XSTRING_LENGTH (abspath) - 1))
      && !IS_DEVICE_SEP (XSTRING_BYTE (abspath, XSTRING_LENGTH (abspath) - 2)))
    /* We cannot take shortcuts; they might be wrong for magic file names.  */
    abspath = Fdirectory_file_name (abspath);
#endif
  UNGCPRO;
  return abspath;
}

/* Signal an error if the file ABSNAME already exists.
   If INTERACTIVE is nonzero, ask the user whether to proceed,
   and bypass the error if the user says to go ahead.
   QUERYSTRING is a name for the action that is being considered
   to alter the file.
   *STATPTR is used to store the stat information if the file exists.
   If the file does not exist, STATPTR->st_mode is set to 0.  */

static void
barf_or_query_if_file_exists (Lisp_Object absname, CONST char *querystring,
			      int interactive, struct stat *statptr)
{
  struct stat statbuf;

  /* stat is a good way to tell whether the file exists,
     regardless of what access permissions it has.  */
  if (stat ((char *) XSTRING_DATA (absname), &statbuf) >= 0)
    {
      Lisp_Object tem;
      struct gcpro gcpro1;

      GCPRO1 (absname);
      if (interactive)
        tem = call1
	  (Qyes_or_no_p,
	   (emacs_doprnt_string_c
	    ((CONST Bufbyte *) GETTEXT ("File %s already exists; %s anyway? "),
	     Qnil, -1, XSTRING_DATA (absname),
	     GETTEXT (querystring))));
      else
        tem = Qnil;
      UNGCPRO;
      if (NILP (tem))
	Fsignal (Qfile_already_exists,
		 list2 (build_translated_string ("File already exists"),
			absname));
      if (statptr)
	*statptr = statbuf;
    }
  else
    {
      if (statptr)
	statptr->st_mode = 0;
    }
  return;
}

DEFUN ("copy-file", Fcopy_file, 2, 4,
       "fCopy file: \nFCopy %s to file: \np\nP", /*
Copy FILE to NEWNAME.  Both args must be strings.
Signals a `file-already-exists' error if file NEWNAME already exists,
unless a third argument OK-IF-ALREADY-EXISTS is supplied and non-nil.
A number as third arg means request confirmation if NEWNAME already exists.
This is what happens in interactive use with M-x.
Fourth arg KEEP-TIME non-nil means give the new file the same
last-modified time as the old one.  (This works on only some systems.)
A prefix arg makes KEEP-TIME non-nil.
*/
       (filename, newname, ok_if_already_exists, keep_time))
{
  /* This function can GC */
  int ifd, ofd, n;
  char buf[16 * 1024];
  struct stat st, out_st;
  Lisp_Object handler;
  int speccount = specpdl_depth ();
  struct gcpro gcpro1, gcpro2;
  /* Lisp_Object args[6]; */
  int input_file_statable_p;

  GCPRO2 (filename, newname);
  CHECK_STRING (filename);
  CHECK_STRING (newname);
  filename = Fexpand_file_name (filename, Qnil);
  newname = Fexpand_file_name (newname, Qnil);

  /* If the input file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (filename, Qcopy_file);
  /* Likewise for output file name.  */
  if (NILP (handler))
    handler = Ffind_file_name_handler (newname, Qcopy_file);
  if (!NILP (handler))
  {
    UNGCPRO;
    return call5 (handler, Qcopy_file, filename, newname,
		  ok_if_already_exists, keep_time);
  }

  /* When second argument is a directory, copy the file into it.
     (copy-file "foo" "bar/") == (copy-file "foo" "bar/foo")
   */
  if (!NILP (Ffile_directory_p (newname)))
    {
      Lisp_Object args[3];
      struct gcpro ngcpro1;
      int i = 1;

      args[0] = newname; 
      args[1] = Qnil; args[2] = Qnil;
      NGCPRO1 (*args); 
      ngcpro1.nvars = 3;
      if (XSTRING_BYTE (newname, XSTRING_LENGTH (newname) - 1) != '/')
	args[i++] = build_string ("/");
      args[i++] = Ffile_name_nondirectory (filename);
      newname = Fconcat (i, args);
      NUNGCPRO;
    }

  if (NILP (ok_if_already_exists)
      || INTP (ok_if_already_exists))
    barf_or_query_if_file_exists (newname, "copy to it",
				  INTP (ok_if_already_exists), &out_st);
  else if (stat ((CONST char *) XSTRING_DATA (newname), &out_st) < 0)
    out_st.st_mode = 0;

  ifd = open ((char *) XSTRING_DATA (filename), O_RDONLY, 0);
  if (ifd < 0)
    report_file_error ("Opening input file", Fcons (filename, Qnil));

  record_unwind_protect (close_file_unwind, make_int (ifd));

  /* We can only copy regular files and symbolic links.  Other files are not
     copyable by us. */
  input_file_statable_p = (fstat (ifd, &st) >= 0);

#ifndef DOS_NT
  if (out_st.st_mode != 0
      && st.st_dev == out_st.st_dev && st.st_ino == out_st.st_ino)
    {
      errno = 0;
      report_file_error ("Input and output files are the same",
			 Fcons (filename, Fcons (newname, Qnil)));
    }
#endif

#if defined (S_ISREG) && defined (S_ISLNK)
  if (input_file_statable_p)
    {
      if (!(S_ISREG (st.st_mode))
	  /* XEmacs: have to allow S_ISCHR in order to copy /dev/null */
#ifdef S_ISCHR
	  && !(S_ISCHR (st.st_mode))
#endif
	  && !(S_ISLNK (st.st_mode)))
	{
#if defined (EISDIR)
	  /* Get a better looking error message. */
	  errno = EISDIR;
#endif /* EISDIR */
	report_file_error ("Non-regular file", Fcons (filename, Qnil));
	}
    }
#endif /* S_ISREG && S_ISLNK */

#ifdef VMS
  /* Create the copy file with the same record format as the input file */
  ofd = sys_creat ((char *) XSTRING_DATA (newname), 0666, ifd);
#else
#ifdef MSDOS
  /* System's default file type was set to binary by _fmode in emacs.c.  */
  ofd = creat ((char *) XSTRING_DATA (newname), S_IREAD | S_IWRITE);
#else /* not MSDOS */
  ofd = creat ((char *) XSTRING_DATA (newname), 0666);
#endif /* not MSDOS */
#endif /* VMS */
  if (ofd < 0)
    report_file_error ("Opening output file", list1 (newname));

  {
    Lisp_Object ofd_locative = noseeum_cons (make_int (ofd), Qnil);

    record_unwind_protect (close_file_unwind, ofd_locative);

    while ((n = read_allowing_quit (ifd, buf, sizeof (buf))) > 0)
    {
      if (write_allowing_quit (ofd, buf, n) != n)
	report_file_error ("I/O error", list1 (newname));
    }

    /* Closing the output clobbers the file times on some systems.  */
    if (close (ofd) < 0)
      report_file_error ("I/O error", Fcons (newname, Qnil));

    if (input_file_statable_p)
    {
      if (!NILP (keep_time))
      {
        EMACS_TIME atime, mtime;
        EMACS_SET_SECS_USECS (atime, st.st_atime, 0);
        EMACS_SET_SECS_USECS (mtime, st.st_mtime, 0);
        if (set_file_times ((char *) XSTRING_DATA (newname), atime,
			    mtime))
	  report_file_error ("I/O error", Fcons (newname, Qnil));
      }
#ifndef MSDOS
      chmod ((CONST char *) XSTRING_DATA (newname),
	     st.st_mode & 07777);
#else /* MSDOS */
#if defined (__DJGPP__) && __DJGPP__ > 1
      /* In DJGPP v2.0 and later, fstat usually returns true file mode bits,
         and if it can't, it tells so.  Otherwise, under MSDOS we usually
         get only the READ bit, which will make the copied file read-only,
         so it's better not to chmod at all.  */
      if ((_djstat_flags & _STFAIL_WRITEBIT) == 0)
	chmod ((char *) XSTRING_DATA (newname), st.st_mode & 07777);
#endif /* DJGPP version 2 or newer */
#endif /* MSDOS */
    }

    /* We'll close it by hand */
    XCAR (ofd_locative) = Qnil;

    /* Close ifd */
    unbind_to (speccount, Qnil);
  }

  UNGCPRO;
  return Qnil;
}

DEFUN ("make-directory-internal", Fmake_directory_internal, 1, 1, 0, /*
Create a directory.  One argument, a file name string.
*/
       (dirname))
{
  /* This function can GC */
  char dir [MAXPATHLEN];
  Lisp_Object handler;

  struct gcpro gcpro1;
  
  GCPRO1 (dirname);
  CHECK_STRING (dirname);
  dirname = Fexpand_file_name (dirname, Qnil);

  handler = Ffind_file_name_handler (dirname, Qmake_directory_internal);
  UNGCPRO;
  if (!NILP (handler))
    return (call2 (handler, Qmake_directory_internal,
		   dirname));
 
  if (XSTRING_LENGTH (dirname) > (sizeof (dir) - 1))
    {
      return Fsignal (Qfile_error,
		      list3 (build_translated_string ("Creating directory"),
			     build_translated_string ("pathame too long"),
			     dirname));
    }
  strncpy (dir, (char *) XSTRING_DATA (dirname),
	   XSTRING_LENGTH (dirname) + 1);

#ifndef VMS
  if (dir [XSTRING_LENGTH (dirname) - 1] == '/')
    dir [XSTRING_LENGTH (dirname) - 1] = 0;
#endif

#ifdef WINDOWSNT
  if (mkdir (dir) != 0)
#else
  if (mkdir (dir, 0777) != 0)
#endif
    report_file_error ("Creating directory", list1 (dirname));

  return Qnil;
}

DEFUN ("delete-directory", Fdelete_directory, 1, 1, "FDelete directory: ", /*
Delete a directory.  One argument, a file name or directory name string.
*/
       (dirname))
{
  /* This function can GC */
  Lisp_Object handler;
  struct gcpro gcpro1;
  
  GCPRO1 (dirname);
  CHECK_STRING (dirname);
  dirname =
    Fdirectory_file_name (Fexpand_file_name (dirname, Qnil));

  handler = Ffind_file_name_handler (dirname, Qdelete_directory);
  UNGCPRO;
  if (!NILP (handler))
    return (call2 (handler, Qdelete_directory, dirname));

  if (rmdir ((char *) XSTRING_DATA (dirname)) != 0)
    report_file_error ("Removing directory", list1 (dirname));

  return Qnil;
}

DEFUN ("delete-file", Fdelete_file, 1, 1, "fDelete file: ", /*
Delete specified file.  One argument, a file name string.
If file has multiple names, it continues to exist with the other names.
*/
       (filename))
{
  /* This function can GC */
  Lisp_Object handler;
  struct gcpro gcpro1;
  
  GCPRO1 (filename);
  CHECK_STRING (filename);
  filename = Fexpand_file_name (filename, Qnil);

  handler = Ffind_file_name_handler (filename, Qdelete_file);
  UNGCPRO;
  if (!NILP (handler))
    return call2 (handler, Qdelete_file, filename);

  if (0 > unlink ((char *) XSTRING_DATA (filename)))
    report_file_error ("Removing old name", list1 (filename));
  return Qnil;
}

static Lisp_Object
internal_delete_file_1 (Lisp_Object ignore, Lisp_Object ignore2)
{
  return Qt;
}

/* Delete file FILENAME, returning 1 if successful and 0 if failed.  */

int
internal_delete_file (Lisp_Object filename)
{
  return NILP (condition_case_1 (Qt, Fdelete_file, filename,
				 internal_delete_file_1, Qnil));
}

DEFUN ("rename-file", Frename_file, 2, 3,
       "fRename file: \nFRename %s to file: \np", /*
Rename FILE as NEWNAME.  Both args strings.
If file has names other than FILE, it continues to have those names.
Signals a `file-already-exists' error if a file NEWNAME already exists
unless optional third argument OK-IF-ALREADY-EXISTS is non-nil.
A number as third arg means request confirmation if NEWNAME already exists.
This is what happens in interactive use with M-x.
*/
       (filename, newname, ok_if_already_exists))
{
  /* This function can GC */
  Lisp_Object handler;
  struct gcpro gcpro1, gcpro2;

  GCPRO2 (filename, newname);
  CHECK_STRING (filename);
  CHECK_STRING (newname);
  filename = Fexpand_file_name (filename, Qnil);
  newname = Fexpand_file_name (newname, Qnil);

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (filename, Qrename_file);
  if (NILP (handler))
    handler = Ffind_file_name_handler (newname, Qrename_file);
  if (!NILP (handler))
  {
    UNGCPRO;
    return call4 (handler, Qrename_file,
		  filename, newname, ok_if_already_exists);
  }

  /* When second argument is a directory, rename the file into it.
     (rename-file "foo" "bar/") == (rename-file "foo" "bar/foo")
   */
  if (!NILP (Ffile_directory_p (newname)))
    {
      Lisp_Object args[3];
      struct gcpro ngcpro1;
      int i = 1;

      args[0] = newname; 
      args[1] = Qnil; args[2] = Qnil;
      NGCPRO1 (*args); 
      ngcpro1.nvars = 3;
      if (XSTRING_BYTE (newname, XSTRING_LENGTH (newname) - 1) != '/')
	args[i++] = build_string ("/");
      args[i++] = Ffile_name_nondirectory (filename);
      newname = Fconcat (i, args);
      NUNGCPRO;
    }

  if (NILP (ok_if_already_exists)
      || INTP (ok_if_already_exists))
    barf_or_query_if_file_exists (newname, "rename to it",
				  INTP (ok_if_already_exists), 0);

#ifdef WINDOWSNT
  if (!MoveFile (XSTRING (filename)->data, XSTRING (newname)->data))
#else  /* not WINDOWSNT */
    /* FSFmacs only calls rename() here under BSD 4.1, and calls
       link() and unlink() otherwise, but that's bogus.  Sometimes
       rename() succeeds where link()/unlink() fail, and we have
       configure check for rename() and emulate using link()/unlink()
       if necessary. */
  if (0 > rename ((char *) XSTRING_DATA (filename),
		  (char *) XSTRING_DATA (newname)))
#endif /* not WINDOWSNT */
    {
#ifdef  WINDOWSNT
      /* Why two?  And why doesn't MS document what MoveFile will return?  */
      if (GetLastError () == ERROR_FILE_EXISTS
	  || GetLastError () == ERROR_ALREADY_EXISTS)
#else  /* not WINDOWSNT */
      if (errno == EXDEV)
#endif /* not WINDOWSNT */
	{
	  Fcopy_file (filename, newname,
		      /* We have already prompted if it was an integer,
			 so don't have copy-file prompt again.  */
		      ((NILP (ok_if_already_exists)) ? Qnil : Qt),
                      Qt);
	  Fdelete_file (filename);
	}
      else
	{
	  report_file_error ("Renaming", list2 (filename, newname));
	}
    }
  UNGCPRO;
  return Qnil;
}

DEFUN ("add-name-to-file", Fadd_name_to_file, 2, 3,
       "fAdd name to file: \nFName to add to %s: \np", /*
Give FILE additional name NEWNAME.  Both args strings.
Signals a `file-already-exists' error if a file NEWNAME already exists
unless optional third argument OK-IF-ALREADY-EXISTS is non-nil.
A number as third arg means request confirmation if NEWNAME already exists.
This is what happens in interactive use with M-x.
*/
       (filename, newname, ok_if_already_exists))
{
  /* This function can GC */
  Lisp_Object handler;
  struct gcpro gcpro1, gcpro2;

  GCPRO2 (filename, newname);
  CHECK_STRING (filename);
  CHECK_STRING (newname);
  filename = Fexpand_file_name (filename, Qnil);
  newname = Fexpand_file_name (newname, Qnil);

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (filename, Qadd_name_to_file);
  if (!NILP (handler))
    RETURN_UNGCPRO (call4 (handler, Qadd_name_to_file, filename,
			   newname, ok_if_already_exists));

  /* If the new name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (newname, Qadd_name_to_file);
  if (!NILP (handler))
    RETURN_UNGCPRO (call4 (handler, Qadd_name_to_file, filename,
			   newname, ok_if_already_exists));

  if (NILP (ok_if_already_exists)
      || INTP (ok_if_already_exists))
    barf_or_query_if_file_exists (newname, "make it a new name",
				  INTP (ok_if_already_exists), 0);
#ifdef WINDOWSNT
  /* Windows does not support this operation.  */
  report_file_error ("Adding new name", Flist (2, &filename));
#else /* not WINDOWSNT */

  unlink ((char *) XSTRING_DATA (newname));
  if (0 > link ((char *) XSTRING_DATA (filename),
		(char *) XSTRING_DATA (newname)))
    {
      report_file_error ("Adding new name",
			 list2 (filename, newname));
    }
#endif /* not WINDOWSNT */

  UNGCPRO;
  return Qnil;
}

#ifdef S_IFLNK
DEFUN ("make-symbolic-link", Fmake_symbolic_link, 2, 3,
       "FMake symbolic link to file: \nFMake symbolic link to file %s: \np", /*
Make a symbolic link to FILENAME, named LINKNAME.  Both args strings.
Signals a `file-already-exists' error if a file LINKNAME already exists
unless optional third argument OK-IF-ALREADY-EXISTS is non-nil.
A number as third arg means request confirmation if LINKNAME already exists.
This happens for interactive use with M-x.
*/
       (filename, linkname, ok_if_already_exists))
{
  /* This function can GC */
  Lisp_Object handler;
  struct gcpro gcpro1, gcpro2;

  GCPRO2 (filename, linkname);
  CHECK_STRING (filename);
  CHECK_STRING (linkname);
  /* If the link target has a ~, we must expand it to get
     a truly valid file name.  Otherwise, do not expand;
     we want to permit links to relative file names.  */
  if (XSTRING_BYTE (filename, 0) == '~') /* #### Un*x-specific */
    filename = Fexpand_file_name (filename, Qnil);
  linkname = Fexpand_file_name (linkname, Qnil);

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (filename, Qmake_symbolic_link);
  if (!NILP (handler))
    RETURN_UNGCPRO (call4 (handler, Qmake_symbolic_link, filename, linkname,
			   ok_if_already_exists));

  /* If the new link name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (linkname, Qmake_symbolic_link);
  if (!NILP (handler))
    RETURN_UNGCPRO (call4 (handler, Qmake_symbolic_link, filename,
			   linkname, ok_if_already_exists));

  if (NILP (ok_if_already_exists)
      || INTP (ok_if_already_exists))
    barf_or_query_if_file_exists (linkname, "make it a link",
				  INTP (ok_if_already_exists), 0);

  unlink ((char *) XSTRING_DATA (linkname));
  if (0 > symlink ((char *) XSTRING_DATA (filename),
		   (char *) XSTRING_DATA (linkname)))
    {
      report_file_error ("Making symbolic link",
			 list2 (filename, linkname));
    }
  UNGCPRO;
  return Qnil;
}
#endif /* S_IFLNK */

#ifdef VMS

DEFUN ("define-logical-name", Fdefine_logical_name, 2, 2,
       "sDefine logical name: \nsDefine logical name %s as: ", /*
Define the job-wide logical name NAME to have the value STRING.
If STRING is nil or a null string, the logical name NAME is deleted.
*/
       (varname, string))
{
  CHECK_STRING (varname);
  if (NILP (string))
    delete_logical_name ((char *) XSTRING_DATA (varname));
  else
    {
      CHECK_STRING (string);

      if (XSTRING_LENGTH (string) == 0)
        delete_logical_name ((char *) XSTRING_DATA (varname));
      else
        define_logical_name ((char *) XSTRING_DATA (varname), (char *) XSTRING_DATA (string));
    }

  return string;
}
#endif /* VMS */

#ifdef HPUX_NET

DEFUN ("sysnetunam", Fsysnetunam, 2, 2, 0, /*
Open a network connection to PATH using LOGIN as the login string.
*/
       (path, login))
{
  int netresult;
  
  CHECK_STRING (path);
  CHECK_STRING (login);  

  /* netunam, being a strange-o system call only used once, is not
     encapsulated. */
  {
    char *path_ext;
    char *login_ext;

    GET_C_STRING_FILENAME_DATA_ALLOCA (path, path_ext);
    GET_C_STRING_EXT_DATA_ALLOCA (login, FORMAT_OS, login_ext);
    
    netresult = netunam (path_ext, login_ext);
  }

  if (netresult == -1)
    return Qnil;
  else
    return Qt;
}
#endif /* HPUX_NET */

DEFUN ("file-name-absolute-p", Ffile_name_absolute_p, 1, 1, 0, /*
Return t if file FILENAME specifies an absolute path name.
On Unix, this is a name starting with a `/' or a `~'.
*/
       (filename))
{
  Bufbyte *ptr;

  CHECK_STRING (filename);
  ptr = XSTRING_DATA (filename);
  if (IS_DIRECTORY_SEP (*ptr) || *ptr == '~'
#ifdef VMS
/* ??? This criterion is probably wrong for '<'.  */
      || strchr (ptr, ':') || strchr (ptr, '<')
      || (*ptr == '[' && (ptr[1] != '-' || (ptr[2] != '.' && ptr[2] != ']'))
	  && ptr[1] != '.')
#endif /* VMS */
#ifdef DOS_NT
      || (*ptr != 0 && ptr[1] == ':' && (ptr[2] == '/' || ptr[2] == '\\'))
#endif
      )
    return Qt;
  else
    return Qnil;
}

/* Return nonzero if file FILENAME exists and can be executed.  */

static int
check_executable (char *filename)
{
#ifdef DOS_NT
  int len = strlen (filename);
  char *suffix;
  struct stat st;
  if (stat (filename, &st) < 0)
    return 0;
  return (S_ISREG (st.st_mode)
	  && len >= 5
	  && (stricmp ((suffix = filename + len-4), ".com") == 0
	      || stricmp (suffix, ".exe") == 0
	      || stricmp (suffix, ".bat") == 0)
	  || (st.st_mode & S_IFMT) == S_IFDIR);
#else /* not DOS_NT */
#ifdef HAVE_EACCESS
  return (eaccess (filename, 1) >= 0);
#else
  /* Access isn't quite right because it uses the real uid
     and we really want to test with the effective uid.
     But Unix doesn't give us a right way to do it.  */
  return (access (filename, 1) >= 0);
#endif
#endif /* not DOS_NT */
}

/* Return nonzero if file FILENAME exists and can be written.  */

static int
check_writable (CONST char *filename)
{
#ifdef MSDOS
  struct stat st;
  if (stat (filename, &st) < 0)
    return 0;
  return (st.st_mode & S_IWRITE || (st.st_mode & S_IFMT) == S_IFDIR);
#else /* not MSDOS */
#ifdef HAVE_EACCESS
  return (eaccess (filename, 2) >= 0);
#else
  /* Access isn't quite right because it uses the real uid
     and we really want to test with the effective uid.
     But Unix doesn't give us a right way to do it.
     Opening with O_WRONLY could work for an ordinary file,
     but would lose for directories.  */
  return (access (filename, 2) >= 0);
#endif
#endif /* not MSDOS */
}

DEFUN ("file-exists-p", Ffile_exists_p, 1, 1, 0, /*
Return t if file FILENAME exists.  (This does not mean you can read it.)
See also `file-readable-p' and `file-attributes'.
*/
       (filename))
{
  /* This function can GC */
  Lisp_Object abspath;
  Lisp_Object handler;
  struct stat statbuf;
  struct gcpro gcpro1;
  
  GCPRO1 (filename);
  CHECK_STRING (filename);
  abspath = Fexpand_file_name (filename, Qnil);
  UNGCPRO;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  GCPRO1 (abspath);
  handler = Ffind_file_name_handler (abspath, Qfile_exists_p);
  UNGCPRO;
  if (!NILP (handler))
    return call2 (handler, Qfile_exists_p, abspath);

  if (stat ((char *) XSTRING_DATA (abspath), &statbuf) >= 0)
    return (Qt);
  else
    return (Qnil);
}

DEFUN ("file-executable-p", Ffile_executable_p, 1, 1, 0, /*
Return t if FILENAME can be executed by you.
For a directory, this means you can access files in that directory.
*/
       (filename))

{
  /* This function can GC */
  Lisp_Object abspath;
  Lisp_Object handler;
  struct gcpro gcpro1;
  
  GCPRO1 (filename);
  CHECK_STRING (filename);
  abspath = Fexpand_file_name (filename, Qnil);
  UNGCPRO;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  GCPRO1 (abspath);
  handler = Ffind_file_name_handler (abspath, Qfile_executable_p);
  UNGCPRO;
  if (!NILP (handler))
    return call2 (handler, Qfile_executable_p, abspath);

  return (check_executable ((char *) XSTRING_DATA (abspath))
	  ? Qt : Qnil);
}

DEFUN ("file-readable-p", Ffile_readable_p, 1, 1, 0, /*
Return t if file FILENAME exists and you can read it.
See also `file-exists-p' and `file-attributes'.
*/
       (filename))
{
  /* This function can GC */
  Lisp_Object abspath;
  Lisp_Object handler;
  int desc;
  struct gcpro gcpro1;
  
  GCPRO1 (filename);
  CHECK_STRING (filename);
  abspath = Fexpand_file_name (filename, Qnil);
  UNGCPRO;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  GCPRO1 (abspath);
  handler = Ffind_file_name_handler (abspath, Qfile_readable_p);
  UNGCPRO;
  if (!NILP (handler))
    return call2 (handler, Qfile_readable_p, abspath);

  desc = open ((char *) XSTRING_DATA (abspath), O_RDONLY, 0);
  if (desc < 0)
    return Qnil;
  close (desc);
  return Qt;
}

/* Having this before file-symlink-p mysteriously caused it to be forgotten
   on the RT/PC.  */
DEFUN ("file-writable-p", Ffile_writable_p, 1, 1, 0, /*
Return t if file FILENAME can be written or created by you.
*/
       (filename))
{
  /* This function can GC */
  Lisp_Object abspath, dir;
  Lisp_Object handler;
  struct stat statbuf;
  struct gcpro gcpro1;
  
  GCPRO1 (filename);
  CHECK_STRING (filename);
  abspath = Fexpand_file_name (filename, Qnil);
  UNGCPRO;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  GCPRO1 (abspath);
  handler = Ffind_file_name_handler (abspath, Qfile_writable_p);
  UNGCPRO;
  if (!NILP (handler))
    return call2 (handler, Qfile_writable_p, abspath);

  if (stat ((char *) XSTRING_DATA (abspath), &statbuf) >= 0)
    return (check_writable ((char *) XSTRING_DATA (abspath))
	    ? Qt : Qnil);


  dir = Ffile_name_directory (abspath);
#if defined (VMS) || defined (MSDOS)
  if (!NILP (dir))
    dir = Fdirectory_file_name (dir);
#endif /* VMS or MSDOS */
  return (check_writable (!NILP (dir) ? (char *) XSTRING_DATA (dir)
			  : "")
	  ? Qt : Qnil);
}

DEFUN ("file-symlink-p", Ffile_symlink_p, 1, 1, 0, /*
Return non-nil if file FILENAME is the name of a symbolic link.
The value is the name of the file to which it is linked.
Otherwise returns nil.
*/
       (filename))
{
  /* This function can GC */
#ifdef S_IFLNK
  char *buf;
  int bufsize;
  int valsize;
  Lisp_Object val;
  Lisp_Object handler;
  struct gcpro gcpro1;
  
  GCPRO1 (filename);
  CHECK_STRING (filename);
  filename = Fexpand_file_name (filename, Qnil);
  UNGCPRO;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (filename, Qfile_symlink_p);
  if (!NILP (handler))
    return call2 (handler, Qfile_symlink_p, filename);

  bufsize = 100;
  while (1)
    {
      buf = (char *) xmalloc (bufsize);
      memset (buf, 0, bufsize);
      valsize = readlink ((char *) XSTRING_DATA (filename),
			  buf, bufsize);
      if (valsize < bufsize) break;
      /* Buffer was not long enough */
      xfree (buf);
      bufsize *= 2;
    }
  if (valsize == -1)
    {
      xfree (buf);
      return Qnil;
    }
  val = make_string ((Bufbyte *) buf, valsize);
  xfree (buf);
  return val;
#else /* not S_IFLNK */
  return Qnil;
#endif /* not S_IFLNK */
}

DEFUN ("file-directory-p", Ffile_directory_p, 1, 1, 0, /*
Return t if file FILENAME is the name of a directory as a file.
A directory name spec may be given instead; then the value is t
if the directory so specified exists and really is a directory.
*/
       (filename))
{
  /* This function can GC */
  Lisp_Object abspath;
  struct stat st;
  Lisp_Object handler;
  struct gcpro gcpro1;
  
  GCPRO1 (filename);
  abspath = expand_and_dir_to_file (filename,
				    current_buffer->directory);
  UNGCPRO;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  GCPRO1 (abspath);
  handler = Ffind_file_name_handler (abspath, Qfile_directory_p);
  UNGCPRO;
  if (!NILP (handler))
    return call2 (handler, Qfile_directory_p, abspath);

  if (stat ((char *) XSTRING_DATA (abspath), &st) < 0)
    return Qnil;
  return (st.st_mode & S_IFMT) == S_IFDIR ? Qt : Qnil;
}

DEFUN ("file-accessible-directory-p", Ffile_accessible_directory_p, 1, 1, 0, /*
Return t if file FILENAME is the name of a directory as a file,
and files in that directory can be opened by you.  In order to use a
directory as a buffer's current directory, this predicate must return true.
A directory name spec may be given instead; then the value is t
if the directory so specified exists and really is a readable and
searchable directory.
*/
       (filename))
{
  /* This function can GC */
  Lisp_Object handler;
  struct gcpro gcpro1;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (filename, Qfile_accessible_directory_p);
  if (!NILP (handler))
    return call2 (handler, Qfile_accessible_directory_p,
		  filename);

  GCPRO1 (filename);
  if (NILP (Ffile_directory_p (filename)))
    {
      UNGCPRO;
      return (Qnil);
    }
  handler = Ffile_executable_p (filename);
  UNGCPRO;
  return (handler);
}

DEFUN ("file-regular-p", Ffile_regular_p, 1, 1, 0, /*
  "Return t if file FILENAME is the name of a regular file.
This is the sort of file that holds an ordinary stream of data bytes.
*/
       (filename))
{
  REGISTER Lisp_Object abspath;
  struct stat st;
  Lisp_Object handler;

  abspath = expand_and_dir_to_file (filename, current_buffer->directory);

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (abspath, Qfile_regular_p);
  if (!NILP (handler))
    return call2 (handler, Qfile_regular_p, abspath);

  if (stat ((char *) XSTRING_DATA (abspath), &st) < 0)
    return Qnil;
  return (st.st_mode & S_IFMT) == S_IFREG ? Qt : Qnil;
}

DEFUN ("file-modes", Ffile_modes, 1, 1, 0, /*
Return mode bits of FILE, as an integer.
*/
       (filename))
{
  /* This function can GC */
  Lisp_Object abspath;
  struct stat st;
  Lisp_Object handler;
  struct gcpro gcpro1;
  
  GCPRO1 (filename);
  abspath = expand_and_dir_to_file (filename,
				    current_buffer->directory);
  UNGCPRO;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  GCPRO1 (abspath);
  handler = Ffind_file_name_handler (abspath, Qfile_modes);
  UNGCPRO;
  if (!NILP (handler))
    return call2 (handler, Qfile_modes, abspath);

  if (stat ((char *) XSTRING_DATA (abspath), &st) < 0)
    return Qnil;
#ifdef DOS_NT
  if (check_executable (XSTRING (abspath)->data))
    st.st_mode |= S_IEXEC;
#endif /* DOS_NT */

  return make_int (st.st_mode & 07777);
}

DEFUN ("set-file-modes", Fset_file_modes, 2, 2, 0, /*
Set mode bits of FILE to MODE (an integer).
Only the 12 low bits of MODE are used.
*/
       (filename, mode))
{
  /* This function can GC */
  Lisp_Object abspath;
  Lisp_Object handler;
  struct gcpro gcpro1, gcpro2;
  
  GCPRO2 (filename, mode);
  abspath = Fexpand_file_name (filename, current_buffer->directory);
  CHECK_INT (mode);
  UNGCPRO;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  GCPRO1 (abspath);
  handler = Ffind_file_name_handler (abspath, Qset_file_modes);
  UNGCPRO;
  if (!NILP (handler))
    return call3 (handler, Qset_file_modes, abspath, mode);

  if (chmod ((char *) XSTRING_DATA (abspath), XINT (mode)) < 0)
    report_file_error ("Doing chmod", Fcons (abspath, Qnil));

  return Qnil;
}

DEFUN ("set-default-file-modes", Fset_default_file_modes, 1, 1, 0, /*
Set the file permission bits for newly created files.
MASK should be an integer; if a permission's bit in MASK is 1,
subsequently created files will not have that permission enabled.
Only the low 9 bits are used.
This setting is inherited by subprocesses.
*/
       (mode))
{
  CHECK_INT (mode);
  
  umask ((~ XINT (mode)) & 0777);

  return Qnil;
}

DEFUN ("default-file-modes", Fdefault_file_modes, 0, 0, 0, /*
Return the default file protection for created files.
The umask value determines which permissions are enabled in newly
created files.  If a permission's bit in the umask is 1, subsequently
created files will not have that permission enabled.
*/
       ())
{
  int mode;

  mode = umask (0);
  umask (mode);

  return make_int ((~ mode) & 0777);
}

#ifndef VMS
DEFUN ("unix-sync", Funix_sync, 0, 0, "", /*
Tell Unix to finish all pending disk updates.
*/
       ())
{
  sync ();
  return Qnil;
}
#endif /* !VMS */


DEFUN ("file-newer-than-file-p", Ffile_newer_than_file_p, 2, 2, 0, /*
Return t if file FILE1 is newer than file FILE2.
If FILE1 does not exist, the answer is nil;
otherwise, if FILE2 does not exist, the answer is t.
*/
       (file1, file2))
{
  /* This function can GC */
  Lisp_Object abspath1, abspath2;
  struct stat st;
  int mtime1;
  Lisp_Object handler;
  struct gcpro gcpro1, gcpro2;

  CHECK_STRING (file1);
  CHECK_STRING (file2);

  abspath1 = Qnil;
  GCPRO2 (abspath1, file2);
  abspath1 = expand_and_dir_to_file (file1,
				     current_buffer->directory);
  abspath2 = expand_and_dir_to_file (file2,
				     current_buffer->directory);
  UNGCPRO;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  GCPRO2 (abspath1, abspath2);
  handler = Ffind_file_name_handler (abspath1, Qfile_newer_than_file_p);
  if (NILP (handler))
    handler = Ffind_file_name_handler (abspath2, Qfile_newer_than_file_p);
  UNGCPRO;
  if (!NILP (handler))
    return call3 (handler, Qfile_newer_than_file_p, abspath1,
		  abspath2);

  if (stat ((char *) XSTRING_DATA (abspath1), &st) < 0)
    return Qnil;

  mtime1 = st.st_mtime;

  if (stat ((char *) XSTRING_DATA (abspath2), &st) < 0)
    return Qt;

  return (mtime1 > st.st_mtime) ? Qt : Qnil;
}


#ifdef DOS_NT
Lisp_Object Qfind_buffer_file_type;
#endif /* DOS_NT */

/* Stack sizes > 2**16 is a good way to elicit compiler bugs */
/* #define READ_BUF_SIZE (2 << 16) */
#define READ_BUF_SIZE (1 << 15)

DEFUN ("insert-file-contents-internal",
       Finsert_file_contents_internal, 1, 7, 0, /*
Insert contents of file FILENAME after point; no coding-system frobbing.
This function is identical to `insert-file-contents' except for the
handling of the CODESYS and USED-CODESYS arguments under
XEmacs/Mule. (When Mule support is not present, both functions are
identical and ignore the CODESYS and USED-CODESYS arguments.)

If support for Mule exists in this Emacs, the file is decoded according
to CODESYS; if omitted, no conversion happens.  If USED-CODESYS is non-nil,
it should be a symbol, and the actual coding system that was used for the
decoding is stored into it.  It will in general be different from CODESYS
if CODESYS specifies automatic encoding detection or end-of-line detection.

Currently BEG and END refer to byte positions (as opposed to character
positions), even in Mule. (Fixing this is very difficult.)
*/
     (filename, visit, beg, end, replace, codesys, used_codesys))
{
  /* This function can GC */
  struct stat st;
  int fd;
  int saverrno = 0;
  Charcount inserted = 0;
  int speccount;
  struct gcpro gcpro1, gcpro2, gcpro3, gcpro4;
  Lisp_Object handler = Qnil, val;
  int total;
  Bufbyte read_buf[READ_BUF_SIZE];
  int mc_count;
  struct buffer *buf = current_buffer;
  int not_regular = 0;

  if (buf->base_buffer && ! NILP (visit))
    error ("Cannot do file visiting in an indirect buffer");

  /* No need to call Fbarf_if_buffer_read_only() here.
     That's called in begin_multiple_change() or wherever. */

  val = Qnil;

  GCPRO4 (filename, val, visit, handler);
  
  mc_count = (NILP (replace)) ?
    begin_multiple_change (buf, BUF_PT  (buf), BUF_PT (buf)) :
    begin_multiple_change (buf, BUF_BEG (buf), BUF_Z  (buf));

  speccount = specpdl_depth (); /* begin_multiple_change also adds
				   an unwind_protect */

  filename = Fexpand_file_name (filename, Qnil);

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (filename, Qinsert_file_contents);
  if (!NILP (handler))
    {
      val = call6 (handler, Qinsert_file_contents, filename,
		   visit, beg, end, replace);
      goto handled;
    }

#ifdef MULE
  if (!NILP (used_codesys))
    CHECK_SYMBOL (used_codesys);
#endif

  if ( (!NILP (beg) || !NILP (end)) && !NILP (visit) )
    error ("Attempt to visit less than an entire file");
  
  if (!NILP (beg))
    CHECK_INT (beg);
  else
    beg = Qzero;

  if (!NILP (end))
    CHECK_INT (end);

  fd = -1;

#ifndef APOLLO
  if (stat ((char *) XSTRING_DATA (filename), &st) < 0)
#else /* APOLLO */
  if ((fd = open ((char *) XSTRING_DATA (filename), O_RDONLY, 0)) < 0
      || fstat (fd, &st) < 0)
#endif /* APOLLO */
    {
      if (fd >= 0) close (fd);
    badopen:
      if (NILP (visit))
	report_file_error ("Opening input file",
			   Fcons (filename, Qnil));
      st.st_mtime = -1;
      goto notfound;
    }

#ifdef S_IFREG
  /* This code will need to be changed in order to work on named
     pipes, and it's probably just not worth it.  So we should at
     least signal an error.  */
  if (!S_ISREG (st.st_mode))
    {
      if (NILP (visit))
	{
	  end_multiple_change (buf, mc_count);

	  return Fsignal (Qfile_error,
			  list2 (build_translated_string("not a regular file"),
				 filename));
	}
      else
	{
	  not_regular = 1;
	  goto notfound;
	}
    }
#endif

  if (fd < 0)
    if ((fd = open ((char *) XSTRING_DATA (filename), O_RDONLY, 0)) < 0)
      goto badopen;

  /* Replacement should preserve point as it preserves markers.  */
  if (!NILP (replace))
    record_unwind_protect (restore_point_unwind, Fpoint_marker (Qnil, Qnil));

  record_unwind_protect (close_file_unwind, make_int (fd));

  /* Supposedly happens on VMS.  */
  if (st.st_size < 0)
    error ("File size is negative");

  if (NILP (end))
    {
      end = make_int (st.st_size);
      if (XINT (end) != st.st_size)
	error ("maximum buffer size exceeded");
    }

  /* If requested, replace the accessible part of the buffer
     with the file contents.  Avoid replacing text at the
     beginning or end of the buffer that matches the file contents;
     that preserves markers pointing to the unchanged parts.  */
#if !defined (DOS_NT) && !defined (MULE)
  /* The replace-mode code currently only works when the assumption
     'one byte == one char' holds true.  This fails under MSDOS and
     Windows NT (because newlines are represented as CR-LF in text
     files) and under Mule because files may contain multibyte characters. */
# define FSFMACS_SPEEDY_INSERT
#endif
#ifndef FSFMACS_SPEEDY_INSERT
  if (!NILP (replace))
    {
      buffer_delete_range (buf, BUF_BEG (buf), BUF_Z (buf),
			   !NILP (visit) ? INSDEL_NO_LOCKING : 0);
    }
#else /* FSFMACS_SPEEDY_INSERT */
  if (!NILP (replace))
    {
      char buffer[1 << 14];
      Bufpos same_at_start = BUF_BEGV (buf);
      Bufpos same_at_end = BUF_ZV (buf);
      int overlap;

      /* Count how many chars at the start of the file
	 match the text at the beginning of the buffer.  */
      while (1)
	{
	  int nread;
	  Bufpos bufpos;

	  nread = read_allowing_quit (fd, buffer, sizeof buffer);
	  if (nread < 0)
	    error ("IO error reading %s: %s",
		   XSTRING_DATA (filename), strerror (errno));
	  else if (nread == 0)
	    break;
	  bufpos = 0;
	  while (bufpos < nread && same_at_start < BUF_ZV (buf)
		 && BUF_FETCH_CHAR (buf, same_at_start) == buffer[bufpos])
	    same_at_start++, bufpos++;
	  /* If we found a discrepancy, stop the scan.
	     Otherwise loop around and scan the next bufferfull.  */
	  if (bufpos != nread)
	    break;
	}
      /* If the file matches the buffer completely,
	 there's no need to replace anything.  */
      if (same_at_start - BUF_BEGV (buf) == st.st_size)
	{
	  close (fd);
          unbind_to (speccount, Qnil);
	  /* Truncate the buffer to the size of the file.  */
	  buffer_delete_range (buf, same_at_start, same_at_end,
			       !NILP (visit) ? INSDEL_NO_LOCKING : 0);
	  goto handled;
	}
      /* Count how many chars at the end of the file
	 match the text at the end of the buffer.  */
      while (1)
	{
	  int total_read, nread;
	  Bufpos bufpos, curpos, trial;

	  /* At what file position are we now scanning?  */
	  curpos = st.st_size - (BUF_ZV (buf) - same_at_end);
	  /* If the entire file matches the buffer tail, stop the scan.  */
	  if (curpos == 0)
	    break;
	  /* How much can we scan in the next step?  */
	  trial = min (curpos, sizeof buffer);
	  if (lseek (fd, curpos - trial, 0) < 0)
	    report_file_error ("Setting file position",
			       Fcons (filename, Qnil));

	  total_read = 0;
	  while (total_read < trial)
	    {
	      nread = read_allowing_quit (fd, buffer + total_read,
					  trial - total_read);
	      if (nread <= 0)
		error ("IO error reading %s: %s",
		       XSTRING_DATA (filename), strerror (errno));
	      total_read += nread;
	    }
	  /* Scan this bufferfull from the end, comparing with
	     the Emacs buffer.  */
	  bufpos = total_read;
	  /* Compare with same_at_start to avoid counting some buffer text
	     as matching both at the file's beginning and at the end.  */
	  while (bufpos > 0 && same_at_end > same_at_start
		 && BUF_FETCH_CHAR (buf, same_at_end - 1) ==
		 buffer[bufpos - 1])
	    same_at_end--, bufpos--;
	  /* If we found a discrepancy, stop the scan.
	     Otherwise loop around and scan the preceding bufferfull.  */
	  if (bufpos != 0)
	    break;
	  /* If display current starts at beginning of line,
	     keep it that way.  */
	  if (XBUFFER (XWINDOW (Fselected_window (Qnil))->buffer) == buf)
	    XWINDOW (Fselected_window (Qnil))->start_at_line_beg =
	      !NILP (Fbolp (make_buffer (buf)));
	}

      /* Don't try to reuse the same piece of text twice.  */
      overlap = same_at_start - BUF_BEGV (buf) -
	(same_at_end + st.st_size - BUF_ZV (buf));
      if (overlap > 0)
	same_at_end += overlap;

      /* Arrange to read only the nonmatching middle part of the file.  */
      beg = make_int (same_at_start - BUF_BEGV (buf));
      end = make_int (st.st_size - (BUF_ZV (buf) - same_at_end));

      buffer_delete_range (buf, same_at_start, same_at_end,
			   !NILP (visit) ? INSDEL_NO_LOCKING : 0);
      /* Insert from the file at the proper position.  */
      BUF_SET_PT (buf, same_at_start);
    }
#endif /* FSFMACS_SPEEDY_INSERT */

  total = XINT (end) - XINT (beg);

  if (XINT (beg) != 0
#ifdef FSFMACS_SPEEDY_INSERT
      /* why was this here? asked jwz.  The reason is that the replace-mode
	 connivings above will normally put the file pointer other than
	 where it should be. */
      || !NILP (replace)
#endif /* !FSFMACS_SPEEDY_INSERT */
      )
    {
      if (lseek (fd, XINT (beg), 0) < 0)
	report_file_error ("Setting file position",
			   Fcons (filename, Qnil));
    }

  {
    Bufpos cur_point = BUF_PT (buf);
    struct gcpro ngcpro1;
    Lisp_Object stream = make_filedesc_input_stream (fd, 0, total,
						     LSTR_ALLOW_QUIT);

    NGCPRO1 (stream);
    Lstream_set_buffering (XLSTREAM (stream), LSTREAM_BLOCKN_BUFFERED, 65536);
#ifdef MULE
    stream = make_decoding_input_stream
      (XLSTREAM (stream), Fget_coding_system (codesys));
    Lstream_set_character_mode (XLSTREAM (stream));
    Lstream_set_buffering (XLSTREAM (stream), LSTREAM_BLOCKN_BUFFERED, 65536);
#endif

    record_unwind_protect (close_stream_unwind, stream);

    /* No need to limit the amount of stuff we attempt to read. (It would
       be incorrect, anyway, when Mule is enabled.) Instead, the limiting
       occurs inside of the filedesc stream. */
    while (1)
      {
	Bytecount this_len;
	Charcount cc_inserted;

	QUIT;
	this_len = Lstream_read (XLSTREAM (stream), read_buf,
				 sizeof (read_buf));
      
	if (this_len <= 0)
	  {
	    if (this_len < 0)
	      saverrno = errno;
	    break;
	  }

	cc_inserted = buffer_insert_raw_string_1 (buf, cur_point, read_buf,
						  this_len,
						  !NILP (visit)
						  ? INSDEL_NO_LOCKING : 0);
	inserted  += cc_inserted;
	cur_point += cc_inserted;
      }
#ifdef MULE
    if (!NILP (used_codesys))
      {
	Fset (used_codesys,
	      XCODING_SYSTEM_NAME (decoding_stream_coding_system (XLSTREAM (stream))));
      }
#endif
    NUNGCPRO;
  }

#ifdef DOS_NT
  /* Determine file type from name and remove LFs from CR-LFs if the file
     is deemed to be a text file.  */
  {
    struct gcpro gcpro1;
    GCPRO1 (filename);
    buf->buffer_file_type
      = call1_in_buffer (buf, Qfind_buffer_file_type, filename);
    UNGCPRO;
    if (NILP (buf->buffer_file_type))
      {
	buffer_do_msdos_crlf_to_lf (buf, ####);
      }
  }
#endif

  /* Close the file/stream */
  unbind_to (speccount, Qnil);

  if (saverrno != 0)
    {
      error ("IO error reading %s: %s",
	     XSTRING_DATA (filename), strerror (saverrno));
    }

 notfound:
 handled:

  end_multiple_change (buf, mc_count);

  if (!NILP (visit))
    {
      if (!EQ (buf->undo_list, Qt))
	buf->undo_list = Qnil;
#ifdef APOLLO
      stat ((char *) XSTRING_DATA (filename), &st);
#endif
      if (NILP (handler))
	{
	  buf->modtime = st.st_mtime;
	  buf->filename = filename;
	  /* XEmacs addition: */
	  /* This function used to be in C, ostensibly so that
	     it could be called here.  But that's just silly.
	     There's no reason C code can't call out to Lisp
	     code, and it's a lot cleaner this way. */
	  if (!NILP (Ffboundp (Qcompute_buffer_file_truename)))
	    call1 (Qcompute_buffer_file_truename, make_buffer (buf));
	}
      BUF_SAVE_MODIFF (buf) = BUF_MODIFF (buf);
      buf->auto_save_modified = BUF_MODIFF (buf);
      buf->save_length = make_int (BUF_SIZE (buf));
#ifdef CLASH_DETECTION
      if (NILP (handler))
	{
	  if (!NILP (buf->file_truename))
	    unlock_file (buf->file_truename);
	  unlock_file (filename);
	}
#endif /* CLASH_DETECTION */
      if (not_regular)
	RETURN_UNGCPRO (Fsignal (Qfile_error,
				 list2 (build_string ("not a regular file"),
			         filename)));

      /* If visiting nonexistent file, return nil.  */
      if (buf->modtime == -1)
	report_file_error ("Opening input file",
			   list1 (filename));
    }

  /* Decode file format */
  if (inserted > 0)
    {
      Lisp_Object insval = call3 (Qformat_decode, 
                                  Qnil, make_int (inserted), visit);
      CHECK_INT (insval);
      inserted = XINT (insval);
    }

  if (inserted > 0)
    {
      Lisp_Object p = Vafter_insert_file_functions;
      struct gcpro ngcpro1;

      NGCPRO1 (p);
      while (!NILP (p))
	{
	  Lisp_Object insval =
	    call1 (Fcar (p), make_int (inserted));
	  if (!NILP (insval))
	    {
	      CHECK_NATNUM (insval);
	      inserted = XINT (insval);
	    }
	  QUIT;
	  p = Fcdr (p);
	}
      NUNGCPRO;
    }

  UNGCPRO;

  if (!NILP (val))
    return (val);
  else
    return (list2 (filename, make_int (inserted)));
}


static int a_write (Lisp_Object outstream, Lisp_Object instream, int pos,
		    Lisp_Object *annot);
static Lisp_Object build_annotations (Lisp_Object start, Lisp_Object end);

/* If build_annotations switched buffers, switch back to BUF.
   Kill the temporary buffer that was selected in the meantime.  */

static Lisp_Object 
build_annotations_unwind (Lisp_Object buf)
{
  Lisp_Object tembuf;

  if (XBUFFER (buf) == current_buffer)
    return Qnil;
  tembuf = Fcurrent_buffer ();
  Fset_buffer (buf);
  Fkill_buffer (tembuf);
  return Qnil;
}

DEFUN ("write-region-internal", Fwrite_region_internal, 3, 7,
       "r\nFWrite region to file: ", /*
Write current region into specified file; no coding-system frobbing.
This function is identical to `write-region' except for the handling
of the CODESYS argument under XEmacs/Mule. (When Mule support is not
present, both functions are identical and ignore the CODESYS argument.)
If support for Mule exists in this Emacs, the file is encoded according
to the value of CODESYS.  If this is nil, no code conversion occurs.
*/
       (start, end, filename, append, visit, lockname, codesys))
{
  /* This function can GC */
  int desc;
  int failure;
  int save_errno = 0;
  struct stat st;
  Lisp_Object fn;
  int speccount = specpdl_depth ();
#ifdef VMS
  unsigned char *fname = 0;     /* If non-0, original filename (must rename) */
#endif /* VMS */
  int visiting_other = STRINGP (visit);
  int visiting = (EQ (visit, Qt) || visiting_other);
  int quietly = (!visiting && !NILP (visit));
  Lisp_Object visit_file = Qnil;
  Lisp_Object annotations = Qnil;
  struct buffer *given_buffer;
  Bufpos start1, end1;

#ifdef DOS_NT
  int buffer_file_type
    = NILP (current_buffer->buffer_file_type) ? O_TEXT : O_BINARY;
#endif /* DOS_NT */

#ifdef MULE
  codesys = Fget_coding_system (codesys);
#endif /* MULE */

  if (current_buffer->base_buffer && ! NILP (visit))
    error ("Cannot do file visiting in an indirect buffer");

  if (!NILP (start) && !STRINGP (start))
    get_buffer_range_char (current_buffer, start, end, &start1, &end1, 0);

  {
    Lisp_Object handler;
    struct gcpro gcpro1, gcpro2, gcpro3, gcpro4, gcpro5;
    GCPRO5 (start, filename, visit, visit_file, lockname);

    if (visiting_other)
      visit_file = Fexpand_file_name (visit, Qnil);
    else
      visit_file = filename;
    filename = Fexpand_file_name (filename, Qnil);

    UNGCPRO;

    if (NILP (lockname))
      lockname = visit_file;

    /* If the file name has special constructs in it,
       call the corresponding file handler.  */
    handler = Ffind_file_name_handler (filename, Qwrite_region);
    /* If FILENAME has no handler, see if VISIT has one.  */
    if (NILP (handler) && STRINGP (visit))
      handler = Ffind_file_name_handler (visit, Qwrite_region);

    if (!NILP (handler))
      {
        Lisp_Object val = call8 (handler, Qwrite_region, start, end, 
                                 filename, append, visit, lockname, codesys);
	if (visiting)
	  {
	    BUF_SAVE_MODIFF (current_buffer) = BUF_MODIFF (current_buffer);
	    current_buffer->save_length =
	      make_int (BUF_SIZE (current_buffer));
	    current_buffer->filename = visit_file;
	    MARK_MODELINE_CHANGED;
	  }
	return val;
      }
  }

#ifdef CLASH_DETECTION
  if (!auto_saving)
    {
      struct gcpro gcpro1, gcpro2, gcpro3, gcpro4;
      GCPRO4 (start, filename, visit_file, lockname);
      lock_file (lockname);
      UNGCPRO;
    }
#endif /* CLASH_DETECTION */

  /* Special kludge to simplify auto-saving.  */
  if (NILP (start))
    {
      start1 = BUF_BEG (current_buffer);
      end1 = BUF_Z (current_buffer);
    }

  record_unwind_protect (build_annotations_unwind, Fcurrent_buffer ());

  given_buffer = current_buffer;
  annotations = build_annotations (start, end);
  if (current_buffer != given_buffer)
    {
      start1 = BUF_BEGV (current_buffer);
      end1 = BUF_ZV (current_buffer);
    }

  fn = filename;
  desc = -1;
  if (!NILP (append))
#ifdef DOS_NT
    desc = open ((char *) XSTRING_DATA (fn),
                       (O_WRONLY | buffer_file_type), 0);
#else /* not DOS_NT */
    desc = open ((char *) XSTRING_DATA (fn), O_WRONLY, 0);
#endif /* not DOS_NT */

  if (desc < 0)
#ifndef VMS
    {
#ifdef DOS_NT
      desc = open ((char *) XSTRING_DATA (fn), 
                   (O_WRONLY | O_TRUNC | O_CREAT | buffer_file_type), 
                   (S_IREAD | S_IWRITE));
#else /* not DOS_NT */
      desc = creat ((char *) XSTRING_DATA (fn),
		    ((auto_saving) ? auto_save_mode_bits : 0666));
#endif /* DOS_NT */
    }
#else /* VMS */
  {
    if (auto_saving)	/* Overwrite any previous version of autosave file */
      {
	char *fn_data = XSTRING_DATA (fn);
	/* if fn exists, truncate to zero length */
	vms_truncate (fn_data);
	desc = open (fn_data, O_RDWR, 0);
	if (desc < 0)
	  desc = creat_copy_attrs ((STRINGP (current_buffer->filename)
				    ? (char *)
				    XSTRING_DATA (current_buffer->filename)
				    : 0),
				   fn_data);
      }
    else		/* Write to temporary name and rename if no errors */
      {
	Lisp_Object temp_name;

	struct gcpro gcpro1, gcpro2, gcpro3;
	GCPRO3 (start, filename, visit_file);
	{
	  struct gcpro gcpro1, gcpro2, gcpro3; /* Don't have GCPRO6 */

	  GCPRO3 (fn, fname, annotations);

	  temp_name = Ffile_name_directory (filename);

	  if (NILP (temp_name))
	    desc = creat ((char *) XSTRING_DATA (fn), 0666);
	  else
	    {
	      temp_name =
		Fmake_temp_name (concat2 (temp_name,
					  build_string ("$$SAVE$$")));
	      fname = filename;
	      fn = temp_name;
	      desc = creat_copy_attrs (fname,
				       (char *) XSTRING_DATA (fn));
	      if (desc < 0)
		{
		  char *fn_data;
		  /* If we can't open the temporary file, try creating a new
		     version of the original file.  VMS "creat" creates a
		     new version rather than truncating an existing file. */
		  fn = fname;
		  fname = Qnil;
		  fn_data = XSTRING_DATA (fn);
		  desc = creat (fn_data, 0666);
#if 0                           /* This can clobber an existing file and fail
				   to replace it, if the user runs out of
				   space.  */
		  if (desc < 0)
		    {
		      /* We can't make a new version;
			 try to truncate and rewrite existing version if any.
		       */
		      vms_truncate (fn_data);
		      desc = open (fn_data, O_RDWR, 0);
		    }
#endif
		}
	    }
	  UNGCPRO;
	}
	UNGCPRO;
      }
  }
#endif /* VMS */

  if (desc < 0)
    {
#ifdef CLASH_DETECTION
      save_errno = errno;
      if (!auto_saving) unlock_file (lockname);
      errno = save_errno;
#endif /* CLASH_DETECTION */
      report_file_error ("Opening output file",
			 Fcons (filename, Qnil));
    }

  {
    Lisp_Object desc_locative = Fcons (make_int (desc), Qnil);
    Lisp_Object instream = Qnil, outstream = Qnil;
    struct gcpro gcpro1, gcpro2;
    /* need to gcpro; QUIT could happen out of call to write() */
    GCPRO2 (instream, outstream);

    record_unwind_protect (close_file_unwind, desc_locative);

    if (!NILP (append))
      {
	if (lseek (desc, 0, 2) < 0)
	  {
#ifdef CLASH_DETECTION
	    if (!auto_saving) unlock_file (lockname);
#endif /* CLASH_DETECTION */
	    report_file_error ("Lseek error",
			       list1 (filename));
	  }
      }

#ifdef VMS
/*
 * Kludge Warning: The VMS C RTL likes to insert carriage returns
 * if we do writes that don't end with a carriage return. Furthermore
 * it cannot handle writes of more then 16K. The modified
 * version of "sys_write" in SYSDEP.C (see comment there) copes with
 * this EXCEPT for the last record (iff it doesn't end with a carriage
 * return). This implies that if your buffer doesn't end with a carriage
 * return, you get one free... tough. However it also means that if
 * we make two calls to sys_write (a la the following code) you can
 * get one at the gap as well. The easiest way to fix this (honest)
 * is to move the gap to the next newline (or the end of the buffer).
 * Thus this change.
 *
 * Yech!
 */
    you lose -- fix this
    if (GPT > BUF_BEG (current_buffer) && *GPT_ADDR[-1] != '\n')
      move_gap (find_next_newline (current_buffer, GPT, 1));
#endif

    failure = 0;

    /* Note: I tried increasing the buffering size, along with
       various other tricks, but nothing seemed to make much of
       a difference in the time it took to save a large file.
       (Actually that's not true.  With a local disk, changing
       the buffer size doesn't seem to make much difference.
       With an NFS-mounted disk, it could make a lot of difference
       because you're affecting the number of network requests
       that need to be made, and there could be a large latency
       for each request.  So I've increased the buffer size
       to 64K.) */
    outstream = make_filedesc_output_stream (desc, 0, -1, 0);
    Lstream_set_buffering (XLSTREAM (outstream),
			   LSTREAM_BLOCKN_BUFFERED, 65536);
#ifdef MULE
    outstream =
      make_encoding_output_stream ( XLSTREAM (outstream), codesys);
    Lstream_set_buffering (XLSTREAM (outstream),
			   LSTREAM_BLOCKN_BUFFERED, 65536);
#endif
    if (STRINGP (start))
      {
	instream = make_lisp_string_input_stream (start, 0, -1);
	start1 = 0;
      }
    else
      instream = make_lisp_buffer_input_stream (current_buffer, start1, end1,
						LSTR_SELECTIVE |
						LSTR_IGNORE_ACCESSIBLE);
    failure = (0 > (a_write (outstream, instream, start1,
			     &annotations)));
    save_errno = errno;
    /* Note that this doesn't close the desc since we created the
       stream without the LSTR_CLOSING flag, but it does
       flush out any buffered data. */
    if (Lstream_close (XLSTREAM (outstream)) < 0)
      {
	failure = 1;
	save_errno = errno;
      }
    Lstream_close (XLSTREAM (instream));
    UNGCPRO;

#ifdef HAVE_FSYNC
    /* Note fsync appears to change the modtime on BSD4.2 (both vax and sun).
       Disk full in NFS may be reported here.  */
    /* mib says that closing the file will try to write as fast as NFS can do
       it, and that means the fsync here is not crucial for autosave files.  */
    if (!auto_saving && fsync (desc) < 0
	/* If fsync fails with EINTR, don't treat that as serious.  */
	&& errno != EINTR)
      {
	failure = 1;
	save_errno = errno;
      }
#endif

    /* Spurious "file has changed on disk" warnings have been 
       observed on Suns as well.
       It seems that `close' can change the modtime, under nfs.

       (This has supposedly been fixed in Sunos 4,
       but who knows about all the other machines with NFS?)  */
    /* On VMS and APOLLO, must do the stat after the close
       since closing changes the modtime.  */
#if 0 /* !defined (VMS) && !defined (APOLLO) */
    fstat (desc, &st);
#endif

    /* NFS can report a write failure now.  */
    if (close (desc) < 0)
      {
	failure = 1;
	save_errno = errno;
      }

    /* Discard the close unwind-protect.  Execute the one for
       build_annotations (switches back to the original current buffer
       as necessary). */
    XCAR (desc_locative) = Qnil;
    unbind_to (speccount, Qnil);
  }
    

#ifdef VMS
  /* If we wrote to a temporary name and had no errors, rename to real name. */
  if (!NILP (fname))
    {
      if (!failure)
	{
	  failure = (rename ((char *) XSTRING_DATA (fn), 
			     (char *) XSTRING_DATA (fname))
		     != 0);
	  save_errno = errno;
	}
      fn = fname;
    }
#endif /* VMS */

#if 1 /* defined (VMS) || defined (APOLLO) */
  stat ((char *) XSTRING_DATA (fn), &st);
#endif

#ifdef CLASH_DETECTION
  if (!auto_saving)
    unlock_file (lockname);
#endif /* CLASH_DETECTION */

  /* Do this before reporting IO error
     to avoid a "file has changed on disk" warning on
     next attempt to save.  */
  if (visiting)
    current_buffer->modtime = st.st_mtime;

  if (failure)
    error ("IO error writing %s: %s", 
           XSTRING_DATA (fn), 
           strerror (save_errno));

  if (visiting)
    {
      BUF_SAVE_MODIFF (current_buffer) = BUF_MODIFF (current_buffer);
      current_buffer->save_length = make_int (BUF_SIZE (current_buffer));
      current_buffer->filename = visit_file;
      MARK_MODELINE_CHANGED;
    }
  else if (quietly)
    {
      return Qnil;
    }

  if (!auto_saving)
    {
      if (visiting_other)
        message ("Wrote %s", XSTRING_DATA (visit_file));
      else
	{
	  struct gcpro gcpro1;
	  Lisp_Object fsp;
	  GCPRO1 (fn);

	  fsp = Ffile_symlink_p (fn);
	  if (NILP (fsp))
	    message ("Wrote %s", XSTRING_DATA (fn));
	  else
	    message ("Wrote %s (symlink to %s)", 
		     XSTRING_DATA (fn), XSTRING_DATA (fsp));
	  UNGCPRO;
	}
    }
  return Qnil;
}

/* #### This is such a load of shit!!!!  There is no way we should define
   something so stupid as a subr, just sort the fucking list more
   intelligently. */
DEFUN ("car-less-than-car", Fcar_less_than_car, 2, 2, 0, /*
Return t if (car A) is numerically less than (car B).
*/
       (a, b))
{
  return Flss (Fcar (a), Fcar (b));
}

/* Heh heh heh, let's define this too, just to aggravate the person who
   wrote the above comment. */
DEFUN ("cdr-less-than-cdr", Fcdr_less_than_cdr, 2, 2, 0, /*
Return t if (cdr A) is numerically less than (cdr B).
*/
       (a, b))
{
  return Flss (Fcdr (a), Fcdr (b));
}

/* Build the complete list of annotations appropriate for writing out
   the text between START and END, by calling all the functions in
   write-region-annotate-functions and merging the lists they return.
   If one of these functions switches to a different buffer, we assume
   that buffer contains altered text.  Therefore, the caller must
   make sure to restore the current buffer in all cases,
   as save-excursion would do.  */

static Lisp_Object
build_annotations (Lisp_Object start, Lisp_Object end)
{
  /* This function can GC */
  Lisp_Object annotations;
  Lisp_Object p, res;
  struct gcpro gcpro1, gcpro2;

  annotations = Qnil;
  p = Vwrite_region_annotate_functions;
  GCPRO2 (annotations, p);
  while (!NILP (p))
    {
      struct buffer *given_buffer = current_buffer;
      Vwrite_region_annotations_so_far = annotations;
      res = call2 (Fcar (p), start, end);
      /* If the function makes a different buffer current,
	 assume that means this buffer contains altered text to be output.
	 Reset START and END from the buffer bounds
	 and discard all previous annotations because they should have
	 been dealt with by this function.  */
      if (current_buffer != given_buffer)
	{
	  start = make_int (BUF_BEGV (current_buffer));
	  end = make_int (BUF_ZV (current_buffer));
	  annotations = Qnil;
	}
      (void) Flength (res);     /* Check basic validity of return value */
      annotations = merge (annotations, res, Qcar_less_than_car);
      p = Fcdr (p);
    }

  /* Now do the same for annotation functions implied by the file-format */
  if (auto_saving && (!EQ (Vauto_save_file_format, Qt)))
    p = Vauto_save_file_format;
  else
    p = current_buffer->file_format;
  while (!NILP (p))
    {
      struct buffer *given_buffer = current_buffer;
      Vwrite_region_annotations_so_far = annotations;
      res = call3 (Qformat_annotate_function, Fcar (p), start, end);
      if (current_buffer != given_buffer)
	{
	  start = make_int (BUF_BEGV (current_buffer));
	  end = make_int (BUF_ZV (current_buffer));
	  annotations = Qnil;
	}
      (void) Flength (res);
      annotations = merge (annotations, res, Qcar_less_than_car);
      p = Fcdr (p);
    }
  UNGCPRO;
  return annotations;
}

/* Write to stream OUTSTREAM the characters from INSTREAM (it is read until
   EOF is encountered), assuming they start at position POS in the buffer
   of string that STREAM refers to.  Intersperse with them the annotations
   from *ANNOT that fall into the range of positions we are reading from,
   each at its appropriate position.

   Modify *ANNOT by discarding elements as we output them.
   The return value is negative in case of system call failure.  */

/* 4K should probably be fine.  We just need to reduce the number of
   function calls to reasonable level.  The Lstream stuff itself will
   batch to 64K to reduce the number of system calls. */

#define A_WRITE_BATCH_SIZE 4096

static int
a_write (Lisp_Object outstream, Lisp_Object instream, int pos,
	 Lisp_Object *annot)
{
  Lisp_Object tem;
  int nextpos;
  unsigned char largebuf[A_WRITE_BATCH_SIZE];
  Lstream *instr = XLSTREAM (instream);
  Lstream *outstr = XLSTREAM (outstream);

  while (NILP (*annot) || CONSP (*annot))
    {
      tem = Fcar_safe (Fcar (*annot));
      if (INTP (tem))
	nextpos = XINT (tem);
      else
	nextpos = INT_MAX;
#ifdef MULE
      /* If there are annotations left and we have Mule, then we
	 have to do the I/O one emchar at a time so we can
	 determine when to insert the annotation. */
      if (!NILP (*annot))
	{
	  Emchar ch;
	  while (pos != nextpos && (ch = Lstream_get_emchar (instr)) != EOF)
	    {
	      if (Lstream_put_emchar (outstr, ch) < 0)
		return -1;
	      pos++;
	    }
	}
      else
#endif
	{
	  while (pos != nextpos)
	    {
	      /* Otherwise there is no point to that.  Just go in batches. */
	      int chunk = min (nextpos - pos, A_WRITE_BATCH_SIZE);
	      
	      chunk = Lstream_read (instr, largebuf, chunk);
	      if (chunk < 0)
		return -1;
	      if (chunk == 0) /* EOF */
		break;
	      if (Lstream_write (outstr, largebuf, chunk) < chunk)
		return -1;
	      pos += chunk;
	    }
	}
      if (pos == nextpos)
	{
	  tem = Fcdr (Fcar (*annot));
	  if (STRINGP (tem))
	    {
	      if (Lstream_write (outstr, XSTRING_DATA (tem),
				 XSTRING_LENGTH (tem)) < 0)
		return -1;
	    }
	  *annot = Fcdr (*annot);
	}
      else
	return 0;
    }
  return -1;
}



#if 0
#include <des_crypt.h>

#define CRYPT_BLOCK_SIZE 8	/* bytes */
#define CRYPT_KEY_SIZE 8	/* bytes */

DEFUN ("encrypt-string", Fencrypt_string, 2, 2, 0, /*
Encrypt STRING using KEY.
*/
       (string, key))
{
  char *encrypted_string, *raw_key;
  int rounded_size, extra, key_size;

  /* !!#### May produce bogus data under Mule. */
  CHECK_STRING (string);
  CHECK_STRING (key);

  extra = XSTRING_LENGTH (string) % CRYPT_BLOCK_SIZE;
  rounded_size = XSTRING_LENGTH (string) + extra;
  encrypted_string = alloca (rounded_size + 1);
  memcpy (encrypted_string, XSTRING_DATA (string), XSTRING_LENGTH (string));
  memset (encrypted_string + rounded_size - extra, 0, extra + 1);

  if (XSTRING_LENGTH (key) > CRYPT_KEY_SIZE)
    key_size = CRYPT_KEY_SIZE;
  else
    key_size = XSTRING_LENGTH (key);

  raw_key = alloca (CRYPT_KEY_SIZE + 1);
  memcpy (raw_key, XSTRING_DATA (key), key_size);
  memset (raw_key + key_size, 0, (CRYPT_KEY_SIZE + 1) - key_size);

  (void) ecb_crypt (raw_key, encrypted_string, rounded_size,
		    DES_ENCRYPT | DES_SW);
  return make_string (encrypted_string, rounded_size);
}

DEFUN ("decrypt-string", Fdecrypt_string, 2, 2, 0, /*
Decrypt STRING using KEY.
*/
       (string, key))
{
  char *decrypted_string, *raw_key;
  int string_size, key_size;

  CHECK_STRING (string);
  CHECK_STRING (key);

  string_size = XSTRING_LENGTH (string) + 1;
  decrypted_string = alloca (string_size);
  memcpy (decrypted_string, XSTRING_DATA (string), string_size);
  decrypted_string[string_size - 1] = '\0';

  if (XSTRING_LENGTH (key) > CRYPT_KEY_SIZE)
    key_size = CRYPT_KEY_SIZE;
  else
    key_size = XSTRING_LENGTH (key);

  raw_key = alloca (CRYPT_KEY_SIZE + 1);
  memcpy (raw_key, XSTRING_DATA (key), key_size);
  memset (raw_key + key_size, 0, (CRYPT_KEY_SIZE + 1) - key_size);


  (void) ecb_crypt (raw_key, decrypted_string, string_size,
		    DES_DECRYPT | DES_SW);
  return make_string (decrypted_string, string_size - 1);
}
#endif


DEFUN ("verify-visited-file-modtime", Fverify_visited_file_modtime, 1, 1, 0, /*
Return t if last mod time of BUF's visited file matches what BUF records.
This means that the file has not been changed since it was visited or saved.
*/
       (buf))
{
  /* This function can GC */
  struct buffer *b;
  struct stat st;
  Lisp_Object handler;

  CHECK_BUFFER (buf);
  b = XBUFFER (buf);

  if (!STRINGP (b->filename)) return Qt;
  if (b->modtime == 0) return Qt;

  /* If the file name has special constructs in it,
     call the corresponding file handler.  */
  handler = Ffind_file_name_handler (b->filename,
                                     Qverify_visited_file_modtime);
  if (!NILP (handler))
    return call2 (handler, Qverify_visited_file_modtime, buf);

  if (stat ((char *) XSTRING_DATA (b->filename), &st) < 0)
    {
      /* If the file doesn't exist now and didn't exist before,
	 we say that it isn't modified, provided the error is a tame one.  */
      if (errno == ENOENT || errno == EACCES || errno == ENOTDIR)
	st.st_mtime = -1;
      else
	st.st_mtime = 0;
    }
  if (st.st_mtime == b->modtime
      /* If both are positive, accept them if they are off by one second.  */
      || (st.st_mtime > 0 && b->modtime > 0
	  && (st.st_mtime == b->modtime + 1
	      || st.st_mtime == b->modtime - 1)))
    return Qt;
  return Qnil;
}

DEFUN ("clear-visited-file-modtime", Fclear_visited_file_modtime, 0, 0, 0, /*
Clear out records of last mod time of visited file.
Next attempt to save will certainly not complain of a discrepancy.
*/
       ())
{
  current_buffer->modtime = 0;
  return Qnil;
}

DEFUN ("visited-file-modtime", Fvisited_file_modtime, 0, 0, 0, /*
Return the current buffer's recorded visited file modification time.
The value is a list of the form (HIGH . LOW), like the time values
that `file-attributes' returns.
*/
       ())
{
  return time_to_lisp ((time_t) current_buffer->modtime);
}

DEFUN ("set-visited-file-modtime", Fset_visited_file_modtime, 0, 1, 0, /*
Update buffer's recorded modification time from the visited file's time.
Useful if the buffer was not read from the file normally
or if the file itself has been changed for some known benign reason.
An argument specifies the modification time value to use
(instead of that of the visited file), in the form of a list
(HIGH . LOW) or (HIGH LOW).
*/
       (time_list))
{
  /* This function can GC */
  if (!NILP (time_list))
    {
      time_t the_time;
      lisp_to_time (time_list, &the_time);
      current_buffer->modtime = (int) the_time;
    }
  else
    {
      Lisp_Object filename;
      struct stat st;
      Lisp_Object handler;
      struct gcpro gcpro1, gcpro2;
      
      GCPRO2 (filename, time_list);
      filename = Fexpand_file_name (current_buffer->filename,
				    Qnil);

      /* If the file name has special constructs in it,
	 call the corresponding file handler.  */
      handler = Ffind_file_name_handler (filename, Qset_visited_file_modtime);
      UNGCPRO;
      if (!NILP (handler))
	/* The handler can find the file name the same way we did.  */
	return call2 (handler, Qset_visited_file_modtime, Qnil);
      else if (stat ((char *) XSTRING_DATA (filename), &st) >= 0)
	current_buffer->modtime = st.st_mtime;
    }

  return Qnil;
}

DEFUN ("set-buffer-modtime", Fset_buffer_modtime, 1, 2, 0, /*
Update BUFFER's recorded modification time from the associated 
file's modtime, if there is an associated file. If not, use the 
current time. In either case, if the optional arg TIME is supplied,
it will be used if it is either an integer or a cons of two integers.
*/
       (buf, in_time))
{
  /* This function can GC */
  unsigned long time_to_use = 0;
  int set_time_to_use = 0;
  struct stat st;

  CHECK_BUFFER (buf);

  if (!NILP (in_time))
    {
      if (INTP (in_time))
        {
          time_to_use = XINT (in_time);
          set_time_to_use = 1;
        }
      else if ((CONSP (in_time)) &&
               (INTP (Fcar (in_time))) && 
               (INTP (Fcdr (in_time))))
	{
	  time_t the_time;
	  lisp_to_time (in_time, &the_time);
	  time_to_use = (unsigned long) the_time;
          set_time_to_use = 1;
        }
    }

  if (!set_time_to_use)
    {
      Lisp_Object filename = Qnil;
      struct gcpro gcpro1, gcpro2;
      GCPRO2 (buf, filename);

      if (STRINGP (XBUFFER (buf)->filename))
        filename = Fexpand_file_name (XBUFFER (buf)->filename,
				      Qnil);
      else
        filename = Qnil;
  
      UNGCPRO;

      if (!NILP (filename) && !NILP (Ffile_exists_p (filename)))
        {
	  Lisp_Object handler;

	  /* If the file name has special constructs in it,
	     call the corresponding file handler.  */
	  GCPRO1 (filename);
	  handler = Ffind_file_name_handler (filename, Qset_buffer_modtime);
	  UNGCPRO;
	  if (!NILP (handler))
	    /* The handler can find the file name the same way we did. */
	    return (call2 (handler, Qset_buffer_modtime, Qnil));
	  else
	    {
	      if (stat ((char *) XSTRING_DATA (filename), &st) >= 0)
		time_to_use = st.st_mtime;
	      else
		time_to_use = time ((time_t *) 0);
	    }
        }
      else
	time_to_use = time ((time_t *) 0);
    }

  XBUFFER (buf)->modtime = time_to_use;

  return Qnil;
}


static Lisp_Object
auto_save_error (Lisp_Object condition_object, Lisp_Object ignored)
{
  /* This function can GC */
  if (gc_in_progress)
    return Qnil;
  clear_echo_area (selected_frame (), Qauto_saving, 1);
  Fding (Qt, Qauto_save_error, Qnil);
  message ("Auto-saving...error for %s", XSTRING_DATA (current_buffer->name));
  Fsleep_for (make_int (1));
  message ("Auto-saving...error!for %s", XSTRING_DATA (current_buffer->name));
  Fsleep_for (make_int (1));
  message ("Auto-saving...error for %s", XSTRING_DATA (current_buffer->name));
  Fsleep_for (make_int (1));
  return Qnil;
}

static Lisp_Object
auto_save_1 (Lisp_Object ignored)
{
  /* This function can GC */
  struct stat st;
  Lisp_Object fn = current_buffer->filename;
  Lisp_Object a  = current_buffer->auto_save_file_name;

  if (!STRINGP (a))
    return (Qnil);

  /* Get visited file's mode to become the auto save file's mode.  */
  if (STRINGP (fn) &&
      stat ((char *) XSTRING_DATA (fn), &st) >= 0)
    /* But make sure we can overwrite it later!  */
    auto_save_mode_bits = st.st_mode | 0600;
  else
    /* default mode for auto-save files of buffers with no file is
       readable by owner only.  This may annoy some small number of
       people, but the alternative removes all privacy from email. */
    auto_save_mode_bits = 0600;

  return
    /* !!#### need to deal with this 'escape-quoted everywhere */
    Fwrite_region_internal (Qnil, Qnil, a, Qnil, Qlambda, Qnil,
#ifdef MULE
			    Qescape_quoted
#else
			    Qnil
#endif
			    );
}


static Lisp_Object
do_auto_save_unwind (Lisp_Object fd)
{
  close (XINT (fd));
  return (fd);
}

static Lisp_Object
do_auto_save_unwind_2 (Lisp_Object old_auto_saving)
{
  auto_saving = XINT (old_auto_saving);
  return Qnil;
}

/* Fdo_auto_save() checks whether a GC is in progress when it is called,
   and if so, tries to avoid touching lisp objects.

   The only time that Fdo_auto_save() is called while GC is in progress
   is if we're going down, as a result of an abort() or a kill signal.
   It's fairly important that we generate autosave files in that case!
 */

DEFUN ("do-auto-save", Fdo_auto_save, 0, 2, "", /*
Auto-save all buffers that need it.
This is all buffers that have auto-saving enabled
and are changed since last auto-saved.
Auto-saving writes the buffer into a file
so that your editing is not lost if the system crashes.
This file is not the file you visited; that changes only when you save.
Normally we run the normal hook `auto-save-hook' before saving.

Non-nil first argument means do not print any message if successful.
Non-nil second argument means save only current buffer.
*/
       (no_message, current_only))
{
  /* This function can GC */
  struct buffer *old = current_buffer, *b;
  Lisp_Object tail, buf;
  int auto_saved = 0;
  int do_handled_files;
  Lisp_Object oquit = Qnil;
  Lisp_Object listfile = Qnil;
  int listdesc = -1;
  int speccount = specpdl_depth ();
  struct gcpro gcpro1, gcpro2;

  GCPRO2 (oquit, listfile);
  check_quit (); /* make Vquit_flag accurate */
  /* Ordinarily don't quit within this function,
     but don't make it impossible to quit (in case we get hung in I/O).  */
  oquit = Vquit_flag;
  Vquit_flag = Qnil;

  /* No further GCPRO needed, because (when it matters) all Lisp_Object
     variables point to non-strings reached from Vbuffer_alist.  */

  if (minibuf_level != 0 || preparing_for_armageddon)
    no_message = Qt;

  run_hook (Qauto_save_hook);

  if (GC_STRINGP (Vauto_save_list_file_name))
    listfile = Fexpand_file_name (Vauto_save_list_file_name, Qnil);

  /* Make sure auto_saving is reset. */
  record_unwind_protect (do_auto_save_unwind_2, make_int (auto_saving));

  auto_saving = 1;

  /* First, save all files which don't have handlers.  If Emacs is
     crashing, the handlers may tweak what is causing Emacs to crash
     in the first place, and it would be a shame if Emacs failed to
     autosave perfectly ordinary files because it couldn't handle some
     ange-ftp'd file.  */
  for (do_handled_files = 0; do_handled_files < 2; do_handled_files++)
    {
      for (tail = Vbuffer_alist;
	   GC_CONSP (tail);
	   tail = XCDR (tail))
	{
	  buf = XCDR (XCAR (tail));
	  b = XBUFFER (buf);

	  if (!GC_NILP (current_only)
	      && b != current_buffer)
	    continue;
      
	  /* Don't auto-save indirect buffers.
	     The base buffer takes care of it.  */
	  if (b->base_buffer)
	    continue;

	  /* Check for auto save enabled
	     and file changed since last auto save
	     and file changed since last real save.  */
	  if (GC_STRINGP (b->auto_save_file_name)
	      && BUF_SAVE_MODIFF (b) < BUF_MODIFF (b)
	      && b->auto_save_modified < BUF_MODIFF (b)
	      /* -1 means we've turned off autosaving for a while--see below.  */
	      && XINT (b->save_length) >= 0
	      && (do_handled_files
		  || NILP (Ffind_file_name_handler (b->auto_save_file_name,
						    Qwrite_region))))
	    {
	      EMACS_TIME before_time, after_time;

	      EMACS_GET_TIME (before_time);
	      /* If we had a failure, don't try again for 20 minutes.  */
	      if (!preparing_for_armageddon
		  && b->auto_save_failure_time >= 0
		  && (EMACS_SECS (before_time) - b->auto_save_failure_time <
		      1200))
		continue;

	      if (!preparing_for_armageddon &&
		  (XINT (b->save_length) * 10
		   > (BUF_Z (b) - BUF_BEG (b)) * 13)
		  /* A short file is likely to change a large fraction;
		     spare the user annoying messages.  */
		  && XINT (b->save_length) > 5000
		  /* These messages are frequent and annoying for `*mail*'.  */
		  && !EQ (b->filename, Qnil)
		  && NILP (no_message)
		  && disable_auto_save_when_buffer_shrinks)
		{
		  /* It has shrunk too much; turn off auto-saving here.
		     Unless we're about to crash, in which case auto-save it
		     anyway.
		     */
		  message
		    ("Buffer %s has shrunk a lot; auto save turned off there",
		     XSTRING_DATA (b->name));
		  /* Turn off auto-saving until there's a real save,
		     and prevent any more warnings.  */
		  b->save_length = make_int (-1);
		  if (!gc_in_progress)
		    Fsleep_for (make_int (1));
		  continue;
		}
	      set_buffer_internal (b);
	      if (!auto_saved && GC_NILP (no_message))
		{
		  static CONST unsigned char *msg
		    = (CONST unsigned char *) "Auto-saving...";
		  echo_area_message (selected_frame (), msg, Qnil,
				     0, strlen ((CONST char *) msg),
				     Qauto_saving);
		}

	      /* Open the auto-save list file, if necessary.
		 We only do this now so that the file only exists
		 if we actually auto-saved any files. */
	      if (!auto_saved && GC_STRINGP (listfile) && listdesc < 0)
		{
#ifdef DOS_NT
		  listdesc = open ((char *) XSTRING_DATA (listfile),
				   O_WRONLY | O_TRUNC | O_CREAT | O_TEXT,
				   S_IREAD | S_IWRITE);
#else /* not DOS_NT */
		  listdesc = creat ((char *) XSTRING_DATA (listfile), 0666);
#endif /* not DOS_NT */

		  /* Arrange to close that file whether or not we get
		     an error. */
		  if (listdesc >= 0)
		    record_unwind_protect (do_auto_save_unwind,
					   make_int (listdesc));
		}

	      /* Record all the buffers that we are auto-saving in
		 the special file that lists them.  For each of
		 these buffers, record visited name (if any) and
		 auto save name.  */
	      if (listdesc >= 0)
		{
		  Extbyte *auto_save_file_name_ext;
		  Extcount auto_save_file_name_ext_len;
		  
		  GET_STRING_FILENAME_DATA_ALLOCA
		    (b->auto_save_file_name,
		     auto_save_file_name_ext,
		     auto_save_file_name_ext_len);
		  if (!NILP (b->filename))
		    {
		      Extbyte *filename_ext;
		      Extcount filename_ext_len;
		      
		      GET_STRING_FILENAME_DATA_ALLOCA (b->filename,
						       filename_ext,
						       filename_ext_len);
		      write (listdesc, filename_ext, filename_ext_len);
		    }
		  write (listdesc, "\n", 1);
		  write (listdesc, auto_save_file_name_ext,
			 auto_save_file_name_ext_len);
		  write (listdesc, "\n", 1);
		}

	      condition_case_1 (Qt,
				auto_save_1, Qnil,
				auto_save_error, Qnil);
	      auto_saved++;
	      b->auto_save_modified = BUF_MODIFF (b);
	      b->save_length = make_int (BUF_SIZE (b));
	      set_buffer_internal (old);

	      EMACS_GET_TIME (after_time);
	      /* If auto-save took more than 60 seconds,
		 assume it was an NFS failure that got a timeout.  */
	      if (EMACS_SECS (after_time) - EMACS_SECS (before_time) > 60)
		b->auto_save_failure_time = EMACS_SECS (after_time);
	    }
	}
    }

  /* Prevent another auto save till enough input events come in.  */
  if (auto_saved)
    record_auto_save ();

  /* If we didn't save anything into the listfile, remove the old
     one because nothing needed to be auto-saved.  Do this afterwards
     rather than before in case we get a crash attempting to autosave
     (in that case we'd still want the old one around). */
  if (listdesc < 0 && !auto_saved && GC_STRINGP (listfile))
    unlink ((char *) XSTRING_DATA (listfile));

  /* Show "...done" only if the echo area would otherwise be empty. */
  if (auto_saved && NILP (no_message)
      && NILP (clear_echo_area (selected_frame (), Qauto_saving, 0)))
    {
      static CONST unsigned char *msg
        = (CONST unsigned char *)"Auto-saving...done";
      echo_area_message (selected_frame (), msg, Qnil, 0,
			 strlen ((CONST char *) msg), Qauto_saving);
    }

  Vquit_flag = oquit;

  RETURN_UNGCPRO (unbind_to (speccount, Qnil));
}

DEFUN ("set-buffer-auto-saved", Fset_buffer_auto_saved, 0, 0, 0, /*
Mark current buffer as auto-saved with its current text.
No auto-save file will be written until the buffer changes again.
*/
       ())
{
  current_buffer->auto_save_modified = BUF_MODIFF (current_buffer);
  current_buffer->save_length = make_int (BUF_SIZE (current_buffer));
  current_buffer->auto_save_failure_time = -1;
  return Qnil;
}

DEFUN ("clear-buffer-auto-save-failure", Fclear_buffer_auto_save_failure, 0, 0, 0, /*
Clear any record of a recent auto-save failure in the current buffer.
*/
       ())
{
  current_buffer->auto_save_failure_time = -1;
  return Qnil;
}

DEFUN ("recent-auto-save-p", Frecent_auto_save_p, 0, 0, 0, /*
Return t if buffer has been auto-saved since last read in or saved.
*/
       ())
{
  return (BUF_SAVE_MODIFF (current_buffer) <
	  current_buffer->auto_save_modified) ? Qt : Qnil;
}


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_fileio (void)
{
  defsymbol (&Qexpand_file_name, "expand-file-name");
  defsymbol (&Qfile_truename, "file-truename");
  defsymbol (&Qsubstitute_in_file_name, "substitute-in-file-name");
  defsymbol (&Qdirectory_file_name, "directory-file-name");
  defsymbol (&Qfile_name_directory, "file-name-directory");
  defsymbol (&Qfile_name_nondirectory, "file-name-nondirectory");
  defsymbol (&Qunhandled_file_name_directory, "unhandled-file-name-directory");
  defsymbol (&Qfile_name_as_directory, "file-name-as-directory");
  defsymbol (&Qcopy_file, "copy-file");
  defsymbol (&Qmake_directory_internal, "make-directory-internal");
  defsymbol (&Qdelete_directory, "delete-directory");
  defsymbol (&Qdelete_file, "delete-file");
  defsymbol (&Qrename_file, "rename-file");
  defsymbol (&Qadd_name_to_file, "add-name-to-file");
  defsymbol (&Qmake_symbolic_link, "make-symbolic-link");
  defsymbol (&Qfile_exists_p, "file-exists-p");
  defsymbol (&Qfile_executable_p, "file-executable-p");
  defsymbol (&Qfile_readable_p, "file-readable-p");
  defsymbol (&Qfile_symlink_p, "file-symlink-p");
  defsymbol (&Qfile_writable_p, "file-writable-p");
  defsymbol (&Qfile_directory_p, "file-directory-p");
  defsymbol (&Qfile_regular_p, "file-regular-p");
  defsymbol (&Qfile_accessible_directory_p, "file-accessible-directory-p");
  defsymbol (&Qfile_modes, "file-modes");
  defsymbol (&Qset_file_modes, "set-file-modes");
  defsymbol (&Qfile_newer_than_file_p, "file-newer-than-file-p");
  defsymbol (&Qinsert_file_contents, "insert-file-contents");
  defsymbol (&Qwrite_region, "write-region");
  defsymbol (&Qverify_visited_file_modtime, "verify-visited-file-modtime");
  defsymbol (&Qset_visited_file_modtime, "set-visited-file-modtime");
  defsymbol (&Qset_buffer_modtime, "set-buffer-modtime");
#ifdef DOS_NT
  defsymbol (&Qfind_buffer_file_type, "find-buffer-file-type");
#endif /* DOS_NT */
  defsymbol (&Qcar_less_than_car, "car-less-than-car"); /* Vomitous! */

  defsymbol (&Qfile_name_handler_alist, "file-name-handler-alist");
  defsymbol (&Qauto_save_hook, "auto-save-hook");
  defsymbol (&Qauto_save_error, "auto-save-error");
  defsymbol (&Qauto_saving, "auto-saving");

  defsymbol (&Qformat_decode, "format-decode");
  defsymbol (&Qformat_annotate_function, "format-annotate-function");

  defsymbol (&Qcompute_buffer_file_truename, "compute-buffer-file-truename");
  deferror (&Qfile_error, "file-error", "File error", Qio_error);
  deferror (&Qfile_already_exists, "file-already-exists",
	    "File already exists", Qfile_error);

  DEFSUBR (Ffind_file_name_handler);

  DEFSUBR (Ffile_name_directory);
  DEFSUBR (Ffile_name_nondirectory);
  DEFSUBR (Funhandled_file_name_directory);
  DEFSUBR (Ffile_name_as_directory);
  DEFSUBR (Fdirectory_file_name);
  DEFSUBR (Fmake_temp_name);
  DEFSUBR (Fexpand_file_name);
  DEFSUBR (Ffile_truename);
  DEFSUBR (Fsubstitute_in_file_name);
  DEFSUBR (Fcopy_file);
  DEFSUBR (Fmake_directory_internal);
  DEFSUBR (Fdelete_directory);
  DEFSUBR (Fdelete_file);
  DEFSUBR (Frename_file);
  DEFSUBR (Fadd_name_to_file);
#ifdef S_IFLNK
  DEFSUBR (Fmake_symbolic_link);
#endif /* S_IFLNK */
#ifdef VMS
  DEFSUBR (Fdefine_logical_name);
#endif /* VMS */
#ifdef HPUX_NET
  DEFSUBR (Fsysnetunam);
#endif /* HPUX_NET */
  DEFSUBR (Ffile_name_absolute_p);
  DEFSUBR (Ffile_exists_p);
  DEFSUBR (Ffile_executable_p);
  DEFSUBR (Ffile_readable_p);
  DEFSUBR (Ffile_writable_p);
  DEFSUBR (Ffile_symlink_p);
  DEFSUBR (Ffile_directory_p);
  DEFSUBR (Ffile_accessible_directory_p);
  DEFSUBR (Ffile_regular_p);
  DEFSUBR (Ffile_modes);
  DEFSUBR (Fset_file_modes);
  DEFSUBR (Fset_default_file_modes);
  DEFSUBR (Fdefault_file_modes);
  DEFSUBR (Funix_sync);
  DEFSUBR (Ffile_newer_than_file_p);
  DEFSUBR (Finsert_file_contents_internal);
  DEFSUBR (Fwrite_region_internal);
  DEFSUBR (Fcar_less_than_car); /* Vomitous! */
  DEFSUBR (Fcdr_less_than_cdr); /* Yeah oh yeah bucko .... */
#if 0
  DEFSUBR (Fencrypt_string);
  DEFSUBR (Fdecrypt_string);
#endif
  DEFSUBR (Fverify_visited_file_modtime);
  DEFSUBR (Fclear_visited_file_modtime);
  DEFSUBR (Fvisited_file_modtime);
  DEFSUBR (Fset_visited_file_modtime);
  DEFSUBR (Fset_buffer_modtime);

  DEFSUBR (Fdo_auto_save);
  DEFSUBR (Fset_buffer_auto_saved);
  DEFSUBR (Fclear_buffer_auto_save_failure);
  DEFSUBR (Frecent_auto_save_p);
}

void
vars_of_fileio (void)
{
  DEFVAR_LISP ("auto-save-file-format", &Vauto_save_file_format /*
*Format in which to write auto-save files.
Should be a list of symbols naming formats that are defined in `format-alist'.
If it is t, which is the default, auto-save files are written in the
same format as a regular save would use.
*/ );
  Vauto_save_file_format = Qt;

  DEFVAR_BOOL ("vms-stmlf-recfm", &vms_stmlf_recfm /*
*Non-nil means write new files with record format `stmlf'.
nil means use format `var'.  This variable is meaningful only on VMS.
*/ );
  vms_stmlf_recfm = 0;

  DEFVAR_LISP ("file-name-handler-alist", &Vfile_name_handler_alist /*
*Alist of elements (REGEXP . HANDLER) for file names handled specially.
If a file name matches REGEXP, then all I/O on that file is done by calling
HANDLER.

The first argument given to HANDLER is the name of the I/O primitive
to be handled; the remaining arguments are the arguments that were
passed to that primitive.  For example, if you do
    (file-exists-p FILENAME)
and FILENAME is handled by HANDLER, then HANDLER is called like this:
    (funcall HANDLER 'file-exists-p FILENAME)
The function `find-file-name-handler' checks this list for a handler
for its argument.
*/ );
  Vfile_name_handler_alist = Qnil;

  DEFVAR_LISP ("after-insert-file-functions", &Vafter_insert_file_functions /*
A list of functions to be called at the end of `insert-file-contents'.
Each is passed one argument, the number of bytes inserted.  It should return
the new byte count, and leave point the same.  If `insert-file-contents' is
intercepted by a handler from `file-name-handler-alist', that handler is
responsible for calling the after-insert-file-functions if appropriate.
*/ );
  Vafter_insert_file_functions = Qnil;

  DEFVAR_LISP ("write-region-annotate-functions",
	       &Vwrite_region_annotate_functions /*
A list of functions to be called at the start of `write-region'.
Each is passed two arguments, START and END as for `write-region'.
It should return a list of pairs (POSITION . STRING) of strings to be
effectively inserted at the specified positions of the file being written
\(1 means to insert before the first byte written).  The POSITIONs must be
sorted into increasing order.  If there are several functions in the list,
the several lists are merged destructively.
*/ );
  Vwrite_region_annotate_functions = Qnil;

  DEFVAR_LISP ("write-region-annotations-so-far",
	       &Vwrite_region_annotations_so_far /*
When an annotation function is called, this holds the previous annotations.
These are the annotations made by other annotation functions
that were already called.  See also `write-region-annotate-functions'.
*/ );
  Vwrite_region_annotations_so_far = Qnil;

  DEFVAR_LISP ("inhibit-file-name-handlers", &Vinhibit_file_name_handlers /*
A list of file name handlers that temporarily should not be used.
This applies only to the operation `inhibit-file-name-operation'.
*/ );
  Vinhibit_file_name_handlers = Qnil;

  DEFVAR_LISP ("inhibit-file-name-operation", &Vinhibit_file_name_operation /*
The operation for which `inhibit-file-name-handlers' is applicable.
*/ );
  Vinhibit_file_name_operation = Qnil;

  DEFVAR_LISP ("auto-save-list-file-name", &Vauto_save_list_file_name /*
File name in which we write a list of all auto save file names.
*/ );
  Vauto_save_list_file_name = Qnil;

  DEFVAR_BOOL ("disable-auto-save-when-buffer-shrinks",
	       &disable_auto_save_when_buffer_shrinks /*
If non-nil, auto-saving is disabled when a buffer shrinks too much.
This is to prevent you from losing your edits if you accidentally
delete a large chunk of the buffer and don't notice it until too late.
Saving the buffer normally turns auto-save back on.
*/ );
  disable_auto_save_when_buffer_shrinks = 1;
}
