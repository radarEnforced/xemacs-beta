/* Generic toolbar implementation.
   Copyright (C) 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1995 Sun Microsystems, Inc.
   Copyright (C) 1995, 1996 Ben Wing.
   Copyright (C) 1996 Chuck Thompson.

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

/* Original implementation by Chuck Thompson for 19.12.
   Default-toolbar-position and specifier-related stuff by Ben Wing. */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "frame.h"
#include "device.h"
#include "glyphs.h"
#include "redisplay.h"
#include "toolbar.h"
#include "window.h"

Lisp_Object Vtoolbar[4];
Lisp_Object Vtoolbar_size[4];
Lisp_Object Vtoolbar_visible_p[4];

Lisp_Object Vdefault_toolbar, Vdefault_toolbar_visible_p;
Lisp_Object Vdefault_toolbar_width, Vdefault_toolbar_height;

Lisp_Object Vdefault_toolbar_position;
Lisp_Object Vtoolbar_buttons_captioned_p;

Lisp_Object Qtoolbar_buttonp;
Lisp_Object Q2D, Q3D, Q2d, Q3d;
Lisp_Object Q_size;
extern Lisp_Object Q_style;	/* defined in menubar.c */

Lisp_Object Qinit_toolbar_from_resources;


static Lisp_Object
mark_toolbar_data (Lisp_Object obj, void (*markobj) (Lisp_Object))
{
  struct toolbar_data *data = (struct toolbar_data *) XPNTR (obj);
  ((markobj) (data->last_toolbar_buffer));
  return (data->toolbar_buttons);
}

DEFINE_LRECORD_IMPLEMENTATION ("toolbar-data", toolbar_data,
			       mark_toolbar_data, internal_object_printer,
			       0, 0, 0, struct toolbar_data);

static Lisp_Object
mark_toolbar_button (Lisp_Object obj, void (*markobj) (Lisp_Object))
{
  struct toolbar_button *data = (struct toolbar_button *) XPNTR (obj);
  ((markobj) (data->next));
  ((markobj) (data->frame));
  ((markobj) (data->up_glyph));
  ((markobj) (data->down_glyph));
  ((markobj) (data->disabled_glyph));
  ((markobj) (data->cap_up_glyph));
  ((markobj) (data->cap_down_glyph));
  ((markobj) (data->cap_disabled_glyph));
  ((markobj) (data->callback));
  ((markobj) (data->enabled_p));
  return (data->help_string);
}

static void
print_toolbar_button (Lisp_Object obj, Lisp_Object printcharfun,
		      int escapeflag)
{
  struct toolbar_button *tb = XTOOLBAR_BUTTON (obj);
  char buf[100];

  if (print_readably)
    error ("printing unreadable object #<toolbar-button 0x%x>",
	   tb->header.uid);

  sprintf (buf, "#<toolbar-button 0x%x>", tb->header.uid);
  write_c_string (buf, printcharfun);
}

DEFINE_LRECORD_IMPLEMENTATION ("toolbar-button", toolbar_button,
			       mark_toolbar_button, print_toolbar_button,
			       0, 0, 0,
			       struct toolbar_button);

DEFUN ("toolbar-button-p", Ftoolbar_button_p, 1, 1, 0, /*
Return non-nil if OBJECT is a toolbar button.
*/
       (object))
{
  return (TOOLBAR_BUTTONP (object) ? Qt : Qnil);
}

/* Only query functions are provided for toolbar buttons.  They are
   generated and updated from a toolbar description list.  Any
   directly made changes would be wiped out the first time the toolbar
   was marked as dirty and was regenerated.  The exception to this is
   set-toolbar-button-down-flag.  Having this allows us to control the
   toolbar from elisp.  Since we only trigger the button callbacks on
   up-mouse events and we reset the flag first, there shouldn't be any
   way for this to get us in trouble (like if someone decides to
   change the toolbar from a toolbar callback). */

DEFUN ("toolbar-button-callback", Ftoolbar_button_callback, 1, 1, 0, /*
Return the callback function associated with the toolbar BUTTON.
*/
       (button))
{
  CHECK_TOOLBAR_BUTTON (button);

  return (XTOOLBAR_BUTTON (button)->callback);
}

DEFUN ("toolbar-button-help-string", Ftoolbar_button_help_string, 1, 1, 0, /*
Return the help string function associated with the toolbar BUTTON.
*/
       (button))
{
  CHECK_TOOLBAR_BUTTON (button);

  return (XTOOLBAR_BUTTON (button)->help_string);
}

DEFUN ("toolbar-button-enabled-p", Ftoolbar_button_enabled_p, 1, 1, 0, /*
Return t if BUTTON is active.
*/
       (button))
{
  CHECK_TOOLBAR_BUTTON (button);

  return (XTOOLBAR_BUTTON (button)->enabled ? Qt : Qnil);
}

DEFUN ("set-toolbar-button-down-flag", Fset_toolbar_button_down_flag, 2, 2, 0, /*
Don't touch.
*/
       (button, flag))
{
  struct toolbar_button *tb;
  char old_flag;

  CHECK_TOOLBAR_BUTTON (button);
  tb = XTOOLBAR_BUTTON (button);
  old_flag = tb->down;

  /* If the button is ignored, don't do anything. */
  if (!tb->enabled)
    return Qnil;

  /* If flag is nil, unset the down flag, otherwise set it to true.
     This also triggers an immediate redraw of the button if the flag
     does change. */

  if (NILP (flag))
    tb->down = 0;
  else
    tb->down = 1;

  if (tb->down != old_flag)
    {
      struct frame *f = XFRAME (tb->frame);
      struct device *d;

      if (DEVICEP (f->device))
	{
	  d = XDEVICE (f->device);

	  if (DEVICE_LIVE_P (XDEVICE (f->device)))
	    {
	      tb->dirty = 1;
	      MAYBE_DEVMETH (d, output_toolbar_button, (f, button));
	    }
	}
    }

  return Qnil;
}    

Lisp_Object
get_toolbar_button_glyph (struct window *w, struct toolbar_button *tb)
{
  Lisp_Object glyph = Qnil;

  /* The selected glyph logic:

     UP:		up
     DOWN:		down -> up
     DISABLED:	disabled -> up
     CAP-UP:	cap-up -> up
     CAP-DOWN:	cap-down -> cap-up -> down -> up
     CAP-DISABLED:	cap-disabled -> cap-up -> disabled -> up
     */

  if (!NILP (w->toolbar_buttons_captioned_p))
    {
      if (tb->enabled && tb->down)
	glyph = tb->cap_down_glyph;
      else if (!tb->enabled)
	glyph = tb->cap_disabled_glyph;
      
      if (NILP (glyph))
	glyph = tb->cap_up_glyph;
    }

  if (NILP (glyph))
    {
      if (tb->enabled && tb->down)
	glyph = tb->down_glyph;
      else if (!tb->enabled)
	glyph = tb->disabled_glyph;
    }

  /* The non-captioned up button is the ultimate fallback.  It is
     the only one we guarantee exists. */
  if (NILP (glyph))
    glyph = tb->up_glyph;

  return glyph;
}


static enum toolbar_pos
decode_toolbar_position (Lisp_Object position)
{
  if (EQ (position, Qtop))    return TOP_TOOLBAR;
  if (EQ (position, Qbottom)) return BOTTOM_TOOLBAR;
  if (EQ (position, Qleft))   return LEFT_TOOLBAR;
  if (EQ (position, Qright))  return RIGHT_TOOLBAR;
  signal_simple_error ("Invalid toolbar position", position);
  
  return TOP_TOOLBAR; /* not reached */
}

