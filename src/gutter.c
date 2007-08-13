/* Gutter implementation.
   Copyright (C) 1999 Andy Piper.

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

/* written by Andy Piper <andy@xemacs.org> with specifiers partially
   ripped-off from toolbar.c */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "frame.h"
#include "device.h"
#include "faces.h"
#include "glyphs.h"
#include "redisplay.h"
#include "window.h"
#include "gutter.h"

Lisp_Object Vgutter[4];
Lisp_Object Vgutter_size[4];
Lisp_Object Vgutter_visible_p[4];
Lisp_Object Vgutter_border_width[4];

Lisp_Object Vdefault_gutter, Vdefault_gutter_visible_p;
Lisp_Object Vdefault_gutter_width, Vdefault_gutter_height;
Lisp_Object Vdefault_gutter_border_width;

Lisp_Object Vdefault_gutter_position;

Lisp_Object Qgutter_size;

#define SET_GUTTER_WAS_VISIBLE_FLAG(frame, pos, flag)	\
  do {							\
    switch (pos)					\
      {							\
      case TOP_GUTTER:					\
	(frame)->top_gutter_was_visible = flag;		\
	break;						\
      case BOTTOM_GUTTER:				\
	(frame)->bottom_gutter_was_visible = flag;	\
	break;						\
      case LEFT_GUTTER:					\
	(frame)->left_gutter_was_visible = flag;	\
	break;						\
      case RIGHT_GUTTER:				\
	(frame)->right_gutter_was_visible = flag;	\
	break;						\
      default:						\
	abort ();					\
      }							\
  } while (0)

static int gutter_was_visible (struct frame* frame, enum gutter_pos pos)
{
  switch (pos)
    {
    case TOP_GUTTER:
      return frame->top_gutter_was_visible;
    case BOTTOM_GUTTER:
      return frame->bottom_gutter_was_visible;
    case LEFT_GUTTER:
      return frame->left_gutter_was_visible;
    case RIGHT_GUTTER:
      return frame->right_gutter_was_visible;
    default:
      abort ();
    }
}

static Lisp_Object
frame_topmost_window (struct frame *f)
{
  Lisp_Object w = FRAME_ROOT_WINDOW (f);

  do {
    while (!NILP (XWINDOW (w)->vchild))
      {
	w = XWINDOW (w)->vchild;
      }
  } while (!NILP (XWINDOW (w)->hchild) && !NILP (w = XWINDOW (w)->hchild));

  return w;
}

static Lisp_Object
frame_bottommost_window (struct frame *f)
{
  Lisp_Object w = FRAME_ROOT_WINDOW (f);

  do {
    while (!NILP (XWINDOW (w)->vchild))
      {
	w = XWINDOW (w)->vchild;
	while (!NILP (XWINDOW (w)->next))
	  {
	    w = XWINDOW (w)->next;
	  }
      }
  } while (!NILP (XWINDOW (w)->hchild) && !NILP (w = XWINDOW (w)->hchild));

  return w;
}

#if 0
static Lisp_Object
frame_leftmost_window (struct frame *f)
{
  Lisp_Object w = FRAME_ROOT_WINDOW (f);

  do {
    while (!NILP (XWINDOW (w)->hchild))
      {
	w = XWINDOW (w)->hchild;
      }
  } while (!NILP (XWINDOW (w)->vchild) && !NILP (w = XWINDOW (w)->vchild));

  return w;
}

static Lisp_Object
frame_rightmost_window (struct frame *f)
{
  Lisp_Object w = FRAME_ROOT_WINDOW (f);

  do {
    while (!NILP (XWINDOW (w)->hchild))
      {
	w = XWINDOW (w)->hchild;
	while (!NILP (XWINDOW (w)->next))
	  {
	    w = XWINDOW (w)->next;
	  }
      }
  } while (!NILP (XWINDOW (w)->vchild) && !NILP (w = XWINDOW (w)->vchild));
  return w;
}
#endif

/* calculate the coordinates of a gutter for the current frame and
   selected window. we have to be careful in calculating this as we
   need to use *two* windows, the currently selected window will give
   us the actual height, width and contents of the gutter, but if we
   use this for calculating the gutter positions we run into trouble
   if it is not the window nearest the gutter. Instead we predetermine
   the nearest window and then use that.*/
