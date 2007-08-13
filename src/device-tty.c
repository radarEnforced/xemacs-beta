/* TTY device functions.
   Copyright (C) 1994, 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1994, 1995 Free Software Foundation, Inc.
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

/* Synched up with: Not in FSF. */

/* Authors: Ben Wing and Chuck Thompson. */

#include <config.h>
#include "lisp.h"

#include "console-tty.h"
#include "console-stream.h"
#include "events.h"
#include "faces.h"
#include "frame.h"
#include "lstream.h"
#include "redisplay.h"
#include "sysdep.h"

#include "syssignal.h" /* for SIGWINCH */

#ifdef HAVE_GPM
#include <gpm.h>
#endif

#include <errno.h>

Lisp_Object Qinit_pre_tty_win, Qinit_post_tty_win;


static void
allocate_tty_device_struct (struct device *d)
{
  d->device_data = xnew_and_zero (struct tty_device);
}

static void
tty_init_device (struct device *d, Lisp_Object props)
{
  struct console *con = XCONSOLE (DEVICE_CONSOLE (d));
  Lisp_Object terminal_type = CONSOLE_TTY_DATA (con)->terminal_type;

  DEVICE_INFD (d) = CONSOLE_TTY_DATA (con)->infd;
  DEVICE_OUTFD (d) = CONSOLE_TTY_DATA (con)->outfd;

  allocate_tty_device_struct (d);
  init_baud_rate (d);

  switch (init_tty_for_redisplay (d, (char *) XSTRING_DATA (terminal_type)))
    {
#if 0
    case TTY_UNABLE_OPEN_DATABASE:
      suppress_early_error_handler_backtrace = 1;
      error ("Can't access terminal information database");
      break;
#endif
    case TTY_TYPE_UNDEFINED:
      suppress_early_error_handler_backtrace = 1;
      error ("Terminal type `%s' undefined (or can't access database?)",
	     XSTRING_DATA (terminal_type));
      break;
    case TTY_TYPE_INSUFFICIENT:
      suppress_early_error_handler_backtrace = 1;
      error ("Terminal type `%s' not powerful enough to run Emacs",
	     XSTRING_DATA (terminal_type));
      break;
    case TTY_SIZE_UNSPECIFIED:
      suppress_early_error_handler_backtrace = 1;
      error ("Can't determine window size of terminal");
      break;
    case TTY_INIT_SUCCESS:
      break;
    default:
      abort ();
    }

  init_one_device (d);

  /* Run part of the elisp side of the TTY device initialization.
     The post-init is run in the tty_after_init_frame() method. */
  call0 (Qinit_pre_tty_win);
}

static void
free_tty_device_struct (struct device *d)
{
  struct tty_device *td = (struct tty_device *) d->device_data;
  if (td)
    xfree (td);
}

static void
tty_delete_device (struct device *d)
{
  free_tty_device_struct (d);
}

#ifdef SIGWINCH

static SIGTYPE
tty_device_size_change_signal (int signo)
{
  int old_errno = errno;
  asynch_device_change_pending++;
#ifdef HAVE_UNIXOID_EVENT_LOOP
  signal_fake_event ();
#endif
  EMACS_REESTABLISH_SIGNAL (SIGWINCH, tty_device_size_change_signal);
  errno = old_errno;
  SIGRETURN;
}

/* frame_change_signal does nothing but set a flag that it was called.
   When redisplay is called, it will notice that the flag is set and
   call handle_pending_device_size_change to do the actual work. */
static void
tty_asynch_device_change (void)
{
  Lisp_Object devcons, concons;

  DEVICE_LOOP_NO_BREAK (devcons, concons)
    {
      int width, height;
      Lisp_Object tail;
      struct device *d = XDEVICE (XCAR (devcons));
      struct console *con = XCONSOLE (DEVICE_CONSOLE (d));

      if (!DEVICE_TTY_P (d))
	continue;

      get_tty_device_size (d, &width, &height);
      if (width > 0 && height > 0
	  && (CONSOLE_TTY_DATA (con)->width != width
	      || CONSOLE_TTY_DATA (con)->height != height))
	{
	  CONSOLE_TTY_DATA (con)->width = width;
	  CONSOLE_TTY_DATA (con)->height = height;

#ifdef HAVE_GPM
	  /* We need to tell GPM how big our screen is now
	  ** I am pretty sure the GPM library will get incredibly confused
	  ** if you try to connect to more than one mouse-capable device,
	  ** so I don't think it will cause any more damage in that case.
	  */
	  gpm_mx = width;
	  gpm_my = height;
#endif
	  for (tail = DEVICE_FRAME_LIST (d);
	       !NILP (tail);
	       tail = XCDR (tail))
	    {
	      struct frame *f = XFRAME (XCAR (tail));

	      /* We know the frame is tty because we made sure that the
		 device is tty. */
	      change_frame_size (f, height, width, 1);
	    }
	}
    }
}

#endif /* SIGWINCH */

static Lisp_Object
tty_device_system_metrics (struct device *d,
			   enum device_metrics m)
{
  struct console *con = XCONSOLE (DEVICE_CONSOLE (d));
  switch (m)
    {
    case DM_size_device:
      return Fcons (make_int (CONSOLE_TTY_DATA (con)->width),
		    make_int (CONSOLE_TTY_DATA (con)->height));
      break;
    }

  /* Do not know such property */
  return Qunbound;
}

/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_device_tty (void)
{
  defsymbol (&Qinit_pre_tty_win, "init-pre-tty-win");
  defsymbol (&Qinit_post_tty_win, "init-post-tty-win");
}

void
console_type_create_device_tty (void)
{
  /* device methods */
  CONSOLE_HAS_METHOD (tty, init_device);
  CONSOLE_HAS_METHOD (tty, delete_device);
#ifdef SIGWINCH
  CONSOLE_HAS_METHOD (tty, asynch_device_change);
#endif /* SIGWINCH */
  CONSOLE_HAS_METHOD (tty, device_system_metrics);
}

void
init_device_tty (void)
{
#ifdef SIGWINCH
  if (initialized && !noninteractive)
    signal (SIGWINCH, tty_device_size_change_signal);
#endif /* SIGWINCH */
}