DEFUN ("set-default-toolbar-position", Fset_default_toolbar_position, 1, 1, 0, /*
Set the position that the `default-toolbar' will be displayed at.
Valid positions are 'top, 'bottom, 'left and 'right.
See `default-toolbar-position'.
*/
       (position))
{
  enum toolbar_pos cur = decode_toolbar_position (Vdefault_toolbar_position);
  enum toolbar_pos new = decode_toolbar_position (position);

  if (cur != new)
    {
      /* The following calls will automatically cause the dirty
	 flags to be set; we delay frame size changes to avoid
	 lots of frame flickering. */
      hold_frame_size_changes ();
      set_specifier_fallback (Vtoolbar[cur], list1 (Fcons (Qnil, Qnil)));
      set_specifier_fallback (Vtoolbar[new], Vdefault_toolbar);
      set_specifier_fallback (Vtoolbar_size[cur], list1 (Fcons (Qnil, Qzero)));
      set_specifier_fallback (Vtoolbar_size[new],
			      new == TOP_TOOLBAR || new == BOTTOM_TOOLBAR
			      ? Vdefault_toolbar_height
			      : Vdefault_toolbar_width);
      set_specifier_fallback (Vtoolbar_visible_p[cur],
			      list1 (Fcons (Qnil, Qt)));
      set_specifier_fallback (Vtoolbar_visible_p[new],
			      Vdefault_toolbar_visible_p);
      Vdefault_toolbar_position = position;
      unhold_frame_size_changes ();
    }

  return position;
}

DEFUN ("default-toolbar-position", Fdefault_toolbar_position, 0, 0, 0, /*
Return the position that the `default-toolbar' will be displayed at.
The `default-toolbar' will only be displayed here if the corresponding
position-specific toolbar specifier does not provide a value.
*/
       ())
{
  return Vdefault_toolbar_position;
}


static Lisp_Object
update_toolbar_button (struct frame *f, struct toolbar_button *tb,
		       Lisp_Object desc, int pushright)
{
  Lisp_Object *elt, glyphs, retval, buffer;
  struct gcpro gcpro1, gcpro2;

  elt = vector_data (XVECTOR (desc));
  buffer = XWINDOW (FRAME_LAST_NONMINIBUF_WINDOW (f))->buffer;

  if (!tb)
    {
      tb = alloc_lcrecord (sizeof (struct toolbar_button),
			   lrecord_toolbar_button);
      tb->next = Qnil;
      XSETFRAME (tb->frame, f);
      tb->up_glyph = Qnil;
      tb->down_glyph = Qnil;
      tb->disabled_glyph = Qnil;
      tb->cap_up_glyph = Qnil;
      tb->cap_down_glyph = Qnil;
      tb->cap_disabled_glyph = Qnil;
      tb->callback = Qnil;
      tb->enabled_p = Qnil;
      tb->help_string = Qnil;

      tb->enabled = 0;
      tb->down = 0;
      tb->pushright = pushright;
      tb->blank = 0;
      tb->x = tb->y = tb->width = tb->height = -1;
      tb->dirty = 1;
    }
  XSETTOOLBAR_BUTTON (retval, tb);

  /* Let's make sure nothing gets mucked up by the potential call to
     eval farther down. */
  GCPRO2 (retval, desc);

  glyphs = (CONSP (elt[0]) ? elt[0] : symbol_value_in_buffer (elt[0], buffer));

  /* If this is true we have a blank, otherwise it is an actual
     button. */
  if (KEYWORDP (glyphs))
    {
      int pos;
      int style_seen = 0;
      int size_seen = 0;

      if (!tb->blank)
	{
	  tb->blank = 1;
	  tb->dirty = 1;
	}

      for (pos = 0; pos < vector_length (XVECTOR (desc)); pos += 2)
	{
	  Lisp_Object key = elt[pos];
	  Lisp_Object val = elt[pos + 1];

	  if (EQ (key, Q_style))
	    {
	      style_seen = 1;

	      if (EQ (val, Q2D) || EQ (val, Q2d))
		{
		  if (!EQ (Qnil, tb->up_glyph) || !EQ (Qt, tb->disabled_glyph))
		    {
		      tb->up_glyph = Qnil;
		      tb->disabled_glyph = Qt;
		      tb->dirty = 1;
		    }
		}
	      else if (EQ (val, Q3D) || (EQ (val, Q3d)))
		{
		  if (!EQ (Qt, tb->up_glyph) || !EQ (Qnil, tb->disabled_glyph))
		    {
		      tb->up_glyph = Qt;
		      tb->disabled_glyph = Qnil;
		      tb->dirty = 1;
		    }
		}
	    }
	  else if (EQ (key, Q_size))
	    {
	      size_seen = 1;

	      if (!EQ (val, tb->down_glyph))
		{
		  tb->down_glyph = val;
		  tb->dirty = 1;
		}
	    }
	}

      if (!style_seen)
	{
	  /* The default style is 3D. */
	  if (!EQ (Qt, tb->up_glyph) || !EQ (Qnil, tb->disabled_glyph))
	    {
	      tb->up_glyph = Qt;
	      tb->disabled_glyph = Qnil;
	      tb->dirty = 1;
	    }
	}

      if (!size_seen)
	{
	  /* The default width is set to nil.  The device specific
             code will fill it in at its discretion. */
	  if (!NILP (tb->down_glyph))
	    {
	      tb->down_glyph = Qnil;
	      tb->dirty = 1;
	    }
	}

      /* The rest of these fields are not used by blanks.  We make
         sure they are nulled out in case this button object formerly
         represented a real button. */
      if (!NILP (tb->callback)
	  || !NILP (tb->enabled_p)
	  || !NILP (tb->help_string))
	{
	  tb->cap_up_glyph = Qnil;
	  tb->cap_down_glyph = Qnil;
	  tb->cap_disabled_glyph = Qnil;
	  tb->callback = Qnil;
	  tb->enabled_p = Qnil;
	  tb->help_string = Qnil;
	  tb->dirty = 1;
	}
    }
  else
    {
      if (tb->blank)
	{
	  tb->blank = 0;
	  tb->dirty = 1;
	}

      /* We know that we at least have an up_glyph.  Well, no, we
         don't.  The user may have changed the button glyph on us. */
      if (!NILP (glyphs) && CONSP (glyphs))
	{
	  if (!EQ (XCAR (glyphs), tb->up_glyph))
	    {
	      tb->up_glyph = XCAR (glyphs);
	      tb->dirty = 1;
	    }
	  glyphs = XCDR (glyphs);
	}
      else
	tb->up_glyph = Qnil;

      /* We might have a down_glyph. */
      if (!NILP (glyphs) && CONSP (glyphs))
	{
	  if (!EQ (XCAR (glyphs), tb->down_glyph))
	    {
	      tb->down_glyph = XCAR (glyphs);
	      tb->dirty = 1;
	    }
	  glyphs = XCDR (glyphs);
	}
      else
	tb->down_glyph = Qnil;

      /* We might have a disabled_glyph. */
      if (!NILP (glyphs) && CONSP (glyphs))
	{
	  if (!EQ (XCAR (glyphs), tb->disabled_glyph))
	    {
	      tb->disabled_glyph = XCAR (glyphs);
	      tb->dirty = 1;
	    }
	  glyphs = XCDR (glyphs);
	}
      else
	tb->disabled_glyph = Qnil;

      /* We might have a cap_up_glyph. */
      if (!NILP (glyphs) && CONSP (glyphs))
	{
	  if (!EQ (XCAR (glyphs), tb->cap_up_glyph))
	    {
	      tb->cap_up_glyph = XCAR (glyphs);
	      tb->dirty = 1;
	    }
	  glyphs = XCDR (glyphs);
	}
      else
	tb->cap_up_glyph = Qnil;

      /* We might have a cap_down_glyph. */
      if (!NILP (glyphs) && CONSP (glyphs))
	{
	  if (!EQ (XCAR (glyphs), tb->cap_down_glyph))
	    {
	      tb->cap_down_glyph = XCAR (glyphs);
	      tb->dirty = 1;
	    }
	  glyphs = XCDR (glyphs);
	}
      else
	tb->cap_down_glyph = Qnil;

      /* We might have a cap_disabled_glyph. */
      if (!NILP (glyphs) && CONSP (glyphs))
	{
	  if (!EQ (XCAR (glyphs), tb->cap_disabled_glyph))
	    {
	      tb->cap_disabled_glyph = XCAR (glyphs);
	      tb->dirty = 1;
	    }
	}
      else
	tb->cap_disabled_glyph = Qnil;

      /* Update the callback. */
      if (!EQ (tb->callback, elt[1]))
	{
	  tb->callback = elt[1];
	  /* This does not have an impact on the display properties of the
	     button so we do not mark it as dirty if it has changed. */
	}

      /* Update the enabled field. */
      if (!EQ (tb->enabled_p, elt[2]))
	{
	  tb->enabled_p = elt[2];
	  tb->dirty = 1;
	}

      /* We always do the following because if the enabled status is
	 determined by a function its decision may change without us being
	 able to detect it. */
      {
	int old_enabled = tb->enabled;

	if (NILP (tb->enabled_p))
	  tb->enabled = 0;
	else if (EQ (tb->enabled_p, Qt))
	  tb->enabled = 1;
	else
	  {
	    if (NILP (tb->enabled_p) || EQ (tb->enabled_p, Qt))
	      /* short-circuit the common case for speed */
	      tb->enabled = !NILP (tb->enabled_p);
	    else
	      {
		Lisp_Object result =
		  eval_in_buffer_trapping_errors
		    ("Error in toolbar enabled-p form",
		     XBUFFER
		     (WINDOW_BUFFER
		      (XWINDOW (FRAME_LAST_NONMINIBUF_WINDOW (f)))),
		     tb->enabled_p);
		if (UNBOUNDP (result))
		  /* #### if there was an error in the enabled-p
		     form, should we pretend like it's enabled
		     or disabled? */
		  tb->enabled = 0;
		else
		  tb->enabled = !NILP (result);
	      }
	  }

	if (old_enabled != tb->enabled)
	  tb->dirty = 1;
      }

      /* Update the help echo string. */
      if (!EQ (tb->help_string, elt[3]))
	{
	  tb->help_string = elt[3];
	  /* This does not have an impact on the display properties of the
	     button so we do not mark it as dirty if it has changed. */
	}
    }

  /* If this flag changes, the position is changing for sure unless
     some very unlikely geometry occurs. */
  if (tb->pushright != pushright)
    {
      tb->pushright = pushright;
      tb->dirty = 1;
    }

  /* The position and size fields are only manipulated in the
     device-dependent code. */
  UNGCPRO;
  return retval;
}