static void
get_gutter_coords (struct frame *f, enum gutter_pos pos, int *x, int *y,
		   int *width, int *height)
{
  struct window
    * top = XWINDOW (frame_topmost_window (f)),
    * bot = XWINDOW (frame_bottommost_window (f));
  /* The top and bottom gutters take precedence over the left and
     right. */
  switch (pos)
    {
    case TOP_GUTTER:
      *x = FRAME_LEFT_BORDER_END (f);
      *y = FRAME_TOP_BORDER_END (f);
      *width = FRAME_RIGHT_BORDER_START (f) 
	- FRAME_LEFT_BORDER_END (f);
      *height = FRAME_TOP_GUTTER_BOUNDS (f);
      break;

    case BOTTOM_GUTTER:
      *x = FRAME_LEFT_BORDER_END (f);
      *y = WINDOW_BOTTOM (bot)
	- FRAME_BOTTOM_GUTTER_BOUNDS (f);
      *width = FRAME_RIGHT_BORDER_START (f) 
	- FRAME_LEFT_BORDER_END (f);
      *height = FRAME_BOTTOM_GUTTER_BOUNDS (f);
      break;

    case LEFT_GUTTER:
      *x = FRAME_LEFT_BORDER_END (f);
      *y = WINDOW_TEXT_TOP (top);
      *width = FRAME_LEFT_GUTTER_BOUNDS (f);
      *height = WINDOW_BOTTOM (bot)
	- (WINDOW_TEXT_TOP (top)
	   + FRAME_BOTTOM_GUTTER_BOUNDS (f));
      break;
      
    case RIGHT_GUTTER:
      *x = FRAME_RIGHT_BORDER_START (f)
	- FRAME_RIGHT_GUTTER_BOUNDS (f);
      *y = WINDOW_TEXT_TOP (top);
      *width = FRAME_RIGHT_GUTTER_BOUNDS (f);
      *height = WINDOW_BOTTOM (bot)
	- (WINDOW_TEXT_TOP (top) 
	   + FRAME_BOTTOM_GUTTER_BOUNDS (f));
      break;

    default:
      abort ();
    }
}

static void
output_gutter (struct frame *f, enum gutter_pos pos)
{
  Lisp_Object frame;
  Lisp_Object window = FRAME_LAST_NONMINIBUF_WINDOW (f);
  struct device *d = XDEVICE (f->device);
  struct window* w = XWINDOW (window);
  int x, y, width, height, ypos;
  int line, border_width;
  face_index findex;
  display_line_dynarr* ddla, *cdla;
  struct display_line *dl;
  int cdla_len;

  if (!WINDOW_LIVE_P (w))
    return;

  border_width = FRAME_GUTTER_BORDER_WIDTH (f, pos);
  findex = get_builtin_face_cache_index (w, Vgui_element_face);

  if (!f->current_display_lines)
    f->current_display_lines = Dynarr_new (display_line);
  if (!f->desired_display_lines)
    f->desired_display_lines = Dynarr_new (display_line);

  ddla = f->desired_display_lines;
  cdla = f->current_display_lines;
  cdla_len = Dynarr_length (cdla);

  XSETFRAME (frame, f);

  get_gutter_coords (f, pos, &x, &y, &width, &height);
  /* generate some display lines */
  generate_displayable_area (w, WINDOW_GUTTER (w, pos),
			     x + border_width, y + border_width,
			     width - 2 * border_width, 
			     height - 2 * border_width, ddla, 0, findex);
  /* Output each line. */
  for (line = 0; line < Dynarr_length (ddla); line++)
    {
      output_display_line (w, cdla, ddla, line, -1, -1);
    }

  /* If the number of display lines has shrunk, adjust. */
  if (cdla_len > Dynarr_length (ddla))
    {
      Dynarr_length (cdla) = Dynarr_length (ddla);
    }

  /* grab coordinates of last line and blank after it. */
  dl = Dynarr_atp (ddla, Dynarr_length (ddla) - 1);
  ypos = dl->ypos + dl->descent - dl->clip;
  redisplay_clear_region (window, findex, x + border_width , ypos,
			  width - 2 * border_width, height - (ypos - y) - border_width);
  /* bevel the gutter area if so desired */
  if (border_width != 0)
    {
      MAYBE_DEVMETH (d, bevel_area, 
		     (w, findex, x, y, width, height, border_width,
		      EDGE_ALL, EDGE_BEVEL_OUT));
    }
}

/* sizing gutters is a pain so we try and help the user by detemining
   what height will accommodate all lines. This is useless on left and
   right gutters as we always have a maximal number of lines. */
static Lisp_Object
calculate_gutter_size (struct window *w, enum gutter_pos pos)
{
  struct frame* f = XFRAME (WINDOW_FRAME (w));
  int ypos;
  display_line_dynarr* ddla;
  struct display_line *dl;

  /* we cannot autodetect gutter sizes for the left and right as there
     is no reasonable metric to use */
  assert (pos == TOP_GUTTER || pos == BOTTOM_GUTTER);
  /* degenerate case */
  if (NILP (WINDOW_GUTTER (w, pos))
      ||
      !FRAME_VISIBLE_P (f)
      ||
      NILP (w->buffer))
    return Qnil;

  ddla = Dynarr_new (display_line);
  /* generate some display lines */
  generate_displayable_area (w, WINDOW_GUTTER (w, pos),
			     FRAME_LEFT_BORDER_END (f),
			     0,
			     FRAME_RIGHT_BORDER_START (f)
			     - FRAME_LEFT_BORDER_END (f),
			     200,
			     ddla, 0, 0);
  /* grab coordinates of last line  */
  if (Dynarr_length (ddla))
    {
      dl = Dynarr_atp (ddla, Dynarr_length (ddla) - 1);
      ypos = dl->ypos + dl->descent - dl->clip;
      free_display_lines (ddla);
      return make_int (ypos);
    }
  else
    {
      free_display_lines (ddla);
      return Qnil;
    }
}

static void
clear_gutter (struct frame *f, enum gutter_pos pos)
{
  int x, y, width, height;
  Lisp_Object window = FRAME_LAST_NONMINIBUF_WINDOW (f);
  face_index findex = get_builtin_face_cache_index (XWINDOW (window),
						    Vgui_element_face);
  get_gutter_coords (f, pos, &x, &y, &width, &height);

  SET_GUTTER_WAS_VISIBLE_FLAG (f, pos, 0);

  redisplay_clear_region (window, findex, x, y, width, height);
}

