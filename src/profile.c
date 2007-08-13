/* Why the hell is XEmacs so fucking slow?
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

#include <config.h>
#include "lisp.h"

#include "backtrace.h"
#include "bytecode.h"
#include "hash.h"

#include "syssignal.h"
#include "systime.h"

/*

We implement our own profiling scheme so that we can determine things
like which Lisp functions are occupying the most time.  Any standard
OS-provided profiling works on C functions, which is somewhat useless.

The basic idea is simple.  We set a profiling timer using
setitimer (ITIMER_PROF), which generates a SIGPROF every so often.
\(This runs not in real time but rather when the process is executing
or the system is running on behalf of the process.) When the signal
goes off, we see what we're in, and add by 1 the count associated with
that function.

It would be nice to use the Lisp allocation mechanism etc. to keep
track of the profiling information, but we can't because that's not
safe, and trying to make it safe would be much more work than is
worth.

*/

c_hashtable big_profile_table;

int default_profiling_interval;

int profiling_active;

/* The normal flag in_display is used as a critical-section flag
   and is not set the whole time we're in redisplay. */
int profiling_redisplay_flag;

Lisp_Object QSin_redisplay;
Lisp_Object QSin_garbage_collection;
Lisp_Object QSprocessing_events_at_top_level;
Lisp_Object QSunknown;

/* We use inside_profiling to prevent the handler from writing to
   the table while another routine is operating on it.  We also set
   inside_profiling in case the timeout between signal calls is short
   enough to catch us while we're already in there. */
static volatile int inside_profiling;

static SIGTYPE
sigprof_handler (int signo)
{
  /* Don't do anything if we are shutting down, or are doing a maphash
     or clrhash on the table. */
  if (!inside_profiling && !preparing_for_armageddon)
    {
      Lisp_Object fun;

      /* If something below causes an error to be signaled, we'll
	 not correctly reset this flag.  But we'll be in worse shape
	 than that anyways, since we'll longjmp back to the last
	 condition case. */
      inside_profiling = 1;

      if (profiling_redisplay_flag)
	fun = QSin_redisplay;
      else if (gc_in_progress)
	fun = QSin_garbage_collection;
      else if (backtrace_list)
	{
	  fun = *backtrace_list->function;

	  /* #### dmoore - why do we need to unmark it, we aren't in GC. */
	  XUNMARK (fun);
	  if (!GC_SYMBOLP (fun) && !GC_COMPILED_FUNCTIONP (fun))
	    fun = QSunknown;
	}
      else
	fun = QSprocessing_events_at_top_level;

      {
	/* #### see comment about memory allocation in start-profiling.
	   Allocating memory in a signal handler is BAD BAD BAD.
	   If you are using the non-mmap rel-alloc code, you might
	   lose because of this.  Even worse, if the memory allocation 
	   fails, the `error' generated whacks everything hard. */
	long count;
	CONST void *vval;
    
	if (gethash (LISP_TO_VOID (fun), big_profile_table, &vval))
	  count = (long) vval;
	else
	  count = 0;
	count++;
	vval = (CONST void *) count;
	puthash (LISP_TO_VOID (fun), (void *) vval, big_profile_table);
      }
      
      inside_profiling = 0;
    }
}

DEFUN ("start-profiling", Fstart_profiling, 0, 1, 0, /*
Start profiling, with profile queries every MICROSECS.
If MICROSECS is nil or omitted, the value of `default-profiling-interval'
is used.

You can retrieve the recorded profiling info using `get-profiling-info'.

Starting and stopping profiling does not clear the currently recorded
info.  Thus you can start and stop as many times as you want and everything
will be properly accumulated.
*/
       (microsecs))
{
  /* This function can GC */
  int msecs;
  struct itimerval foo;

  /* #### The hash code can safely be called from a signal handler
     except when it has to grow the hashtable.  In this case, it calls
     realloc(), which is not (in general) re-entrant.  We just be
     sleazy and make the table large enough that it (hopefully) won't
     need to be realloc()ed. */
  if (!big_profile_table)
    big_profile_table = make_hashtable (10000);
  if (NILP (microsecs))
    msecs = default_profiling_interval;
  else
    {
      CHECK_NATNUM (microsecs);
      msecs = XINT (microsecs);
    }
  if (msecs <= 0)
    msecs = 1000;

  signal (SIGPROF, sigprof_handler);
  foo.it_value.tv_sec = 0;
  foo.it_value.tv_usec = msecs;
  EMACS_NORMALIZE_TIME (foo.it_value);
  foo.it_interval = foo.it_value;
  profiling_active = 1;
  inside_profiling = 0;
  setitimer (ITIMER_PROF, &foo, 0);
  return Qnil;
}