static Lisp_Object
compute_frame_toolbar_buttons (struct frame *f, enum toolbar_pos pos,
			       Lisp_Object toolbar)
{
  Lisp_Object buttons, prev_button, first_button;
  Lisp_Object orig_toolbar = toolbar;
  int pushright_seen = 0;
  struct gcpro gcpro1, gcpro2, gcpro3, gcpro4, gcpro5;

  first_button = FRAME_TOOLBAR_DATA (f, pos)->toolbar_buttons;
  buttons = prev_button = first_button;

  /* Yes, we're being paranoid. */
  GCPRO5 (toolbar, buttons, prev_button, first_button, orig_toolbar);

  if (NILP (toolbar))
    {
      /* The output mechanisms will take care of clearing the former
         toolbar. */
      UNGCPRO;
      return Qnil;
    }

  if (!CONSP (toolbar))
    signal_simple_error ("toolbar description must be a list", toolbar);

  /* First synchronize any existing buttons. */
  while (!NILP (toolbar) && !NILP (buttons))
    {
      struct toolbar_button *tb;

      if (NILP (XCAR (toolbar)))
	{
	  if (pushright_seen)
	    signal_simple_error
	      ("more than one partition (nil) in toolbar description",
	       orig_toolbar);
	  else
	    pushright_seen = 1;
	}
      else
	{
	  tb = XTOOLBAR_BUTTON (buttons);
	  update_toolbar_button (f, tb, XCAR (toolbar), pushright_seen);
	  prev_button = buttons;
	  buttons = tb->next;
	}

      toolbar = XCDR (toolbar);
    }

  /* If we hit the end of the toolbar, then clean up any excess
     buttons and return. */
  if (NILP (toolbar))
    {
      if (!NILP (buttons))
	{
	  /* If this is the case the only thing we saw was a
             pushright marker. */
	  if (EQ (buttons, first_button))
	    {
	      UNGCPRO;
	      return Qnil;
	    }
	  else
	    XTOOLBAR_BUTTON (prev_button)->next = Qnil;
	}
      UNGCPRO;
      return first_button;
    }

  /* At this point there are more buttons on the toolbar than we
     actually have in existence. */
  while (!NILP (toolbar))
    {
      Lisp_Object new_button;

      if (NILP (XCAR (toolbar)))
	{
	  if (pushright_seen)
	    signal_simple_error
	      ("more than one partition (nil) in toolbar description",
	       orig_toolbar);
	  else
	    pushright_seen = 1;
	}
      else
	{
	  new_button = update_toolbar_button (f, NULL, XCAR (toolbar),
					      pushright_seen);

	  if (NILP (first_button))
	    {
	      first_button = prev_button = new_button;
	    }
	  else
	    {
	      XTOOLBAR_BUTTON (prev_button)->next = new_button;
	      prev_button = new_button;
	    }
	}

      toolbar = XCDR (toolbar);
    }

  UNGCPRO;
  return first_button;
}

static int
set_frame_toolbar (struct frame *f, enum toolbar_pos pos, int first_time_p)
{
  Lisp_Object toolbar, buttons;
  struct window *w = XWINDOW (FRAME_LAST_NONMINIBUF_WINDOW (f));
  Lisp_Object buffer = w->buffer;
  int visible = FRAME_THEORETICAL_TOOLBAR_SIZE (f, pos);

  toolbar = w->toolbar[pos];

  if (NILP (f->toolbar_data[pos]))
    {
      struct toolbar_data *td = alloc_lcrecord (sizeof (struct toolbar_data),
						lrecord_toolbar_data);

      td->last_toolbar_buffer = Qnil;
      td->toolbar_buttons = Qnil;
      XSETTOOLBAR_DATA (f->toolbar_data[pos], td);
    }

  if (visible)
    buttons = compute_frame_toolbar_buttons (f, pos, toolbar);
  else
    buttons = Qnil;

  FRAME_TOOLBAR_DATA (f, pos)->last_toolbar_buffer = buffer;
  FRAME_TOOLBAR_DATA (f, pos)->toolbar_buttons = buttons;

  return (visible);
}

#define COMPUTE_TOOLBAR_DATA(position)					 \
  do									 \
    {									 \
      local_toolbar_changed =						 \
	(f->toolbar_changed						 \
	 || NILP (f->toolbar_data[position])				 \
	 || (!EQ (FRAME_TOOLBAR_DATA (f, position)->last_toolbar_buffer, \
		  XWINDOW (FRAME_LAST_NONMINIBUF_WINDOW (f))->buffer))); \
									 \
      toolbar_was_visible =						 \
         (!NILP (f->toolbar_data[position])				 \
          && !NILP (FRAME_TOOLBAR_DATA (f, position)->toolbar_buttons)); \
      toolbar_will_be_visible = toolbar_was_visible;			 \
									 \
      if (local_toolbar_changed)					 \
	toolbar_will_be_visible =					 \
           set_frame_toolbar (f, position, first_time_p);		 \
									 \
      toolbar_visibility_changed =					 \
	(toolbar_was_visible != toolbar_will_be_visible);		 \
									 \
      if (toolbar_visibility_changed)					 \
        frame_changed_size = 1;						 \
    } while (0)

static void
compute_frame_toolbars_data (struct frame *f, int first_time_p)
{
  int local_toolbar_changed;
  int toolbar_was_visible, toolbar_will_be_visible;
  int toolbar_visibility_changed;
  int frame_changed_size = 0;

  COMPUTE_TOOLBAR_DATA (TOP_TOOLBAR);
  COMPUTE_TOOLBAR_DATA (BOTTOM_TOOLBAR);
  COMPUTE_TOOLBAR_DATA (LEFT_TOOLBAR);
  COMPUTE_TOOLBAR_DATA (RIGHT_TOOLBAR);

  /* The frame itself doesn't actually change size, but the usable
     text area does.  All we have to do is call change_frame_size with
     the current height and width parameters and it will readjust for
     all changes in the toolbars. */
  if (frame_changed_size && !first_time_p)
    change_frame_size (f, FRAME_HEIGHT (f), FRAME_WIDTH (f), 0);
}
#undef COMPUTE_TOOLBAR_DATA