/* #### I don't currently believe that redisplay needs to mark the
   glyphs in its structures since these will always be referenced from
   somewhere else. However, I'm not sure enough to stake my life on it
   at this point, so we do the safe thing. */

/* See the comment in image_instantiate_cache_result as to why marking
   the glyph will also mark the image_instance. */
void
mark_gutters (struct frame* f)
{
  if (f->current_display_lines)
    mark_redisplay_structs (f->current_display_lines);
  if (f->desired_display_lines)
    mark_redisplay_structs (f->desired_display_lines);
}

void
update_frame_gutters (struct frame *f)
{
  if (f->gutter_changed || f->clear ||
      f->glyphs_changed || f->subwindows_changed || 
      f->windows_changed || f->windows_structure_changed || 
      f->extents_changed || f->faces_changed)
    {
      enum gutter_pos pos;
      
      /* We don't actually care about these when outputting the gutter
         so locally disable them. */
      int local_clip_changed = f->clip_changed;
      int local_buffers_changed = f->buffers_changed;
      f->clip_changed = 0;
      f->buffers_changed = 0;

      /* and output */
      GUTTER_POS_LOOP (pos)
	{
	  if (FRAME_GUTTER_VISIBLE (f, pos))
	      output_gutter (f, pos);
	  else if (gutter_was_visible (f, pos))
	    clear_gutter (f, pos);
	}
      f->clip_changed = local_clip_changed;
      f->buffers_changed = local_buffers_changed;
      f->gutter_changed = 0;
    }
}

void
reset_gutter_display_lines (struct frame* f)
{
  if (f->current_display_lines)
    Dynarr_reset (f->current_display_lines);
}

static void
redraw_exposed_gutter (struct frame *f, enum gutter_pos pos, int x, int y,
		       int width, int height)
{
  int g_x, g_y, g_width, g_height;

  get_gutter_coords (f, pos, &g_x, &g_y, &g_width, &g_height);

  if (((y + height) < g_y) || (y > (g_y + g_height)) || !height || !width || !g_height || !g_width)
    return;
  if (((x + width) < g_x) || (x > (g_x + g_width)))
    return;

  /* #### optimize this - redrawing the whole gutter for every expose
     is very expensive. We reset the current display lines because if
     they're being exposed they are no longer current. */
  reset_gutter_display_lines (f);

  /* Even if none of the gutter is in the area, the blank region at
     the very least must be because the first thing we did is verify
     that some portion of the gutter is in the exposed region. */
  output_gutter (f, pos);
}

void
redraw_exposed_gutters (struct frame *f, int x, int y, int width,
			int height)
{
  enum gutter_pos pos;
  GUTTER_POS_LOOP (pos)
    {
      if (FRAME_GUTTER_VISIBLE (f, pos))
	redraw_exposed_gutter (f, pos, x, y, width, height);
    }
}

void
free_frame_gutters (struct frame *f)
{
  if (f->current_display_lines)
    {
      free_display_lines (f->current_display_lines);
      f->current_display_lines = 0;
    }
  if (f->desired_display_lines)
    {
      free_display_lines (f->desired_display_lines);
      f->desired_display_lines = 0;
    }
}

static enum gutter_pos
decode_gutter_position (Lisp_Object position)
{
  if (EQ (position, Qtop))    return TOP_GUTTER;
  if (EQ (position, Qbottom)) return BOTTOM_GUTTER;
  if (EQ (position, Qleft))   return LEFT_GUTTER;
  if (EQ (position, Qright))  return RIGHT_GUTTER;
  signal_simple_error ("Invalid gutter position", position);

  return TOP_GUTTER; /* not reached */
}

DEFUN ("set-default-gutter-position", Fset_default_gutter_position, 1, 1, 0, /*
Set the position that the `default-gutter' will be displayed at.
Valid positions are 'top, 'bottom, 'left and 'right.
See `default-gutter-position'.
*/
       (position))
{
  enum gutter_pos cur = decode_gutter_position (Vdefault_gutter_position);
  enum gutter_pos new = decode_gutter_position (position);

  if (cur != new)
    {
      /* The following calls will automatically cause the dirty
	 flags to be set; we delay frame size changes to avoid
	 lots of frame flickering. */
      /* #### I think this should be GC protected. -sb */
      hold_frame_size_changes ();
      set_specifier_fallback (Vgutter[cur], list1 (Fcons (Qnil, Qnil)));
      set_specifier_fallback (Vgutter[new], Vdefault_gutter);
      set_specifier_fallback (Vgutter_size[cur], list1 (Fcons (Qnil, Qzero)));
      set_specifier_fallback (Vgutter_size[new],
			      new == TOP_GUTTER || new == BOTTOM_GUTTER
			      ? Vdefault_gutter_height
			      : Vdefault_gutter_width);
      set_specifier_fallback (Vgutter_border_width[cur],
			      list1 (Fcons (Qnil, Qzero)));
      set_specifier_fallback (Vgutter_border_width[new],
			      Vdefault_gutter_border_width);
      set_specifier_fallback (Vgutter_visible_p[cur],
			      list1 (Fcons (Qnil, Qt)));
      set_specifier_fallback (Vgutter_visible_p[new],
			      Vdefault_gutter_visible_p);
      Vdefault_gutter_position = position;
      unhold_frame_size_changes ();
    }

  return position;
}