DEFUN ("stop-profiling", Fstop_profiling, 0, 0, 0, /*
Stop profiling.
*/
       ())
{
  /* This function does not GC */
  struct itimerval foo;

  foo.it_value.tv_sec = 0;
  foo.it_value.tv_usec = 0;
  foo.it_interval = foo.it_value;
  setitimer (ITIMER_PROF, &foo, 0);
  profiling_active = 0;
  signal (SIGPROF, fatal_error_signal);
  return Qnil;
}

static Lisp_Object
profile_lock_unwind (Lisp_Object ignore)
{
  inside_profiling = 0;	
  return Qnil;
}

struct get_profiling_info_closure
{
  Lisp_Object accum;
};

static void
get_profiling_info_maphash (CONST void *void_key,
			    void *void_val,
			    void *void_closure)
{
  /* This function does not GC */
  Lisp_Object key;
  struct get_profiling_info_closure *closure = void_closure;
  EMACS_INT val;

  CVOID_TO_LISP (key, void_key);
  val = (EMACS_INT) void_val;

  closure->accum = Fcons (Fcons (key, make_int (val)),
			  closure->accum);
}

DEFUN ("get-profiling-info", Fget_profiling_info, 0, 0, 0, /*
Return the profiling info as an alist.
*/
       ())
{
  /* This function does not GC */
  struct get_profiling_info_closure closure;

  closure.accum = Qnil;
  if (big_profile_table)
    {
      int count = specpdl_depth ();
      record_unwind_protect (profile_lock_unwind, Qnil);
      inside_profiling = 1;
      maphash (get_profiling_info_maphash, big_profile_table, &closure);
      unbind_to (count, Qnil);
    }
  return closure.accum;
}

struct mark_profiling_info_closure
{
  void (*markfun) (Lisp_Object);
};

static void
mark_profiling_info_maphash (CONST void *void_key,
			     void *void_val,
			     void *void_closure)
{
  Lisp_Object key;
  struct mark_profiling_info_closure *closure = void_closure;

  CVOID_TO_LISP (key, void_key);
  (closure->markfun) (key);
}

void
mark_profiling_info (void (*markfun) (Lisp_Object))
{
  /* This function does not GC (if markfun doesn't) */
  struct mark_profiling_info_closure closure;

  closure.markfun = markfun;
  if (big_profile_table)
    {
      inside_profiling = 1;
      maphash (mark_profiling_info_maphash, big_profile_table, &closure);
      inside_profiling = 0;
    }
}

DEFUN ("clear-profiling-info", Fclear_profiling_info, 0, 0, 0, /*
Clear out the recorded profiling info.
*/
       ())
{
  /* This function does not GC */
  if (big_profile_table)
    {
      inside_profiling = 1;
      clrhash (big_profile_table);
      inside_profiling = 0;
    }
  return Qnil;
}

DEFUN ("profiling-active-p", Fprofiling_active_p, 0, 0, 0, /*
Return non-nil if profiling information is currently being recorded.
*/
       ())
{
  return profiling_active ? Qt : Qnil;
}

void
syms_of_profile (void)
{
  DEFSUBR (Fstart_profiling);
  DEFSUBR (Fstop_profiling);
  DEFSUBR (Fget_profiling_info);
  DEFSUBR (Fclear_profiling_info);
  DEFSUBR (Fprofiling_active_p);
}

void
vars_of_profile (void)
{
  DEFVAR_INT ("default-profiling-interval", &default_profiling_interval /*
Default time in microseconds between profiling queries.
Used when the argument to `start-profiling' is nil or omitted.
Note that the time in question is CPU time (when the program is executing
or the kernel is executing on behalf of the program) and not real time.
*/ );
  default_profiling_interval = 1000;

  inside_profiling = 0;

  QSin_redisplay = build_string ("(in redisplay)");
  staticpro (&QSin_redisplay);
  QSin_garbage_collection = build_string ("(in garbage collection)");
  staticpro (&QSin_garbage_collection);
  QSunknown = build_string ("(unknown)");
  staticpro (&QSunknown);
  QSprocessing_events_at_top_level =
    build_string ("(processing events at top level)");
  staticpro (&QSprocessing_events_at_top_level);
}