void
update_frame_toolbars (struct frame *f)
{
  struct device *d = XDEVICE (f->device);
  Lisp_Object buffer = XWINDOW (FRAME_LAST_NONMINIBUF_WINDOW (f))->buffer;

  /* If the buffer of the selected window is not equal to the
     last_toolbar_buffer value for any of the toolbars, then the
     toolbars need to be recomputed. */
  if ((HAS_DEVMETH_P (d, output_frame_toolbars))
      && (f->toolbar_changed
	  || !EQ (FRAME_TOOLBAR_BUFFER (f, TOP_TOOLBAR), buffer)
	  || !EQ (FRAME_TOOLBAR_BUFFER (f, BOTTOM_TOOLBAR), buffer)
	  || !EQ (FRAME_TOOLBAR_BUFFER (f, LEFT_TOOLBAR), buffer)
	  || !EQ (FRAME_TOOLBAR_BUFFER (f, RIGHT_TOOLBAR), buffer)))
    {
      /* Removed the check for the minibuffer here.  We handle this
	 more correctly now by consistently using
	 FRAME_LAST_NONMINIBUF_WINDOW instead of FRAME_SELECTED_WINDOW
	 throughout the toolbar code. */
      compute_frame_toolbars_data (f, 0);

      DEVMETH (d, output_frame_toolbars, (f));
    }

  f->toolbar_changed = 0;
}

void
init_frame_toolbars (struct frame *f)
{
  struct device *d = XDEVICE (f->device);

  /* If there isn't any output routine, then this device type doesn't
     support toolbars. */
  if (HAS_DEVMETH_P (d, output_frame_toolbars))
    {
      Lisp_Object frame = Qnil;

      compute_frame_toolbars_data (f, 1);
      XSETFRAME (frame, f);
      call_critical_lisp_code (XDEVICE (FRAME_DEVICE (f)),
			       Qinit_toolbar_from_resources,
			       frame);
      MAYBE_DEVMETH (d, initialize_frame_toolbars, (f));
    }
}

void
init_device_toolbars (struct device *d)
{
  Lisp_Object device = Qnil;

  XSETDEVICE (device, d);
  if (HAS_DEVMETH_P (d, output_frame_toolbars))
    call_critical_lisp_code (d,
			     Qinit_toolbar_from_resources,
			     device);
}

void
init_global_toolbars (struct device *d)
{
  if (HAS_DEVMETH_P (d, output_frame_toolbars))
    call_critical_lisp_code (d,
			     Qinit_toolbar_from_resources,
			     Qglobal);
}

void
free_frame_toolbars (struct frame *f)
{
  /* If we had directly allocated any memory for the toolbars instead
     of using all Lisp_Objects this is where we would now free it. */

  MAYBE_FRAMEMETH (f, free_frame_toolbars, (f));
}

void
get_toolbar_coords (struct frame *f, enum toolbar_pos pos, int *x, int *y,
		    int *width, int *height, int *vert, int for_layout)
{
  int visible_top_toolbar_height, visible_bottom_toolbar_height;
  int adjust = (for_layout ? 1 : 0);

  /* The top and bottom toolbars take precedence over the left and
     right. */
  visible_top_toolbar_height = (FRAME_REAL_TOP_TOOLBAR_VISIBLE (f)
				? FRAME_REAL_TOP_TOOLBAR_HEIGHT (f)
				: 0);
  visible_bottom_toolbar_height = (FRAME_REAL_BOTTOM_TOOLBAR_VISIBLE (f)
				? FRAME_REAL_BOTTOM_TOOLBAR_HEIGHT (f)
				: 0);

  /* We adjust the width and height by one to give us a narrow border
     at the outside edges.  However, when we are simply determining
     toolbar location we don't want to do that. */

  switch (pos)
    {
    case TOP_TOOLBAR:
      *x = 1;
      *y = 0;	/* #### should be 1 if no menubar */
      *width = FRAME_PIXWIDTH (f) - 2;
      *height = FRAME_REAL_TOP_TOOLBAR_HEIGHT (f) - adjust;
      *vert = 0;
      break;
    case BOTTOM_TOOLBAR:
      *x = 1;
      *y = FRAME_PIXHEIGHT (f) - FRAME_REAL_BOTTOM_TOOLBAR_HEIGHT (f);
      *width = FRAME_PIXWIDTH (f) - 2;
      *height = FRAME_REAL_BOTTOM_TOOLBAR_HEIGHT (f) - adjust;
      *vert = 0;
      break;
    case LEFT_TOOLBAR:
      *x = 1;
      *y = visible_top_toolbar_height;
      *width = FRAME_REAL_LEFT_TOOLBAR_WIDTH (f) - adjust;
      *height = (FRAME_PIXHEIGHT (f) - visible_top_toolbar_height -
		 visible_bottom_toolbar_height - 1);
      *vert = 1;
      break;
    case RIGHT_TOOLBAR:
      *x = FRAME_PIXWIDTH (f) - FRAME_REAL_RIGHT_TOOLBAR_WIDTH (f);
      *y = visible_top_toolbar_height;
      *width = FRAME_REAL_RIGHT_TOOLBAR_WIDTH (f) - adjust;
      *height = (FRAME_PIXHEIGHT (f) - visible_top_toolbar_height -
		 visible_bottom_toolbar_height);
      *vert = 1;
      break;
    default:
      abort ();
    }
}

#define CHECK_TOOLBAR(pos)						\
  do									\
    {									\
      get_toolbar_coords (f, pos, &x, &y, &width, &height, &vert, 0);	\
      if ((x_coord >= x) && (x_coord < (x + width)))			\
	{								\
	  if ((y_coord >= y) && (y_coord < (y + height)))		\
	    {								\
	      return (FRAME_TOOLBAR_DATA (f, pos)->toolbar_buttons);	\
	    }								\
	}								\
    } while (0)

static Lisp_Object
toolbar_buttons_at_pixpos (struct frame *f, int x_coord, int y_coord)
{
  int x, y, width, height, vert;

  if (FRAME_REAL_TOP_TOOLBAR_VISIBLE (f))
    CHECK_TOOLBAR (TOP_TOOLBAR);
  if (FRAME_REAL_BOTTOM_TOOLBAR_VISIBLE (f))
    CHECK_TOOLBAR (BOTTOM_TOOLBAR);
  if (FRAME_REAL_LEFT_TOOLBAR_VISIBLE (f))
    CHECK_TOOLBAR (LEFT_TOOLBAR);
  if (FRAME_REAL_RIGHT_TOOLBAR_VISIBLE (f))
    CHECK_TOOLBAR (RIGHT_TOOLBAR);

  return Qnil;
}
#undef CHECK_TOOLBAR

/* The device dependent code actually does the work of positioning the
   buttons, but we are free to access that information at this
   level. */
Lisp_Object
toolbar_button_at_pixpos (struct frame *f, int x_coord, int y_coord)
{
  Lisp_Object buttons = toolbar_buttons_at_pixpos (f, x_coord, y_coord);

  if (NILP (buttons))
    return Qnil;

  while (!NILP (buttons))
    {
      struct toolbar_button *tb = XTOOLBAR_BUTTON (buttons);

      if ((x_coord >= tb->x) && (x_coord < (tb->x + tb->width)))
	{
	  if ((y_coord >= tb->y) && (y_coord < (tb->y + tb->height)))
	    {
	      /* If we are over a blank, return nil. */
	      if (tb->blank)
		return Qnil;
	      else
		return buttons;
	    }
	}

      buttons = tb->next;
    }

  /* We must be over a blank in the toolbar. */
  return Qnil;
}


/************************************************************************/
/*                        Toolbar specifier type                        */
/************************************************************************/

DEFINE_SPECIFIER_TYPE (toolbar);

#define CTB_ERROR(msg)						\
  do								\
    {								\
      maybe_signal_simple_error (msg, button, Qtoolbar, errb);	\
      RETURN__ Qnil;						\
    }								\
  while (0)