DEFUN ("default-gutter-position", Fdefault_gutter_position, 0, 0, 0, /*
Return the position that the `default-gutter' will be displayed at.
The `default-gutter' will only be displayed here if the corresponding
position-specific gutter specifier does not provide a value.
*/
       ())
{
  return Vdefault_gutter_position;
}

DEFUN ("gutter-pixel-width", Fgutter_pixel_width, 0, 2, 0, /*
Return the pixel width of the gutter at POS in LOCALE.
POS defaults to the default gutter position. LOCALE defaults to
the current window.
*/
       (pos, locale))
{
  int x, y, width, height;
  enum gutter_pos p = TOP_GUTTER;
  struct frame *f = decode_frame (FW_FRAME (locale));

  if (NILP (pos))
    pos = Vdefault_gutter_position;
  p = decode_gutter_position (pos);

  get_gutter_coords (f, p, &x, &y, &width, &height);
  width -= (FRAME_GUTTER_BORDER_WIDTH (f, p) * 2);

  return make_int (width);
}

DEFUN ("gutter-pixel-height", Fgutter_pixel_height, 0, 2, 0, /*
Return the pixel height of the gutter at POS in LOCALE.
POS defaults to the default gutter position. LOCALE defaults to
the current window.
*/
       (pos, locale))
{
  int x, y, width, height;
  enum gutter_pos p = TOP_GUTTER;
  struct frame *f = decode_frame (FW_FRAME (locale));

  if (NILP (pos))
    pos = Vdefault_gutter_position;
  p = decode_gutter_position (pos);

  get_gutter_coords (f, p, &x, &y, &width, &height);
  height -= (FRAME_GUTTER_BORDER_WIDTH (f, p) * 2);

  return make_int (height);
}

DEFINE_SPECIFIER_TYPE (gutter);

static void
gutter_after_change (Lisp_Object specifier, Lisp_Object locale)
{
  MARK_GUTTER_CHANGED;
}

static void
gutter_validate (Lisp_Object instantiator)
{
  if (NILP (instantiator))
    return;

  if (!STRINGP (instantiator))
    signal_simple_error ("Gutter spec must be string or nil", instantiator);
}

DEFUN ("gutter-specifier-p", Fgutter_specifier_p, 1, 1, 0, /*
Return non-nil if OBJECT is a gutter specifier.
Gutter specifiers are used to specify the format of a gutter.
The values of the variables `default-gutter', `top-gutter',
`left-gutter', `right-gutter', and `bottom-gutter' are always
gutter specifiers.

Valid gutter instantiators are called "gutter descriptors"
and are lists of vectors.  See `default-gutter' for a description
of the exact format.
*/
       (object))
{
  return GUTTER_SPECIFIERP (object) ? Qt : Qnil;
}


/*
  Helper for invalidating the real specifier when default
  specifier caching changes
*/
static void
recompute_overlaying_specifier (Lisp_Object real_one[4])
{
  enum gutter_pos pos = decode_gutter_position (Vdefault_gutter_position);
  Fset_specifier_dirty_flag (real_one[pos]);
}

static void
gutter_specs_changed (Lisp_Object specifier, struct window *w,
		       Lisp_Object oldval)
{
  enum gutter_pos pos;
  GUTTER_POS_LOOP (pos)
    {
      w->real_gutter_size[pos] = w->gutter_size[pos];
      if (EQ (w->real_gutter_size[pos], Qautodetect)
	  && !NILP (w->gutter_visible_p[pos]))
	{
	  w->real_gutter_size [pos] = calculate_gutter_size (w, pos);
	}
    }
  MARK_GUTTER_CHANGED;
  MARK_WINDOWS_CHANGED (w);
}

static void
default_gutter_specs_changed (Lisp_Object specifier, struct window *w,
			       Lisp_Object oldval)
{
  recompute_overlaying_specifier (Vgutter);
}

static void
gutter_geometry_changed_in_window (Lisp_Object specifier, struct window *w,
				    Lisp_Object oldval)
{
  enum gutter_pos pos;
  GUTTER_POS_LOOP (pos)
    {
      w->real_gutter_size[pos] = w->gutter_size[pos];
      if (EQ (w->real_gutter_size[pos], Qautodetect)
	  && !NILP (w->gutter_visible_p[pos]))
	{
	  w->real_gutter_size [pos] = calculate_gutter_size (w, pos);
	}
    }
  
  MARK_GUTTER_CHANGED;
  MARK_WINDOWS_CHANGED (w);
}

static void
default_gutter_size_changed_in_window (Lisp_Object specifier, struct window *w,
					Lisp_Object oldval)
{
  recompute_overlaying_specifier (Vgutter_size);
}

static void
default_gutter_border_width_changed_in_window (Lisp_Object specifier,
						struct window *w,
						Lisp_Object oldval)
{
  recompute_overlaying_specifier (Vgutter_border_width);
}

static void
default_gutter_visible_p_changed_in_window (Lisp_Object specifier,
					     struct window *w,
					     Lisp_Object oldval)
{
  recompute_overlaying_specifier (Vgutter_visible_p);
}


DECLARE_SPECIFIER_TYPE (gutter_size);
#define GUTTER_SIZE_SPECIFIERP(x) SPECIFIER_TYPEP (x, gutter_size)
DEFINE_SPECIFIER_TYPE (gutter_size);

static void
gutter_size_validate (Lisp_Object instantiator)
{
  if (NILP (instantiator))
    return;

  if (!INTP (instantiator) && !EQ (instantiator, Qautodetect))
    signal_simple_error ("Gutter size must be an integer or 'autodetect", instantiator);
}

DEFUN ("gutter-size-specifier-p", Fgutter_size_specifier_p, 1, 1, 0, /*
Return non-nil if OBJECT is a gutter-size specifier.
*/
       (object))
{
  return GUTTER_SIZE_SPECIFIERP (object) ? Qt : Qnil;
}

DEFUN ("redisplay-gutter-area", Fredisplay_gutter_area, 0, 0, 0, /*
Ensure that all gutters are correctly showing their gutter specifier.
*/
       ())
{
  Lisp_Object devcons, concons;

  DEVICE_LOOP_NO_BREAK (devcons, concons)
    {
      struct device *d = XDEVICE (XCAR (devcons));
      Lisp_Object frmcons;

      DEVICE_FRAME_LOOP (frmcons, d)
	{
	  struct frame *f = XFRAME (XCAR (frmcons));

	  if (FRAME_REPAINT_P (f))
	    {
	      update_frame_gutters (f);
	    }
	}

      /* We now call the output_end routine for tty frames.  We delay
	 doing so in order to avoid cursor flicker.  So much for 100%
	 encapsulation. */
      if (DEVICE_TTY_P (d))
	DEVMETH (d, output_end, (d));
    }

  return Qnil;
}

void
init_frame_gutters (struct frame *f)
{
  enum gutter_pos pos;
  struct window* w = XWINDOW (FRAME_LAST_NONMINIBUF_WINDOW (f));
  /* We are here as far in frame creation so cached specifiers are
     already recomputed, and possibly modified by resource
     initialization. We need to recalculate autodetected gutters. */
  GUTTER_POS_LOOP (pos)
    {
      w->real_gutter_size[pos] = w->gutter_size[pos];
      if (EQ (w->gutter_size[pos], Qautodetect)
	  && !NILP (w->gutter_visible_p[pos]))
	{
	  w->real_gutter_size [pos] = calculate_gutter_size (w, pos);
	  MARK_GUTTER_CHANGED;
	  MARK_WINDOWS_CHANGED (w);
	}
    }
}

void
syms_of_gutter (void)
{
  DEFSUBR (Fgutter_specifier_p);
  DEFSUBR (Fgutter_size_specifier_p);
  DEFSUBR (Fset_default_gutter_position);
  DEFSUBR (Fdefault_gutter_position);
  DEFSUBR (Fgutter_pixel_height);
  DEFSUBR (Fgutter_pixel_width);
  DEFSUBR (Fredisplay_gutter_area);

  defsymbol (&Qgutter_size, "gutter-size");
}

void
vars_of_gutter (void)
{
  staticpro (&Vdefault_gutter_position);
  Vdefault_gutter_position = Qtop;

  Fprovide (Qgutter);
}

void
specifier_type_create_gutter (void)
{
  INITIALIZE_SPECIFIER_TYPE (gutter, "gutter", "gutter-specifier-p");

  SPECIFIER_HAS_METHOD (gutter, validate);
  SPECIFIER_HAS_METHOD (gutter, after_change);

  INITIALIZE_SPECIFIER_TYPE (gutter_size, "gutter-size", "gutter-size-specifier-p");

  SPECIFIER_HAS_METHOD (gutter_size, validate);
}

void
reinit_specifier_type_create_gutter (void)
{
  REINITIALIZE_SPECIFIER_TYPE (gutter);
  REINITIALIZE_SPECIFIER_TYPE (gutter_size);
}