/* Returns Q_style if key was :style, Qt if ok otherwise, Qnil if error. */
static Lisp_Object
check_toolbar_button_keywords (Lisp_Object button, Lisp_Object key,
			       Lisp_Object val, Error_behavior errb)
{
  if (!KEYWORDP (key))
    {
      maybe_signal_simple_error_2 ("not a keyword", key, button, Qtoolbar,
				   errb);
      return Qnil;
    }

  if (EQ (key, Q_style))
    {
      if (!EQ (val, Q2D)
	  && !EQ (val, Q3D)
	  && !EQ (val, Q2d)
	  && !EQ (val, Q3d))
	CTB_ERROR ("unrecognized toolbar blank style");

      return Q_style;
    }
  else if (EQ (key, Q_size))
    {
      if (!NATNUMP (val))
	CTB_ERROR ("invalid toolbar blank size");
    }
  else
    {
      CTB_ERROR ("invalid toolbar blank keyword");
    }

  return Qt;
}

/* toolbar button spec is [pixmap-pair function enabled-p help]
	               or [:style 2d-or-3d :size width-or-height] */

DEFUN ("check-toolbar-button-syntax", Fcheck_toolbar_button_syntax, 1, 2, 0, /*
Verify the syntax of entry BUTTON in a toolbar description list.
If you want to verify the syntax of a toolbar description list as a
whole, use `check-valid-instantiator' with a specifier type of 'toolbar.
*/
       (button, no_error))
{
  Lisp_Object *elt, glyphs, value;
  int len;
  Error_behavior errb = decode_error_behavior_flag (no_error);

  if (!VECTORP (button))
    CTB_ERROR ("toolbar button descriptors must be vectors");
  elt = vector_data (XVECTOR (button));

  if (vector_length (XVECTOR (button)) == 2)
    {
      if (!EQ (Q_style, check_toolbar_button_keywords (button, elt[0],
						       elt[1], errb)))
	CTB_ERROR ("must specify toolbar blank style");

      return Qt;
    }

  if (vector_length (XVECTOR (button)) != 4)
    CTB_ERROR ("toolbar button descriptors must be 2 or 4 long");

  /* The first element must be a list of glyphs of length 1-6.  The
     first entry is the pixmap for the up state, the second for the
     down state, the third for the disabled state, the fourth for the
     captioned up state, the fifth for the captioned down state and
     the sixth for the captioned disabled state.  Only the up state is
     mandatory. */
  if (!CONSP (elt[0]))
    {
      /* We can't check the buffer-local here because we don't know
         which buffer to check in.  #### I think this is a bad thing.
         See if we can't get enough information to this function so
         that it can check.

	 #### Wrong.  We shouldn't be checking the value at all here.
	 The user might set or change the value at any time. */
      value = Fsymbol_value (elt[0]);

      if (!CONSP (value))
	{
	  if (KEYWORDP (elt[0]))
	    {
	      int fsty = 0;

	      if (EQ (Q_style, check_toolbar_button_keywords (button, elt[0],
							      elt[1],
							      errb)))
		fsty++;

	      if (EQ (Q_style, check_toolbar_button_keywords (button, elt[2],
							      elt[3],
							      errb)))
		fsty++;

	      if (!fsty)
		CTB_ERROR ("must specify toolbar blank style");
	      else if (EQ (elt[0], elt[2]))
		CTB_ERROR
		  ("duplicate keywords in toolbar button blank description");

	      return Qt;
	    }
	  else
	    CTB_ERROR ("first element of button must be a list (of glyphs)");
	}
    }
  else
    value = elt[0];

  len = XINT (Flength (value));
  if (len < 1)
    CTB_ERROR ("toolbar button glyph list must have at least 1 entry");
  
  if (len > 6)
    CTB_ERROR ("toolbar button glyph list can have at most 6 entries");

  glyphs = value;
  while (!NILP (glyphs))
    {
      if (!GLYPHP (XCAR (glyphs)))
	{
	  /* We allow nil for the down and disabled glyphs but not for
             the up glyph. */
	  if (EQ (glyphs, value) || !NILP (XCAR (glyphs)))
	    {
	      CTB_ERROR
		("all elements of toolbar button glyph list must be glyphs.");
	    }
	}
      glyphs = XCDR (glyphs);
    }

  /* The second element is the function to run when the button is
     activated.  We do not do any checking on it because it is legal
     for the function to not be defined until after the toolbar is.
     It is the user's problem to get this right.

     The third element is either a boolean indicating the enabled
     status or a function used to determine it.  Again, it is the
     user's problem if this is wrong.

     The fourth element, if not nil, must be a string which will be
     displayed as the help echo. */

  /* #### This should be allowed to be a function returning a string
     as well as just a string. */
  if (!NILP (elt[3]) && !STRINGP (elt[3]))
    CTB_ERROR ("toolbar button help echo string must be a string");

  return Qt;
}
#undef CTB_ERROR

static void
toolbar_validate (Lisp_Object instantiator)
{
  int pushright_seen = 0;
  Lisp_Object rest;

  if (NILP (instantiator))
    return;

  if (!CONSP (instantiator))
    signal_simple_error ("toolbar spec must be list or nil", instantiator);

  for (rest = instantiator; !NILP (rest); rest = XCDR (rest))
    {
      if (!CONSP (rest))
	signal_simple_error ("bad list in toolbar spec", instantiator);

      if (NILP (XCAR (rest)))
	{
	  if (pushright_seen)
	    error
	      ("more than one partition (nil) in instantiator description");
	  else
	    pushright_seen = 1;
	}
      else
	Fcheck_toolbar_button_syntax (XCAR (rest), Qnil);
    }
}

static void
toolbar_after_change (Lisp_Object specifier, Lisp_Object locale)
{
  /* #### This is overkill.  I really need to rethink the after-change
     functions to make them easier to use. */
  MARK_TOOLBAR_CHANGED;
}

DEFUN ("toolbar-specifier-p", Ftoolbar_specifier_p, 1, 1, 0, /*
Return non-nil if OBJECT is a toolbar specifier.
Toolbar specifiers are used to specify the format of a toolbar.
The values of the variables `default-toolbar', `top-toolbar',
`left-toolbar', `right-toolbar', and `bottom-toolbar' are always
toolbar specifiers.

Valid toolbar instantiators are called \"toolbar descriptors\"
and are lists of vectors.  See `default-toolbar' for a description
of the exact format.
*/
       (object))
{
  return (TOOLBAR_SPECIFIERP (object) ? Qt : Qnil);
}


static void
toolbar_specs_changed (Lisp_Object specifier, struct window *w,
		       Lisp_Object oldval)
{
  /* This could be smarter but I doubt that it would make any
     noticable difference given the infrequency with which this is
     probably going to be called.
     */
  MARK_TOOLBAR_CHANGED;
}

static void
default_toolbar_specs_changed (Lisp_Object specifier, struct window *w,
			       Lisp_Object oldval)
{
  enum toolbar_pos pos = decode_toolbar_position (Vdefault_toolbar_position);

  Fset_specifier_dirty_flag (Vtoolbar[pos]);
}

static void
toolbar_size_changed_in_frame (Lisp_Object specifier, struct frame *f,
			       Lisp_Object oldval)
{
  int pos;

  for (pos = 0; pos < countof (Vtoolbar_size); pos++)
    if (EQ (specifier, Vtoolbar_size[(enum toolbar_pos) pos]))
      break;

  assert (pos < countof (Vtoolbar_size));

  MAYBE_FRAMEMETH (f, toolbar_size_changed_in_frame,
		   (f, pos, oldval));

  /* Let redisplay know that something has possibly changed. */
  MARK_TOOLBAR_CHANGED;
}

static void
toolbar_visible_p_changed_in_frame (Lisp_Object specifier, struct frame *f,
				    Lisp_Object oldval)
{
  int pos;

  for (pos = 0; pos < countof (Vtoolbar_visible_p); pos++)
    if (EQ (specifier, Vtoolbar_visible_p[(enum toolbar_pos) pos]))
      break;

  assert (pos < countof (Vtoolbar_visible_p));

  MAYBE_FRAMEMETH (f, toolbar_visible_p_changed_in_frame,
		   (f, pos, oldval));

  /* Let redisplay know that something has possibly changed. */
  MARK_TOOLBAR_CHANGED;
}

static void
default_toolbar_size_changed_in_frame (Lisp_Object specifier, struct frame *f,
				       Lisp_Object oldval)
{
  enum toolbar_pos pos = decode_toolbar_position (Vdefault_toolbar_position);

  Fset_specifier_dirty_flag (Vtoolbar_size[pos]);

  /* Let redisplay know that something has possibly changed. */
  MARK_TOOLBAR_CHANGED;
}

static void
default_toolbar_visible_p_changed_in_frame (Lisp_Object specifier,
					    struct frame *f,
					    Lisp_Object oldval)
{
  enum toolbar_pos pos = decode_toolbar_position (Vdefault_toolbar_position);

  Fset_specifier_dirty_flag (Vtoolbar_visible_p[pos]);

  /* Let redisplay know that something has possibly changed. */
  MARK_TOOLBAR_CHANGED;
}


static void
toolbar_buttons_captioned_p_changed (Lisp_Object specifier, struct window *w,
				     Lisp_Object oldval)
{
  /* This could be smarter but I doubt that it would make any
     noticable difference given the infrequency with which this is
     probably going to be called. */
  MARK_TOOLBAR_CHANGED;
}


void
syms_of_toolbar (void)
{
  defsymbol (&Qtoolbar_buttonp, "toolbar-button-p");
  defsymbol (&Q2D, "2D");
  defsymbol (&Q3D, "3D");
  defsymbol (&Q2d, "2d");
  defsymbol (&Q3d, "3d");
  defsymbol (&Q_size, ":size");	Fset (Q_size, Q_size);

  defsymbol (&Qinit_toolbar_from_resources, "init-toolbar-from-resources");
  DEFSUBR (Ftoolbar_button_p);
  DEFSUBR (Ftoolbar_button_callback);
  DEFSUBR (Ftoolbar_button_help_string);
  DEFSUBR (Ftoolbar_button_enabled_p);
  DEFSUBR (Fset_toolbar_button_down_flag);
  DEFSUBR (Fcheck_toolbar_button_syntax);
  DEFSUBR (Fset_default_toolbar_position);
  DEFSUBR (Fdefault_toolbar_position);
  DEFSUBR (Ftoolbar_specifier_p);
}

void
vars_of_toolbar (void)
{
  staticpro (&Vdefault_toolbar_position);
  Vdefault_toolbar_position = Qtop;

#ifdef HAVE_WINDOW_SYSTEM
  Fprovide (Qtoolbar);
#endif
}

void
specifier_type_create_toolbar (void)
{
  INITIALIZE_SPECIFIER_TYPE (toolbar, "toolbar", "toolbar-specifier-p");

  SPECIFIER_HAS_METHOD (toolbar, validate);
  SPECIFIER_HAS_METHOD (toolbar, after_change);
}