void
specifier_vars_of_gutter (void)
{
  Lisp_Object fb;

  DEFVAR_SPECIFIER ("default-gutter", &Vdefault_gutter /*
Specifier for a fallback gutter.
Use `set-specifier' to change this.

The position of this gutter is specified in the function
`default-gutter-position'.  If the corresponding position-specific
gutter (e.g. `top-gutter' if `default-gutter-position' is 'top)
does not specify a gutter in a particular domain (usually a window),
then the value of `default-gutter' in that domain, if any, will be
used instead.

Note that the gutter at any particular position will not be
displayed unless its visibility flag is true and its thickness
\(width or height, depending on orientation) is non-zero.  The
visibility is controlled by the specifiers `top-gutter-visible-p',
`bottom-gutter-visible-p', `left-gutter-visible-p', and
`right-gutter-visible-p', and the thickness is controlled by the
specifiers `top-gutter-height', `bottom-gutter-height',
`left-gutter-width', and `right-gutter-width'.

Note that one of the four visibility specifiers inherits from
`default-gutter-visibility' and one of the four thickness
specifiers inherits from either `default-gutter-width' or
`default-gutter-height' (depending on orientation), just
like for the gutter description specifiers (e.g. `top-gutter')
mentioned above.

Therefore, if you are setting `default-gutter', you should control
the visibility and thickness using `default-gutter-visible-p',
`default-gutter-width', and `default-gutter-height', rather than
using position-specific specifiers.  That way, you will get sane
behavior if the user changes the default gutter position.

The gutter value should be a string or nil. You can attach extents and
glyphs to the string and hence display glyphs and text in other fonts
in the gutter area.

*/ );

  Vdefault_gutter = Fmake_specifier (Qgutter);
  /* #### It would be even nicer if the specifier caching
     automatically knew about specifier fallbacks, so we didn't
     have to do it ourselves. */
  set_specifier_caching (Vdefault_gutter,
			 offsetof (struct window, default_gutter),
			 default_gutter_specs_changed,
			 0, 0);

  DEFVAR_SPECIFIER ("top-gutter",
		    &Vgutter[TOP_GUTTER] /*
Specifier for the gutter at the top of the frame.
Use `set-specifier' to change this.
See `default-gutter' for a description of a valid gutter instantiator.
*/ );
  Vgutter[TOP_GUTTER] = Fmake_specifier (Qgutter);
  set_specifier_caching (Vgutter[TOP_GUTTER],
			 offsetof (struct window, gutter[TOP_GUTTER]),
			 gutter_specs_changed,
			 0, 0);

  DEFVAR_SPECIFIER ("bottom-gutter",
		    &Vgutter[BOTTOM_GUTTER] /*
Specifier for the gutter at the bottom of the frame.
Use `set-specifier' to change this.
See `default-gutter' for a description of a valid gutter instantiator.

Note that, unless the `default-gutter-position' is `bottom', by
default the height of the bottom gutter (controlled by
`bottom-gutter-height') is 0; thus, a bottom gutter will not be
displayed even if you provide a value for `bottom-gutter'.
*/ );
  Vgutter[BOTTOM_GUTTER] = Fmake_specifier (Qgutter);
  set_specifier_caching (Vgutter[BOTTOM_GUTTER],
			 offsetof (struct window, gutter[BOTTOM_GUTTER]),
			 gutter_specs_changed,
			 0, 0);

  DEFVAR_SPECIFIER ("left-gutter",
		    &Vgutter[LEFT_GUTTER] /*
Specifier for the gutter at the left edge of the frame.
Use `set-specifier' to change this.
See `default-gutter' for a description of a valid gutter instantiator.

Note that, unless the `default-gutter-position' is `left', by
default the height of the left gutter (controlled by
`left-gutter-width') is 0; thus, a left gutter will not be
displayed even if you provide a value for `left-gutter'.
*/ );
  Vgutter[LEFT_GUTTER] = Fmake_specifier (Qgutter);
  set_specifier_caching (Vgutter[LEFT_GUTTER],
			 offsetof (struct window, gutter[LEFT_GUTTER]),
			 gutter_specs_changed,
			 0, 0);

  DEFVAR_SPECIFIER ("right-gutter",
		    &Vgutter[RIGHT_GUTTER] /*
Specifier for the gutter at the right edge of the frame.
Use `set-specifier' to change this.
See `default-gutter' for a description of a valid gutter instantiator.

Note that, unless the `default-gutter-position' is `right', by
default the height of the right gutter (controlled by
`right-gutter-width') is 0; thus, a right gutter will not be
displayed even if you provide a value for `right-gutter'.
*/ );
  Vgutter[RIGHT_GUTTER] = Fmake_specifier (Qgutter);
  set_specifier_caching (Vgutter[RIGHT_GUTTER],
			 offsetof (struct window, gutter[RIGHT_GUTTER]),
			 gutter_specs_changed,
			 0, 0);

  /* initially, top inherits from default; this can be
     changed with `set-default-gutter-position'. */
  fb = list1 (Fcons (Qnil, Qnil));
  set_specifier_fallback (Vdefault_gutter, fb);
  set_specifier_fallback (Vgutter[TOP_GUTTER], Vdefault_gutter);
  set_specifier_fallback (Vgutter[BOTTOM_GUTTER], fb);
  set_specifier_fallback (Vgutter[LEFT_GUTTER],   fb);
  set_specifier_fallback (Vgutter[RIGHT_GUTTER],  fb);

  DEFVAR_SPECIFIER ("default-gutter-height", &Vdefault_gutter_height /*
*Height of the default gutter, if it's oriented horizontally.
This is a specifier; use `set-specifier' to change it.

The position of the default gutter is specified by the function
`set-default-gutter-position'.  If the corresponding position-specific
gutter thickness specifier (e.g. `top-gutter-height' if
`default-gutter-position' is 'top) does not specify a thickness in a
particular domain (a window or a frame), then the value of
`default-gutter-height' or `default-gutter-width' (depending on the
gutter orientation) in that domain, if any, will be used instead.

Note that `default-gutter-height' is only used when
`default-gutter-position' is 'top or 'bottom, and `default-gutter-width'
is only used when `default-gutter-position' is 'left or 'right.

Note that all of the position-specific gutter thickness specifiers
have a fallback value of zero when they do not correspond to the
default gutter.  Therefore, you will have to set a non-zero thickness
value if you want a position-specific gutter to be displayed.

If you set the height to 'autodetect the size of the gutter will be
calculated to be large enough to hold the contents of the gutter. This
is the default.
*/ );
  Vdefault_gutter_height = Fmake_specifier (Qgutter_size);
  set_specifier_caching (Vdefault_gutter_height,
			 offsetof (struct window, default_gutter_height),
			 default_gutter_size_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("default-gutter-width", &Vdefault_gutter_width /*
*Width of the default gutter, if it's oriented vertically.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-height' for more information.
*/ );
  Vdefault_gutter_width = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vdefault_gutter_width,
			 offsetof (struct window, default_gutter_width),
			 default_gutter_size_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("top-gutter-height",
		    &Vgutter_size[TOP_GUTTER] /*
*Height of the top gutter.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-height' for more information.
*/ );
  Vgutter_size[TOP_GUTTER] = Fmake_specifier (Qgutter_size);
  set_specifier_caching (Vgutter_size[TOP_GUTTER],
			 offsetof (struct window, gutter_size[TOP_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("bottom-gutter-height",
		    &Vgutter_size[BOTTOM_GUTTER] /*
*Height of the bottom gutter.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-height' for more information.
*/ );
  Vgutter_size[BOTTOM_GUTTER] = Fmake_specifier (Qgutter_size);
  set_specifier_caching (Vgutter_size[BOTTOM_GUTTER],
			 offsetof (struct window, gutter_size[BOTTOM_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("left-gutter-width",
		    &Vgutter_size[LEFT_GUTTER] /*
*Width of left gutter.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-height' for more information.
*/ );
  Vgutter_size[LEFT_GUTTER] = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vgutter_size[LEFT_GUTTER],
			 offsetof (struct window, gutter_size[LEFT_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("right-gutter-width",
		    &Vgutter_size[RIGHT_GUTTER] /*
*Width of right gutter.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-height' for more information.
*/ );
  Vgutter_size[RIGHT_GUTTER] = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vgutter_size[RIGHT_GUTTER],
			 offsetof (struct window, gutter_size[RIGHT_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  fb = Qnil;
#ifdef HAVE_TTY
  fb = Fcons (Fcons (list1 (Qtty), Qautodetect), fb);
#endif
#ifdef HAVE_X_WINDOWS
  fb = Fcons (Fcons (list1 (Qx), Qautodetect), fb);
#endif
#ifdef HAVE_MS_WINDOWS
  fb = Fcons (Fcons (list1 (Qmsprinter), Qautodetect), fb);
  fb = Fcons (Fcons (list1 (Qmswindows), Qautodetect), fb);
#endif
  if (!NILP (fb))
    set_specifier_fallback (Vdefault_gutter_height, fb);

  fb = Qnil;
#ifdef HAVE_TTY
  fb = Fcons (Fcons (list1 (Qtty), Qzero), fb);
#endif
#ifdef HAVE_X_WINDOWS
  fb = Fcons (Fcons (list1 (Qx), make_int (DEFAULT_GUTTER_WIDTH)), fb);
#endif
#ifdef HAVE_MS_WINDOWS
  fb = Fcons (Fcons (list1 (Qmsprinter), Qzero), fb);
  fb = Fcons (Fcons (list1 (Qmswindows), 
		     make_int (DEFAULT_GUTTER_WIDTH)), fb);
#endif
  if (!NILP (fb))
    set_specifier_fallback (Vdefault_gutter_width, fb);

  set_specifier_fallback (Vgutter_size[TOP_GUTTER], Vdefault_gutter_height);
  fb = list1 (Fcons (Qnil, Qzero));
  set_specifier_fallback (Vgutter_size[BOTTOM_GUTTER], fb);
  set_specifier_fallback (Vgutter_size[LEFT_GUTTER],   fb);
  set_specifier_fallback (Vgutter_size[RIGHT_GUTTER],  fb);

  DEFVAR_SPECIFIER ("default-gutter-border-width",
		    &Vdefault_gutter_border_width /*
*Width of the border around the default gutter.
This is a specifier; use `set-specifier' to change it.

The position of the default gutter is specified by the function
`set-default-gutter-position'.  If the corresponding position-specific
gutter border width specifier (e.g. `top-gutter-border-width' if
`default-gutter-position' is 'top) does not specify a border width in a
particular domain (a window or a frame), then the value of
`default-gutter-border-width' in that domain, if any, will be used
instead.

*/ );
  Vdefault_gutter_border_width = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vdefault_gutter_border_width,
			 offsetof (struct window, default_gutter_border_width),
			 default_gutter_border_width_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("top-gutter-border-width",
		    &Vgutter_border_width[TOP_GUTTER] /*
*Border width of the top gutter.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-height' for more information.
*/ );
  Vgutter_border_width[TOP_GUTTER] = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vgutter_border_width[TOP_GUTTER],
			 offsetof (struct window,
				   gutter_border_width[TOP_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("bottom-gutter-border-width",
		    &Vgutter_border_width[BOTTOM_GUTTER] /*
*Border width of the bottom gutter.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-height' for more information.
*/ );
  Vgutter_border_width[BOTTOM_GUTTER] = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vgutter_border_width[BOTTOM_GUTTER],
			 offsetof (struct window,
				   gutter_border_width[BOTTOM_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("left-gutter-border-width",
		    &Vgutter_border_width[LEFT_GUTTER] /*
*Border width of left gutter.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-height' for more information.
*/ );
  Vgutter_border_width[LEFT_GUTTER] = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vgutter_border_width[LEFT_GUTTER],
			 offsetof (struct window,
				   gutter_border_width[LEFT_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("right-gutter-border-width",
		    &Vgutter_border_width[RIGHT_GUTTER] /*
*Border width of right gutter.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-height' for more information.
*/ );
  Vgutter_border_width[RIGHT_GUTTER] = Fmake_specifier (Qnatnum);
  set_specifier_caching (Vgutter_border_width[RIGHT_GUTTER],
			 offsetof (struct window,
				   gutter_border_width[RIGHT_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  fb = Qnil;
#ifdef HAVE_TTY
  fb = Fcons (Fcons (list1 (Qtty), Qzero), fb);
#endif
#ifdef HAVE_X_WINDOWS
  fb = Fcons (Fcons (list1 (Qx), make_int (DEFAULT_GUTTER_BORDER_WIDTH)), fb);
#endif
#ifdef HAVE_MS_WINDOWS
  fb = Fcons (Fcons (list1 (Qmsprinter), Qzero), fb);
  fb = Fcons (Fcons (list1 (Qmswindows), make_int (DEFAULT_GUTTER_BORDER_WIDTH)), fb);
#endif
  if (!NILP (fb))
    set_specifier_fallback (Vdefault_gutter_border_width, fb);

  set_specifier_fallback (Vgutter_border_width[TOP_GUTTER], Vdefault_gutter_border_width);
  fb = list1 (Fcons (Qnil, Qzero));
  set_specifier_fallback (Vgutter_border_width[BOTTOM_GUTTER], fb);
  set_specifier_fallback (Vgutter_border_width[LEFT_GUTTER],   fb);
  set_specifier_fallback (Vgutter_border_width[RIGHT_GUTTER],  fb);

  DEFVAR_SPECIFIER ("default-gutter-visible-p", &Vdefault_gutter_visible_p /*
*Whether the default gutter is visible.
This is a specifier; use `set-specifier' to change it.

The position of the default gutter is specified by the function
`set-default-gutter-position'.  If the corresponding position-specific
gutter visibility specifier (e.g. `top-gutter-visible-p' if
`default-gutter-position' is 'top) does not specify a visible-p value
in a particular domain (a window or a frame), then the value of
`default-gutter-visible-p' in that domain, if any, will be used
instead.

`default-gutter-visible-p' and all of the position-specific gutter
visibility specifiers have a fallback value of true.
*/ );
  Vdefault_gutter_visible_p = Fmake_specifier (Qboolean);
  set_specifier_caching (Vdefault_gutter_visible_p,
			 offsetof (struct window,
				   default_gutter_visible_p),
			 default_gutter_visible_p_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("top-gutter-visible-p",
		    &Vgutter_visible_p[TOP_GUTTER] /*
*Whether the top gutter is visible.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-visible-p' for more information.
*/ );
  Vgutter_visible_p[TOP_GUTTER] = Fmake_specifier (Qboolean);
  set_specifier_caching (Vgutter_visible_p[TOP_GUTTER],
			 offsetof (struct window,
				   gutter_visible_p[TOP_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("bottom-gutter-visible-p",
		    &Vgutter_visible_p[BOTTOM_GUTTER] /*
*Whether the bottom gutter is visible.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-visible-p' for more information.
*/ );
  Vgutter_visible_p[BOTTOM_GUTTER] = Fmake_specifier (Qboolean);
  set_specifier_caching (Vgutter_visible_p[BOTTOM_GUTTER],
			 offsetof (struct window,
				   gutter_visible_p[BOTTOM_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("left-gutter-visible-p",
		    &Vgutter_visible_p[LEFT_GUTTER] /*
*Whether the left gutter is visible.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-visible-p' for more information.
*/ );
  Vgutter_visible_p[LEFT_GUTTER] = Fmake_specifier (Qboolean);
  set_specifier_caching (Vgutter_visible_p[LEFT_GUTTER],
			 offsetof (struct window,
				   gutter_visible_p[LEFT_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  DEFVAR_SPECIFIER ("right-gutter-visible-p",
		    &Vgutter_visible_p[RIGHT_GUTTER] /*
*Whether the right gutter is visible.
This is a specifier; use `set-specifier' to change it.

See `default-gutter-visible-p' for more information.
*/ );
  Vgutter_visible_p[RIGHT_GUTTER] = Fmake_specifier (Qboolean);
  set_specifier_caching (Vgutter_visible_p[RIGHT_GUTTER],
			 offsetof (struct window,
				   gutter_visible_p[RIGHT_GUTTER]),
			 gutter_geometry_changed_in_window,
			 0, 0);

  /* initially, top inherits from default; this can be
     changed with `set-default-gutter-position'. */
  fb = list1 (Fcons (Qnil, Qt));
  set_specifier_fallback (Vdefault_gutter_visible_p, fb);
  set_specifier_fallback (Vgutter_visible_p[TOP_GUTTER],
			  Vdefault_gutter_visible_p);
  set_specifier_fallback (Vgutter_visible_p[BOTTOM_GUTTER], fb);
  set_specifier_fallback (Vgutter_visible_p[LEFT_GUTTER],   fb);
  set_specifier_fallback (Vgutter_visible_p[RIGHT_GUTTER],  fb);
}