void
specifier_vars_of_toolbar (void)
{
  Lisp_Object elt;
      
  DEFVAR_SPECIFIER ("default-toolbar", &Vdefault_toolbar /*
Specifier for a fallback toolbar.
Use `set-specifier' to change this.

The position of this toolbar is specified in the function
`default-toolbar-position'.  If the corresponding position-specific
toolbar (e.g. `top-toolbar' if `default-toolbar-position' is 'top)
does not specify a toolbar in a particular domain (usually a window),
then the value of `default-toolbar' in that domain, if any, will be
used instead.

Note that the toolbar at any particular position will not be
displayed unless its visibility flag is true and its thickness
\(width or height, depending on orientation) is non-zero.  The
visibility is controlled by the specifiers `top-toolbar-visible-p',
`bottom-toolbar-visible-p', `left-toolbar-visible-p', and
`right-toolbar-visible-p', and the thickness is controlled by the
specifiers `top-toolbar-height', `bottom-toolbar-height',
`left-toolbar-width', and `right-toolbar-width'.

Note that one of the four visibility specifiers inherits from
`default-toolbar-visibility' and one of the four thickness
specifiers inherits from either `default-toolbar-width' or
`default-toolbar-height' (depending on orientation), just
like for the toolbar description specifiers (e.g. `top-toolbar')
mentioned above.

Therefore, if you are setting `default-toolbar', you should control
the visibility and thickness using `default-toolbar-visible-p',
`default-toolbar-width', and `default-toolbar-height', rather than
using position-specific specifiers.  That way, you will get sane
behavior if the user changes the default toolbar position.

The format of the instantiator for a toolbar is a list of
toolbar-button-descriptors.  Each toolbar-button-descriptor
is a vector in one of the following formats:

  [GLYPH-LIST FUNCTION ENABLED-P HELP] or
  [:style 2D-OR-3D] or
  [:style 2D-OR-3D :size WIDTH-OR-HEIGHT] or
  [:size WIDTH-OR-HEIGHT :style 2D-OR-3D]

Optionally, one of the toolbar-button-descriptors may be nil
instead of a vector; this signifies the division between
the toolbar buttons that are to be displayed flush-left,
and the buttons to be displayed flush-right.

The first vector format above specifies a normal toolbar button;
the others specify blank areas in the toolbar.

For the first vector format:

-- GLYPH-LIST should be a list of one to six glyphs (as created by
   `make-glyph') or a symbol whose value is such a list.  The first
   glyph, which must be provided, is the glyph used to display the
   toolbar button when it is in the \"up\" (not pressed) state.  The
   optional second glyph is for displaying the button when it is in
   the \"down\" (pressed) state.  The optional third glyph is for when
   the button is disabled.  The optional fourth, fifth and sixth glyphs
   are used to specify captioned versions for the up, down and disabled
   states respectively.  The function `toolbar-make-button-list' is
   useful in creating these glyph lists.  The specifier variable
   `toolbar-buttons-captioned-p' controls which glyphs are actually used.

-- Even if you do not provide separate down-state and disabled-state
   glyphs, the user will still get visual feedback to indicate which
   state the button is in.  Buttons in the up-state are displayed
   with a shadowed border that gives a raised appearance to the
   button.  Buttons in the down-state are displayed with shadows that
   give a recessed appearance.  Buttons in the disabled state are
   displayed with no shadows, giving a 2-d effect.

-- If some of the toolbar glyphs are not provided, they inherit as follows:

     UP:                up
     DOWN:              down -> up
     DISABLED:          disabled -> up
     CAP-UP:            cap-up -> up
     CAP-DOWN:          cap-down -> cap-up -> down -> up
     CAP-DISABLED:      cap-disabled -> cap-up -> disabled -> up

-- The second element FUNCTION is a function to be called when the
   toolbar button is activated (i.e. when the mouse is released over
   the toolbar button, if the press occurred in the toolbar).  It
   can be any form accepted by `call-interactively', since this is
   how it is invoked.

-- The third element ENABLED-P specifies whether the toolbar button
   is enabled (disabled buttons do nothing when they are activated,
   and are displayed differently; see above).  It should be either
   a boolean or a form that evaluates to a boolean.

-- The fourth element HELP, if non-nil, should be a string.  This
   string is displayed in the echo area when the mouse passes over
   the toolbar button.

For the other vector formats (specifying blank areas of the toolbar):

-- 2D-OR-3D should be one of the symbols '2d or '3d, indicating
   whether the area is displayed with shadows (giving it a raised,
   3-d appearance) or without shadows (giving it a flat appearance).

-- WIDTH-OR-HEIGHT specifies the length, in pixels, of the blank
   area.  If omitted, it defaults to a device-specific value
   (8 pixels for X devices).
*/ );

  Vdefault_toolbar = Fmake_specifier (Qtoolbar);
  /* #### It would be even nicer if the specifier caching
     automatically knew about specifier fallbacks, so we didn't
     have to do it ourselves. */
  set_specifier_caching (Vdefault_toolbar,
			 slot_offset (struct window,
				      default_toolbar),
			 default_toolbar_specs_changed,
			 0, 0);

  DEFVAR_SPECIFIER ("top-toolbar",
		    &Vtoolbar[TOP_TOOLBAR] /*
Specifier for the toolbar at the top of the frame.
Use `set-specifier' to change this.
See `default-toolbar' for a description of a valid toolbar instantiator.
*/ );
  Vtoolbar[TOP_TOOLBAR] = Fmake_specifier (Qtoolbar);
  set_specifier_caching (Vtoolbar[TOP_TOOLBAR],
			 slot_offset (struct window,
				      toolbar[TOP_TOOLBAR]),
			 toolbar_specs_changed,
			 0, 0);

  DEFVAR_SPECIFIER ("bottom-toolbar",
		    &Vtoolbar[BOTTOM_TOOLBAR] /*
Specifier for the toolbar at the bottom of the frame.
Use `set-specifier' to change this.
See `default-toolbar' for a description of a valid toolbar instantiator.

Note that, unless the `default-toolbar-position' is `bottom', by
default the height of the bottom toolbar (controlled by
`bottom-toolbar-height') is 0; thus, a bottom toolbar will not be
displayed even if you provide a value for `bottom-toolbar'.
*/ );
  Vtoolbar[BOTTOM_TOOLBAR] = Fmake_specifier (Qtoolbar);
  set_specifier_caching (Vtoolbar[BOTTOM_TOOLBAR],
			 slot_offset (struct window,
				      toolbar[BOTTOM_TOOLBAR]),
			 toolbar_specs_changed,
			 0, 0);

  DEFVAR_SPECIFIER ("left-toolbar",
		    &Vtoolbar[LEFT_TOOLBAR] /*
Specifier for the toolbar at the left edge of the frame.
Use `set-specifier' to change this.
See `default-toolbar' for a description of a valid toolbar instantiator.

Note that, unless the `default-toolbar-position' is `left', by
default the height of the left toolbar (controlled by
`left-toolbar-width') is 0; thus, a left toolbar will not be
displayed even if you provide a value for `left-toolbar'.
*/ );
  Vtoolbar[LEFT_TOOLBAR] = Fmake_specifier (Qtoolbar);
  set_specifier_caching (Vtoolbar[LEFT_TOOLBAR],
			 slot_offset (struct window,
				      toolbar[LEFT_TOOLBAR]),
			 toolbar_specs_changed,
			 0, 0);

  DEFVAR_SPECIFIER ("right-toolbar",
		    &Vtoolbar[RIGHT_TOOLBAR] /*
Specifier for the toolbar at the right edge of the frame.
Use `set-specifier' to change this.
See `default-toolbar' for a description of a valid toolbar instantiator.

Note that, unless the `default-toolbar-position' is `right', by
default the height of the right toolbar (controlled by
`right-toolbar-width') is 0; thus, a right toolbar will not be
displayed even if you provide a value for `right-toolbar'.
*/ );
  Vtoolbar[RIGHT_TOOLBAR] = Fmake_specifier (Qtoolbar);
  set_specifier_caching (Vtoolbar[RIGHT_TOOLBAR],
			 slot_offset (struct window,
				      toolbar[RIGHT_TOOLBAR]),
			 toolbar_specs_changed,
			 0, 0);

  /* initially, top inherits from default; this can be
     changed with `set-default-toolbar-position'. */
  elt = list1 (Fcons (Qnil, Qnil));
  set_specifier_fallback (Vdefault_toolbar, elt);
  set_specifier_fallback (Vtoolbar[TOP_TOOLBAR], Vdefault_toolbar);
  set_specifier_fallback (Vtoolbar[BOTTOM_TOOLBAR], elt);
  set_specifier_fallback (Vtoolbar[LEFT_TOOLBAR], elt);
  set_specifier_fallback (Vtoolbar[RIGHT_TOOLBAR], elt);

  DEFVAR_SPECIFIER ("default-toolbar-height", &Vdefault_toolbar_height /*
*Height of the default toolbar, if it's oriented horizontally.
This is a specifier; use `set-specifier' to change it.

The position of the default toolbar is specified by the function
`set-default-toolbar-position'.  If the corresponding position-specific
toolbar thickness specifier (e.g. `top-toolbar-height' if
`default-toolbar-position' is 'top) does not specify a thickness in a
particular domain (a window or a frame), then the value of
`default-toolbar-height' or `default-toolbar-width' (depending on the
toolbar orientation) in that domain, if any, will be used instead.

Note that `default-toolbar-height' is only used when
`default-toolbar-position' is 'top or 'bottom, and `default-toolbar-width'
is only used when `default-toolbar-position' is 'left or 'right.

Note that all of the position-specific toolbar thickness specifiers
have a fallback value of zero when they do not correspond to the
default toolbar.  Therefore, you will have to set a non-zero thickness
value if you want a position-specific toolbar to be displayed.

Internally, toolbar thickness specifiers are instantiated in both
window and frame domains, for different purposes.  The value in the
domain of a frame's selected window specifies the actual toolbar
thickness that you will see in that frame.  The value in the domain of
a frame itself specifies the toolbar thickness that is used in frame
geometry calculations.

Thus, for example, if you set the frame width to 80 characters and the
left toolbar width for that frame to 68 pixels, then the frame will
be sized to fit 80 characters plus a 68-pixel left toolbar.  If you
then set the left toolbar width to 0 for a particular buffer (or if
that buffer does not specify a left toolbar or has a nil value
specified for `left-toolbar-visible-p'), you will find that, when
that buffer is displayed in the selected window, the window will have
a width of 86 or 87 characters -- the frame is sized for a 68-pixel
left toolbar but the selected window specifies that the left toolbar
is not visible, so it is expanded to take up the slack.
*/ );
  Vdefault_toolbar_height = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vdefault_toolbar_height,
			 slot_offset (struct window,
				      default_toolbar_height),
			 some_window_value_changed,
			 slot_offset (struct frame,
				      default_toolbar_height),
			 default_toolbar_size_changed_in_frame);

  DEFVAR_SPECIFIER ("default-toolbar-width", &Vdefault_toolbar_width /*
*Width of the default toolbar, if it's oriented vertically.
This is a specifier; use `set-specifier' to change it.

See `default-toolbar-height' for more information.
*/ );
  Vdefault_toolbar_width = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vdefault_toolbar_width,
			 slot_offset (struct window,
				      default_toolbar_width),
			 some_window_value_changed,
			 slot_offset (struct frame,
				      default_toolbar_width),
			 default_toolbar_size_changed_in_frame);

  DEFVAR_SPECIFIER ("top-toolbar-height",
		    &Vtoolbar_size[TOP_TOOLBAR] /*
*Height of the top toolbar.
This is a specifier; use `set-specifier' to change it.

See `default-toolbar-height' for more information.
*/ );
  Vtoolbar_size[TOP_TOOLBAR] = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vtoolbar_size[TOP_TOOLBAR],
			 slot_offset (struct window,
				      toolbar_size[TOP_TOOLBAR]),
			 some_window_value_changed,
			 slot_offset (struct frame,
				      toolbar_size[TOP_TOOLBAR]),
			 toolbar_size_changed_in_frame);

  DEFVAR_SPECIFIER ("bottom-toolbar-height",
		    &Vtoolbar_size[BOTTOM_TOOLBAR] /*
*Height of the bottom toolbar.
This is a specifier; use `set-specifier' to change it.

See `default-toolbar-height' for more information.
*/ );
  Vtoolbar_size[BOTTOM_TOOLBAR] = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vtoolbar_size[BOTTOM_TOOLBAR],
			 slot_offset (struct window,
				      toolbar_size[BOTTOM_TOOLBAR]),
			 some_window_value_changed,
			 slot_offset (struct frame,
				      toolbar_size[BOTTOM_TOOLBAR]),
			 toolbar_size_changed_in_frame);

  DEFVAR_SPECIFIER ("left-toolbar-width",
		    &Vtoolbar_size[LEFT_TOOLBAR] /*
*Width of left toolbar.
This is a specifier; use `set-specifier' to change it.

See `default-toolbar-height' for more information.
*/ );
  Vtoolbar_size[LEFT_TOOLBAR] = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vtoolbar_size[LEFT_TOOLBAR],
			 slot_offset (struct window,
				      toolbar_size[LEFT_TOOLBAR]),
			 some_window_value_changed,
			 slot_offset (struct frame,
				      toolbar_size[LEFT_TOOLBAR]),
			 toolbar_size_changed_in_frame);

  DEFVAR_SPECIFIER ("right-toolbar-width",
		    &Vtoolbar_size[RIGHT_TOOLBAR] /*
*Width of right toolbar.
This is a specifier; use `set-specifier' to change it.

See `default-toolbar-height' for more information.
*/ );
  Vtoolbar_size[RIGHT_TOOLBAR] = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vtoolbar_size[RIGHT_TOOLBAR],
			 slot_offset (struct window,
				      toolbar_size[RIGHT_TOOLBAR]),
			 some_window_value_changed,
			 slot_offset (struct frame,
				      toolbar_size[RIGHT_TOOLBAR]),
			 toolbar_size_changed_in_frame);

  /* #### this is ugly. */
  elt = list1 (Fcons (list1 (Qtty), Qzero));
#ifdef HAVE_X_WINDOWS
  elt = Fcons (Fcons (list1 (Qx), make_int (DEFAULT_TOOLBAR_HEIGHT)), elt);
#endif
#ifdef HAVE_NEXTSTEP
  elt = Fcons (Fcons (list1 (Qns), make_int (DEFAULT_TOOLBAR_HEIGHT)), elt);
#endif
  set_specifier_fallback (Vdefault_toolbar_height, elt);
  elt = list1 (Fcons (list1 (Qtty), Qzero));
#ifdef HAVE_X_WINDOWS
  elt = Fcons (Fcons (list1 (Qx), make_int (DEFAULT_TOOLBAR_WIDTH)), elt);
#endif
#ifdef HAVE_NEXTSTEP
  elt = Fcons (Fcons (list1 (Qns), make_int (DEFAULT_TOOLBAR_WIDTH)), elt);
#endif
  set_specifier_fallback (Vdefault_toolbar_width, elt);

  set_specifier_fallback (Vtoolbar_size[TOP_TOOLBAR], Vdefault_toolbar_height);
  elt = list1 (Fcons (Qnil, Qzero));
  set_specifier_fallback (Vtoolbar_size[BOTTOM_TOOLBAR], elt);
  set_specifier_fallback (Vtoolbar_size[LEFT_TOOLBAR], elt);
  set_specifier_fallback (Vtoolbar_size[RIGHT_TOOLBAR], elt);

  DEFVAR_SPECIFIER ("default-toolbar-visible-p", &Vdefault_toolbar_visible_p /*
*Whether the default toolbar is visible.
This is a specifier; use `set-specifier' to change it.

The position of the default toolbar is specified by the function
`set-default-toolbar-position'.  If the corresponding position-specific
toolbar visibility specifier (e.g. `top-toolbar-visible-p' if
`default-toolbar-position' is 'top) does not specify a visible-p value
in a particular domain (a window or a frame), then the value of
`default-toolbar-visible-p' in that domain, if any, will be used
instead.

Both window domains and frame domains are used internally, for
different purposes.  The distinction here is exactly the same as
for thickness specifiers; see `default-toolbar-height' for more
information.

`default-toolbar-visible-p' and all of the position-specific toolbar
visibility specifiers have a fallback value of true.
*/ );
  Vdefault_toolbar_visible_p = Fmake_specifier (Qboolean);
  set_specifier_caching (Vdefault_toolbar_visible_p,
			 slot_offset (struct window,
				      default_toolbar_visible_p),
			 some_window_value_changed,
			 slot_offset (struct frame,
				      default_toolbar_visible_p),
			 default_toolbar_visible_p_changed_in_frame);

  DEFVAR_SPECIFIER ("top-toolbar-visible-p",
		    &Vtoolbar_visible_p[TOP_TOOLBAR] /*
*Whether the top toolbar is visible.
This is a specifier; use `set-specifier' to change it.

See `default-toolbar-visible-p' for more information.
*/ );
  Vtoolbar_visible_p[TOP_TOOLBAR] = Fmake_specifier (Qboolean);
  set_specifier_caching (Vtoolbar_visible_p[TOP_TOOLBAR],
			 slot_offset (struct window,
				      toolbar_visible_p[TOP_TOOLBAR]),
			 some_window_value_changed,
			 slot_offset (struct frame,
				      toolbar_visible_p[TOP_TOOLBAR]),
			 toolbar_visible_p_changed_in_frame);

  DEFVAR_SPECIFIER ("bottom-toolbar-visible-p",
		    &Vtoolbar_visible_p[BOTTOM_TOOLBAR] /*
*Whether the bottom toolbar is visible.
This is a specifier; use `set-specifier' to change it.

See `default-toolbar-visible-p' for more information.
*/ );
  Vtoolbar_visible_p[BOTTOM_TOOLBAR] = Fmake_specifier (Qboolean);
  set_specifier_caching (Vtoolbar_visible_p[BOTTOM_TOOLBAR],
			 slot_offset (struct window,
				      toolbar_visible_p[BOTTOM_TOOLBAR]),
			 some_window_value_changed,
			 slot_offset (struct frame,
				      toolbar_visible_p[BOTTOM_TOOLBAR]),
			 toolbar_visible_p_changed_in_frame);

  DEFVAR_SPECIFIER ("left-toolbar-visible-p",
		    &Vtoolbar_visible_p[LEFT_TOOLBAR] /*
*Whether the left toolbar is visible.
This is a specifier; use `set-specifier' to change it.

See `default-toolbar-visible-p' for more information.
*/ );
  Vtoolbar_visible_p[LEFT_TOOLBAR] = Fmake_specifier (Qboolean);
  set_specifier_caching (Vtoolbar_visible_p[LEFT_TOOLBAR],
			 slot_offset (struct window,
				      toolbar_visible_p[LEFT_TOOLBAR]),
			 some_window_value_changed,
			 slot_offset (struct frame,
				      toolbar_visible_p[LEFT_TOOLBAR]),
			 toolbar_visible_p_changed_in_frame);

  DEFVAR_SPECIFIER ("right-toolbar-visible-p",
		    &Vtoolbar_visible_p[RIGHT_TOOLBAR] /*
*Whether the right toolbar is visible.
This is a specifier; use `set-specifier' to change it.

See `default-toolbar-visible-p' for more information.
*/ );
  Vtoolbar_visible_p[RIGHT_TOOLBAR] = Fmake_specifier (Qboolean);
  set_specifier_caching (Vtoolbar_visible_p[RIGHT_TOOLBAR],
			 slot_offset (struct window,
				      toolbar_visible_p[RIGHT_TOOLBAR]),
			 some_window_value_changed,
			 slot_offset (struct frame,
				      toolbar_visible_p[RIGHT_TOOLBAR]),
			 toolbar_visible_p_changed_in_frame);

  /* initially, top inherits from default; this can be
     changed with `set-default-toolbar-position'. */
  elt = list1 (Fcons (Qnil, Qt));
  set_specifier_fallback (Vdefault_toolbar_visible_p, elt);
  set_specifier_fallback (Vtoolbar_visible_p[TOP_TOOLBAR],
			  Vdefault_toolbar_visible_p);
  set_specifier_fallback (Vtoolbar_visible_p[BOTTOM_TOOLBAR], elt);
  set_specifier_fallback (Vtoolbar_visible_p[LEFT_TOOLBAR], elt);
  set_specifier_fallback (Vtoolbar_visible_p[RIGHT_TOOLBAR], elt);

  DEFVAR_SPECIFIER ("toolbar-buttons-captioned-p",
		    &Vtoolbar_buttons_captioned_p /*
*Whether the toolbar buttons are captioned.
This will only have a visible effect for those toolbar buttons which had
captioned versions specified.
This is a specifier; use `set-specifier' to change it.
*/ );
  Vtoolbar_buttons_captioned_p = Fmake_specifier (Qboolean);
  set_specifier_caching (Vtoolbar_buttons_captioned_p,
			 slot_offset (struct window,
				      toolbar_buttons_captioned_p),
			 toolbar_buttons_captioned_p_changed,
			 0, 0);
  set_specifier_fallback (Vtoolbar_buttons_captioned_p,
			  list1 (Fcons (Qnil, Qt)));
}
