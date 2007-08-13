/* Window creation, deletion and examination for XEmacs.
   Copyright (C) 1985-1987, 1992-1995 Free Software Foundation, Inc.
   Copyright (C) 1994, 1995 Board of Trustees, University of Illinois.
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

/* Synched up with: FSF 19.30. */
/* Beginning to diverge significantly. */

/* This file has been Mule-ized. */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "faces.h"
#include "frame.h"
#include "objects.h"
#include "glyphs.h"
#include "redisplay.h"
#include "window.h"
#include "elhash.h"
#include "commands.h"

Lisp_Object Qwindowp, Qwindow_live_p, Qwindow_configurationp;
Lisp_Object Qscroll_up, Qscroll_down, Qdisplay_buffer;

#ifdef MEMORY_USAGE_STATS
Lisp_Object Qface_cache, Qglyph_cache, Qline_start_cache, Qother_redisplay;
#ifdef HAVE_SCROLLBARS
Lisp_Object Qscrollbar_instances;
#endif
#endif

EXFUN (Fnext_window, 4);

static int window_pixel_width_to_char_width (struct window *w,
					     int pixel_width,
					     int include_margins_p);
static int window_char_width_to_pixel_width (struct window *w,
					     int char_width,
					     int include_margins_p);
static int window_pixel_height_to_char_height (struct window *w,
					       int pixel_height,
					       int include_gutters_p);
static int window_char_height_to_pixel_height (struct window *w,
					       int char_height,
					       int include_gutters_p);
static void change_window_height (struct window *w, int delta, int widthflag,
                                  int inpixels);

/* Thickness of shadow border around 3d modelines. */
Lisp_Object Vmodeline_shadow_thickness;

/* Whether vertical dividers are draggable and displayed */
Lisp_Object Vvertical_divider_always_visible_p;

/* Whether a modeline should be displayed. */
Lisp_Object Vhas_modeline_p;

/* Thickness of shadow border around vertical dividers. */
Lisp_Object Vvertical_divider_shadow_thickness;

/* Divider surface width (not counting 3-d borders) */
Lisp_Object Vvertical_divider_line_width;

/* Spacing between outer egde of divider border and window edge */
Lisp_Object Vvertical_divider_spacing;

/* Scroll if point lands on the bottom line and that line is partially
   clipped. */
int scroll_on_clipped_lines;

/* The minibuffer window of the selected frame.
   Note that you cannot test for minibufferness of an arbitrary window
   by comparing against this; but you can test for minibufferness of
   the selected window.  */
Lisp_Object minibuf_window;

/* Non-nil means it is the window for C-M-v to scroll
   when the minibuffer is selected.  */
Lisp_Object Vminibuffer_scroll_window;

/* Non-nil means this is the buffer whose window C-M-v should scroll.  */
Lisp_Object Vother_window_scroll_buffer;

/* Non-nil means it's the function to call to display temp buffers.  */
Lisp_Object Vtemp_buffer_show_function;

Lisp_Object Vtemp_buffer_show_hook;

/* If a window gets smaller than either of these, it is removed. */
int window_min_height;
int window_min_width;

/* Hook run at end of temp_output_buffer_show.  */
Lisp_Object Qtemp_buffer_show_hook;

/* Number of lines of continuity in scrolling by screenfuls.  */
int next_screen_context_lines;

/* List of freed window configurations with 1 - 10 windows. */
Lisp_Object Vwindow_configuration_free_list[10];

#define SET_LAST_MODIFIED(w, cache_too)		\
do {						\
  (w)->last_modified[CURRENT_DISP] = Qzero;	\
  (w)->last_modified[DESIRED_DISP] = Qzero;	\
  (w)->last_modified[CMOTION_DISP] = Qzero;	\
  if (cache_too)				\
    (w)->line_cache_last_updated = Qzero;	\
} while (0)

#define SET_LAST_FACECHANGE(w)			\
do {						\
  (w)->last_facechange[CURRENT_DISP] = Qzero;	\
  (w)->last_facechange[DESIRED_DISP] = Qzero;	\
  (w)->last_facechange[CMOTION_DISP] = Qzero;	\
} while (0)


#define MARK_DISP_VARIABLE(field)		\
  markobj (window->field[CURRENT_DISP]);	\
  markobj (window->field[DESIRED_DISP]);	\
  markobj (window->field[CMOTION_DISP]);

static Lisp_Object
mark_window (Lisp_Object obj, void (*markobj) (Lisp_Object))
{
  struct window *window = XWINDOW (obj);
  markobj (window->frame);
  markobj (window->mini_p);
  markobj (window->next);
  markobj (window->prev);
  markobj (window->hchild);
  markobj (window->vchild);
  markobj (window->parent);
  markobj (window->buffer);
  MARK_DISP_VARIABLE (start);
  MARK_DISP_VARIABLE (pointm);
  markobj (window->sb_point);	/* #### move to scrollbar.c? */
  markobj (window->use_time);
  MARK_DISP_VARIABLE (last_modified);
  MARK_DISP_VARIABLE (last_point);
  MARK_DISP_VARIABLE (last_start);
  MARK_DISP_VARIABLE (last_facechange);
  markobj (window->line_cache_last_updated);
  markobj (window->redisplay_end_trigger);
  markobj (window->subwindow_instance_cache);

  mark_face_cachels (window->face_cachels, markobj);
  mark_glyph_cachels (window->glyph_cachels, markobj);

#define WINDOW_SLOT(slot, compare) ((void) (markobj (window->slot)))
#include "winslots.h"

  return Qnil;
}

static void
print_window (Lisp_Object obj, Lisp_Object printcharfun, int escapeflag)
{
  char buf[200];

  if (print_readably)
    error ("printing unreadable object #<window 0x%x>",
           XWINDOW (obj)->header.uid);

  write_c_string ("#<window", printcharfun);
  if (!NILP (XWINDOW (obj)->buffer))
    {
      Lisp_Object name = XBUFFER (XWINDOW (obj)->buffer)->name;
      write_c_string (" on ", printcharfun);
      print_internal (name, printcharfun, 1);
    }
  sprintf (buf, " 0x%x>", XWINDOW (obj)->header.uid);
  write_c_string (buf, printcharfun);
}

static void
finalize_window (void *header, int for_disksave)
{
  struct window *w = (struct window *) header;

  if (w->line_start_cache)
    {
      Dynarr_free (w->line_start_cache);
      w->line_start_cache = 0;
    }

  if (w->face_cachels)
    {
      int i;

      for (i = 0; i < Dynarr_length (w->face_cachels); i++)
	{
	  struct face_cachel *cachel = Dynarr_atp (w->face_cachels, i);
	  if (cachel->merged_faces)
	    {
	      Dynarr_free (cachel->merged_faces);
	      cachel->merged_faces = 0;
	    }
	}
      Dynarr_free (w->face_cachels);
      w->face_cachels = 0;
    }

  if (w->glyph_cachels)
    {
      Dynarr_free (w->glyph_cachels);
      w->glyph_cachels = 0;
    }
}

DEFINE_LRECORD_IMPLEMENTATION ("window", window,
                               mark_window, print_window, finalize_window,
			       0, 0, 0, struct window);


#define INIT_DISP_VARIABLE(field, initialization)	\
  p->field[CURRENT_DISP] = initialization;		\
  p->field[DESIRED_DISP] = initialization;		\
  p->field[CMOTION_DISP] = initialization;

/* We have an implicit assertion that the first two elements (default
   and modeline faces) are always present in the face_element_cache.
   Normally redisplay ensures this.  However, it is possible for a
   window to get created and functions which reference these values
   called before redisplay works with the window for the first time.
   All callers of allocate_window should therefore call
   reset_face_cachels on the created window.  We can't do it
   here because the window must have its frame pointer set or
   reset_face_cachels will fail. */
Lisp_Object
allocate_window (void)
{
  Lisp_Object val;
  struct window *p = alloc_lcrecord_type (struct window, &lrecord_window);

  zero_lcrecord (p);
  XSETWINDOW (val, p);

  p->dead = 0;
  p->frame = Qnil;
  p->mini_p = Qnil;
  p->next = Qnil;
  p->prev = Qnil;
  p->hchild = Qnil;
  p->vchild = Qnil;
  p->parent = Qnil;
  p->buffer = Qnil;
  INIT_DISP_VARIABLE (start, Fmake_marker ());
  INIT_DISP_VARIABLE (pointm, Fmake_marker ());
  p->sb_point = Fmake_marker ();
  p->use_time = Qzero;
  INIT_DISP_VARIABLE (last_modified, Qzero);
  INIT_DISP_VARIABLE (last_point, Fmake_marker ());
  INIT_DISP_VARIABLE (last_start, Fmake_marker ());
  INIT_DISP_VARIABLE (last_facechange, Qzero);
  p->face_cachels     = Dynarr_new (face_cachel);
  p->glyph_cachels    = Dynarr_new (glyph_cachel);
  p->line_start_cache = Dynarr_new (line_start_cache);
  p->subwindow_instance_cache = make_lisp_hash_table (10,
						      HASH_TABLE_KEY_WEAK,
						      HASH_TABLE_EQ);
  p->line_cache_last_updated = Qzero;
  INIT_DISP_VARIABLE (last_point_x, 0);
  INIT_DISP_VARIABLE (last_point_y, 0);
  INIT_DISP_VARIABLE (window_end_pos, 0);
  p->redisplay_end_trigger = Qnil;

#define WINDOW_SLOT(slot, compare) p->slot = Qnil
#include "winslots.h"

  p->windows_changed = 1;
  p->shadow_thickness_changed = 1;

  return val;
}
#undef INIT_DISP_VARIABLE

/*
 * The redisplay structures used to be stored with each window.  While
 * they are logically something associated with frames they can't be
 * stored there with a redisplay which handles variable height lines.
 * Lines in horizontally split windows might not line up.  So they get
 * stored with the windows.
 *
 * The problem with this is window configurations.  When restoring a
 * window configuration it now becomes problematic to do an
 * incremental redisplay.  The solution is to store the redisplay
 * structures with the frame as they should be but laid out in the
 * same manner as the window structure.  Thus is born the window
 * mirror.
 *
 * It also becomes a convenient place to stick scrollbar instances
 * since they extrapolate out to having the same problem described for
 * the display structures.
 */

/* Create a new window mirror structure and associated redisplay
   structs. */
static struct window_mirror *
new_window_mirror (struct frame *f)
{
  struct window_mirror *t = xnew_and_zero (struct window_mirror);

  t->frame = f;

  t->current_display_lines = Dynarr_new (display_line);
  t->desired_display_lines = Dynarr_new (display_line);
  t->buffer = NULL;

#ifdef HAVE_SCROLLBARS
  t->scrollbar_vertical_instance = NULL;
  t->scrollbar_horizontal_instance = NULL;
#endif

  return t;
}

/* Synchronize the mirror structure with a given window structure.
   This is normally called from update_frame_window_mirror with a
   starting window of f->root_window. */
static struct window_mirror *
update_mirror_internal (Lisp_Object win, struct window_mirror *mir)
{
  if (NILP (win))
    {
      if (mir)
	{
	  free_window_mirror (mir);
	  mir = NULL;
	}
      return mir;
    }
  else
    if (!mir)
      mir = new_window_mirror (XFRAME (XWINDOW (win)->frame));

  mir->next   = update_mirror_internal (XWINDOW (win)->next,   mir->next);
  mir->hchild = update_mirror_internal (XWINDOW (win)->hchild, mir->hchild);
  mir->vchild = update_mirror_internal (XWINDOW (win)->vchild, mir->vchild);

  /*
   * If the redisplay structs are not empty and the mirror has
   * children, then this mirror structure was formerly being used for
   * display but is no longer.  Reset its current display structs so
   * that redisplay doesn't accidentally think they are accurate if it
   * is later used for display purposes once again.  Also, mark the
   * scrollbar instance as not active.
   */
  if (mir->vchild || mir->hchild)
    {
      /* The redisplay structures are big.  Leaving them around in
         non-leaf windows can add up to a lot of wasted space.  So
         don't do it. */
      free_display_structs (mir);
      mir->current_display_lines = Dynarr_new (display_line);
      mir->desired_display_lines = Dynarr_new (display_line);

#ifdef HAVE_SCROLLBARS
      update_window_scrollbars (XWINDOW (win), mir, 0, 0);
#endif
      mir->buffer = NULL;
    }

  return mir;
}

/* Given a window mirror, determine which real window it contains the
   redisplay structures for. */
static Lisp_Object
real_window_internal (Lisp_Object win, struct window_mirror *rmir,
		      struct window_mirror *mir)
{
  for (; !NILP (win) && rmir ; win = XWINDOW (win)->next, rmir = rmir->next)
    {
      if (mir == rmir)
	return win;
      if (!NILP (XWINDOW (win)->vchild))
	{
	  Lisp_Object retval =
	    real_window_internal (XWINDOW (win)->vchild, rmir->vchild, mir);
	  if (!NILP (retval))
	    return retval;
	}
      if (!NILP (XWINDOW (win)->hchild))
	{
	  Lisp_Object retval =
	    real_window_internal (XWINDOW (win)->hchild, rmir->hchild, mir);
	  if (!NILP (retval))
	    return retval;
	}
    }

  return Qnil;
}

/* Given a real window, find the mirror structure which contains its
   redisplay structures. */
static struct window_mirror *
find_window_mirror_internal (Lisp_Object win, struct window_mirror *rmir,
			    struct window *w)
{
  for (; !NILP (win); win = XWINDOW (win)->next, rmir = rmir->next)
    {
      if (w == XWINDOW (win))
	return rmir;

      if (!NILP (XWINDOW (win)->vchild))
	{
	  struct window_mirror *retval =
	    find_window_mirror_internal (XWINDOW (win)->vchild,
					 rmir->vchild, w);
	  if (retval) return retval;
	}

      if (!NILP (XWINDOW (win)->hchild))
	{
	  struct window_mirror *retval =
	    find_window_mirror_internal (XWINDOW (win)->hchild,
					 rmir->hchild, w);
	  if (retval) return retval;
	}
    }

  return 0;
}

/* Update the mirror structure for the given frame. */
void
update_frame_window_mirror (struct frame *f)
{
  f->root_mirror = update_mirror_internal (f->root_window, f->root_mirror);
  f->mirror_dirty = 0;
}

/* Free a given mirror structure along with all of its children as
   well as their associated display structures. */
void
free_window_mirror (struct window_mirror *mir)
{
  while (mir)
    {
      struct window_mirror *prev = mir;
      if (mir->hchild) free_window_mirror (mir->hchild);
      if (mir->vchild) free_window_mirror (mir->vchild);
#ifdef HAVE_SCROLLBARS
      release_window_mirror_scrollbars (mir);
#endif
      free_display_structs (mir);
      mir = mir->next;
      xfree (prev);
    }
}

/* Given a mirror structure, return the window it mirrors.  Calls
   real_window_internal to do most of the work. */
Lisp_Object
real_window (struct window_mirror *mir, int no_abort)
{
  Lisp_Object retval = real_window_internal (mir->frame->root_window,
					     mir->frame->root_mirror, mir);
  if (NILP (retval) && !no_abort)
    abort ();

  return retval;
}

/* Given a real window, return its mirror structure.  Calls
   find_window_mirror_internal to do all of the work. */
struct window_mirror *
find_window_mirror (struct window *w)
{
  struct frame *f = XFRAME (w->frame);
  if (f->mirror_dirty)
    update_frame_window_mirror (f);
  return find_window_mirror_internal (f->root_window, f->root_mirror, w);
}

/*****************************************************************************
 find_window_by_pixel_pos

 Given a pixel position relative to a frame, find the window at that
 position.
 ****************************************************************************/
struct window *
find_window_by_pixel_pos (int pix_x, int pix_y, Lisp_Object win)
{
  if (NILP (win))
    return 0;

  for (; !NILP (win); win = XWINDOW (win)->next)
    {
      struct window *w;

      if (!NILP (XWINDOW (win)->vchild))
	{
	  w = find_window_by_pixel_pos (pix_x, pix_y, XWINDOW (win)->vchild);
	  if (w) return w;
	}
      if (!NILP (XWINDOW (win)->hchild))
	{
	  w = find_window_by_pixel_pos (pix_x, pix_y, XWINDOW (win)->hchild);
	  if (w) return w;
	}
      w = XWINDOW (win);
      if (pix_x >= WINDOW_LEFT (w)
	  && pix_x <= WINDOW_RIGHT (w)
	  && pix_y >= WINDOW_TOP (w)
	  && pix_y <= WINDOW_BOTTOM (w))
	return w;
    }
  return NULL;
}

/* Return a pointer to the display structures for the given window. */
display_line_dynarr *
window_display_lines (struct window *w, int which)
{
  struct window_mirror *t;

  if (XFRAME (w->frame)->mirror_dirty)
    update_frame_window_mirror (XFRAME (w->frame));
  t = find_window_mirror (w);
  if (!t)
    abort ();

  if (which == CURRENT_DISP)
    return t->current_display_lines;
  else if (which == DESIRED_DISP)
    return t->desired_display_lines;
  else if (which == CMOTION_DISP)
    /* The CMOTION_DISP display lines are global. */
    return cmotion_display_lines;
  else
    abort ();

  return 0;	/* shut up compiler */
}

struct buffer *
window_display_buffer (struct window *w)
{
  struct window_mirror *t;

  if (XFRAME (w->frame)->mirror_dirty)
    update_frame_window_mirror (XFRAME (w->frame));
  t = find_window_mirror (w);
  if (!t)
    abort ();

  return t->buffer;
}

void
set_window_display_buffer (struct window *w, struct buffer *b)
{
  struct window_mirror *t;

  if (XFRAME (w->frame)->mirror_dirty)
    update_frame_window_mirror (XFRAME (w->frame));
  t = find_window_mirror (w);
  if (!t)
    abort ();

  t->buffer = b;
}


/* Determining a window's position based solely on its pixel
   positioning doesn't work.  Instead, we do it the intelligent way,
   by checking its positioning in the window hierarchy. */
int
window_is_leftmost (struct window *w)
{
  Lisp_Object parent, current_ancestor, window;

  XSETWINDOW (window, w);

  parent = XWINDOW (window)->parent;
  current_ancestor = window;

  while (!NILP (parent))
    {
      if (!NILP (XWINDOW (parent)->hchild) &&
	  !EQ (XWINDOW (parent)->hchild, current_ancestor))
	return 0;

      current_ancestor = parent;
      parent = XWINDOW (parent)->parent;
    }

  return 1;
}

int
window_is_rightmost (struct window *w)
{
  Lisp_Object parent, current_ancestor, window;

  XSETWINDOW (window, w);

  parent = XWINDOW (window)->parent;
  current_ancestor = window;

  while (!NILP (parent))
    {
      if (!NILP (XWINDOW (parent)->hchild)
	  && !NILP (XWINDOW (current_ancestor)->next))
	return 0;

      current_ancestor = parent;
      parent = XWINDOW (parent)->parent;
    }

  return 1;
}

static int
window_full_width_p (struct window *w)
{
  return window_is_leftmost (w) && window_is_rightmost (w);
}

static int
window_is_highest (struct window *w)
{
  Lisp_Object parent, current_ancestor, window;

  XSETWINDOW (window, w);

  parent = XWINDOW (window)->parent;
  current_ancestor = window;

  while (!NILP (parent))
    {
      if (!NILP (XWINDOW (parent)->vchild) &&
	  !EQ (XWINDOW (parent)->vchild, current_ancestor))
	return 0;

      current_ancestor = parent;
      parent = XWINDOW (parent)->parent;
    }

  /* This is really to catch the minibuffer but we make it generic in
     case we ever change things around to let the minibuffer be on top. */
  if (NILP (XWINDOW (current_ancestor)->prev))
    return 1;
  else
    return 0;
}

static int
window_is_lowest (struct window *w)
{
  Lisp_Object parent, current_ancestor, window;

  XSETWINDOW (window, w);

  parent = XWINDOW (window)->parent;
  current_ancestor = window;

  while (!NILP (parent))
    {
      if (!NILP (XWINDOW (parent)->vchild)
	  && !NILP (XWINDOW (current_ancestor)->next))
	return 0;

      current_ancestor = parent;
      parent = XWINDOW (parent)->parent;
    }

  return 1;
}

#if 0 /* not currently used */

static int
window_full_height_p (struct window *w)
{
  return window_is_highest (w) && window_is_lowest (w);
}

#endif

int
window_truncation_on (struct window *w)
{
  /* Horizontally scrolled windows are truncated. */
  if (w->hscroll)
    return 1;

  /* If truncate_partial_width_windows is true and the window is not
     the full width of the frame it is truncated. */
  if (truncate_partial_width_windows
      && !(window_is_leftmost (w) && window_is_rightmost (w)))
    return 1;

  /* If the window's buffer's value of truncate_lines is non-nil, then
     the window is truncated. */
  if (!NILP (XBUFFER (w->buffer)->truncate_lines))
    return 1;

  return 0;
}

static int
have_undivided_common_edge (struct window *w_right, void *closure)
{
  struct window *w_left = (struct window *) closure;
  return (WINDOW_RIGHT (w_left) == WINDOW_LEFT (w_right)
	  && WINDOW_TOP (w_left) < WINDOW_BOTTOM (w_right)
	  && WINDOW_TOP (w_right) < WINDOW_BOTTOM (w_left)
#ifdef HAVE_SCROLLBARS
	  && (NILP (w_right->scrollbar_on_left_p)
	      || NILP (w_right->vertical_scrollbar_visible_p)
	      || ZEROP (w_right->scrollbar_width))
#endif
	  );
}

static int
window_needs_vertical_divider_1 (struct window *w)
{
  /* Never if we're on the right */
  if (window_is_rightmost (w))
    return 0;

  /* Always if draggable */
  if (!NILP (w->vertical_divider_always_visible_p))
    return 1;

#ifdef HAVE_SCROLLBARS
  /* Our right scrollbar is enough to separate us at the right */
  if (NILP (w->scrollbar_on_left_p)
      && !NILP (w->vertical_scrollbar_visible_p)
      && !ZEROP (w->scrollbar_width))
    return 0;
#endif

  /* Ok. to determine whether we need a divider on the left, we must
     check that our right neighbor windows have scrollbars on their
     left sides. We must check all such windows which have common
     left edge with our window's right edge. */
  return map_windows (XFRAME (WINDOW_FRAME (w)),
		      have_undivided_common_edge, (void*)w);
}

int
window_needs_vertical_divider (struct window *w)
{
  if (!w->need_vertical_divider_valid_p)
    {
      w->need_vertical_divider_p =
	window_needs_vertical_divider_1 (w);
      w->need_vertical_divider_valid_p = 1;
    }
  return w->need_vertical_divider_p;
}

/* Called from invalidate_vertical_divider_cache_in_frame */
int
invalidate_vertical_divider_cache_in_window (struct window *w,
					     void *u_n_u_s_e_d)
{
  w->need_vertical_divider_valid_p = 0;
  return 0;
}

/* Calculate width of vertical divider, including its shadows
   and spacing. The returned value is effectively the distance
   between adjacent window edges. This function does not check
   whether a window needs a vertical divider, so the returned
   value is a "theoretical" one */
int
window_divider_width (struct window *w)
{
  /* the shadow thickness can be negative. This means that the divider
     will have a depressed look */

  if (FRAME_WIN_P (XFRAME (WINDOW_FRAME (w))))
    return
      XINT (w->vertical_divider_line_width)
      + 2 * XINT (w->vertical_divider_spacing)
      + 2 * abs (XINT (w->vertical_divider_shadow_thickness));
  else
    return XINT (w->vertical_divider_line_width) == 0 ? 0 : 1;
}

int
window_scrollbar_width (struct window *w)
{
#ifdef HAVE_SCROLLBARS
  if (!WINDOW_WIN_P (w)
      || MINI_WINDOW_P (w)
      || NILP (w->buffer)
      || NILP (w->vertical_scrollbar_visible_p))
    /* #### when does NILP (w->buffer) happen? */
    return 0;

  return XINT (w->scrollbar_width);
#else
  return 0;
#endif /* HAVE_SCROLLBARS */
}

/* Horizontal scrollbars are only active on windows with truncation
   turned on. */
int
window_scrollbar_height (struct window *w)
{
#ifdef HAVE_SCROLLBARS
  if (!WINDOW_WIN_P (w)
      || MINI_WINDOW_P (w)
      || NILP (w->buffer)
      || NILP (w->horizontal_scrollbar_visible_p)
      || !window_truncation_on (w))
    return 0;

  return XINT (w->scrollbar_height);
#else
  return 0;
#endif /* HAVE_SCROLLBARS */
}

int
window_modeline_height (struct window *w)
{
  struct frame *f = XFRAME (w->frame);
  int modeline_height;

  if (MINI_WINDOW_P (w) || NILP (w->buffer))
    {
      modeline_height = 0;
    }
  else if (!WINDOW_HAS_MODELINE_P (w))
    {
      if (window_scrollbar_height (w))
	modeline_height = 0;
      else
	{
	  modeline_height = FRAMEMETH (f, divider_height, ());

	  if (!EQ (Qzero, w->modeline_shadow_thickness) && FRAME_WIN_P (f))
	    modeline_height += (2 * MODELINE_SHADOW_THICKNESS (w));
	}
    }
  else
    {
      if (noninteractive)
	modeline_height = 0;
      else
	{
	  display_line_dynarr *dla;

	  /* We don't force a regeneration of the modeline here.
             Instead it is now a precondition that any function calling
             this should make sure that one of these structures is
             up-to-date.  In practice this only affects two internal
             redisplay functions, regenerate_window and
             regenerate_window_point_center. */
	  /* We check DESIRED_DISP because if it is valid it is more
             up-to-date than CURRENT_DISP.  For calls to this outside
             of redisplay it doesn't matter which structure we check
             since there is a redisplay condition that these
             structures be identical outside of redisplay. */
	  dla = window_display_lines (w, DESIRED_DISP);
	  if (dla && Dynarr_length (dla) && Dynarr_atp (dla, 0)->modeline)
	    modeline_height = (Dynarr_atp (dla, 0)->ascent +
			       Dynarr_atp (dla, 0)->descent);
	  else
	    {
	      dla = window_display_lines (w, CURRENT_DISP);
	      if (dla && Dynarr_length (dla) && Dynarr_atp (dla, 0)->modeline)
		modeline_height = (Dynarr_atp (dla, 0)->ascent +
				   Dynarr_atp (dla, 0)->descent);
	      else
		/* This should be an abort except I'm not yet 100%
                   confident that it won't ever get hit (though I
                   haven't been able to trigger it).  It is extremely
                   unlikely to cause any noticeable problem and even if
                   it does it will be a minor display glitch. */
		/* #### Bullshit alert.  It does get hit and it causes
                   noticeable glitches.  real_current_modeline_height
                   is a kludge to fix this for 19.14. */
		modeline_height = real_current_modeline_height (w);
	    }

	  if (!EQ (Qzero, w->modeline_shadow_thickness) && FRAME_WIN_P (f))
	    modeline_height += (2 * MODELINE_SHADOW_THICKNESS (w));
	}
    }

  return modeline_height;
}

/*****************************************************************************
 margin_width_internal

 For a given window, return the width in pixels of the specified margin.
 ****************************************************************************/
static int
margin_width_internal (struct window *w, int left_margin)
{
  struct buffer *b;
  int window_cwidth = window_char_width (w, 1);
  int margin_cwidth;
  int font_width;
  Lisp_Object window;

  /* We might be getting called on a non-leaf. */
  if (NILP (w->buffer))
    return 0;

  /* The minibuffer never has margins. */
  if (MINI_WINDOW_P (w))
    return 0;

  XSETWINDOW (window, w);
  b = XBUFFER (w->buffer);
  margin_cwidth = (left_margin ? XINT (w->left_margin_width) :
		   XINT (w->right_margin_width));

  default_face_height_and_width (window, 0, &font_width);

  /* The left margin takes precedence over the right margin so we
     subtract its width from the space available for the right
     margin. */
  if (!left_margin)
    window_cwidth -= XINT (w->left_margin_width);

  /* The margin cannot be wider than the window is.  We allow the
     value to be bigger since it is possible for the user to enlarge
     the window such that the left margin value would no longer be too
     big, but we won't return a value that is larger. */
  if (margin_cwidth > window_cwidth)
    margin_cwidth = window_cwidth;

  /* At the user level the margin is always specified in characters.
     Internally however it is manipulated in terms of pixels. */
  return margin_cwidth * font_width;
}

int
window_left_margin_width (struct window *w)
{
  return margin_width_internal (w, 1);
}

int
window_right_margin_width (struct window *w)
{
  return margin_width_internal (w, 0);
}

static int
window_top_toolbar_height (struct window *w)
{
  /* #### implement this shit. */
  return 0;
}

/* #### Currently used in scrollbar.c.  Does it actually need to be? */
int
window_bottom_toolbar_height (struct window *w)
{
  return 0;
}

static int
window_left_toolbar_width (struct window *w)
{
  return 0;
}

static int
window_right_toolbar_width (struct window *w)
{
  return 0;
}

/*****************************************************************************
 Window Gutters

 The gutters of a window are those areas in the boundary defined by
 w->pixel_top, w->pixel_left, w->pixel_height and w->pixel_width which
 do not contain text.  Items which may be in the gutters include
 scrollbars, toolbars and modelines.  The margin areas are not
 included.  This is an exception made because redisplay special cases
 the handling of those areas in many places in such a way that
 including them in the gutter area would make life difficult.

 The size functions refer to height for the bottom and top gutters and
 width for the left and right gutters.  The starting position
 functions refer to the Y coord for bottom and top gutters and the X
 coord for left and right gutters.  All starting positions are
 relative to the frame, not the window.
 ****************************************************************************/

int
window_top_gutter_height (struct window *w)
{
  int toolbar_height = window_top_toolbar_height (w);

  if (!NILP (w->hchild) || !NILP (w->vchild))
    return 0;

#ifdef HAVE_SCROLLBARS
  if (!NILP (w->scrollbar_on_top_p))
    return window_scrollbar_height (w) + toolbar_height;
  else
#endif
    return toolbar_height;
}

int
window_bottom_gutter_height (struct window *w)
{
  int other_height;

  if (!NILP (w->hchild) || !NILP (w->vchild))
    return 0;
  else
    other_height =
      window_modeline_height (w) + window_bottom_toolbar_height (w);

#ifdef HAVE_SCROLLBARS
  if (NILP (w->scrollbar_on_top_p))
    return window_scrollbar_height (w) + other_height;
  else
#endif
    return other_height;
}

int
window_left_gutter_width (struct window *w, int modeline)
{
  int gutter = window_left_toolbar_width (w);

  if (!NILP (w->hchild) || !NILP (w->vchild))
    return 0;


#ifdef HAVE_SCROLLBARS
  if (!modeline && !NILP (w->scrollbar_on_left_p))
    gutter += window_scrollbar_width (w);
#endif

  return gutter;
}

int
window_right_gutter_width (struct window *w, int modeline)
{
  int gutter = window_right_toolbar_width (w);

  if (!NILP (w->hchild) || !NILP (w->vchild))
    return 0;

#ifdef HAVE_SCROLLBARS
  if (!modeline && NILP (w->scrollbar_on_left_p))
    gutter += window_scrollbar_width (w);
#endif

  if (window_needs_vertical_divider (w))
    gutter += window_divider_width (w);

  return gutter;
}


DEFUN ("windowp", Fwindowp, 1, 1, 0, /*
Return t if OBJ is a window.
*/
       (obj))
{
  return WINDOWP (obj) ? Qt : Qnil;
}

DEFUN ("window-live-p", Fwindow_live_p, 1, 1, 0, /*
Return t if OBJ is a window which is currently visible.
*/
       (obj))
{
  return WINDOWP (obj) && WINDOW_LIVE_P (XWINDOW (obj)) ? Qt : Qnil;
}

DEFUN ("selected-window", Fselected_window, 0, 1, 0, /*
Return the window that the cursor now appears in and commands apply to.
If the optional argument CON-DEV-OR-FRAME is specified and is a frame, return
the selected window used by that frame.  If CON-DEV-OR-FRAME is a device,
then the selected frame on that device will be used.  If CON-DEV-OR-FRAME
is a console, the selected frame on that console's selected device will
be used.  Otherwise, the selected frame is used.
*/
       (con_dev_or_frame))
{
  if (NILP (con_dev_or_frame) && NILP (Fselected_device (Qnil)))
    return Qnil; /* happens at startup */

  {
    struct frame *f = decode_frame_or_selected (con_dev_or_frame);
    return FRAME_SELECTED_WINDOW (f);
  }
}

DEFUN ("minibuffer-window", Fminibuffer_window, 0, 1, 0, /*
Return the window used now for minibuffers.
If the optional argument CON-DEV-OR-FRAME is specified and is a frame, return
the minibuffer window used by that frame.  If CON-DEV-OR-FRAME is a device,
then the selected frame on that device will be used.  If CON-DEV-OR-FRAME
is a console, the selected frame on that console's selected device will
be used.  Otherwise, the selected frame is used.
*/
       (con_dev_or_frame))
{
  return FRAME_MINIBUF_WINDOW (decode_frame_or_selected (con_dev_or_frame));
}

DEFUN ("window-minibuffer-p", Fwindow_minibuffer_p, 1, 1, 0, /*
Return non-nil if WINDOW is a minibuffer window.
*/
       (window))
{
  return MINI_WINDOW_P (decode_window (window)) ? Qt : Qnil;
}

DEFUN ("window-first-hchild", Fwindow_first_hchild, 1, 1, 0, /*
Return the first horizontal child of WINDOW, or nil.
*/
       (window))
{
  return decode_window (window)->hchild;
}

DEFUN ("window-first-vchild", Fwindow_first_vchild, 1, 1, 0, /*
Return the first vertical child of WINDOW, or nil.
*/
       (window))
{
  return decode_window (window)->vchild;
}

DEFUN ("window-next-child", Fwindow_next_child, 1, 1, 0, /*
Return the next window on the same level as WINDOW, or nil.
*/
       (window))
{
  return decode_window (window)->next;
}

DEFUN ("window-previous-child", Fwindow_previous_child, 1, 1, 0, /*
Return the previous window on the same level as WINDOW, or nil.
*/
       (window))
{
  return decode_window (window)->prev;
}

DEFUN ("window-parent", Fwindow_parent, 1, 1, 0, /*
Return the parent of WINDOW, or nil.
*/
       (window))
{
  return decode_window (window)->parent;
}

DEFUN ("window-lowest-p", Fwindow_lowest_p, 1, 1, 0, /*
Return non-nil if WINDOW is along the bottom of its frame.
*/
       (window))
{
  return window_is_lowest (decode_window (window)) ? Qt : Qnil;
}

DEFUN ("window-highest-p", Fwindow_highest_p, 1, 1, 0, /*
Return non-nil if WINDOW is along the top of its frame.
*/
       (window))
{
  return window_is_highest (decode_window (window)) ? Qt : Qnil;
}

DEFUN ("window-leftmost-p", Fwindow_leftmost_p, 1, 1, 0, /*
Return non-nil if WINDOW is along the left edge of its frame.
*/
       (window))
{
  return window_is_leftmost (decode_window (window)) ? Qt : Qnil;
}

DEFUN ("window-rightmost-p", Fwindow_rightmost_p, 1, 1, 0, /*
Return non-nil if WINDOW is along the right edge of its frame.
*/
       (window))
{
  return window_is_rightmost (decode_window (window)) ? Qt : Qnil;
}

DEFUN ("pos-visible-in-window-p", Fpos_visible_in_window_p, 0, 2, 0, /*
Return t if position POS is currently on the frame in WINDOW.
Returns nil if that position is scrolled vertically out of view.
POS defaults to point in WINDOW's buffer; WINDOW, to the selected window.
*/
       (pos, window))
{
  struct window *w = decode_window (window);
  Bufpos top = marker_position (w->start[CURRENT_DISP]);
  Bufpos posint;
  struct buffer *buf = XBUFFER (w->buffer);

  if (NILP (pos))
    posint = BUF_PT (buf);
  else
    {
      CHECK_INT_COERCE_MARKER (pos);
      posint = XINT (pos);
    }

  if (posint < top || posint > BUF_ZV (buf))
    return Qnil;

  /* w->start can be out of range.  If it is, do something reasonable.  */
  if (top < BUF_BEGV (buf) || top > BUF_ZV (buf))
    return Qnil;

  return point_would_be_visible (w, top, posint) ? Qt : Qnil;
}


struct window *
decode_window (Lisp_Object window)
{
  if (NILP (window))
    return XWINDOW (Fselected_window (Qnil));

  CHECK_LIVE_WINDOW (window);
  return XWINDOW (window);
}

DEFUN ("window-buffer", Fwindow_buffer, 0, 1, 0, /*
Return the buffer that WINDOW is displaying.
*/
       (window))
{
  return decode_window (window)->buffer;
}

DEFUN ("window-frame", Fwindow_frame, 0, 1, 0, /*
Return the frame that window WINDOW is on.
*/
       (window))
{
  return decode_window (window)->frame;
}

DEFUN ("window-height", Fwindow_height, 0, 1, 0, /*
Return the number of default lines in WINDOW.
This actually works by dividing the window's pixel height (including
the modeline and horizontal scrollbar, if any) by the height of the
default font; therefore, the number of displayed lines will probably
be different.
Use `window-height' to get consistent results in geometry calculations.
Use `window-displayed-height' to get the actual number of lines
currently displayed in a window.
*/
       (window))
{
  return make_int (window_char_height (decode_window (window), 1));
}

DEFUN ("window-displayed-height", Fwindow_displayed_height, 0, 1, 0, /*
Return the number of lines currently displayed in WINDOW.
This counts the actual number of lines displayed in WINDOW
\(as opposed to `window-height').  The modeline and horizontal
scrollbar do not count as lines.  If there is some blank space
between the end of the buffer and the end of the window, this
function pretends that there are lines of text in the default
font there.
*/
     (window))
{
  return make_int (window_displayed_height (decode_window (window)));
}

DEFUN ("window-pixel-height", Fwindow_pixel_height, 0, 1, 0, /*
Return the height of WINDOW in pixels.  Defaults to current window.
This includes the window's modeline and horizontal scrollbar (if any).
*/
       (window))
{
  return make_int (decode_window (window)->pixel_height);
}

DEFUN ("window-text-area-pixel-height",
       Fwindow_text_area_pixel_height, 0, 1, 0, /*
Return the height in pixels of the text-displaying portion of WINDOW.
Unlike `window-pixel-height', the space occupied by the modeline and
horizontal scrollbar, if any, is not counted.
*/
     (window))
{
  struct window *w = decode_window (window);

  return make_int (WINDOW_TEXT_HEIGHT (w));
}

DEFUN ("window-displayed-text-pixel-height",
       Fwindow_displayed_text_pixel_height, 0, 2, 0, /*
Return the height in pixels of the text displayed in WINDOW.
Unlike `window-text-area-pixel-height', any blank space below the
end of the buffer is not included.  If optional argument NOCLIPPED
is non-nil, do not include space occupied by clipped lines.
*/
     (window, noclipped))
{
  struct window *w;
  Bufpos start, eobuf;
  int defheight;
  int hlimit, height, prev_height = -1;
  int line;
  int elt, nelt, i;
  int needed;
  line_start_cache_dynarr *cache;

  if (NILP (window))
    window = Fselected_window (Qnil);

  CHECK_LIVE_WINDOW (window);
  w = XWINDOW (window);

  start  = marker_position (w->start[CURRENT_DISP]);
  hlimit = WINDOW_TEXT_HEIGHT (w);
  eobuf  = BUF_ZV (XBUFFER (w->buffer));

  default_face_height_and_width (window, &defheight, NULL);

  /* guess lines needed in line start cache + a few extra */
  needed = (hlimit + defheight-1) / defheight + 3;

  while (1) {
    elt = point_in_line_start_cache (w, start, needed);
    assert (elt >= 0); /* in the cache */

    cache = w->line_start_cache;
    nelt  = Dynarr_length (cache);

    height = 0;
    for (i = elt; i < nelt; i++) {
      line = Dynarr_atp (cache, i)->height;

      if (height + line > hlimit)
        return make_int (!NILP (noclipped) ? height : hlimit);

      height += line;

      if (height == hlimit || Dynarr_atp (cache, i)->end >= eobuf)
        return make_int (height);
    }

    /* get here => need more cache lines.  try again. */
    assert(height > prev_height); /* progress? */
    prev_height = height;

    needed += ((hlimit - height)*(nelt - elt) + height-1)/height + 3;
  }

  RETURN_NOT_REACHED(make_int (0))        /* shut up compiler */
}

DEFUN ("window-width", Fwindow_width, 0, 1, 0, /*
Return the number of display columns in WINDOW.
This is the width that is usable columns available for text in WINDOW.
*/
       (window))
{
  return make_int (window_char_width (decode_window (window), 0));
}

DEFUN ("window-pixel-width", Fwindow_pixel_width, 0, 1, 0, /*
Return the width of WINDOW in pixels.  Defaults to current window.
*/
       (window))
{
  return make_int (decode_window (window)->pixel_width);
}

DEFUN ("window-text-area-pixel-width",
       Fwindow_text_area_pixel_width, 0, 1, 0, /*
Return the width in pixels of the text-displaying portion of WINDOW.
Unlike `window-pixel-width', the space occupied by the vertical
scrollbar or divider, if any, is not counted.
*/
     (window))
{
  struct window *w = decode_window (window);

  return make_int (WINDOW_TEXT_WIDTH (w));
}

DEFUN ("window-hscroll", Fwindow_hscroll, 0, 1, 0, /*
Return the number of columns by which WINDOW is scrolled from left margin.
*/
       (window))
{
  return make_int (decode_window (window)->hscroll);
}

#ifdef MODELINE_IS_SCROLLABLE
DEFUN ("modeline-hscroll", Fmodeline_hscroll, 0, 1, 0, /*
Return the number of columns by which WINDOW's modeline is scrolled from
left margin. If the window has no modeline, return nil.
*/
       (window))
{
  struct window *w = decode_window (window);

  return (WINDOW_HAS_MODELINE_P (w)) ? make_int (w->modeline_hscroll) : Qnil;
}

DEFUN ("set-modeline-hscroll", Fset_modeline_hscroll, 2, 2, 0, /*
Set number of columns WINDOW's modeline is scrolled from left margin to NCOL.
NCOL should be zero or positive. If NCOL is negative, it will be forced to 0.
If the window has no modeline, do nothing and return nil.
*/
       (window, ncol))
{
  struct window *w = decode_window (window);

  if (WINDOW_HAS_MODELINE_P (w))
    {
      int ncols;
      CHECK_INT (ncol);
      ncols = XINT (ncol);
      if (ncols < 0) ncols = 0;
      if (w->modeline_hscroll != ncols)
	MARK_MODELINE_CHANGED;
      w->modeline_hscroll = ncols;
      return ncol;
    }
  return Qnil;
}
#endif /* MODELINE_IS_SCROLLABLE */

DEFUN ("set-window-hscroll", Fset_window_hscroll, 2, 2, 0, /*
Set number of columns WINDOW is scrolled from left margin to NCOL.
NCOL should be zero or positive.
*/
       (window, ncol))
{
  struct window *w;
  int ncols;

  CHECK_INT (ncol);
  ncols = XINT (ncol);
  if (ncols < 0) ncols = 0;
  w = decode_window (window);
  if (w->hscroll != ncols)
    MARK_CLIP_CHANGED;	/* FSF marks differently but we aren't FSF. */
  w->hscroll = ncols;
  return ncol;
}

#if 0 /* bogus FSF crock */

xxDEFUN ("window-redisplay-end-trigger",
	 Fwindow_redisplay_end_trigger, 0, 1, 0, /*
Return WINDOW's redisplay end trigger value.
See `set-window-redisplay-end-trigger' for more information.
*/
	 (window))
{
  return decode_window (window)->redisplay_end_trigger;
}

xxDEFUN ("set-window-redisplay-end-trigger",
	 Fset_window_redisplay_end_trigger, 2, 2, 0, /*
Set WINDOW's redisplay end trigger value to VALUE.
VALUE should be a buffer position (typically a marker) or nil.
If it is a buffer position, then if redisplay in WINDOW reaches a position
beyond VALUE, the functions in `redisplay-end-trigger-functions' are called
with two arguments: WINDOW, and the end trigger value.
Afterwards the end-trigger value is reset to nil.
*/
	 (window, value))
{
  return (decode_window (window)->redisplay_end_trigger = value);
}

#endif /* 0 */

DEFUN ("window-pixel-edges", Fwindow_pixel_edges, 0, 1, 0, /*
Return a list of the pixel edge coordinates of WINDOW.
\(LEFT TOP RIGHT BOTTOM), all relative to 0, 0 at top left corner of frame.
The frame toolbars and menubars are considered to be outside of this area.
*/
       (window))
{
  struct window *w = decode_window (window);
  struct frame *f = XFRAME (w->frame);

  int left = w->pixel_left - FRAME_LEFT_BORDER_END (f);
  int top  = w->pixel_top  - FRAME_TOP_BORDER_END  (f);

  return list4 (make_int (left),
		make_int (top),
		make_int (left + w->pixel_width),
		make_int (top + w->pixel_height));
}

DEFUN ("window-text-area-pixel-edges",
       Fwindow_text_area_pixel_edges, 0, 1, 0, /*
Return a list of the pixel edge coordinates of the text area of WINDOW.
Returns the list \(LEFT TOP RIGHT BOTTOM), all relative to 0, 0 at the
top left corner of the window.
*/
       (window))
{
  struct window *w = decode_window (window);

  int left   = window_left_gutter_width (w, /* modeline = */ 0);
  int top    = window_top_gutter_height (w);
  int right  = WINDOW_WIDTH (w) - window_right_gutter_width (w, 0);
  int bottom = WINDOW_HEIGHT (w) - window_bottom_gutter_height (w);

  return list4 (make_int (left),
		make_int (top),
		make_int (right),
		make_int (bottom));
}

DEFUN ("window-point", Fwindow_point, 0, 1, 0, /*
Return current value of point in WINDOW.
For a nonselected window, this is the value point would have
if that window were selected.

Note that, when WINDOW is the selected window and its buffer
is also currently selected, the value returned is the same as (point).
It would be more strictly correct to return the `top-level' value
of point, outside of any save-excursion forms.
But that is hard to define.
*/
       (window))
{
  struct window *w = decode_window (window);

  /* The special check for current buffer is necessary for this
     function to work as defined when called within an excursion. */
  if (w == XWINDOW (Fselected_window (XFRAME (w->frame)->device))
      && current_buffer == XBUFFER (w->buffer))
    return Fpoint (Qnil);
  return Fmarker_position (w->pointm[CURRENT_DISP]);
}

DEFUN ("window-start", Fwindow_start, 0, 1, 0, /*
Return position at which display currently starts in WINDOW.
This is updated by redisplay or by calling `set-window-start'.
*/
       (window))
{
  return Fmarker_position (decode_window (window)->start[CURRENT_DISP]);
}

DEFUN ("window-end", Fwindow_end, 0, 2, 0, /*
Return position at which display currently ends in WINDOW.
This is updated by redisplay, when it runs to completion.
Simply changing the buffer text or setting `window-start'
does not update this value.
If GUARANTEE is non-nil, then the return value is guaranteed to be
the value of window-end at the end of the next full redisplay assuming
nothing else changes in the meantime.  This function is potentially much
slower with this flag set.
*/
       (window, guarantee))
{
  struct window *w = decode_window (window);

  if (NILP (guarantee))
    {
      Lisp_Object buf;
      buf = w->buffer;
      CHECK_BUFFER (buf);
      return make_int (BUF_Z (XBUFFER (buf)) - w->window_end_pos[CURRENT_DISP]);
    }
  else
    {
      Bufpos startp = marker_position (w->start[CURRENT_DISP]);
      return make_int (end_of_last_line (w, startp));
    }
}

DEFUN ("set-window-point", Fset_window_point, 2, 2, 0, /*
Make point value in WINDOW be at position POS in WINDOW's buffer.
*/
       (window, pos))
{
  struct window *w = decode_window (window);

  CHECK_INT_COERCE_MARKER (pos);
  if (w == XWINDOW (Fselected_window (Qnil)))
    Fgoto_char (pos, Qnil);
  else
    set_marker_restricted (w->pointm[CURRENT_DISP], pos, w->buffer);

  MARK_POINT_CHANGED;
  return pos;
}

DEFUN ("set-window-start", Fset_window_start, 2, 3, 0, /*
Make display in WINDOW start at position POS in WINDOW's buffer.
Optional third arg NOFORCE non-nil inhibits next redisplay
from overriding motion of point in order to display at this exact start.
*/
       (window, pos, noforce))
{
  struct window *w = decode_window (window);

  CHECK_INT_COERCE_MARKER (pos);
  set_marker_restricted (w->start[CURRENT_DISP], pos, w->buffer);
  /* this is not right, but much easier than doing what is right. */
  /* w->start_at_line_beg = 0; */
  /* WTF is the above supposed to mean?  GE */
  w->start_at_line_beg = beginning_of_line_p (XBUFFER (w->buffer),
					      marker_position (w->start[CURRENT_DISP]));
  if (NILP (noforce))
    w->force_start = 1;
  w->redo_modeline = 1;
  SET_LAST_MODIFIED (w, 0);
  SET_LAST_FACECHANGE (w);

  MARK_WINDOWS_CHANGED (w);

  return pos;
}

DEFUN ("window-dedicated-p", Fwindow_dedicated_p, 1, 1, 0, /*
Return WINDOW's dedicated object, usually t or nil.
See also `set-window-dedicated-p'.
*/
       (window))
{
  return decode_window (window)->dedicated;
}

DEFUN ("set-window-dedicated-p", Fset_window_dedicated_p, 2, 2, 0, /*
Control whether WINDOW is dedicated to the buffer it displays.
If it is dedicated, Emacs will not automatically change
which buffer appears in it.
The second argument is the new value for the dedication flag;
non-nil means yes.
*/
       (window, arg))
{
  struct window *w = decode_window (window);

  w->dedicated = NILP (arg) ? Qnil : Qt;

  return w->dedicated;
}

/* FSFmacs has window-display-table here.  We have display table as a
   specifier. */


/* Record info on buffer window w is displaying
   when it is about to cease to display that buffer.  */
static void
unshow_buffer (struct window *w)
{
  Lisp_Object buf = w->buffer;

  if (XBUFFER (buf) != XMARKER (w->pointm[CURRENT_DISP])->buffer)
    abort ();

  /* FSF disables this check, so I'll do it too.  I hope it won't
     break things.  --ben */
#if 0
  if (w == XWINDOW (Fselected_window (Qnil))
      || ! EQ (buf, XWINDOW (Fselected_window (Qnil))->buffer))
    /* Do this except when the selected window's buffer
       is being removed from some other window.  */
#endif
    /* last_window_start records the start position that this buffer
       had in the last window to be disconnected from it.
       Now that this statement is unconditional,
       it is possible for the buffer to be displayed in the
       selected window, while last_window_start reflects another
       window which was recently showing the same buffer.
       Some people might say that might be a good thing.  Let's see.  */
    XBUFFER (buf)->last_window_start =
      marker_position (w->start[CURRENT_DISP]);

  /* Point in the selected window's buffer
     is actually stored in that buffer, and the window's pointm isn't used.
     So don't clobber point in that buffer.  */
  if (! EQ (buf, XWINDOW (Fselected_window (Qnil))->buffer))
    {
      struct buffer *b= XBUFFER (buf);
      BUF_SET_PT (b, bufpos_clip_to_bounds (BUF_BEGV (b),
                                     marker_position (w->pointm[CURRENT_DISP]),
                                     BUF_ZV (b)));
    }
}

/* Put REPLACEMENT into the window structure in place of OLD. */
static void
replace_window (Lisp_Object old, Lisp_Object replacement)
{
  Lisp_Object tem;
  struct window *o = XWINDOW (old), *p = XWINDOW (replacement);

  /* If OLD is its frame's root_window, then replacement is the new
     root_window for that frame.  */

  if (EQ (old, FRAME_ROOT_WINDOW (XFRAME (o->frame))))
    FRAME_ROOT_WINDOW (XFRAME (o->frame)) = replacement;

  WINDOW_LEFT (p) = WINDOW_LEFT (o);
  WINDOW_TOP (p) = WINDOW_TOP (o);
  WINDOW_WIDTH (p) = WINDOW_WIDTH (o);
  WINDOW_HEIGHT (p) = WINDOW_HEIGHT (o);

  p->next = tem = o->next;
  if (!NILP (tem))
    XWINDOW (tem)->prev = replacement;

  p->prev = tem = o->prev;
  if (!NILP (tem))
    XWINDOW (tem)->next = replacement;

  p->parent = tem = o->parent;
  if (!NILP (tem))
    {
      if (EQ (XWINDOW (tem)->vchild, old))
	XWINDOW (tem)->vchild = replacement;
      if (EQ (XWINDOW (tem)->hchild, old))
	XWINDOW (tem)->hchild = replacement;
    }

  /* #### Here, if replacement is a vertical combination
     and so is its new parent, we should make replacement's
     children be children of that parent instead. */
}

/* we're deleting W; set the structure of W to indicate this. */

static void
mark_window_as_deleted (struct window *w)
{
  /* In the loop
     (while t (split-window) (delete-window))
     we end up with a tree of deleted windows which are all connected
     through the `next' slot.  This might not seem so bad, as they're
     deleted, and will presumably be GCed - but if even *one* of those
     windows is still being pointed to, by the user, or by a window
     configuration, then *all* of those windows stick around.

     Since the window-configuration code doesn't need any of the
     pointers to other windows (they are all recreated from the
     window-config data), we set them all to nil so that we
     are able to collect more actual garbage.
   */
  w->next = Qnil;
  w->prev = Qnil;
  w->hchild = Qnil;
  w->vchild = Qnil;
  w->parent = Qnil;

  w->dead = 1;

  /* Free the extra data structures attached to windows immediately so
     they don't sit around consuming excess space.  They will be
     reinitialized by the window-configuration code as necessary. */
  finalize_window ((void *) w, 0);
}

DEFUN ("delete-window", Fdelete_window, 0, 2, "", /*
Remove WINDOW from the display.  Default is selected window.
If window is the only one on the frame, the frame is destroyed.
Normally, you cannot delete the last non-minibuffer-only frame (you must
use `save-buffers-kill-emacs' or `kill-emacs').  However, if optional
second argument FORCE is non-nil, you can delete the last frame. (This
will automatically call `save-buffers-kill-emacs'.)
*/
       (window, force))
{
  /* This function can GC if this is the only window in the frame */
  struct window *w;
  Lisp_Object parent;
  struct window *par;
  Lisp_Object frame;
  struct frame *f;
  struct device *d;

  /* Note: this function is called by other C code on non-leaf
     windows. */

  /* Do the equivalent of decode_window() but don't error out on
     deleted window; it's OK to delete an already-deleted window. */
  if (NILP (window))
    window = Fselected_window (Qnil);
  else
    CHECK_WINDOW (window);
  w = XWINDOW (window);

  /* It's okay to delete an already-deleted window.  */
  if (! WINDOW_LIVE_P (w))
    return Qnil;

  frame = WINDOW_FRAME (w);
  f = XFRAME (frame);
  d = XDEVICE (FRAME_DEVICE (f));

  if (TOP_LEVEL_WINDOW_P (w))
    {
      if (NILP (memq_no_quit (frame, DEVICE_FRAME_LIST (d))))
	/* this frame isn't fully initialized yet; don't blow up. */
	return Qnil;

      if (MINI_WINDOW_P (XWINDOW (window)))
	error ("Attempt to delete the minibuffer window");

      /* It has been suggested that it's a good thing for C-x 0 to have this
	 behavior, but not such a good idea for #'delete-window to have it.
	 Maybe C-x 0 should be bound to something else, or maybe frame
	 deletion should only happen when this is called interactively.
       */
      delete_frame_internal (f, !NILP (force), 0, 0);
      return Qnil;
    }

  /* At this point, we know the window has a parent. */
  parent = w->parent;
  par = XWINDOW (parent);

  MARK_FRAME_WINDOWS_STRUCTURE_CHANGED (f);

  /* Are we trying to delete any frame's selected window?
     Note that we could be dealing with a non-leaf window
     where the selected window is one of our children.
     So, we check by scanning all the ancestors of the
     frame's selected window and comparing each one with
     WINDOW.  */
  {
    Lisp_Object pwindow;

    pwindow = FRAME_SELECTED_WINDOW (f);

    while (!NILP (pwindow))
      {
	if (EQ (window, pwindow))
	  break;
	pwindow = XWINDOW (pwindow)->parent;
      }

    if (EQ (window, pwindow))
      {
	/* OK, we found it. */
	Lisp_Object alternative;
	alternative = Fnext_window (window, Qlambda, Qnil, Qnil);

	/* If we're about to delete the selected window on the
	   selected frame, then we should use Fselect_window to select
	   the new window.  On the other hand, if we're about to
	   delete the selected window on any other frame, we shouldn't do
	   anything but set the frame's selected_window slot.  */
	if (EQ (frame, Fselected_frame (Qnil)))
	  Fselect_window (alternative, Qnil);
	else
	  set_frame_selected_window (f, alternative);
      }
  }

  /* w->buffer is nil in a non-leaf window; in this case,
     get rid of the markers we maintain that point into that buffer. */
  if (!NILP (w->buffer))
    {
      unshow_buffer (w);
      unchain_marker (w->pointm[CURRENT_DISP]);
      unchain_marker (w->pointm[DESIRED_DISP]);
      unchain_marker (w->pointm[CMOTION_DISP]);
      unchain_marker (w->start[CURRENT_DISP]);
      unchain_marker (w->start[DESIRED_DISP]);
      unchain_marker (w->start[CMOTION_DISP]);
      unchain_marker (w->sb_point);
      /* This breaks set-window-configuration if windows in the saved
	 configuration get deleted and multiple frames are in use. */
      /* w->buffer = Qnil; */
    }

  /* close up the hole in the sibling list */
  if (!NILP (w->next))
    XWINDOW (w->next)->prev = w->prev;
  if (!NILP (w->prev))
    XWINDOW (w->prev)->next = w->next;
  if (EQ (window, par->hchild))
    par->hchild = w->next;
  if (EQ (window, par->vchild))
    par->vchild = w->next;

  /* Find one of our siblings to give our space to.  */
  {
    Lisp_Object sib = w->prev;
    if (NILP (sib))
      {
	/* If w gives its space to its next sibling, that sibling needs
	   to have its top/left side pulled back to where w's is.
	   set_window_{height,width} will re-position the sibling's
	   children.  */
	sib = w->next;
	WINDOW_TOP (XWINDOW (sib)) = WINDOW_TOP (w);
	WINDOW_LEFT (XWINDOW (sib)) = WINDOW_LEFT (w);
      }

    /* Stretch that sibling.  */
    if (!NILP (par->vchild))
      set_window_pixheight
	(sib, (WINDOW_HEIGHT (XWINDOW (sib)) + WINDOW_HEIGHT (w)), 1);
    if (!NILP (par->hchild))
      set_window_pixwidth
	(sib, (WINDOW_WIDTH (XWINDOW (sib)) + WINDOW_WIDTH (w)), 1);
  }

  /* If parent now has only one child,
     put the child into the parent's place.  */
  {
    Lisp_Object parchild = par->hchild;
    if (NILP (parchild))
      parchild = par->vchild;
    if (NILP (XWINDOW (parchild)->next))
      {
	replace_window (parent, parchild);
	mark_window_as_deleted (XWINDOW (parent));
      }
  }

  /* Since we may be deleting combination windows, we must make sure that
     not only W but all its children have been marked as deleted.  */
  if (!NILP (w->hchild))
    delete_all_subwindows (XWINDOW (w->hchild));
  else if (!NILP (w->vchild))
    delete_all_subwindows (XWINDOW (w->vchild));

  mark_window_as_deleted (w);

  f->mirror_dirty = 1;
  return Qnil;
}


DEFUN ("next-window", Fnext_window, 0, 4, 0, /*
Return next window after WINDOW in canonical ordering of windows.
If omitted, WINDOW defaults to the selected window.

Optional second arg MINIBUF t means count the minibuffer window even
if not active.  MINIBUF nil or omitted means count the minibuffer iff
it is active.  MINIBUF neither t nor nil means not to count the
minibuffer even if it is active.

Several frames may share a single minibuffer; if the minibuffer
counts, all windows on all frames that share that minibuffer count
too.  Therefore, `next-window' can be used to iterate through the
set of windows even when the minibuffer is on another frame.  If the
minibuffer does not count, only windows from WINDOW's frame count.

Optional third arg ALL-FRAMES t means include windows on all frames.
ALL-FRAMES nil or omitted means cycle within the frames as specified
above.  ALL-FRAMES = `visible' means include windows on all visible frames.
ALL-FRAMES = 0 means include windows on all visible and iconified frames.
If ALL-FRAMES is a frame, restrict search to windows on that frame.
Anything else means restrict to WINDOW's frame.

Optional fourth argument CONSOLE controls which consoles or devices the
returned window may be on.  If CONSOLE is a console, return windows only
on that console.  If CONSOLE is a device, return windows only on that
device.  If CONSOLE is a console type, return windows only on consoles
of that type.  If CONSOLE is 'window-system, return any windows on any
window-system consoles.  If CONSOLE is nil or omitted, return windows only
on WINDOW's console.  Otherwise, all windows are considered.

If you use consistent values for MINIBUF, ALL-FRAMES, and CONSOLE, you
can use `next-window' to iterate through the entire cycle of acceptable
windows, eventually ending up back at the window you started with.
`previous-window' traverses the same cycle, in the reverse order.
*/
     (window, minibuf, all_frames, console))
{
  Lisp_Object tem;
  Lisp_Object start_window;

  if (NILP (window))
    window = Fselected_window (Qnil);
  else
    CHECK_LIVE_WINDOW (window);

  start_window = window;

  /* minibuf == nil may or may not include minibuffers.
     Decide if it does.  */
  if (NILP (minibuf))
    minibuf = (minibuf_level ? minibuf_window : Qlambda);
  else if (! EQ (minibuf, Qt))
    minibuf = Qlambda;
  /* Now minibuf can be t => count all minibuffer windows,
     lambda => count none of them,
     or a specific minibuffer window (the active one) to count.  */

  /* all_frames == nil doesn't specify which frames to include.  */
  if (NILP (all_frames))
    all_frames = (! EQ (minibuf, Qlambda)
		  ? (FRAME_MINIBUF_WINDOW
		     (XFRAME
		      (WINDOW_FRAME
		       (XWINDOW (window)))))
		  : Qnil);
  else if (EQ (all_frames, Qvisible))
    ;
  else if (ZEROP (all_frames))
    ;
  else if (FRAMEP (all_frames) && ! EQ (all_frames, Fwindow_frame (window)))
    /* If all_frames is a frame and window arg isn't on that frame, just
       return the first window on the frame.  */
    return frame_first_window (XFRAME (all_frames));
  else if (! EQ (all_frames, Qt))
    all_frames = Qnil;
  /* Now all_frames is t meaning search all frames,
     nil meaning search just current frame,
     visible meaning search just visible frames,
     0 meaning search visible and iconified frames,
     or a window, meaning search the frame that window belongs to.  */

  /* Do this loop at least once, to get the next window, and perhaps
     again, if we hit the minibuffer and that is not acceptable.  */
  do
    {
      /* Find a window that actually has a next one.  This loop
	 climbs up the tree.  */
      while (tem = XWINDOW (window)->next, NILP (tem))
	if (tem = XWINDOW (window)->parent, !NILP (tem))
	  window = tem;
        else  /* window must be minibuffer window now */
	  {
	    /* We've reached the end of this frame.
	       Which other frames are acceptable?  */
	    tem = WINDOW_FRAME (XWINDOW (window));

	    if (! NILP (all_frames))
	      {
		Lisp_Object tem1;

		tem1 = tem;
		tem = next_frame (tem, all_frames, console);
		/* In the case where the minibuffer is active,
		   and we include its frame as well as the selected one,
		   next_frame may get stuck in that frame.
		   If that happens, go back to the selected frame
		   so we can complete the cycle.  */
		if (EQ (tem, tem1))
		  XSETFRAME (tem, selected_frame ());
	      }

	    tem = FRAME_ROOT_WINDOW (XFRAME (tem));
	    break;
	  }

      window = tem;

      /* If we're in a combination window, find its first child and
	 recurse on that.  Otherwise, we've found the window we want.  */
      while (1)
	{
	  if (!NILP (XWINDOW (window)->hchild))
	    window = XWINDOW (window)->hchild;
	  else if (!NILP (XWINDOW (window)->vchild))
	    window = XWINDOW (window)->vchild;
	  else break;
	}
    }
  /* "acceptable" is the correct spelling. */
  /* Which windows are acceptable?
     Exit the loop and accept this window if
     this isn't a minibuffer window,
     or we're accepting all minibuffer windows,
     or this is the active minibuffer and we are accepting that one, or
     we've come all the way around and we're back at the original window.  */
  while (MINI_WINDOW_P (XWINDOW (window))
	 && ! EQ (minibuf, Qt)
	 && ! EQ (minibuf, window)
	 && ! EQ (window, start_window));

  return window;
}

DEFUN ("previous-window", Fprevious_window, 0, 4, 0, /*
Return the window preceding WINDOW in canonical ordering of windows.
If omitted, WINDOW defaults to the selected window.

Optional second arg MINIBUF t means count the minibuffer window even
if not active.  MINIBUF nil or omitted means count the minibuffer iff
it is active.  MINIBUF neither t nor nil means not to count the
minibuffer even if it is active.

Several frames may share a single minibuffer; if the minibuffer
counts, all windows on all frames that share that minibuffer count
too.  Therefore, `previous-window' can be used to iterate through
the set of windows even when the minibuffer is on another frame.  If
the minibuffer does not count, only windows from WINDOW's frame count

If optional third arg ALL-FRAMES t means include windows on all frames.
ALL-FRAMES nil or omitted means cycle within the frames as specified
above.  ALL-FRAMES = `visible' means include windows on all visible frames.
ALL-FRAMES = 0 means include windows on all visible and iconified frames.
If ALL-FRAMES is a frame, restrict search to windows on that frame.
Anything else means restrict to WINDOW's frame.

Optional fourth argument CONSOLE controls which consoles or devices the
returned window may be on.  If CONSOLE is a console, return windows only
on that console.  If CONSOLE is a device, return windows only on that
device.  If CONSOLE is a console type, return windows only on consoles
of that type.  If CONSOLE is 'window-system, return any windows on any
window-system consoles.  If CONSOLE is nil or omitted, return windows only
on WINDOW's console.  Otherwise, all windows are considered.

If you use consistent values for MINIBUF, ALL-FRAMES, and CONSOLE, you
can use `previous-window' to iterate through the entire cycle of acceptable
windows, eventually ending up back at the window you started with.
`next-window' traverses the same cycle, in the reverse order.
*/
     (window, minibuf, all_frames, console))
{
  Lisp_Object tem;
  Lisp_Object start_window;

  if (NILP (window))
    window = Fselected_window (Qnil);
  else
    CHECK_LIVE_WINDOW (window);

  start_window = window;

  /* minibuf == nil may or may not include minibuffers.
     Decide if it does.  */
  if (NILP (minibuf))
    minibuf = (minibuf_level ? minibuf_window : Qlambda);
  else if (! EQ (minibuf, Qt))
    minibuf = Qlambda;
  /* Now minibuf can be t => count all minibuffer windows,
     lambda => count none of them,
     or a specific minibuffer window (the active one) to count.  */

  /* all_frames == nil doesn't specify which frames to include.
     Decide which frames it includes.  */
  if (NILP (all_frames))
    all_frames = (! EQ (minibuf, Qlambda)
		  ? (FRAME_MINIBUF_WINDOW
		     (XFRAME
		      (WINDOW_FRAME
		       (XWINDOW (window)))))
		  : Qnil);
  else if (EQ (all_frames, Qvisible))
    ;
  else if (ZEROP (all_frames))
    ;
  else if (FRAMEP (all_frames) && ! EQ (all_frames, Fwindow_frame (window)))
    /* If all_frames is a frame and window arg isn't on that frame, just
       return the first window on the frame.  */
    return frame_first_window (XFRAME (all_frames));
  else if (! EQ (all_frames, Qt))
    all_frames = Qnil;
  /* Now all_frames is t meaning search all frames,
     nil meaning search just current frame,
     visible meaning search just visible frames,
     0 meaning search visible and iconified frames,
     or a window, meaning search the frame that window belongs to.  */

  /* Do this loop at least once, to get the next window, and perhaps
     again, if we hit the minibuffer and that is not acceptable.  */
  do
    {
      /* Find a window that actually has a next one.  This loop
	 climbs up the tree.  */
      while (tem = XWINDOW (window)->prev, NILP (tem))
	if (tem = XWINDOW (window)->parent, !NILP (tem))
	  window = tem;
        else  /* window must be minibuffer window now */
	  {
	    /* We have found the top window on the frame.
	       Which frames are acceptable?  */
	    tem = WINDOW_FRAME (XWINDOW (window));

	    if (! NILP (all_frames))
	      /* It's actually important that we use prev_frame here,
		 rather than next_frame.  All the windows acceptable
		 according to the given parameters should form a ring;
		 Fnext_window and Fprevious_window should go back and
		 forth around the ring.  If we use next_frame here,
		 then Fnext_window and Fprevious_window take different
		 paths through the set of acceptable windows.
		 window_loop assumes that these `ring' requirement are
		 met.  */
	      {
		Lisp_Object tem1;

		tem1 = tem;
		tem = prev_frame (tem, all_frames, console);
		/* In the case where the minibuffer is active,
		   and we include its frame as well as the selected one,
		   next_frame may get stuck in that frame.
		   If that happens, go back to the selected frame
		   so we can complete the cycle.  */
		if (EQ (tem, tem1))
		  XSETFRAME (tem, selected_frame ());
	      }

	    /* If this frame has a minibuffer, find that window first,
	       because it is conceptually the last window in that frame.  */
	    if (FRAME_HAS_MINIBUF_P (XFRAME (tem)))
	      tem = FRAME_MINIBUF_WINDOW (XFRAME (tem));
	    else
	      tem = FRAME_ROOT_WINDOW (XFRAME (tem));

	    break;
	  }

      window = tem;

      /* If we're in a combination window, find its first child and
	 recurse on that.  Otherwise, we've found the window we want.  */
      while (1)
	{
	  if (!NILP (XWINDOW (window)->hchild))
	    window = XWINDOW (window)->hchild;
	  else if (!NILP (XWINDOW (window)->vchild))
	    window = XWINDOW (window)->vchild;
	  else break;
	  while (tem = XWINDOW (window)->next, !NILP (tem))
	    window = tem;
	}
    }
  /* Which windows are acceptable?
     Exit the loop and accept this window if
     this isn't a minibuffer window,
     or we're accepting all minibuffer windows,
     or this is the active minibuffer and we are accepting that one, or
     we've come all the way around and we're back at the original window.  */
  while (MINI_WINDOW_P (XWINDOW (window))
	 && ! EQ (minibuf, Qt)
	 && ! EQ (minibuf, window)
	 && ! EQ (window, start_window));

  return window;
}

DEFUN ("next-vertical-window", Fnext_vertical_window, 0, 1, 0, /*
Return the next window which is vertically after WINDOW.
*/
       (window))
{
  Lisp_Object root;
  struct window *w = decode_window (window);
  XSETWINDOW (window, w);

  if (MINI_WINDOW_P (XWINDOW (window)))
    return Qnil;

  root = FRAME_ROOT_WINDOW (XFRAME (WINDOW_FRAME (XWINDOW (window))));

  if (EQ (window, root))
    {
      while (1)
	if (!NILP (XWINDOW (window)->hchild))
	  window = XWINDOW (window)->hchild;
	else if (!NILP (XWINDOW (window)->vchild))
	  window = XWINDOW (window)->vchild;
	else
	  return window;
    }

  do
    {
      if (!NILP (XWINDOW (window)->parent) &&
	  !NILP (XWINDOW (XWINDOW (window)->parent)->vchild))
	{
	  if (!NILP (XWINDOW (window)->next))
	    return XWINDOW (window)->next;
	  else
	    window = XWINDOW (window)->parent;
	}
      else
	window = XWINDOW (window)->parent;
    }
  while (!EQ (window, root));

  while (1)
    if (!NILP (XWINDOW (window)->hchild))
      window = XWINDOW (window)->hchild;
    else if (!NILP (XWINDOW (window)->vchild))
      window = XWINDOW (window)->vchild;
    else
      return window;
}

DEFUN ("other-window", Fother_window, 1, 3, "p", /*
Select the N'th different window on this frame.
All windows on current frame are arranged in a cyclic order.
This command selects the window N steps away in that order.
A negative N moves in the opposite order.

If optional argument FRAME is `visible', search all visible frames.
If FRAME is 0, search all visible and iconified frames.
If FRAME is t, search all frames.
If FRAME is nil, search only the selected frame.
If FRAME is a frame, search only that frame.

Optional third argument CONSOLE controls which consoles or devices the
returned window may be on.  If CONSOLE is a console, return windows only
on that console.  If CONSOLE is a device, return windows only on that
device.  If CONSOLE is a console type, return windows only on consoles
of that type.  If CONSOLE is 'window-system, return any windows on any
window-system consoles.  If CONSOLE is nil or omitted, return windows only
on FRAME'S console, or on the selected console if FRAME is not a frame.
Otherwise, all windows are considered.
*/
       (n, frame, console))
{
  int i;
  Lisp_Object w;

  CHECK_INT (n);
  w = Fselected_window (Qnil);
  i = XINT (n);

  while (i > 0)
    {
      w = Fnext_window (w, Qnil, frame, console);
      i--;
    }
  while (i < 0)
    {
      w = Fprevious_window (w, Qnil, frame, console);
      i++;
    }
  Fselect_window (w, Qnil);
  return Qnil;
}


/* Look at all windows, performing an operation specified by TYPE
   with argument OBJ.

   If FRAMES is Qt, look at all frames, if Qnil, look at just the selected
   frame.  If FRAMES is a frame, just look at windows on that frame.
   If MINI is non-zero, perform the operation on minibuffer windows too.
*/

enum window_loop
{
  WINDOW_LOOP_UNUSED,
  GET_BUFFER_WINDOW,		/* Arg is buffer */
  GET_LRU_WINDOW,		/* Arg is t for full-width windows only */
  DELETE_OTHER_WINDOWS,		/* Arg is window not to delete */
  DELETE_BUFFER_WINDOWS,	/* Arg is buffer */
  GET_LARGEST_WINDOW,
  UNSHOW_BUFFER,		/* Arg is buffer */
  GET_BUFFER_WINDOW_COUNT,	/* Arg is buffer */
  GET_BUFFER_MRU_WINDOW		/* Arg is buffer */
};

static Lisp_Object
window_loop (enum window_loop type,
             Lisp_Object obj,
             int mini,
             Lisp_Object frames,
	     int dedicated_too,
	     Lisp_Object console)
{
  /* This function can GC if type == DELETE_BUFFER_WINDOWS or UNSHOW_BUFFER */
  Lisp_Object w;
  Lisp_Object best_window = Qnil;
  Lisp_Object next_window;
  Lisp_Object last_window;
  struct frame *frame;
  Lisp_Object frame_arg = Qt;
  int count = 0;		/* for GET_BUFFER_WINDOW_COUNT */
  /* #### I think the change of "precomputing" last_window and next_window
   * ####  catch the lossage this is meant(?) to punt on...
   */
  int lose_lose = 0;
  Lisp_Object devcons, concons;

  /* FRAME_ARG is Qlambda to stick to one frame,
     Qvisible to consider all visible frames,
     or Qt otherwise.  */

  /* If we're only looping through windows on a particular frame,
     FRAME points to that frame.  If we're looping through windows
     on all frames, FRAME is 0.  */

  if (FRAMEP (frames))
    frame = XFRAME (frames);
  else if (NILP (frames))
    frame = selected_frame ();
  else
    frame = 0;
  if (frame)
    frame_arg = Qlambda;
  else if (ZEROP (frames))
    frame_arg = frames;
  else if (EQ (frames, Qvisible))
    frame_arg = frames;

  DEVICE_LOOP_NO_BREAK (devcons, concons)
    {
      Lisp_Object device = XCAR (devcons);
      Lisp_Object the_frame;

      if (frame)
	XSETFRAME (the_frame, frame);
      else
	the_frame = DEVICE_SELECTED_FRAME (XDEVICE (device));

      if (NILP (the_frame))
	continue;

      if (!device_matches_console_spec (the_frame, device, console))
	continue;

      /* Pick a window to start with.  */
      if (WINDOWP (obj))
	w = obj;
      else
	w = FRAME_SELECTED_WINDOW (XFRAME (the_frame));

      /* Figure out the last window we're going to mess with.  Since
	 Fnext_window, given the same options, is guaranteed to go in a
	 ring, we can just use Fprevious_window to find the last one.

	 We can't just wait until we hit the first window again,
	 because it might be deleted.  */

      last_window = Fprevious_window (w, mini ? Qt : Qnil, frame_arg, Qt);

      best_window = Qnil;
      for (;;)
	{
	  struct window *p = XWINDOW (w);
	  struct frame *w_frame = XFRAME (WINDOW_FRAME (p));

	  /* Pick the next window now, since some operations will delete
	     the current window.  */
	  next_window = Fnext_window (w, mini ? Qt : Qnil, frame_arg, Qt);

	  /* #### Still needed ?? */
	  /* Given the outstanding quality of the rest of this code,
	     I feel no shame about putting this piece of shit in. */
	  if (++lose_lose >= 500)
	    return Qnil;

	  /* Note that we do not pay attention here to whether
	     the frame is visible, since Fnext_window skips non-visible frames
	     if that is desired, under the control of frame_arg.  */
	  if (! MINI_WINDOW_P (p)
	      || (mini && minibuf_level > 0))
	    switch (type)
	      {
	      case GET_BUFFER_WINDOW:
		{
		  if (XBUFFER (p->buffer) == XBUFFER (obj))
		    return w;
		  break;
		}

	      case GET_BUFFER_WINDOW_COUNT:
		{
		  if (XBUFFER (p->buffer) == XBUFFER (obj))
		    count++;
		  break;
		}

	      case GET_LRU_WINDOW:
		{
		  /* t as arg means consider only full-width windows */
		  if (!NILP (obj)
		      && !window_full_width_p (p))
		    break;
		  /* Ignore dedicated windows and minibuffers.  */
		  if (MINI_WINDOW_P (p)
		      || (dedicated_too ? 0 : !NILP (p->dedicated)))
		    break;
		  if (NILP (best_window)
		      || (XINT (XWINDOW (best_window)->use_time)
			  > XINT (p->use_time)))
		    best_window = w;
		  break;
		}

	      case GET_BUFFER_MRU_WINDOW:
		{
		  /* #### what about the first check in GET_LRU_WINDOW? */
		  /* Ignore dedicated windows and minibuffers. */
		  if (MINI_WINDOW_P (p)
		      || (dedicated_too ? 0 : !NILP (p->dedicated)))
		    break;

		  if (XBUFFER (p->buffer) == XBUFFER (obj))
		    {
		      if (NILP (best_window)
			  || (XINT (XWINDOW (best_window)->use_time)
			      < XINT (p->use_time)))
			best_window = w;
		    }
		  break;
		}

	      case DELETE_OTHER_WINDOWS:
		{
		  /* Don't delete the last window on a frame; this can
		     happen when the minibuffer is selected, and would
		     cause the frame to be deleted. */
		  if (p != XWINDOW (obj) && !TOP_LEVEL_WINDOW_P (XWINDOW (w)))
		    Fdelete_window (w, Qnil);
		  break;
		}

	      case DELETE_BUFFER_WINDOWS:
		{
		  if (EQ (p->buffer, obj))
		    {
		      struct frame *f = XFRAME (WINDOW_FRAME (p));

		      /* If this window is dedicated, and in a frame
			 of its own, kill the frame.  */
		      if (EQ (w, FRAME_ROOT_WINDOW (f))
			  && !NILP (p->dedicated)
			  && other_visible_frames (f))
			{
			  /* Skip the other windows on this frame.
			     There might be one, the minibuffer!  */
			  if (! EQ (w, last_window))
			    while (f == XFRAME (WINDOW_FRAME
						(XWINDOW (next_window))))
			      {
				/* As we go, check for the end of the
				   loop.  We mustn't start going
				   around a second time.  */
				if (EQ (next_window, last_window))
				  {
				    last_window = w;
				    break;
				  }
				next_window = Fnext_window (next_window,
							    mini ? Qt : Qnil,
							    frame_arg, Qt);
			      }
			  /* Now we can safely delete the frame.  */
			  Fdelete_frame (WINDOW_FRAME (p), Qnil);
			}
		      else
			/* If we're deleting the buffer displayed in
			   the only window on the frame, find a new
			   buffer to display there.  */
			if (NILP (p->parent))
			  {
			    Lisp_Object new_buffer;
			    new_buffer = Fother_buffer (obj, Qnil, Qnil);
			    if (NILP (new_buffer))
			      new_buffer = Fget_buffer_create (QSscratch);
			    Fset_window_buffer (w, new_buffer);
			    if (EQ (w, Fselected_window (Qnil)))
			      Fset_buffer (p->buffer);
			  }
			else
			  Fdelete_window (w, Qnil);
		    }
		  break;
		}

	      case GET_LARGEST_WINDOW:
		{
		  /* Ignore dedicated windows and minibuffers.  */
		  if (MINI_WINDOW_P (p)
		      || (dedicated_too ? 0 : !NILP (p->dedicated)))
		    break;
		  {
		    /* write the check as follows to avoid tripping
		       error_check_window() --ben */
		    struct window *b = NILP (best_window) ? 0 :
		      XWINDOW (best_window);
		    if (NILP (best_window)
			|| ((WINDOW_HEIGHT (p) * WINDOW_WIDTH (p))
			    > (WINDOW_HEIGHT (b) * WINDOW_WIDTH (b))))
		      best_window = w;
		  }
		  break;
		}

	      case UNSHOW_BUFFER:
		{
		  if (EQ (p->buffer, obj))
		    {
		      /* Find another buffer to show in this window.  */
		      Lisp_Object another_buffer =
			Fother_buffer (obj, Qnil, Qnil);
		      if (NILP (another_buffer))
			another_buffer
			  = Fget_buffer_create (QSscratch);
		      /* If this window is dedicated, and in a frame
			 of its own, kill the frame.  */
		      if (EQ (w, FRAME_ROOT_WINDOW (w_frame))
			  && !NILP (p->dedicated)
			  && other_visible_frames (w_frame))
			{
			  /* Skip the other windows on this frame.
			     There might be one, the minibuffer!  */
			  if (! EQ (w, last_window))
			    while (w_frame == XFRAME (WINDOW_FRAME
						      (XWINDOW (next_window))))
			      {
				/* As we go, check for the end of the
				   loop.  We mustn't start going
				   around a second time.  */
				if (EQ (next_window, last_window))
				  {
				    last_window = w;
				    break;
				  }
				next_window = Fnext_window (next_window,
							    mini ? Qt : Qnil,
							    frame_arg, Qt);
			      }
			  /* Now we can safely delete the frame.  */
			  delete_frame_internal (XFRAME (WINDOW_FRAME (p)),
						 0, 0, 0);
			}
		      else
			{
			  /* Otherwise show a different buffer in the
                             window.  */
			  p->dedicated = Qnil;
			  Fset_window_buffer (w, another_buffer);
			  if (EQ (w, Fselected_window (Qnil)))
			    Fset_buffer (p->buffer);
			}
		    }
		  break;
		}

	      default:
		abort ();
	      }

	  if (EQ (w, last_window))
	    break;

	  w = next_window;
	}
    }

  return type == GET_BUFFER_WINDOW_COUNT ? make_int (count) : best_window;
}

#if 0 /* not currently used */

int
buffer_window_count (struct buffer *b, struct frame *f)
{
  Lisp_Object buffer, frame;

  XSETFRAME (frame, f);
  XSETBUFFER (buffer, b);

  return XINT (window_loop (GET_BUFFER_WINDOW_COUNT, buffer, 0, frame, 1,
			    Qnil));
}

int
buffer_window_mru (struct window *w)
{
  Lisp_Object window =
    window_loop (GET_BUFFER_MRU_WINDOW, w->buffer, 0, w->frame, 1, Qnil);

  if (NILP (window))
    return 0;
  else if (XWINDOW (window) == w)
    return 1;
  else
    return 0;
}

#endif


DEFUN ("get-lru-window", Fget_lru_window, 0, 2, 0, /*
Return the window least recently selected or used for display.
If optional argument FRAME is `visible', search all visible frames.
If FRAME is 0, search all visible and iconified frames.
If FRAME is t, search all frames.
If FRAME is nil, search only the selected frame.
If FRAME is a frame, search only that frame.

Optional second argument CONSOLE controls which consoles or devices the
returned window may be on.  If CONSOLE is a console, return windows only
on that console.  If CONSOLE is a device, return windows only on that
device.  If CONSOLE is a console type, return windows only on consoles
of that type.  If CONSOLE is 'window-system, return any windows on any
window-system consoles.  If CONSOLE is nil or omitted, return windows only
on FRAME'S console, or on the selected console if FRAME is not a frame.
Otherwise, all windows are considered.
*/
       (frame, console))
{
  Lisp_Object w;
  /* First try for a non-dedicated window that is full-width */
  w = window_loop (GET_LRU_WINDOW, Qt, 0, frame, 0, console);
  if (!NILP (w) && !EQ (w, Fselected_window (Qnil)))
    return w;

  /* Then try for any non-dedicated window */
  w = window_loop (GET_LRU_WINDOW, Qnil, 0, frame, 0, console);
  if (!NILP (w) && !EQ (w, Fselected_window (Qnil)))
    return w;

#if 0
  /* FSFmacs never returns a dedicated window here.  If we do,
     it makes `display-buffer' not work right.  #### All of this
     shit is so disgusting and awful that it needs to be rethought
     from scratch. */
  /* then try for a dedicated window that is full-width */
  w = window_loop (GET_LRU_WINDOW, Qt, 0, frame, 1, console);
  if (!NILP (w) && !EQ (w, Fselected_window (Qnil)))
    return w;

  /* If none of them, then all windows, dedicated or not. */
  w = window_loop (GET_LRU_WINDOW, Qnil, 0, frame, 1, console);

  /* At this point we damn well better have found something. */
  if (NILP (w)) abort ();
#endif

  return w;
}

DEFUN ("get-largest-window", Fget_largest_window, 0, 2, 0, /*
Return the window largest in area.
If optional argument FRAME is `visible', search all visible frames.
If FRAME is 0, search all visible and iconified frames.
If FRAME is t, search all frames.
If FRAME is nil, search only the selected frame.
If FRAME is a frame, search only that frame.

Optional second argument CONSOLE controls which consoles or devices the
returned window may be on.  If CONSOLE is a console, return windows only
on that console.  If CONSOLE is a device, return windows only on that
device.  If CONSOLE is a console type, return windows only on consoles
of that type.  If CONSOLE is 'window-system, return any windows on any
window-system consoles.  If CONSOLE is nil or omitted, return windows only
on FRAME'S console, or on the selected console if FRAME is not a frame.
Otherwise, all windows are considered.
*/
       (frame, console))
{
  /* Don't search dedicated windows because FSFmacs doesn't.
     This stuff is all black magic so don't try to apply common
     sense to it. */
  return window_loop (GET_LARGEST_WINDOW, Qnil, 0, frame, 0, console);
}

DEFUN ("get-buffer-window", Fget_buffer_window, 1, 3, 0, /*
Return a window currently displaying BUFFER, or nil if none.
If optional argument FRAME is `visible', search all visible frames.
If optional argument FRAME is 0, search all visible and iconified frames.
If FRAME is t, search all frames.
If FRAME is nil, search only the selected frame.
If FRAME is a frame, search only that frame.

Optional third argument CONSOLE controls which consoles or devices the
returned window may be on.  If CONSOLE is a console, return windows only
on that console.  If CONSOLE is a device, return windows only on that
device.  If CONSOLE is a console type, return windows only on consoles
of that type.  If CONSOLE is 'window-system, return any windows on any
window-system consoles.  If CONSOLE is nil or omitted, return windows only
on FRAME'S console, or on the selected console if FRAME is not a frame.
Otherwise, all windows are considered.
*/
       (buffer, frame, console))
{
  buffer = Fget_buffer (buffer);
  if (BUFFERP (buffer))
    /* Search dedicated windows too. (Doesn't matter here anyway.) */
    return window_loop (GET_BUFFER_WINDOW, buffer, 1, frame, 1, console);
  else
    return Qnil;
}

/* These functions used to be `buffer-left-margin-pixel-width', etc.
   but there is no sensible way to implement those functions, since
   you can't in general derive a window from a buffer. */

DEFUN ("window-left-margin-pixel-width", Fwindow_left_margin_pixel_width,
       0, 1, 0, /*
Return the width in pixels of the left outside margin of window WINDOW.
If WINDOW is nil, the selected window is assumed.
*/
       (window))
{
  return make_int (window_left_margin_width (decode_window (window)));
}

DEFUN ("window-right-margin-pixel-width", Fwindow_right_margin_pixel_width,
       0, 1, 0, /*
Return the width in pixels of the right outside margin of window WINDOW.
If WINDOW is nil, the selected window is assumed.
*/
       (window))
{
  return make_int (window_right_margin_width (decode_window (window)));
}

DEFUN ("delete-other-windows", Fdelete_other_windows, 0, 1, "", /*
Make WINDOW (or the selected window) fill its frame.
Only the frame WINDOW is on is affected.
This function tries to reduce display jumps
by keeping the text previously visible in WINDOW
in the same place on the frame.  Doing this depends on
the value of (window-start WINDOW), so if calling this function
in a program gives strange scrolling, make sure the window-start
value is reasonable when this function is called.
*/
       (window))
{
  struct window *w = decode_window (window);
  struct buffer *b = XBUFFER (w->buffer);
  Bufpos start_pos;
  int old_top = WINDOW_TOP (w);

  XSETWINDOW (window, w);

  if (MINI_WINDOW_P (w) && old_top > 0)
    error ("Can't expand minibuffer to full frame");

  /* Ignore dedicated windows. */
  window_loop (DELETE_OTHER_WINDOWS, window, 0, w->frame, 0, Qnil);

  start_pos = marker_position (w->start[CURRENT_DISP]);

  /* Try to minimize scrolling, by setting the window start to the
     point which will cause the text at the old window start to be at
     the same place on the frame.  But don't try to do this if the
     window start is outside the visible portion (as might happen when
     the display is not current, due to typeahead). */
  if (start_pos >= BUF_BEGV (b) && start_pos <= BUF_ZV (b)
      && !MINI_WINDOW_P (w))
    {
      Bufpos new_start = start_with_line_at_pixpos (w, start_pos, old_top);

      if (new_start >= BUF_BEGV (b) && new_start <= BUF_ZV (b))
	{
	  Fset_marker (w->start[CURRENT_DISP], make_int (new_start),
		       w->buffer);
	  w->start_at_line_beg = beginning_of_line_p (b, new_start);
	}
      /* We need to do this, so that the window-scroll-functions
	 get called.  */
      w->force_start = 1;
    }

  return Qnil;
}

DEFUN ("delete-windows-on", Fdelete_windows_on, 1, 3,
       "bDelete windows on (buffer): ", /*
Delete all windows showing BUFFER.
Optional second argument FRAME controls which frames are affected.
If nil or omitted, delete all windows showing BUFFER in any frame.
If t, delete only windows showing BUFFER in the selected frame.
If `visible', delete all windows showing BUFFER in any visible frame.
If a frame, delete only windows showing BUFFER in that frame.

Optional third argument CONSOLE controls which consoles or devices the
returned window may be on.  If CONSOLE is a console, return windows only
on that console.  If CONSOLE is a device, return windows only on that
device.  If CONSOLE is a console type, return windows only on consoles
of that type.  If CONSOLE is 'window-system, return any windows on any
window-system consoles.  If CONSOLE is nil or omitted, return windows only
on FRAME'S console, or on the selected console if FRAME is not a frame.
Otherwise, all windows are considered.
*/
       (buffer, frame, console))
{
  /* This function can GC */
  /* FRAME uses t and nil to mean the opposite of what window_loop
     expects. */
  if (!FRAMEP (frame))
    frame = NILP (frame) ? Qt : Qnil;

  if (!NILP (buffer))
    {
      buffer = Fget_buffer (buffer);
      CHECK_BUFFER (buffer);
      /* Ignore dedicated windows. */
      window_loop (DELETE_BUFFER_WINDOWS, buffer, 0, frame, 0, console);
    }
  return Qnil;
}

DEFUN ("replace-buffer-in-windows", Freplace_buffer_in_windows, 1, 1,
       "bReplace buffer in windows: ", /*
Replace BUFFER with some other buffer in all windows showing it.
*/
       (buffer))
{
  /* This function can GC */
  if (!NILP (buffer))
    {
      buffer = Fget_buffer (buffer);
      CHECK_BUFFER (buffer);
      /* Ignore dedicated windows. */
      window_loop (UNSHOW_BUFFER, buffer, 0, Qt, 0, Qnil);
    }
  return Qnil;
}

/* The smallest acceptable dimensions for a window.  Anything smaller
   might crash Emacs.  */
#define MIN_SAFE_WINDOW_WIDTH  (2)
#define MIN_SAFE_WINDOW_HEIGHT (2)

/* Make sure that window_min_height and window_min_width are
   not too small; if they are, set them to safe minima.  */

static void
check_min_window_sizes (void)
{
  /* Smaller values might permit a crash.  */
  if (window_min_width < MIN_SAFE_WINDOW_WIDTH)
    window_min_width = MIN_SAFE_WINDOW_WIDTH;
  if (window_min_height < MIN_SAFE_WINDOW_HEIGHT)
    window_min_height = MIN_SAFE_WINDOW_HEIGHT;
}

/* If *ROWS or *COLS are too small a size for FRAME, set them to the
   minimum allowable size.  */
void
check_frame_size (struct frame *frame, int *rows, int *cols)
{
  /* For height, we have to see whether the frame has a minibuffer, and
     whether it wants a modeline.  */
  int min_height =
    (FRAME_MINIBUF_ONLY_P (frame) ? MIN_SAFE_WINDOW_HEIGHT - 1
     : (! FRAME_HAS_MINIBUF_P (frame)) ? MIN_SAFE_WINDOW_HEIGHT
     : 2 * MIN_SAFE_WINDOW_HEIGHT - 1);

  if (*rows < min_height)
    *rows = min_height;
  if (*cols  < MIN_SAFE_WINDOW_WIDTH)
    *cols = MIN_SAFE_WINDOW_WIDTH;
}

/* Normally the window is deleted if it gets too small.
   nodelete nonzero means do not do this.
   (The caller should check later and do so if appropriate)  */
static void
set_window_pixsize (Lisp_Object window, int new_pixsize, int nodelete,
		    int set_height)
{
  struct window *w = XWINDOW (window);
  struct frame *f = XFRAME (w->frame);
  struct window *c;
  int old_pixsize = (set_height ? WINDOW_HEIGHT (w) : WINDOW_WIDTH (w));
  Lisp_Object child, minor_kid, major_kid;
  int minsize;
  int line_size;
  int defheight, defwidth;

  /* #### This is very likely incorrect and instead the char_to_pixel_
     functions should be called. */
  default_face_height_and_width (window, &defheight, &defwidth);
  line_size = (set_height ? defheight : defwidth);

  check_min_window_sizes ();

  minsize = (set_height ? window_min_height : window_min_width);
  minsize *= line_size;

  if (!nodelete
      && !TOP_LEVEL_WINDOW_P (w)
      && new_pixsize < minsize)
    {
      Fdelete_window (window, Qnil);
      return;
    }

  SET_LAST_MODIFIED (w, 0);
  SET_LAST_FACECHANGE (w);
  MARK_FRAME_WINDOWS_STRUCTURE_CHANGED (f);	/* multiple windows affected */
  if (set_height)
    {
      WINDOW_HEIGHT (w) = new_pixsize;
      major_kid = w->vchild;
      minor_kid = w->hchild;
    }
  else
    {
      WINDOW_WIDTH (w) = new_pixsize;
      major_kid = w->hchild;
      minor_kid = w->vchild;
    }

  if (!NILP (minor_kid))
    {
      for (child = minor_kid; !NILP (child); child = XWINDOW (child)->next)
	{
	  if (set_height)
	    WINDOW_TOP (XWINDOW (child)) = WINDOW_TOP (w);
	  else
	    WINDOW_LEFT (XWINDOW (child)) = WINDOW_LEFT (w);

	  set_window_pixsize (child, new_pixsize, nodelete, set_height);
	}
    }
  else if (!NILP (major_kid))
    {
      int last_pos, last_old_pos, pos, old_pos, first;
      int pixel_adj_left = new_pixsize - old_pixsize;
      int div_val = old_pixsize << 1;

      /*
       * Previously we bailed out here if there was no size change.
       * (pixel_adj_left == 0) But this broke toolbar updates.  If a
       * toolbar appears or disappears, windows may not change size,
       * but their top and left coordinates need to be updated.
       *
       * So we don't bail until after the loop below.
       */

      last_pos = first = (set_height ? WINDOW_TOP (w) : WINDOW_LEFT (w));
      last_old_pos = 0;

      for (child = major_kid; !NILP (child); child = c->next)
	{
	  c = XWINDOW (child);

	  if (set_height)
	    {
	      old_pos = last_old_pos + WINDOW_HEIGHT (c);
	      WINDOW_TOP (c) = last_pos;
	    }
	  else
	    {
	      old_pos = last_old_pos + WINDOW_WIDTH (c);
	      WINDOW_LEFT (c) = last_pos;
	    }

	  pos = (((old_pos * new_pixsize) << 1) + old_pixsize) / div_val;
	  /* All but the last window should have a height which is
             a multiple of the default line height. */
	  if (!NILP (c->next))
	    pos = (pos / line_size) * line_size;

	  /* Avoid confusion: don't delete child if it becomes too small */
	  set_window_pixsize (child, pos + first - last_pos, 1, set_height);

	  last_pos = pos + first;
	  last_old_pos = old_pos;
	}

      /* Sometimes we may get called with our old size.  In that case
         we don't need to do anything else. */
      if (!pixel_adj_left)
	return;

      /* Now delete any children that became too small.  */
      if (!nodelete)
	for (child = major_kid; !NILP (child); child = XWINDOW (child)->next)
	  {
	    if (set_height)
	      set_window_pixheight (child, WINDOW_HEIGHT (XWINDOW (child)), 0);
	    else
	      set_window_pixwidth (child, WINDOW_WIDTH (XWINDOW (child)), 0);
	  }
    }
}

/* Set the height of WINDOW and all its inferiors.  */
void
set_window_pixheight (Lisp_Object window, int new_pixheight, int nodelete)
{
  set_window_pixsize (window, new_pixheight, nodelete, 1);
}

/* Recursively set width of WINDOW and its inferiors. */
void
set_window_pixwidth (Lisp_Object window, int new_pixwidth, int nodelete)
{
  set_window_pixsize (window, new_pixwidth, nodelete, 0);
}


static int window_select_count;

DEFUN ("set-window-buffer", Fset_window_buffer, 2, 2, 0, /*
Make WINDOW display BUFFER as its contents.
BUFFER can be a buffer or buffer name.
*/
       (window, buffer))
{
  Lisp_Object tem;
  struct window *w = decode_window (window);

  buffer = Fget_buffer (buffer);
  CHECK_BUFFER (buffer);

  if (!BUFFER_LIVE_P (XBUFFER (buffer)))
    error ("Attempt to display deleted buffer");

  tem = w->buffer;
  if (NILP (tem))
    error ("Window is deleted");

  /* While this seems like a logical thing to do, it causes problems
     because of saved window configurations.  It is possible for a
     buffer to get restored into a window in which it is already being
     displayed, but start and point are actually at completely
     different locations.  So we let this function complete fully and
     it will then make sure redisplay correctly updates things.

     #### This is a kludge.  The correct approach is not to do this
     but to fix set-window-configuration. */
#if 0
  else if (EQ (tem, buffer))
    return Qnil;
#endif
  else if (! EQ (tem, Qt))	/* w->buffer is t when the window
				   is first being set up.  */
    {
      if (!NILP (w->dedicated) && !EQ (tem, buffer))
	error ("Window is dedicated to buffer %s",
	       XSTRING_DATA (XBUFFER (tem)->name));

      unshow_buffer (w);
    }

  w->buffer = buffer;
  w->window_end_pos[CURRENT_DISP] = 0;
  w->hscroll = 0;
  w->modeline_hscroll = 0;
  Fset_marker (w->pointm[CURRENT_DISP],
	       make_int (BUF_PT (XBUFFER (buffer))),
	       buffer);
  set_marker_restricted (w->start[CURRENT_DISP],
			 make_int (XBUFFER (buffer)->last_window_start),
			 buffer);
  Fset_marker (w->sb_point, w->start[CURRENT_DISP], buffer);
  /* set start_at_line_beg correctly. GE */
  w->start_at_line_beg = beginning_of_line_p (XBUFFER (buffer),
					      marker_position (w->start[CURRENT_DISP]));
  w->force_start = 0;           /* Lucid fix */
  SET_LAST_MODIFIED (w, 1);
  SET_LAST_FACECHANGE (w);
  MARK_WINDOWS_CHANGED (w);
  recompute_all_cached_specifiers_in_window (w);
  if (EQ (window, Fselected_window (Qnil)))
    {
      Fset_buffer (buffer);
    }
  return Qnil;
}

DEFUN ("select-window", Fselect_window, 1, 2, 0, /*
Select WINDOW.  Most editing will apply to WINDOW's buffer.
The main editor command loop selects the buffer of the selected window
before each command.

With non-nil optional argument `norecord', do not modify the
global or per-frame buffer ordering.
*/
       (window, norecord))
{
  struct window *w;
  Lisp_Object old_selected_window = Fselected_window (Qnil);

  CHECK_LIVE_WINDOW (window);
  w = XWINDOW (window);

  /* we have already caught dead-window errors */
  if (!NILP (w->hchild) || !NILP (w->vchild))
    error ("Trying to select non-leaf window");

  w->use_time = make_int (++window_select_count);
  if (EQ (window, old_selected_window))
    return window;

  /* deselect the old window, if it exists (it might not exist if
     the selected device has no frames, which occurs at startup) */
  if (!NILP (old_selected_window))
    {
      struct window *ow = XWINDOW (old_selected_window);

      Fset_marker (ow->pointm[CURRENT_DISP],
		   make_int (BUF_PT (XBUFFER (ow->buffer))),
		   ow->buffer);

      MARK_WINDOWS_CHANGED (ow);
    }

  /* now select the window's frame */
  set_frame_selected_window (XFRAME (WINDOW_FRAME (w)), window);

  select_frame_1 (WINDOW_FRAME (w));

  /* also select the window's buffer */
  if (NILP (norecord))
    Frecord_buffer (w->buffer);
  Fset_buffer (w->buffer);

  /* Go to the point recorded in the window.
     This is important when the buffer is in more
     than one window.  It also matters when
     redisplay_window has altered point after scrolling,
     because it makes the change only in the window.  */
  {
    Bufpos new_point = marker_position (w->pointm[CURRENT_DISP]);
    if (new_point < BUF_BEGV (current_buffer))
      new_point = BUF_BEGV (current_buffer);
    else if (new_point > BUF_ZV (current_buffer))
      new_point = BUF_ZV (current_buffer);

    BUF_SET_PT (current_buffer, new_point);
  }

  MARK_WINDOWS_CHANGED (w);

  return window;
}

Lisp_Object
display_buffer (Lisp_Object buffer, Lisp_Object not_this_window_p,
		Lisp_Object override_frame)
{
  return call3 (Qdisplay_buffer, buffer, not_this_window_p, override_frame);
}

void
temp_output_buffer_show (Lisp_Object buf, Lisp_Object same_frame)
{
  /* This function can GC */
  Lisp_Object window;
  struct window *w;
  struct buffer *b = XBUFFER (buf);

  BUF_SAVE_MODIFF (XBUFFER (buf)) = BUF_MODIFF (b);
  widen_buffer (b, 0);
  BUF_SET_PT (b, BUF_BEG (b));

  if (!NILP (Vtemp_buffer_show_function))
    call1 (Vtemp_buffer_show_function, buf);
  else
    {
      window = display_buffer (buf, Qnil, same_frame);

      if (!EQ (XWINDOW (window)->frame, Fselected_frame (Qnil)))
	Fmake_frame_visible (WINDOW_FRAME (XWINDOW (window)));

      Vminibuffer_scroll_window = window;
      w = XWINDOW (window);
      w->hscroll = 0;
      w->modeline_hscroll = 0;
      set_marker_restricted (w->start[CURRENT_DISP], make_int (1), buf);
      set_marker_restricted (w->pointm[CURRENT_DISP], make_int (1), buf);
      set_marker_restricted (w->sb_point, make_int (1), buf);

      /* Run temp-buffer-show-hook, with the chosen window selected.  */
      if (!preparing_for_armageddon)
	{
	  Lisp_Object tem;
	  tem = Fboundp (Qtemp_buffer_show_hook);
	  if (!NILP (tem))
	    {
	      tem = Fsymbol_value (Qtemp_buffer_show_hook);
	      if (!NILP (tem))
		{
		  int count = specpdl_depth ();

		  /* Select the window that was chosen, for running
                     the hook.  */
		  record_unwind_protect (save_window_excursion_unwind,
					 Fcurrent_window_configuration (Qnil));

		  Fselect_window (window, Qnil);
		  run_hook (Qtemp_buffer_show_hook);
		  unbind_to (count, Qnil);
		}
	    }
	}
    }
}

static void
make_dummy_parent (Lisp_Object window)
{
  Lisp_Object new;
  struct window *o = XWINDOW (window);
  struct window *p = alloc_lcrecord_type (struct window, &lrecord_window);

  XSETWINDOW (new, p);
  copy_lcrecord (p, o);

  /* Don't copy the pointers to the line start cache or the face
     instances. */
  p->line_start_cache = Dynarr_new (line_start_cache);
  p->face_cachels     = Dynarr_new (face_cachel);
  p->glyph_cachels    = Dynarr_new (glyph_cachel);

  /* Put new into window structure in place of window */
  replace_window (window, new);

  o->next = Qnil;
  o->prev = Qnil;
  o->vchild = Qnil;
  o->hchild = Qnil;
  o->parent = new;

  p->start[CURRENT_DISP] = Qnil;
  p->start[DESIRED_DISP] = Qnil;
  p->start[CMOTION_DISP] = Qnil;
  p->pointm[CURRENT_DISP] = Qnil;
  p->pointm[DESIRED_DISP] = Qnil;
  p->pointm[CMOTION_DISP] = Qnil;
  p->sb_point = Qnil;
  p->buffer = Qnil;
}

DEFUN ("split-window", Fsplit_window, 0, 3, "", /*
Split WINDOW, putting SIZE lines in the first of the pair.
WINDOW defaults to selected one and SIZE to half its size.
If optional third arg HOR-FLAG is non-nil, split side by side
and put SIZE columns in the first of the pair.
*/
       (window, chsize, horflag))
{
  Lisp_Object new;
  struct window *o, *p;
  struct frame *f;
  int size;
  int psize;

  if (NILP (window))
    window = Fselected_window (Qnil);
  else
    CHECK_LIVE_WINDOW (window);

  o = XWINDOW (window);
  f = XFRAME (WINDOW_FRAME (o));

  if (NILP (chsize))
    {
      if (!NILP (horflag))
	/* In the new scheme, we are symmetric with respect to separators
	   so there is no need to do weird things here. */
	{
	  psize = WINDOW_WIDTH (o) >> 1;
	  size = window_pixel_width_to_char_width (o, psize, 0);
        }
      else
        {
	  psize = WINDOW_HEIGHT (o) >> 1;
	  size = window_pixel_height_to_char_height (o, psize, 1);
        }
    }
  else
    {
      CHECK_INT (chsize);
      size = XINT (chsize);
      if (!NILP (horflag))
	psize = window_char_width_to_pixel_width (o, size, 0);
      else
	psize = window_char_height_to_pixel_height (o, size, 1);
    }

  if (MINI_WINDOW_P (o))
    error ("Attempt to split minibuffer window");
  else if (FRAME_NO_SPLIT_P (XFRAME (WINDOW_FRAME (o))))
    error ("Attempt to split unsplittable frame");

  check_min_window_sizes ();

  if (NILP (horflag))
    {
      if (size < window_min_height)
	error ("Window height %d too small (after splitting)", size);
      if (size + window_min_height > window_char_height (o, 1))
	error ("Window height %d too small (after splitting)",
	       window_char_height (o, 1) - size);
      if (NILP (o->parent)
	  || NILP (XWINDOW (o->parent)->vchild))
	{
	  make_dummy_parent (window);
	  reset_face_cachels (XWINDOW (window));
	  new = o->parent;
	  XWINDOW (new)->vchild = window;
	  XFRAME (o->frame)->mirror_dirty = 1;
	}
    }
  else
    {
      if (size < window_min_width)
	error ("Window width %d too small (after splitting)", size);
      if (size + window_min_width > window_char_width (o, 0))
	error ("Window width %d too small (after splitting)",
	       window_char_width (o, 0) - size);
      if (NILP (o->parent)
	  || NILP (XWINDOW (o->parent)->hchild))
	{
	  make_dummy_parent (window);
	  reset_face_cachels (XWINDOW (window));
	  new = o->parent;
	  XWINDOW (new)->hchild = window;
	  XFRAME (o->frame)->mirror_dirty = 1;
	}
    }

  /* Now we know that window's parent is a vertical combination
     if we are dividing vertically, or a horizontal combination
     if we are making side-by-side windows */

  MARK_FRAME_WINDOWS_STRUCTURE_CHANGED (f);
  new = allocate_window ();
  p = XWINDOW (new);

  p->frame = o->frame;
  p->next = o->next;
  if (!NILP (p->next))
    XWINDOW (p->next)->prev = new;
  p->prev = window;
  o->next = new;
  p->parent = o->parent;
  p->buffer = Qt;

  reset_face_cachels (p);
  reset_glyph_cachels (p);


  /* Apportion the available frame space among the two new windows */

  if (!NILP (horflag))
    {
      WINDOW_HEIGHT (p) = WINDOW_HEIGHT (o);
      WINDOW_TOP (p) = WINDOW_TOP (o);
      WINDOW_WIDTH (p) = WINDOW_WIDTH (o) - psize;
      WINDOW_WIDTH (o) = psize;
      WINDOW_LEFT (p) = WINDOW_LEFT (o) + psize;
    }
  else
    {
      WINDOW_LEFT (p) = WINDOW_LEFT (o);
      WINDOW_WIDTH (p) = WINDOW_WIDTH (o);
      WINDOW_HEIGHT (p) = WINDOW_HEIGHT (o) - psize;
      WINDOW_HEIGHT (o) = psize;
      WINDOW_TOP (p) = WINDOW_TOP (o) + psize;
    }

  XFRAME (p->frame)->mirror_dirty = 1;
  /* do this last (after the window is completely initialized and
     the mirror-dirty flag is set) so that specifier recomputation
     caused as a result of this will work properly and not abort. */
  Fset_window_buffer (new, o->buffer);
  return new;
}


DEFUN ("enlarge-window", Fenlarge_window, 1, 3, "_p", /*
Make the selected window N lines bigger.
From program, optional second arg SIDE non-nil means grow sideways N columns,
and optional third arg WINDOW specifies the window to change instead of the
selected window.
*/
       (n, side, window))
{
  struct window *w = decode_window (window);
  CHECK_INT (n);
  change_window_height (w, XINT (n), !NILP (side), /* inpixels */ 0);
  return Qnil;
}

DEFUN ("enlarge-window-pixels", Fenlarge_window_pixels, 1, 3, "_p", /*
Make the selected window N pixels bigger.
From program, optional second arg SIDE non-nil means grow sideways N pixels,
and optional third arg WINDOW specifies the window to change instead of the
selected window.
*/
       (n, side, window))
{
  struct window *w = decode_window (window);
  CHECK_INT (n);
  change_window_height (w, XINT (n), !NILP (side), /* inpixels */ 1);
  return Qnil;
}

DEFUN ("shrink-window", Fshrink_window, 1, 3, "_p", /*
Make the selected window N lines smaller.
From program, optional second arg SIDE non-nil means shrink sideways N columns,
and optional third arg WINDOW specifies the window to change instead of the
selected window.
*/
       (n, side, window))
{
  CHECK_INT (n);
  change_window_height (decode_window (window), -XINT (n), !NILP (side),
                        /* inpixels */ 0);
  return Qnil;
}

DEFUN ("shrink-window-pixels", Fshrink_window_pixels, 1, 3, "_p", /*
Make the selected window N pixels smaller.
From program, optional second arg SIDE non-nil means shrink sideways N pixels,
and optional third arg WINDOW specifies the window to change instead of the
selected window.
*/
       (n, side, window))
{
  CHECK_INT (n);
  change_window_height (decode_window (window), -XINT (n), !NILP (side),
                        /* inpixels */ 1);
  return Qnil;
}

static int
window_pixel_height (Lisp_Object window)
{
  return WINDOW_HEIGHT (XWINDOW (window));
}

static int
window_pixel_height_to_char_height (struct window *w, int pixel_height,
				    int include_gutters_p)
{
  int avail_height;
  int defheight, defwidth;
  int char_height;
  Lisp_Object window;

  XSETWINDOW (window, w);

  avail_height = (pixel_height -
		  (include_gutters_p ? 0 :
		   window_top_gutter_height (w) +
		   window_bottom_gutter_height (w)));

  default_face_height_and_width (window, &defheight, &defwidth);

  char_height = avail_height / defheight;

  /* It's the calling function's responsibility to check these values
     and make sure they're not out of range.

     #### We need to go through the calling functions and actually
     do this. */
  return max (0, char_height);
}

static int
window_char_height_to_pixel_height (struct window *w, int char_height,
				    int include_gutters_p)
{
  int avail_height;
  int defheight, defwidth;
  int pixel_height;

  Lisp_Object window;

  XSETWINDOW (window, w);

  default_face_height_and_width (window, &defheight, &defwidth);

  avail_height = char_height * defheight;
  pixel_height = (avail_height +
		  (include_gutters_p ? 0 :
		   window_top_gutter_height (w) +
		   window_bottom_gutter_height (w)));

  /* It's the calling function's responsibility to check these values
     and make sure they're not out of range.

     #### We need to go through the calling functions and actually
     do this. */
  return max (0, pixel_height);
}

/* Return number of default lines of text can fit in the window W.
   If INCLUDE_GUTTERS_P is 1, include "gutter" space (modeline plus
   horizontal scrollbar) in the space that is used for the calculation.
   */
int
window_char_height (struct window *w, int include_gutters_p)
{
  return window_pixel_height_to_char_height (w, WINDOW_HEIGHT (w),
					     include_gutters_p);
}

/*
 * Return number of lines currently displayed in window w.  If
 * end-of-buffer is displayed then the area below end-of-buffer is assume
 * to be blank lines of default height.
 * Does not include the modeline.
 */
int
window_displayed_height (struct window *w)
{
  struct buffer *b = XBUFFER (w->buffer);
  display_line_dynarr *dla = window_display_lines (w, CURRENT_DISP);
  int num_lines;
  Charcount end_pos = (BUF_Z (b) - w->window_end_pos[CURRENT_DISP] > BUF_ZV (b)
		       ? -1
		       : w->window_end_pos[CURRENT_DISP]);

  if (!Dynarr_length (dla))
    return window_char_height (w, 0);

  num_lines = Dynarr_length (dla);

  /* #### Document and assert somewhere that w->window_end_pos == -1
     indicates that end-of-buffer is being displayed. */
  if (end_pos == -1)
    {
      struct display_line *dl = Dynarr_atp (dla, 0);
      int ypos1 = dl->ypos + dl->descent;
      int ypos2 = WINDOW_TEXT_BOTTOM (w);
      Lisp_Object window;
      int defheight, defwidth;

      XSETWINDOW (window, w);

      if (dl->modeline)
	{
	  num_lines--;

	  if (Dynarr_length (dla) == 1)
	    ypos1 = WINDOW_TEXT_TOP (w);
	  else
	    {
	      dl = Dynarr_atp (dla, Dynarr_length (dla) - 1);
	      /* If this line is clipped then we know that there is no
                 blank room between eob and the modeline.  If we are
                 scrolling on clipped lines just know off the clipped
                 line and return .*/
	      if (scroll_on_clipped_lines && dl->clip)
		return num_lines - 1;
	      ypos1 = dl->ypos + dl->descent - dl->clip;
	    }
	}

      default_face_height_and_width (window, &defheight, &defwidth);
      /* #### This probably needs to know about the clipping area once a
         final definition is decided on. */
      num_lines += ((ypos2 - ypos1) / defheight);
    }
  else
    {
      if (num_lines > 1 && Dynarr_atp (dla, 0)->modeline)
	num_lines--;

      if (scroll_on_clipped_lines
	  && Dynarr_atp (dla, Dynarr_length (dla) - 1)->clip)
	num_lines--;
    }

  return num_lines;
}

static int
window_pixel_width (Lisp_Object window)
{
  return WINDOW_WIDTH (XWINDOW (window));
}

static int
window_pixel_width_to_char_width (struct window *w, int pixel_width,
				  int include_margins_p)
{
  int avail_width;
  int char_width;
  int defheight, defwidth;
  Lisp_Object window;

  XSETWINDOW (window, w);

  avail_width = (pixel_width -
		 window_left_gutter_width (w, 0) -
		 window_right_gutter_width (w, 0) -
		 (include_margins_p ? 0 : window_left_margin_width (w)) -
		 (include_margins_p ? 0 : window_right_margin_width (w)));

  default_face_height_and_width (window, &defheight, &defwidth);

  char_width = (avail_width / defwidth);

  /* It's the calling function's responsibility to check these values
     and make sure they're not out of range.

     #### We need to go through the calling functions and actually
     do this. */
  return max (0, char_width);
}

static int
window_char_width_to_pixel_width (struct window *w, int char_width,
				  int include_margins_p)
{
  int avail_width;
  int pixel_width;
  int defheight, defwidth;
  Lisp_Object window;

  XSETWINDOW (window, w);

  default_face_height_and_width (window, &defheight, &defwidth);

  avail_width = char_width * defwidth;
  pixel_width = (avail_width +
		 window_left_gutter_width (w, 0) +
		 window_right_gutter_width (w, 0) +
		 (include_margins_p ? 0 : window_left_margin_width (w)) +
		 (include_margins_p ? 0 : window_right_margin_width (w)));

  /* It's the calling function's responsibility to check these values
     and make sure they're not out of range.

     #### We need to go through the calling functions and actually
     do this. */
  return max (0, pixel_width);
}

/* This returns the usable space which doesn't include space needed by
   scrollbars or divider lines. */
int
window_char_width (struct window *w, int include_margins_p)
{
  return window_pixel_width_to_char_width (w, WINDOW_WIDTH (w),
					   include_margins_p);
}

#define MINSIZE(w)						\
  (widthflag							\
   ? window_min_width * defwidth				\
   : (defheight * (MINI_WINDOW_P (XWINDOW (w)) ? 1 : window_min_height)))

#define CURBEG(w) \
  *(widthflag ? (int *) &WINDOW_LEFT (w) : (int *) &WINDOW_TOP (w))

#define CURSIZE(w) \
  *(widthflag ? (int *) &WINDOW_WIDTH (w) : (int *) &WINDOW_HEIGHT (w))

#define CURCHARSIZE(w) \
  (widthflag ? window_char_width (w, 0) : window_char_height (w, 1))

#define MINCHARSIZE(window) \
  (widthflag ? window_min_width : MINI_WINDOW_P (XWINDOW (window)) \
   ? 1 : window_min_height)

/* Unlike set_window_pixheight, this function
   also changes the heights of the siblings so as to
   keep everything consistent. */

static void
change_window_height (struct window *win, int delta, int widthflag,
                      int inpixels)
{
  Lisp_Object parent;
  Lisp_Object window;
  struct window *w;
  struct frame *f;
  int *sizep;
  int (*sizefun) (Lisp_Object) = (widthflag
				  ? window_pixel_width
				  : window_pixel_height);
  void (*setsizefun) (Lisp_Object, int, int) = (widthflag
						? set_window_pixwidth
						: set_window_pixheight);
  int dim;
  int defheight, defwidth;

  if (delta == 0)
    return;

  check_min_window_sizes ();

  XSETWINDOW (window, win);
  f = XFRAME (win->frame);
  if (EQ (window, FRAME_ROOT_WINDOW (f)))
    error ("Won't change only window");

  /* #### This is very likely incorrect and instead the char_to_pixel_
     functions should be called. */
  default_face_height_and_width (window, &defheight, &defwidth);

  while (1)
    {
      w = XWINDOW (window);
      parent = w->parent;
      if (NILP (parent))
	{
	  if (widthflag)
	    error ("No other window to side of this one");
	  break;
	}
      if (widthflag
          ? !NILP (XWINDOW (parent)->hchild)
	  : !NILP (XWINDOW (parent)->vchild))
	break;
      window = parent;
    }

  sizep = &CURSIZE (w);
  dim = CURCHARSIZE (w);

  if ((inpixels  && (*sizep + delta) < MINSIZE (window)) ||
      (!inpixels && (dim + delta) < MINCHARSIZE (window)))
    {
      if (MINI_WINDOW_P (XWINDOW (window)))
	return;
      else if (!NILP (parent))
	{
	  Fdelete_window (window, Qnil);
	  return;
	}
    }

  if (!inpixels)
    delta *= (widthflag ? defwidth : defheight);

  {
    int maxdelta;

    maxdelta = ((!NILP (parent))
                ? (*sizefun) (parent) - *sizep
		: ((!NILP (w->next))
                   ? (*sizefun) (w->next) - MINSIZE (w->next)
                   : ((!NILP (w->prev))
                      ? (*sizefun) (w->prev) - MINSIZE (w->prev)
                      /* This is a frame with only one window,
                         a minibuffer-only or a minibufferless frame.  */
                      : (delta = 0))));

    if (delta > maxdelta)
      /* This case traps trying to make the minibuffer
	 the full frame, or make the only window aside from the
	 minibuffer the full frame.  */
      delta = maxdelta;

    if (delta == 0)
      return;

#if 0 /* FSFmacs */
    /* #### Chuck: is this correct? */
    if (*sizep + delta < MINSIZE (window))
      {
	Fdelete_window (window);
	return;
      }
#endif
  }

  if (!NILP (w->next) &&
      (*sizefun) (w->next) - delta >= (int) MINSIZE (w->next))
    {
      CURBEG (XWINDOW (w->next)) += delta;
      (*setsizefun) (w->next, (*sizefun) (w->next) - delta, 0);
      (*setsizefun) (window, *sizep + delta, 0);
    }
  else if (!NILP (w->prev) &&
	   (*sizefun) (w->prev) - delta >= (int) MINSIZE (w->prev))
    {
      (*setsizefun) (w->prev, (*sizefun) (w->prev) - delta, 0);
      CURBEG (w) -= delta;
      (*setsizefun) (window, *sizep + delta, 0);
    }
  else
    {
      int delta1;
      int opht = (*sizefun) (parent);

      /* If trying to grow this window to or beyond size of the parent,
	 make delta1 so big that, on shrinking back down,
	 all the siblings end up with less than one line and are deleted.  */
      if (opht <= *sizep + delta)
	delta1 = opht * opht * 2;
      /* Otherwise, make delta1 just right so that if we add delta1
	 lines to this window and to the parent, and then shrink
	 the parent back to its original size, the new proportional
	 size of this window will increase by delta.  */
      else
	delta1 = (delta * opht * 100) / ((opht - *sizep - delta) * 100);

      /* Add delta1 lines or columns to this window, and to the parent,
	 keeping things consistent while not affecting siblings.  */
      CURSIZE (XWINDOW (parent)) = opht + delta1;
      (*setsizefun) (window, *sizep + delta1, 0);

      /* Squeeze out delta1 lines or columns from our parent,
	 shrinking this window and siblings proportionately.
	 This brings parent back to correct size.
	 Delta1 was calculated so this makes this window the desired size,
	 taking it all out of the siblings.  */
      (*setsizefun) (parent, opht, 0);
    }

  SET_LAST_MODIFIED (w, 0);
  SET_LAST_FACECHANGE (w);
  MARK_FRAME_WINDOWS_STRUCTURE_CHANGED (f);
}
#undef MINSIZE
#undef CURBEG
#undef CURSIZE
#undef CURCHARSIZE
#undef MINCHARSIZE



/* Scroll contents of window WINDOW up N lines.  */
void
window_scroll (Lisp_Object window, Lisp_Object n, int direction,
	       Error_behavior errb)
{
  struct window *w = XWINDOW (window);
  struct buffer *b = XBUFFER (w->buffer);
  int selected = EQ (window, Fselected_window (Qnil));
  int value = 0;
  Lisp_Object point, tem;

  if (selected)
    point = make_int (BUF_PT (b));
  else
    {
      Bufpos pos = marker_position (w->pointm[CURRENT_DISP]);

      if (pos < BUF_BEGV (b))
	pos = BUF_BEGV (b);
      else if (pos > BUF_ZV (b))
	pos = BUF_ZV (b);

      point = make_int (pos);
    }

  /* Always set force_start so that redisplay_window will run
     the window-scroll-functions.  */
  w->force_start = 1;

  /* #### When the fuck does this happen?  I'm so glad that history has
     completely documented the behavior of the scrolling functions under
     all circumstances. */
  tem = Fpos_visible_in_window_p (point, window);
  if (NILP (tem))
    {
      Fvertical_motion (make_int (-window_char_height (w, 0) / 2),
                        window, Qnil);
      Fset_marker (w->start[CURRENT_DISP], point, w->buffer);
      w->start_at_line_beg = beginning_of_line_p (b, XINT (point));
      MARK_WINDOWS_CHANGED (w);
    }

  if (!NILP (n))
    {
      if (EQ (n, Qminus))
	direction *= -1;
      else
	{
	  n = Fprefix_numeric_value (n);
	  value = XINT (n) * direction;

	  if (!value)
	    return;	/* someone just made a pointless call */
	}
    }

  /* If the user didn't specify how far to scroll then we have to figure it
     out by ourselves. */
  if (NILP (n) || EQ (n, Qminus))
    {
      /* Going forwards is easy.  If that is what we are doing then just
	 set value and the section which handles the user specifying a
	 positive value will work. */
      if (direction == 1)
	{
	  value = window_displayed_height (w) - next_screen_context_lines;
	  value = (value < 1 ? 1 : value);
	}

      /* Going backwards is hard.  We can't use the same loop used if the
         user specified a negative value because we care about
         next_screen_context_lines.  In a variable height world you don't
         know how many lines above you can actually be displayed and still
         have the context lines appear.  So we leave value set to 0 and add
         a separate section to deal with this. */

    }

  if (direction == 1 && !value)
    {
      return;
    }
  else if (value > 0)
    {
      int vtarget;
      Bufpos startp, old_start;

      old_start = marker_position (w->start[CURRENT_DISP]);
      startp = vmotion (w, old_start, value, &vtarget);

      if (vtarget < value &&
	  (w->window_end_pos[CURRENT_DISP] == -1
	   || (BUF_Z (b) - w->window_end_pos[CURRENT_DISP] > BUF_ZV (b))))
	{
	  maybe_signal_error (Qend_of_buffer, Qnil, Qwindow, errb);
	  return;
	}
      else
	{
	  set_marker_restricted (w->start[CURRENT_DISP], make_int (startp),
				 w->buffer);
	  w->force_start = 1;
	  w->start_at_line_beg = beginning_of_line_p (b, startp);
	  MARK_WINDOWS_CHANGED (w);

	  if (!point_would_be_visible (w, startp, XINT (point)))
	    {
	      if (selected)
		BUF_SET_PT (b, startp);
	      else
		set_marker_restricted (w->pointm[CURRENT_DISP],
				       make_int (startp),
				       w->buffer);
	    }
	}
    }
  else if (value < 0)
    {
      int vtarget;
      Bufpos startp, old_start;

      old_start = marker_position (w->start[CURRENT_DISP]);
      startp = vmotion (w, old_start, value, &vtarget);

      if (vtarget > value
	  && marker_position (w->start[CURRENT_DISP]) == BUF_BEGV (b))
	{
	  maybe_signal_error (Qbeginning_of_buffer, Qnil, Qwindow, errb);
	  return;
	}
      else
	{
	  set_marker_restricted (w->start[CURRENT_DISP], make_int (startp),
				 w->buffer);
	  w->force_start = 1;
	  w->start_at_line_beg = beginning_of_line_p (b, startp);
	  MARK_WINDOWS_CHANGED (w);

	  if (!point_would_be_visible (w, startp, XINT (point)))
	    {
	      Bufpos new_point;

	      if (MINI_WINDOW_P (w))
		new_point = startp;
	      else
		new_point = start_of_last_line (w, startp);

	      if (selected)
		BUF_SET_PT (b, new_point);
	      else
		set_marker_restricted (w->pointm[CURRENT_DISP],
				       make_int (new_point),
				       w->buffer);
	    }
	}
    }
  else	/* value == 0 && direction == -1 */
    {
      if (marker_position (w->start[CURRENT_DISP]) == BUF_BEGV (b))
	{
	  maybe_signal_error (Qbeginning_of_buffer, Qnil, Qwindow, errb);
	  return;
	}
      else
	{
	  int vtarget;
	  int movement = next_screen_context_lines - 1;
	  Bufpos old_startp = marker_position (w->start[CURRENT_DISP]);
	  Bufpos bottom = vmotion (w, old_startp, movement, &vtarget);
	  Bufpos startp =
	    start_with_point_on_display_line (w, bottom,
					      -1 - (movement - vtarget));

	  if (startp >= old_startp)
	    startp = vmotion (w, old_startp, -1, NULL);

	  set_marker_restricted (w->start[CURRENT_DISP], make_int (startp),
				 w->buffer);
	  w->force_start = 1;
	  w->start_at_line_beg = beginning_of_line_p (b, startp);
	  MARK_WINDOWS_CHANGED (w);

	  if (!point_would_be_visible (w, startp, XINT (point)))
	    {
	      Bufpos new_point = start_of_last_line (w, startp);

	      if (selected)
		BUF_SET_PT (b, new_point);
	      else
		set_marker_restricted (w->pointm[CURRENT_DISP],
				       make_int (new_point),
				       w->buffer);
	    }
	}
    }

}

DEFUN ("scroll-up", Fscroll_up, 0, 1, "_P", /*
Scroll text of current window upward N lines; or near full screen if no arg.
A near full screen is `next-screen-context-lines' less than a full screen.
Negative N means scroll downward.
When calling from a program, supply an integer as argument or nil.
On attempt to scroll past end of buffer, `end-of-buffer' is signaled.
On attempt to scroll past beginning of buffer, `beginning-of-buffer' is
signaled.
*/
       (n))
{
  window_scroll (Fselected_window (Qnil), n, 1, ERROR_ME);
  return Qnil;
}

DEFUN ("scroll-down", Fscroll_down, 0, 1, "_P", /*
Scroll text of current window downward N lines; or near full screen if no arg.
A near full screen is `next-screen-context-lines' less than a full screen.
Negative N means scroll upward.
When calling from a program, supply a number as argument or nil.
On attempt to scroll past end of buffer, `end-of-buffer' is signaled.
On attempt to scroll past beginning of buffer, `beginning-of-buffer' is
signaled.
*/
       (n))
{
  window_scroll (Fselected_window (Qnil), n, -1, ERROR_ME);
  return Qnil;
}

DEFUN ("other-window-for-scrolling", Fother_window_for_scrolling, 0, 0, 0, /*
Return the other window for "other window scroll" commands.
If in the minibuffer, `minibuffer-scroll-window' if non-nil
specifies the window.
If `other-window-scroll-buffer' is non-nil, a window
showing that buffer is used.
*/
       ())
{
  Lisp_Object window;
  Lisp_Object selected_window = Fselected_window (Qnil);

  if (MINI_WINDOW_P (XWINDOW (selected_window))
      && !NILP (Vminibuffer_scroll_window))
    window = Vminibuffer_scroll_window;
  /* If buffer is specified, scroll that buffer.  */
  else if (!NILP (Vother_window_scroll_buffer))
    {
      window = Fget_buffer_window (Vother_window_scroll_buffer, Qnil, Qnil);
      if (NILP (window))
	window = display_buffer (Vother_window_scroll_buffer, Qt, Qnil);
    }
  else
    {
      /* Nothing specified; look for a neighboring window on the same
	 frame.  */
      window = Fnext_window (selected_window, Qnil, Qnil, Qnil);

      if (EQ (window, selected_window))
	/* That didn't get us anywhere; look for a window on another
           visible frame.  */
	do
	  window = Fnext_window (window, Qnil, Qt, Qnil);
	while (! FRAME_VISIBLE_P (XFRAME (WINDOW_FRAME (XWINDOW (window))))
	       && ! EQ (window, selected_window));
    }

  CHECK_LIVE_WINDOW (window);

  if (EQ (window, selected_window))
    error ("There is no other window");

  return window;
 }

DEFUN ("scroll-other-window", Fscroll_other_window, 0, 1, "_P", /*
Scroll next window upward N lines; or near full frame if no arg.
The next window is the one below the current one; or the one at the top
if the current one is at the bottom.  Negative N means scroll downward.
When calling from a program, supply a number as argument or nil.

If in the minibuffer, `minibuffer-scroll-window' if non-nil
specifies the window to scroll.
If `other-window-scroll-buffer' is non-nil, scroll the window
showing that buffer, popping the buffer up if necessary.
*/
       (n))
{
  window_scroll (Fother_window_for_scrolling (), n, 1, ERROR_ME);
  return Qnil;
}

DEFUN ("scroll-left", Fscroll_left, 0, 1, "_P", /*
Scroll selected window display N columns left.
Default for N is window width minus 2.
*/
       (n))
{
  Lisp_Object window = Fselected_window (Qnil);
  struct window *w = XWINDOW (window);
  int count = (NILP (n) ?
	       window_char_width (w, 0) - 2 :
	       XINT (Fprefix_numeric_value (n)));

  return Fset_window_hscroll (window, make_int (w->hscroll + count));
}

DEFUN ("scroll-right", Fscroll_right, 0, 1, "_P", /*
Scroll selected window display N columns right.
Default for N is window width minus 2.
*/
       (n))
{
  Lisp_Object window = Fselected_window (Qnil);
  struct window *w = XWINDOW (window);
  int count = (NILP (n) ?
	       window_char_width (w, 0) - 2 :
	       XINT (Fprefix_numeric_value (n)));

  return Fset_window_hscroll (window, make_int (w->hscroll - count));
}

DEFUN ("center-to-window-line", Fcenter_to_window_line, 0, 2, "_P", /*
Center point in WINDOW.  With N, put point on line N.
The desired position of point is always relative to the window.
If WINDOW is nil, the selected window is used.
*/
       (n, window))
{
  struct window *w = decode_window (window);
  struct buffer *b = XBUFFER (w->buffer);
  Bufpos opoint = BUF_PT (b);
  Bufpos startp;

  if (NILP (n))
    startp = start_with_line_at_pixpos (w, opoint, window_half_pixpos (w));
  else
    {
      n = Fprefix_numeric_value (n);
      CHECK_INT (n);
      startp = start_with_point_on_display_line (w, opoint, XINT (n));
    }

  Fset_marker (w->start[CURRENT_DISP], make_int (startp), w->buffer);

  w->start_at_line_beg = beginning_of_line_p (b, startp);
  w->force_start = 1;
  MARK_WINDOWS_CHANGED (w);
  return Qnil;
}

DEFUN ("move-to-window-line", Fmove_to_window_line, 1, 2, "_P", /*
Position point relative to WINDOW.
With no argument, position text at center of window.
An argument specifies window line; zero means top of window,
negative means relative to bottom of window.
If WINDOW is nil, the selected window is used.
*/
       (arg, window))
{
  struct window *w;
  struct buffer *b;
  int height;
  Bufpos start, new_point;
  int selected;

  /* Don't use decode_window() because we need the new value of
     WINDOW.  */
  if (NILP (window))
    window = Fselected_window (Qnil);
  else
    CHECK_LIVE_WINDOW (window);
  w = XWINDOW (window);
  b = XBUFFER (w->buffer);

  height = window_displayed_height (w);
  selected = EQ (window, Fselected_window (w->frame));

  if (NILP (arg))
    {
      int retval;

      if (XINT (w->last_modified[CURRENT_DISP]) >= BUF_MODIFF (b)
	  && XINT (w->last_facechange[CURRENT_DISP]) >= BUF_FACECHANGE (b))
	{
	  new_point = point_at_center (w, CURRENT_DISP, 0, 0);

	  if (selected)
	    BUF_SET_PT (b, new_point);
	  else
	    Fset_window_point (window, make_int (new_point));

	  retval = line_at_center (w, CURRENT_DISP, 0, 0);
	}
      else
	{
	  start = marker_position (w->start[CURRENT_DISP]);
	  if (start < BUF_BEGV (b))
	    start = BUF_BEGV (b);
	  else if (start > BUF_ZV (b))
	    start = BUF_ZV (b);

	  if (selected)
	    new_point = BUF_PT (b);
	  else
	    new_point = marker_position (w->pointm[CURRENT_DISP]);

	  new_point = point_at_center (w, CMOTION_DISP, start, BUF_PT (b));

	  if (selected)
	    BUF_SET_PT (b, new_point);
	  else
	    Fset_window_point (window, make_int (new_point));

	  retval = line_at_center (w, CMOTION_DISP, start, BUF_PT (b));
	}

      return make_int (retval);
    }
  else
    {
      /* #### Is this going to work right when at eob? */
      arg = Fprefix_numeric_value (arg);
      if (XINT (arg) < 0)
	XSETINT (arg, XINT (arg) + height);
    }

  start = marker_position (w->start[CURRENT_DISP]);
  if (start < BUF_BEGV (b) || start > BUF_ZV (b))
    {
      if (selected)
	new_point = BUF_PT (b);
      else
	new_point = marker_position (w->pointm[CURRENT_DISP]);

      new_point = vmotion (XWINDOW (window), new_point, -height / 2, 0);

      if (selected)
	BUF_SET_PT (b, new_point);
      else
	Fset_window_point (window, make_int (new_point));

      Fset_marker (w->start[CURRENT_DISP], make_int (new_point),
		   w->buffer);
      w->start_at_line_beg = beginning_of_line_p (b, new_point);
      w->force_start = 1;
    }
  else
    {
      if (selected)
	BUF_SET_PT (b, start);
      else
	Fset_window_point (window, make_int (start));
    }

  if (selected)
    return Fvertical_motion (arg, window, Qnil);
  else
    {
      int vpos;
      new_point = vmotion (XWINDOW (window),
			   marker_position (w->pointm[CURRENT_DISP]),
			   XINT (arg), &vpos);
      Fset_window_point (window, make_int (new_point));
      return make_int (vpos);
    }
}


static int
map_windows_1 (Lisp_Object window,
	       int (*mapfun) (struct window *w, void *closure),
	       void *closure)
{
  for (; !NILP (window); window = XWINDOW (window)->next)
    {
      int retval;
      struct window *w = XWINDOW (window);

      if (!NILP (w->vchild))
	retval = map_windows_1 (w->vchild, mapfun, closure);
      else if (!NILP (w->hchild))
	retval = map_windows_1 (w->hchild, mapfun, closure);
      else
	retval = (mapfun) (w, closure);

      if (retval)
	return retval;
    }

  return 0;
}

/* Map MAPFUN over the windows in F.  CLOSURE is passed to each
   invocation of MAPFUN.  If any invocation of MAPFUN returns
   non-zero, the mapping is halted.  Otherwise, map_windows() maps
   over all windows in F.

   If MAPFUN creates or deletes windows, the behavior is undefined.  */

int
map_windows (struct frame *f, int (*mapfun) (struct window *w, void *closure),
	     void *closure)
{
  if (f)
    return map_windows_1 (FRAME_ROOT_WINDOW (f), mapfun, closure);
  else
    {
      Lisp_Object frmcons, devcons, concons;

      FRAME_LOOP_NO_BREAK(frmcons, devcons, concons)
	{
	  int v = map_windows_1 (FRAME_ROOT_WINDOW (XFRAME (XCAR (frmcons))),
				 mapfun, closure);
	  if (v)
	    return v;
	}
    }

  return 0;
}


static void
modeline_shadow_thickness_changed (Lisp_Object specifier, struct window *w,
				   Lisp_Object oldval)
{
  w->shadow_thickness_changed = 1;
  MARK_WINDOWS_CHANGED (w);
}

static void
vertical_divider_changed_in_window (Lisp_Object specifier,
				    struct window *w,
				    Lisp_Object oldval)
{
  MARK_WINDOWS_CHANGED (w);
  MARK_FRAME_WINDOWS_STRUCTURE_CHANGED (XFRAME (WINDOW_FRAME (w)));
}

/* also used in scrollbar.c */
void
some_window_value_changed (Lisp_Object specifier, struct window *w,
			   Lisp_Object oldval)
{
  MARK_WINDOWS_CHANGED (w);
}

#ifdef MEMORY_USAGE_STATS

struct window_stats
{
  int face;
  int glyph;
#ifdef HAVE_SCROLLBARS
  int scrollbar;
#endif
  int line_start;
  int other_redisplay;
  int other;
};

static void
compute_window_mirror_usage (struct window_mirror *mir,
			     struct window_stats *stats,
			     struct overhead_stats *ovstats)
{
  if (!mir)
    return;
  stats->other += malloced_storage_size (mir, sizeof (struct window_mirror),
					 ovstats);
#ifdef HAVE_SCROLLBARS
  {
    struct device *d = XDEVICE (FRAME_DEVICE (mir->frame));

    stats->scrollbar +=
      compute_scrollbar_instance_usage (d, mir->scrollbar_vertical_instance,
					ovstats);
    stats->scrollbar +=
      compute_scrollbar_instance_usage (d, mir->scrollbar_horizontal_instance,
					ovstats);
  }
#endif /* HAVE_SCROLLBARS */
  stats->other_redisplay +=
    compute_display_line_dynarr_usage (mir->current_display_lines, ovstats);
  stats->other_redisplay +=
    compute_display_line_dynarr_usage (mir->desired_display_lines, ovstats);
}

static void
compute_window_usage (struct window *w, struct window_stats *stats,
		      struct overhead_stats *ovstats)
{
  xzero (*stats);
  stats->other += malloced_storage_size (w, sizeof (struct window), ovstats);
  stats->face += compute_face_cachel_usage (w->face_cachels, ovstats);
  stats->glyph += compute_glyph_cachel_usage (w->glyph_cachels, ovstats);
  stats->line_start +=
    compute_line_start_cache_dynarr_usage (w->line_start_cache, ovstats);
  compute_window_mirror_usage (find_window_mirror (w), stats, ovstats);
}

DEFUN ("window-memory-usage", Fwindow_memory_usage, 1, 1, 0, /*
Return stats about the memory usage of window WINDOW.
The values returned are in the form of an alist of usage types and byte
counts.  The byte counts attempt to encompass all the memory used
by the window (separate from the memory logically associated with a
buffer or frame), including internal structures and any malloc()
overhead associated with them.  In practice, the byte counts are
underestimated because certain memory usage is very hard to determine
\(e.g. the amount of memory used inside the Xt library or inside the
X server) and because there is other stuff that might logically
be associated with a window, buffer, or frame (e.g. window configurations,
glyphs) but should not obviously be included in the usage counts.

Multiple slices of the total memory usage may be returned, separated
by a nil.  Each slice represents a particular view of the memory, a
particular way of partitioning it into groups.  Within a slice, there
is no overlap between the groups of memory, and each slice collectively
represents all the memory concerned.
*/
       (window))
{
  struct window_stats stats;
  struct overhead_stats ovstats;
  Lisp_Object val = Qnil;

  CHECK_WINDOW (window); /* dead windows should be allowed, no? */
  xzero (ovstats);
  compute_window_usage (XWINDOW (window), &stats, &ovstats);

  val = acons (Qface_cache,          make_int (stats.face),              val);
  val = acons (Qglyph_cache,         make_int (stats.glyph),             val);
#ifdef HAVE_SCROLLBARS
  val = acons (Qscrollbar_instances, make_int (stats.scrollbar),         val);
#endif
  val = acons (Qline_start_cache,    make_int (stats.line_start),        val);
  val = acons (Qother_redisplay,     make_int (stats.other_redisplay),   val);
  val = acons (Qother,               make_int (stats.other),             val);
  val = Fcons (Qnil, val);
  val = acons (Qactually_requested,  make_int (ovstats.was_requested),   val);
  val = acons (Qmalloc_overhead,     make_int (ovstats.malloc_overhead), val);
  val = acons (Qdynarr_overhead,     make_int (ovstats.dynarr_overhead), val);

  return Fnreverse (val);
}

#endif /* MEMORY_USAGE_STATS */


/************************************************************************/
/*                         Window configurations                        */
/************************************************************************/

/* #### This window configuration stuff has had serious bugs lurking in it
   for years; it would be a -huge- win if this was reimplemented in lisp.
 */

/* If you add anything to this structure make sure saved_window_equal
   knows about it. */
struct saved_window
{
  Lisp_Object window;         /* window */
  Lisp_Object buffer;         /* buffer */
  Lisp_Object start;          /* copied marker */
  Lisp_Object pointm;         /* copied marker */
  Lisp_Object sb_point;	      /* copied marker */
  Lisp_Object mark;           /* copied marker */
  int pixel_left;
  int pixel_top;
  int pixel_width;
  int pixel_height;
  int hscroll;
  int modeline_hscroll;
  int parent_index;           /* index into saved_windows */
  int prev_index;             /* index into saved_windows */
  char start_at_line_beg; /* boolean */

#define WINDOW_SLOT_DECLARATION
#define WINDOW_SLOT(slot, compare) Lisp_Object slot
#include "winslots.h"
};

/* If you add anything to this structure make sure window_config_equal
   knows about it. */
struct window_config
{
  struct lcrecord_header header;
  /*  int frame_width; No longer needed, JV
      int frame_height; */
#if 0 /* FSFmacs */
  Lisp_Object selected_frame;
#endif
  Lisp_Object current_window;
  Lisp_Object current_buffer;
  Lisp_Object minibuffer_scroll_window;
  Lisp_Object root_window;
  int minibuf_height; /* 0 = no minibuffer, <0, size in lines, >0 in pixels */
  /* Record the values of window-min-width and window-min-height
     so that window sizes remain consistent with them.  */
  int min_width, min_height;
  int saved_windows_count;
  /* Zero-sized arrays aren't ANSI C */
  struct saved_window saved_windows[1];
};

#define SAVED_WINDOW_N(conf, n) (&((conf)->saved_windows[(n)]))
#define XWINDOW_CONFIGURATION(x) XRECORD (x, window_configuration, struct window_config)
#define XSETWINDOW_CONFIGURATION(x, p) XSETRECORD (x, p, window_configuration)
#define WINDOW_CONFIGURATIONP(x) RECORDP (x, window_configuration)
#define GC_WINDOW_CONFIGURATIONP(x) GC_RECORDP (x, window_configuration)
#define CHECK_WINDOW_CONFIGURATION(x) CHECK_RECORD (x, window_configuration)

static Lisp_Object
mark_window_config (Lisp_Object obj, void (*markobj) (Lisp_Object))
{
  struct window_config *config = XWINDOW_CONFIGURATION (obj);
  int i;
  markobj (config->current_window);
  markobj (config->current_buffer);
  markobj (config->minibuffer_scroll_window);
  markobj (config->root_window);

  for (i = 0; i < config->saved_windows_count; i++)
    {
      struct saved_window *s = SAVED_WINDOW_N (config, i);
      markobj (s->window);
      markobj (s->buffer);
      markobj (s->start);
      markobj (s->pointm);
      markobj (s->sb_point);
      markobj (s->mark);
#if 0
      /* #### This looked like this. I do not see why specifier cached
	 values should not be marked, as such specifiers as toolbars
	 might have GC-able instances. Freed configs are not marked,
	 aren't they?  -- kkm */
      markobj (s->dedicated);
#else
#define WINDOW_SLOT(slot, compare) ((void) (markobj (s->slot)))
#include "winslots.h"
#endif
    }
  return Qnil;
}

static size_t
sizeof_window_config_for_n_windows (int n)
{
  return (sizeof (struct window_config) +
	  /* n - 1 because zero-sized arrays aren't ANSI C */
	  (n - 1) *sizeof (struct saved_window));
}

static size_t
sizeof_window_config (CONST void *h)
{
  CONST struct window_config *c = (CONST struct window_config *) h;
  return sizeof_window_config_for_n_windows (c->saved_windows_count);
}

static void
print_window_config (Lisp_Object obj, Lisp_Object printcharfun, int escapeflag)
{
  struct window_config *config = XWINDOW_CONFIGURATION (obj);
  char buf[200];
  if (print_readably)
    error ("printing unreadable object #<window-configuration 0x%x>",
           config->header.uid);
  write_c_string ("#<window-configuration ", printcharfun);
  sprintf (buf, "0x%x>", config->header.uid);
  write_c_string (buf, printcharfun);
}

DEFINE_LRECORD_SEQUENCE_IMPLEMENTATION ("window-configuration",
					window_configuration,
					mark_window_config,
					print_window_config,
					0, 0, 0, 0, sizeof_window_config,
					struct window_config);


/* Returns a boolean indicating whether the two saved windows are
   identical. */
static int
saved_window_equal (struct saved_window *win1, struct saved_window *win2)
{
#define WINDOW_SLOT(slot, compare)		\
  if (!compare (win1->slot, win2->slot))	\
    return 0;
#include "winslots.h"

  return
    EQ (win1->window, win2->window) &&
    EQ (win1->buffer, win2->buffer) &&
    internal_equal (win1->start,    win2->start, 0) &&
    internal_equal (win1->pointm,   win2->pointm, 0) &&
    internal_equal (win1->sb_point, win2->sb_point, 0) &&
    internal_equal (win1->mark,     win2->mark, 0) &&
    win1->pixel_left   == win2->pixel_left &&
    win1->pixel_top    == win2->pixel_top &&
    win1->pixel_width  == win2->pixel_width &&
    win1->pixel_height == win2->pixel_height &&
    win1->hscroll      == win2->hscroll &&
    win1->modeline_hscroll == win2->modeline_hscroll &&
    win1->parent_index == win2->parent_index &&
    win1->prev_index   == win2->prev_index &&
    win1->start_at_line_beg == win2->start_at_line_beg;
}

/* Returns a boolean indicating whether the two given configurations
   are identical. */
static int
window_config_equal (Lisp_Object conf1, Lisp_Object conf2)
{
  struct window_config *fig1, *fig2;
  int i;

  /* First check if they are truly the same. */
  if (EQ (conf1, conf2))
    return 1;

  fig1 = XWINDOW_CONFIGURATION (conf1);
  fig2 = XWINDOW_CONFIGURATION (conf2);

  if (!((fig1->saved_windows_count == fig2->saved_windows_count) &&
	EQ (fig1->current_window,	    fig2->current_window) &&
	EQ (fig1->current_buffer,	    fig2->current_buffer) &&
	EQ (fig1->root_window,		    fig2->root_window) &&
	EQ (fig1->minibuffer_scroll_window, fig2->minibuffer_scroll_window)))
	/* &&
	fig1->frame_width  == fig2->frame_width &&
	fig1->frame_height == fig2->frame_height)) */
    return 0;

  for (i = 0; i < fig1->saved_windows_count; i++)
    {
      if (!saved_window_equal (SAVED_WINDOW_N (fig1, i),
			       SAVED_WINDOW_N (fig2, i)))
	return 0;
    }

  return 1;
}

DEFUN ("window-configuration-p", Fwindow_configuration_p, 1, 1, 0, /*
Return t if OBJECT is a window-configuration object.
*/
       (obj))
{
  return WINDOW_CONFIGURATIONP (obj) ? Qt : Qnil;
}

static int
mark_windows_in_use_closure (struct window *w, void *closure)
{
  int mark = *(int *)closure;
  w->config_mark = mark;
  return 0;
}

static void
mark_windows_in_use (struct frame *f, int mark)
{
  map_windows (f, mark_windows_in_use_closure, &mark);
}

/* Lisp_Object return value so it can be used in record_unwind_protect() */
static Lisp_Object
free_window_configuration (Lisp_Object window_config)
{
  int i;
  struct window_config *config = XWINDOW_CONFIGURATION (window_config);

  /* Free all the markers.  It's not completely necessary that
     we do this (window configs sitting in a free list aren't
     marked normally so the markers wouldn't be marked anyway)
     but it's more efficient. */
  for (i = 0; i < config->saved_windows_count; i++)
    {
      struct saved_window *p = SAVED_WINDOW_N (config, i);

      if (!NILP (p->pointm))
	{
	  free_marker (XMARKER (p->pointm));
	  p->pointm = Qnil;
	}
      if (!NILP (p->start))
	{
	  free_marker (XMARKER (p->start));
	  p->start = Qnil;
	}
      if (!NILP (p->sb_point))
	{
	  free_marker (XMARKER (p->sb_point));
	  p->sb_point = Qnil;
	}
      if (!NILP (p->mark))
	{
	  free_marker (XMARKER (p->mark));
	  p->mark = Qnil;
	}
    }

  if (config->saved_windows_count <= countof (Vwindow_configuration_free_list))
    free_managed_lcrecord (Vwindow_configuration_free_list
			   [config->saved_windows_count - 1],
			   window_config);

  return Qnil;
}

DEFUN ("set-window-configuration", Fset_window_configuration, 1, 1, 0, /*
Set the configuration of windows and buffers as specified by CONFIGURATION.
CONFIGURATION must be a value previously returned
by `current-window-configuration' (which see).
*/
       (configuration))
{
  struct window *w;
  struct window_config *config;
  struct saved_window *p;
  Lisp_Object new_current_buffer;
  int k;
  Lisp_Object frame;
  struct frame *f;
  struct gcpro gcpro1;
  Lisp_Object old_window_config;
  /*  int previous_frame_height;
      int previous_frame_width;*/
  int previous_pixel_top;
  int previous_pixel_height;
  int previous_pixel_left;
  int previous_pixel_width;
  int previous_minibuf_height, previous_minibuf_top,previous_minibuf_width;
  int real_font_height;
  int converted_minibuf_height,target_minibuf_height; 
  int specpdl_count = specpdl_depth ();

  GCPRO1 (configuration);

  CHECK_WINDOW_CONFIGURATION (configuration);
  config = XWINDOW_CONFIGURATION (configuration);

  frame = XWINDOW (SAVED_WINDOW_N (config, 0)->window)->frame;
  f = XFRAME (frame);

  /* Do not signal an error here if the frame was deleted.  There are
     reasonable cases where we could get here with a deleted frame and
     just want to do close to nothing instead. */

  if (FRAME_LIVE_P (f))
    {
      /* restore the frame characteristics */

      new_current_buffer = config->current_buffer;
      if (!BUFFER_LIVE_P (XBUFFER (new_current_buffer)))
	new_current_buffer = Qnil;

      /*
       * Assumed precondition:  w->config_mark = 0 for all w
       * This procedure should ensure this is true by the time it exits
       * to ensure the precondition for future calls.
       *
       * We use w->config_mark to know whether we're modifying a
       * window that is currently visible on the frame (#### we
       * should just be able to check whether the window is dead
       * or not, but this way is safer?).  As we process each
       * window, we set its config_mark to 0.  At the end, we
       * go through all the windows that used to be on the frame,
       * set each one's config_mark to 0 (to maintain the
       * assumed precondition) and delete each one that's no
       * longer in use.
       *
       * #### Using a window-configuration to keep track of
       * the current windows is wasteful.  All we need is the
       * list of windows, so we could just use a dynarr.
       */
      old_window_config = Fcurrent_window_configuration (frame);

      /* If the new configuration is already equal to the old, then stop
	 right here.  This saves the work below and it also saves
	 triggering a full redisplay of this window.  This is a huge win
	 when using the mouse since the mode motion code uses
	 save-window-excursion extensively but will rarely cause the
	 configuration to actually change. */
      if (window_config_equal (configuration, old_window_config))
	{
	  free_window_configuration (old_window_config);
	  UNGCPRO;
	  return Qnil;
	}

      /* We can't quit or even check for quit because that may cause
	 investigation of the frame state, which may crash if the frame is
	 in an inconsistent state. */
      begin_dont_check_for_quit ();
      record_unwind_protect (free_window_configuration, old_window_config);

      mark_windows_in_use (f, 1);

#if 0
      /* JV: This is bogus,
	 First of all, the units are inconsistent. The frame sizes are measured
	 in characters but the window sizes are stored in pixels. So if a 
	 font size change happened between saving and restoring, the
	 frame "sizes" maybe equal but the windows still should be
	 resized. This is tickled alot by the new "character size
	 stays constant" policy in 21.0. It leads to very wierd
	 glitches (and possibly craches when asserts are tickled).

	 Just changing the units doens't help because changing the
	 toolbar configuration can also change the pixel positions.
	 Luckily there is a much simpler way of doing this, see below.
       */
      previous_frame_width = FRAME_WIDTH (f);
      previous_frame_height = FRAME_HEIGHT (f);
      /* If the frame has been resized since this window configuration was
	 made, we change the frame to the size specified in the
	 configuration, restore the configuration, and then resize it
	 back.  We keep track of the prevailing height in these variables.  */
      if (config->frame_height != FRAME_HEIGHT (f)
	  || config->frame_width != FRAME_WIDTH (f))
	change_frame_size (f, config->frame_height, config->frame_width, 0);
#endif
      
      previous_pixel_top = XWINDOW (FRAME_ROOT_WINDOW (f))->pixel_top;
      previous_pixel_height = XWINDOW (FRAME_ROOT_WINDOW (f))->pixel_height;
      previous_pixel_left = XWINDOW (FRAME_ROOT_WINDOW (f))->pixel_left;
      previous_pixel_width = XWINDOW (FRAME_ROOT_WINDOW (f))->pixel_width;

      /* remember some properties of the minibuffer */

      default_face_height_and_width (frame, &real_font_height, 0);
      assert(real_font_height > 0);
  
      if (FRAME_HAS_MINIBUF_P (f) && ! FRAME_MINIBUF_ONLY_P (f))
	{
	  previous_minibuf_height
	    = XWINDOW(FRAME_MINIBUF_WINDOW(f))->pixel_height;
	  previous_minibuf_top
	    = XWINDOW(FRAME_MINIBUF_WINDOW(f))->pixel_top;
	  previous_minibuf_width
	    = XWINDOW(FRAME_MINIBUF_WINDOW(f))->pixel_width;
	}
      else
	{
	  previous_minibuf_height = 0;
	  previous_minibuf_width = 0;
	}
      converted_minibuf_height =
	(previous_minibuf_height % real_font_height) == 0 ?
	- (previous_minibuf_height / real_font_height ) :    /* lines */
	    previous_minibuf_height;   /* pixels */
           
      /* Temporarily avoid any problems with windows that are smaller
	 than they are supposed to be.  */
      window_min_height = 1;
      window_min_width = 1;

      /* OK, now restore all the windows in the window config.
	 This may involve "undeleting" windows, since the
	 windows in the window config may be deleted.
	 */
      for (k = 0; k < config->saved_windows_count; k++)
	{
	  p = SAVED_WINDOW_N (config, k);
	  w = XWINDOW (p->window);
	  w->next = Qnil;

	  /* The window might be dead.  In this case, its redisplay
	     structures were freed, so we need to reallocate them. */
	  if (!w->face_cachels)
	    {
	      w->face_cachels = Dynarr_new (face_cachel);
	      reset_face_cachels (w);
	    }
	  if (!w->glyph_cachels)
	    w->glyph_cachels = Dynarr_new (glyph_cachel);
	  if (!w->line_start_cache)
	    w->line_start_cache = Dynarr_new (line_start_cache);
	  w->dead = 0;

	  if (p->parent_index >= 0)
	    w->parent = SAVED_WINDOW_N (config, p->parent_index)->window;
	  else
	    w->parent = Qnil;

	  if (p->prev_index >= 0)
	    {
	      w->prev = SAVED_WINDOW_N (config, p->prev_index)->window;

	      /* This is true for a minibuffer-only frame. */
	      if (!NILP (w->mini_p) && EQ (w->prev, p->window))
		w->next = Qnil;
	      else
		XWINDOW (w->prev)->next = p->window;
	    }
	  else
	    {
	      w->prev = Qnil;
	      if (!NILP (w->parent))
		{
		  if (WINDOW_WIDTH (p) == WINDOW_WIDTH (XWINDOW (w->parent)))
		    {
		      XWINDOW (w->parent)->vchild = p->window;
		      XWINDOW (w->parent)->hchild = Qnil;
		    }
		  else
		    {
		      XWINDOW (w->parent)->hchild = p->window;
		      XWINDOW (w->parent)->vchild = Qnil;
		    }
		}
	    }
	  if (!w->config_mark)
	    {
	      /* #### This should be equivalent to the window previously
		 having been dead.  If we're brave, we'll put in an
		 assertion to this effect. */
	      MARK_FRAME_WINDOWS_STRUCTURE_CHANGED (f);
	    }
	  else /* if (!EQ (w->buffer, p->buffer)) */
	    {
	      /* With the new redisplay we let it know that a change has
		 been made and it will take care of the rest.  If we don't
		 tell it something has possibly changed it could lead to
		 incorrect display. */
	      MARK_WINDOWS_CHANGED (w);
	    }

	  WINDOW_LEFT (w) = WINDOW_LEFT (p);
	  WINDOW_TOP (w) = WINDOW_TOP (p);
	  WINDOW_WIDTH (w) = WINDOW_WIDTH (p);
	  WINDOW_HEIGHT (w) = WINDOW_HEIGHT (p);
	  w->hscroll = p->hscroll;
	  w->modeline_hscroll = p->modeline_hscroll;
	  w->line_cache_last_updated = Qzero;
	  SET_LAST_MODIFIED (w, 1);
	  SET_LAST_FACECHANGE (w);
	  w->config_mark = 0;

#define WINDOW_SLOT(slot, compare) w->slot = p->slot;
#include "winslots.h"

	  /* Reinstall the saved buffer and pointers into it.  */
	  if (NILP (p->buffer))
	    w->buffer = p->buffer;
	  else
	    {
	      if (BUFFER_LIVE_P (XBUFFER (p->buffer)))
		/* If saved buffer is alive, install it.  */
		{
		  w->buffer = p->buffer;
		  w->start_at_line_beg = p->start_at_line_beg;
		  set_marker_restricted (w->start[CURRENT_DISP],
					 Fmarker_position (p->start),
					 w->buffer);
		  set_marker_restricted (w->pointm[CURRENT_DISP],
					 Fmarker_position (p->pointm),
					 w->buffer);
		  set_marker_restricted (w->sb_point,
					 Fmarker_position (p->sb_point),
					 w->buffer);
		  Fset_marker (XBUFFER (w->buffer)->mark,
			       Fmarker_position (p->mark), w->buffer);

		  /* As documented in Fcurrent_window_configuration, don't
		     save the location of point in the buffer which was current
		     when the window configuration was recorded.  */
		  if (!EQ (p->buffer, new_current_buffer) &&
		      XBUFFER (p->buffer) == current_buffer)
		    Fgoto_char (w->pointm[CURRENT_DISP], Qnil);
		}
	      else if (NILP (w->buffer) ||
		       !BUFFER_LIVE_P (XBUFFER (w->buffer)))
		/* Else if window's old buffer is dead too, get a live one.  */
		{
		  /* #### The following line makes me nervous... */
		  /* w->buffer = Fcdr (Fcar (XFRAME (w->frame)->buffer_alist));*/
		  w->buffer = Fget_buffer_create (QSscratch);
		  /* w->buffer = Fother_buffer (Qnil, w->frame, Qnil); */
		  /* This will set the markers to beginning of visible
                     range.  */
		  set_marker_restricted (w->start[CURRENT_DISP], Qzero, w->buffer);
		  set_marker_restricted (w->pointm[CURRENT_DISP], Qzero,
					 w->buffer);
		  set_marker_restricted (w->sb_point, Qzero, w->buffer);
		  w->start_at_line_beg = 1;
		}
	      else
		/* Keeping window's old buffer; make sure the markers
                   are real.  */
		{
		  /* Set window markers at start of visible range.  */
		  if (XMARKER (w->start[CURRENT_DISP])->buffer == 0)
		    set_marker_restricted (w->start[CURRENT_DISP], Qzero,
					   w->buffer);
		  if (XMARKER (w->sb_point)->buffer == 0)
		    set_marker_restricted (w->sb_point, Qzero, w->buffer);
		  if (XMARKER (w->pointm[CURRENT_DISP])->buffer == 0)
		    set_marker_restricted (w->pointm[CURRENT_DISP],
					   make_int
					   (BUF_PT (XBUFFER (w->buffer))),
					   w->buffer);
		  w->start_at_line_beg = 1;
		}
	    }
	}

      FRAME_ROOT_WINDOW (f) = config->root_window;
      /* Note that FSFmacs unilaterally calls Fselect_window() here, and
	 then calls do_switch_frame() below to select the frame that was
	 recorded in the window config as being selected.

	 Instead, we don't ever change the selected frame, and either
	 call Fselect_window() below if the window config's frame is
	 currently selected, or just set the selected window of the
	 window config's frame. */

#if 0
      /* Set the frame height to the value it had before this function.  */
      if (previous_frame_height != FRAME_HEIGHT (f)
	  || previous_frame_width != FRAME_WIDTH (f))
	change_frame_size (f, previous_frame_height, previous_frame_width, 0);
#endif
      /* We just reset the size and position of the minibuffer, to its old
	 value, which needn't be valid. So we do some magic to see which value
	 to actually take. Then we set it.

	 The magic:
	 We take the old value if is in the same units but differs from the
	 current value.

         #### Now we get more cases correct then ever before, but
	 are we treating all? For instance what if the frames minibuf window
	 is no longer the same one? 
      */
      target_minibuf_height = previous_minibuf_height;
      if (converted_minibuf_height &&
	  (converted_minibuf_height * config->minibuf_height) > 0 &&
	  (converted_minibuf_height !=  config->minibuf_height))
	{
	  target_minibuf_height = config->minibuf_height < 0 ?
	    - (config->minibuf_height * real_font_height) :
	    config->minibuf_height;
	  target_minibuf_height =
	    max(target_minibuf_height,real_font_height);
	}
      if (previous_minibuf_height)
	{
	  XWINDOW (FRAME_MINIBUF_WINDOW (f))->pixel_top
	    = previous_minibuf_top -
	          (target_minibuf_height - previous_minibuf_height);
	  set_window_pixheight (FRAME_MINIBUF_WINDOW (f),
				target_minibuf_height, 0);
	  set_window_pixwidth  (FRAME_MINIBUF_WINDOW (f),
			    previous_minibuf_width, 0);
	}
	
      /* This is a better way to deal with frame resizing, etc.
	 What we _actually_ want is for the old (just restored)
	 root window to fit
	 into the place of the new one. So we just do that. Simple! */
      XWINDOW (FRAME_ROOT_WINDOW (f))->pixel_top = previous_pixel_top;
      /* Note that this function also updates the subwindow
	 "pixel_top"s */
      set_window_pixheight (FRAME_ROOT_WINDOW (f),
	  previous_pixel_height -
		  (target_minibuf_height - previous_minibuf_height), 0);
      XWINDOW (FRAME_ROOT_WINDOW (f))->pixel_left = previous_pixel_left;
      /* Note that this function also updates the subwindow
	 "pixel_left"s */
      set_window_pixwidth (FRAME_ROOT_WINDOW (f), previous_pixel_width, 0);
      
      /* If restoring in the current frame make the window current,
	 otherwise just update the frame selected_window slot to be
	 the restored current_window. */
      if (f == selected_frame ())
	{
	  /* When using `pop-window-configuration', often the minibuffer
	     ends up as the selected window even though it's not active ...
	     I really don't know the cause of this, but it should never
	     happen.  This kludge should fix it.

	     #### Find out why this is really going wrong. */
	  if (!minibuf_level &&
	      MINI_WINDOW_P (XWINDOW (config->current_window)))
	    Fselect_window (Fnext_window (config->current_window,
					  Qnil, Qnil, Qnil),
                            Qnil);
	  else
	    Fselect_window (config->current_window, Qnil);
	  if (!NILP (new_current_buffer))
	    Fset_buffer (new_current_buffer);
	  else
	    Fset_buffer (XWINDOW (Fselected_window (Qnil))->buffer);
	}
      else
	set_frame_selected_window (f, config->current_window);
    }
  else
    old_window_config = Qnil; /* Warning suppression */

  /* Restore the minimum heights recorded in the configuration.  */
  window_min_height = config->min_height;
  window_min_width = config->min_width;

#if 0 /* FSFmacs */
  /* see above comment */
  /* Fselect_window will have made f the selected frame, so we
     reselect the proper frame here.  Fhandle_switch_frame will change the
     selected window too, but that doesn't make the call to
     Fselect_window above totally superfluous; it still sets f's
     selected window.  */
  if (FRAME_LIVE_P (XFRAME (config->selected_frame)))
    do_switch_frame (config->selected_frame, Qnil, 0);
#endif

  Vminibuffer_scroll_window = config->minibuffer_scroll_window;

  if (FRAME_LIVE_P (f))
    {
      /* Do this before calling recompute_all_cached_specifiers_in_window()
	 so that things like redisplay_redraw_cursor() won't abort due
	 to no window mirror present. */
      f->mirror_dirty = 1;

      config = XWINDOW_CONFIGURATION (old_window_config);
      for (k = 0; k < config->saved_windows_count; k++)
	{
	  p = SAVED_WINDOW_N (config, k);
	  w = XWINDOW (p->window);
	  /* Remember, we set w->config_mark on all currently visible
	     windows, and reset it on all newly visible windows.
	     Any windows still marked need to be deleted. */
	  if (w->config_mark)
	    {
	      mark_window_as_deleted (w);
	      w->config_mark = 0;
	    }
	  else
	    {
	      /* We just potentially changed the window's buffer and
		 potentially turned a dead window into a live one,
		 so we need to recompute the cached specifier values. */
	      recompute_all_cached_specifiers_in_window (w);
	    }
	}
    }

  /* Now restore things, when everything else if OK. */

  unbind_to (specpdl_count, Qnil);

  UNGCPRO;

  return Qnil;
}

/* Mark all subwindows of a window as deleted.  The argument
   W is actually the subwindow tree of the window in question. */

void
delete_all_subwindows (struct window *w)
{
  if (!NILP (w->next))   delete_all_subwindows (XWINDOW (w->next));
  if (!NILP (w->vchild)) delete_all_subwindows (XWINDOW (w->vchild));
  if (!NILP (w->hchild)) delete_all_subwindows (XWINDOW (w->hchild));

  mark_window_as_deleted (w);
}


static int
count_windows (struct window *window)
{
  return 1 +
    (!NILP (window->next)   ? count_windows (XWINDOW (window->next))   : 0) +
    (!NILP (window->vchild) ? count_windows (XWINDOW (window->vchild)) : 0) +
    (!NILP (window->hchild) ? count_windows (XWINDOW (window->hchild)) : 0);
}

static int
saved_window_index (Lisp_Object window, struct window_config *config, int lim)
{
  int j;
  for (j = 0; j < lim; j++)
    {
      if (EQ (SAVED_WINDOW_N (config, j)->window, window))
	return j;
    }
  abort ();
  return 0;	/* suppress compiler warning */
}

static int
save_window_save (Lisp_Object window, struct window_config *config, int i)
{
  struct window *w;

  for (; !NILP (window); window = w->next)
    {
      struct saved_window *p = SAVED_WINDOW_N (config, i);

      w = XWINDOW (window);
      i++;
      p->window = window;
      p->buffer = w->buffer;
      WINDOW_LEFT (p) = WINDOW_LEFT (w);
      WINDOW_TOP (p) = WINDOW_TOP (w);
      WINDOW_WIDTH (p) = WINDOW_WIDTH (w);
      WINDOW_HEIGHT (p) = WINDOW_HEIGHT (w);
      p->hscroll = w->hscroll;
      p->modeline_hscroll = w->modeline_hscroll;

#define WINDOW_SLOT(slot, compare) p->slot = w->slot;
#include "winslots.h"

      if (!NILP (w->buffer))
	{
	  /* Save w's value of point in the window configuration.
	     If w is the selected window, then get the value of point
	     from the buffer; pointm is garbage in the selected window.  */
	  if (EQ (window, Fselected_window (Qnil)))
	    {
	      p->pointm = noseeum_make_marker ();
	      Fset_marker (p->pointm,
			   make_int (BUF_PT (XBUFFER (w->buffer))),
			   w->buffer);
	    }
	  else
	    p->pointm = noseeum_copy_marker (w->pointm[CURRENT_DISP], Qnil);

	  p->start = noseeum_copy_marker (w->start[CURRENT_DISP], Qnil);
	  p->sb_point = noseeum_copy_marker (w->sb_point, Qnil);
	  p->start_at_line_beg = w->start_at_line_beg;

	  p->mark = noseeum_copy_marker (XBUFFER (w->buffer)->mark, Qnil);
	}
      else
	{
	  p->pointm = Qnil;
	  p->start = Qnil;
	  p->sb_point = Qnil;
	  p->mark = Qnil;
	  p->start_at_line_beg = 0;
	}

      if (NILP (w->parent))
	p->parent_index = -1;
      else
        p->parent_index = saved_window_index (w->parent, config, i);
      if (NILP (w->prev))
	p->prev_index = -1;
      else
        p->prev_index = saved_window_index (w->prev, config, i);
      if (!NILP (w->vchild))
	i = save_window_save (w->vchild, config, i);
      if (!NILP (w->hchild))
	i = save_window_save (w->hchild, config, i);
    }

  return i;
}

#if 0 /* FSFmacs */
/* Added to doc string:

This also records the currently selected frame, and FRAME's focus
redirection (see `redirect-frame-focus').

*/
#endif

DEFUN ("current-window-configuration", Fcurrent_window_configuration, 0, 1, 0, /*
Return an object representing the current window configuration of FRAME.
If FRAME is nil or omitted, use the selected frame.
This describes the number of windows, their sizes and current buffers,
and for each displayed buffer, where display starts, and the positions of
point and mark.  An exception is made for point in the current buffer:
its value is -not- saved.
*/
       (frame))
{
  Lisp_Object result;
  struct frame *f = decode_frame (frame);
  struct window_config *config;
  int n_windows = count_windows (XWINDOW (FRAME_ROOT_WINDOW (f)));
  int minibuf_height;
  int real_font_height;

  if (n_windows <= countof (Vwindow_configuration_free_list))
    config = XWINDOW_CONFIGURATION (allocate_managed_lcrecord
				    (Vwindow_configuration_free_list
				     [n_windows - 1]));
  else
    /* More than ten windows; just allocate directly */
    config = (struct window_config *)
      alloc_lcrecord (sizeof_window_config_for_n_windows (n_windows),
		      &lrecord_window_configuration);
  XSETWINDOW_CONFIGURATION (result, config);
  /*
  config->frame_width = FRAME_WIDTH (f);
  config->frame_height = FRAME_HEIGHT (f); */
  config->current_window = FRAME_SELECTED_WINDOW (f);
  XSETBUFFER (config->current_buffer, current_buffer);
  config->minibuffer_scroll_window = Vminibuffer_scroll_window;
  config->root_window = FRAME_ROOT_WINDOW (f);
  config->min_height = window_min_height;
  config->min_width = window_min_width;
  config->saved_windows_count = n_windows;
  save_window_save (FRAME_ROOT_WINDOW (f), config, 0);

  /* save the minibuffer height using the heuristics from
     change_frame_size_1 */
  
  XSETFRAME (frame, f); /* frame could have been nil ! */
  default_face_height_and_width (frame, &real_font_height, 0);
  assert(real_font_height > 0);
  
  if (FRAME_HAS_MINIBUF_P (f) && ! FRAME_MINIBUF_ONLY_P (f))
    minibuf_height = XWINDOW(FRAME_MINIBUF_WINDOW(f))->pixel_height;
  else
    minibuf_height = 0;
  config->minibuf_height = (minibuf_height % real_font_height) == 0 ?
    - (minibuf_height / real_font_height ) :    /* lines */
    minibuf_height;   /* pixels */

  return result;
}

Lisp_Object
save_window_excursion_unwind (Lisp_Object window_config)
{
  Lisp_Object val = Fset_window_configuration (window_config);
  free_window_configuration (window_config);
  return val;
}

DEFUN ("save-window-excursion", Fsave_window_excursion, 0, UNEVALLED, 0, /*
Execute body, preserving window sizes and contents.
Restores which buffer appears in which window, where display starts,
as well as the current buffer.
Does not restore the value of point in current buffer.
*/
       (args))
{
  /* This function can GC */
  Lisp_Object val;
  int speccount = specpdl_depth ();

  record_unwind_protect (save_window_excursion_unwind,
			 Fcurrent_window_configuration (Qnil));
  val = Fprogn (args);
  return unbind_to (speccount, val);
}


#ifdef DEBUG_XEMACS
/* This is short and simple in elisp, but... it was written to debug
   problems purely on the C side.  That is where we need to call it so
   here it is. */
static void
debug_print_window (Lisp_Object window, int level)
{
  int i;
  Lisp_Object child = Fwindow_first_vchild (window);

  if (NILP (child))
    child = Fwindow_first_hchild (window);

  for (i = level; i > 0; i--)
    putc ('\t', stderr);

  fputs ("#<window", stderr);
  {
    Lisp_Object buffer = XWINDOW (window)->buffer;
    if (!NILP (buffer) && BUFFERP (buffer))
      fprintf (stderr, " on %s", XSTRING_DATA (XBUFFER (buffer)->name));
  }
  fprintf (stderr, " 0x%x>", XWINDOW (window)->header.uid);

  while (!NILP (child))
    {
      debug_print_window (child, level + 1);
      child = Fwindow_next_child (child);
    }
}

void debug_print_windows (struct frame *f);
void
debug_print_windows (struct frame *f)
{
  debug_print_window (f->root_window, 0);
  putc ('\n', stderr);
}
#endif /* DEBUG_XEMACS */


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_window (void)
{
  defsymbol (&Qwindowp, "windowp");
  defsymbol (&Qwindow_live_p, "window-live-p");
  defsymbol (&Qwindow_configurationp, "window-configuration-p");
  defsymbol (&Qscroll_up, "scroll-up");
  defsymbol (&Qscroll_down, "scroll-down");
  defsymbol (&Qtemp_buffer_show_hook, "temp-buffer-show-hook");
  defsymbol (&Qdisplay_buffer, "display-buffer");

#ifdef MEMORY_USAGE_STATS
  defsymbol (&Qface_cache, "face-cache");
  defsymbol (&Qglyph_cache, "glyph-cache");
  defsymbol (&Qline_start_cache, "line-start-cache");
#ifdef HAVE_SCROLLBARS
  defsymbol (&Qscrollbar_instances, "scrollbar-instances");
#endif
  defsymbol (&Qother_redisplay, "other-redisplay");
  /* Qother in general.c */
#endif

  DEFSUBR (Fselected_window);
  DEFSUBR (Fminibuffer_window);
  DEFSUBR (Fwindow_minibuffer_p);
  DEFSUBR (Fwindowp);
  DEFSUBR (Fwindow_live_p);
  DEFSUBR (Fwindow_first_hchild);
  DEFSUBR (Fwindow_first_vchild);
  DEFSUBR (Fwindow_next_child);
  DEFSUBR (Fwindow_previous_child);
  DEFSUBR (Fwindow_parent);
  DEFSUBR (Fwindow_lowest_p);
  DEFSUBR (Fwindow_highest_p);
  DEFSUBR (Fwindow_leftmost_p);
  DEFSUBR (Fwindow_rightmost_p);
  DEFSUBR (Fpos_visible_in_window_p);
  DEFSUBR (Fwindow_buffer);
  DEFSUBR (Fwindow_frame);
  DEFSUBR (Fwindow_height);
  DEFSUBR (Fwindow_displayed_height);
  DEFSUBR (Fwindow_width);
  DEFSUBR (Fwindow_pixel_height);
  DEFSUBR (Fwindow_pixel_width);
  DEFSUBR (Fwindow_text_area_pixel_height);
  DEFSUBR (Fwindow_displayed_text_pixel_height);
  DEFSUBR (Fwindow_text_area_pixel_width);
  DEFSUBR (Fwindow_hscroll);
#ifdef MODELINE_IS_SCROLLABLE
  DEFSUBR (Fmodeline_hscroll);
  DEFSUBR (Fset_modeline_hscroll);
#endif /* MODELINE_IS_SCROLLABLE */
#if 0 /* bogus FSF crock */
  DEFSUBR (Fwindow_redisplay_end_trigger);
  DEFSUBR (Fset_window_redisplay_end_trigger);
#endif
  DEFSUBR (Fset_window_hscroll);
  DEFSUBR (Fwindow_pixel_edges);
  DEFSUBR (Fwindow_text_area_pixel_edges);
  DEFSUBR (Fwindow_point);
  DEFSUBR (Fwindow_start);
  DEFSUBR (Fwindow_end);
  DEFSUBR (Fset_window_point);
  DEFSUBR (Fset_window_start);
  DEFSUBR (Fwindow_dedicated_p);
  DEFSUBR (Fset_window_dedicated_p);
  DEFSUBR (Fnext_window);
  DEFSUBR (Fprevious_window);
  DEFSUBR (Fnext_vertical_window);
  DEFSUBR (Fother_window);
  DEFSUBR (Fget_lru_window);
  DEFSUBR (Fget_largest_window);
  DEFSUBR (Fget_buffer_window);
  DEFSUBR (Fwindow_left_margin_pixel_width);
  DEFSUBR (Fwindow_right_margin_pixel_width);
  DEFSUBR (Fdelete_other_windows);
  DEFSUBR (Fdelete_windows_on);
  DEFSUBR (Freplace_buffer_in_windows);
  DEFSUBR (Fdelete_window);
  DEFSUBR (Fset_window_buffer);
  DEFSUBR (Fselect_window);
  DEFSUBR (Fsplit_window);
  DEFSUBR (Fenlarge_window);
  DEFSUBR (Fenlarge_window_pixels);
  DEFSUBR (Fshrink_window);
  DEFSUBR (Fshrink_window_pixels);
  DEFSUBR (Fscroll_up);
  DEFSUBR (Fscroll_down);
  DEFSUBR (Fscroll_left);
  DEFSUBR (Fscroll_right);
  DEFSUBR (Fother_window_for_scrolling);
  DEFSUBR (Fscroll_other_window);
  DEFSUBR (Fcenter_to_window_line);
  DEFSUBR (Fmove_to_window_line);
#ifdef MEMORY_USAGE_STATS
  DEFSUBR (Fwindow_memory_usage);
#endif
  DEFSUBR (Fwindow_configuration_p);
  DEFSUBR (Fset_window_configuration);
  DEFSUBR (Fcurrent_window_configuration);
  DEFSUBR (Fsave_window_excursion);
}

void
vars_of_window (void)
{
  /* Make sure all windows get marked */
  minibuf_window = Qnil;
  staticpro (&minibuf_window);

  DEFVAR_BOOL ("scroll-on-clipped-lines", &scroll_on_clipped_lines /*
*Non-nil means to scroll if point lands on a line which is clipped.
*/ );
  scroll_on_clipped_lines = 1;

  DEFVAR_LISP ("temp-buffer-show-hook", &Vtemp_buffer_show_hook /*
See `temp-buffer-show-function'.
*/ );
  Vtemp_buffer_show_hook = Qnil;

  DEFVAR_LISP ("temp-buffer-show-function", &Vtemp_buffer_show_function /*
Non-nil means call as function to display a help buffer.
The function is called with one argument, the buffer to be displayed.
Used by `with-output-to-temp-buffer'.
If this function is used, then it must do the entire job of showing
the buffer; `temp-buffer-show-hook' is not run unless this function runs it.
*/ );
  Vtemp_buffer_show_function = Qnil;

  DEFVAR_LISP ("minibuffer-scroll-window", &Vminibuffer_scroll_window /*
Non-nil means it is the window that \\<minibuffer-local-map>\\[scroll-other-window] in minibuffer should scroll.
*/ );
  Vminibuffer_scroll_window = Qnil;

  DEFVAR_LISP ("other-window-scroll-buffer", &Vother_window_scroll_buffer /*
If non-nil, this is a buffer and \\[scroll-other-window] should scroll its window.
*/ );
  Vother_window_scroll_buffer = Qnil;

  DEFVAR_INT ("next-screen-context-lines", &next_screen_context_lines /*
*Number of lines of continuity when scrolling by screenfuls.
*/ );
  next_screen_context_lines = 2;

  DEFVAR_INT ("window-min-height", &window_min_height /*
*Delete any window less than this tall (including its modeline).
*/ );
  window_min_height = 4;

  DEFVAR_INT ("window-min-width", &window_min_width /*
*Delete any window less than this wide.
*/ );
  window_min_width = 10;

  {
    int i;

    for (i = 0; i < countof (Vwindow_configuration_free_list); i++)
      {
	Vwindow_configuration_free_list[i] =
	  make_lcrecord_list (sizeof_window_config_for_n_windows (i + 1),
			      &lrecord_window_configuration);
	staticpro (&Vwindow_configuration_free_list[i]);
      }
  }
}

void
specifier_vars_of_window (void)
{
  DEFVAR_SPECIFIER ("modeline-shadow-thickness", &Vmodeline_shadow_thickness /*
*How thick to draw 3D shadows around modelines.
If this is set to 0, modelines will be the traditional 2D.  Sizes above
10 will be accepted but the maximum thickness that will be drawn is 10.
This is a specifier; use `set-specifier' to change it.
*/ );
  Vmodeline_shadow_thickness = Fmake_specifier (Qinteger);
  /* The initial value for modeline-shadow-thickness is 2, but if the
     user removes all specifications we provide a fallback value of 0,
     which is probably what was expected. */
  set_specifier_fallback (Vmodeline_shadow_thickness,
			  list1 (Fcons (Qnil, Qzero)));
  Fadd_spec_to_specifier (Vmodeline_shadow_thickness, make_int (2),
			  Qnil, Qnil, Qnil);
  set_specifier_caching (Vmodeline_shadow_thickness,
			 slot_offset (struct window,
				      modeline_shadow_thickness),
			 modeline_shadow_thickness_changed,
			 0, 0);

  DEFVAR_SPECIFIER ("has-modeline-p", &Vhas_modeline_p /*
*Whether the modeline should be displayed.
This is a specifier; use `set-specifier' to change it.
*/ );
  Vhas_modeline_p = Fmake_specifier (Qboolean);
  set_specifier_fallback (Vhas_modeline_p,
			  list1 (Fcons (Qnil, Qt)));
  set_specifier_caching (Vhas_modeline_p,
			 slot_offset (struct window,
				      has_modeline_p),
			 /* #### It's strange that we need a special
			    flag to indicate that the shadow-thickness
			    has changed, but not one to indicate that
			    the modeline has been turned off or on. */
			 some_window_value_changed,
			 0, 0);

  DEFVAR_SPECIFIER ("vertical-divider-always-visible-p",
		    &Vvertical_divider_always_visible_p /*
*Should XEmacs always display vertical dividers between windows.

When this is non-nil, vertical dividers are always shown, and are
draggable.  When it is nil, vertical dividers are shown only when
there are no scrollbars in between windows, and are not draggable.

This is a specifier; use `set-specifier' to change it.
*/ );
  Vvertical_divider_always_visible_p = Fmake_specifier (Qboolean);
  set_specifier_fallback (Vvertical_divider_always_visible_p,
			  list1 (Fcons (Qnil, Qt)));
  set_specifier_caching (Vvertical_divider_always_visible_p,
			 slot_offset (struct window,
				      vertical_divider_always_visible_p),
			 vertical_divider_changed_in_window,
 			 0, 0);

  DEFVAR_SPECIFIER ("vertical-divider-shadow-thickness", &Vvertical_divider_shadow_thickness /*
*How thick to draw 3D shadows around vertical dividers.
This is a specifier; use `set-specifier' to change it.
*/ );
  Vvertical_divider_shadow_thickness = Fmake_specifier (Qinteger);
  set_specifier_fallback (Vvertical_divider_shadow_thickness,
			  list1 (Fcons (Qnil, Qzero)));
  Fadd_spec_to_specifier (Vvertical_divider_shadow_thickness, make_int (2),
			  Qnil, Qnil, Qnil);
  set_specifier_caching (Vvertical_divider_shadow_thickness,
			 slot_offset (struct window,
				      vertical_divider_shadow_thickness),
			 vertical_divider_changed_in_window,
 			 0, 0);
  DEFVAR_SPECIFIER ("vertical-divider-line-width", &Vvertical_divider_line_width /*
*The width of the vertical dividers, not including shadows.

For TTY windows, divider line is always one character wide.  When
instance of this specifier is zero in a TTY window, no divider is
drawn at all between windows.  When non-zero, a one character wide
divider is displayed.

This is a specifier; use `set-specifier' to change it.
*/ );

  Vvertical_divider_line_width = Fmake_specifier (Qnatnum);
  {
    Lisp_Object fb = Qnil;
#ifdef HAVE_TTY
    fb = Fcons (Fcons (list1 (Qtty), make_int (1)), fb);
#endif
#ifdef HAVE_X_WINDOWS
    fb = Fcons (Fcons (list1 (Qx), make_int (3)), fb);
#endif
#ifdef HAVE_MS_WINDOWS
    /* #### This should be made magic and made to obey system settings */
    fb = Fcons (Fcons (list1 (Qmswindows), make_int (3)), fb);
#endif
    set_specifier_fallback (Vvertical_divider_line_width, fb);
  }
  set_specifier_caching (Vvertical_divider_line_width,
                         slot_offset (struct window,
				      vertical_divider_line_width),
			 vertical_divider_changed_in_window,
                         0, 0);

  DEFVAR_SPECIFIER ("vertical-divider-spacing", &Vvertical_divider_spacing /*
*How much space to leave around the vertical dividers.

In TTY windows, spacing is always zero, and the value of this
specifier is ignored.

This is a specifier; use `set-specifier' to change it.
*/ );
  Vvertical_divider_spacing = Fmake_specifier (Qnatnum);
  {
    Lisp_Object fb = Qnil;
#ifdef HAVE_TTY
    fb = Fcons (Fcons (list1 (Qtty), Qzero), fb);
#endif
#ifdef HAVE_X_WINDOWS
    /* #### 3D dividers look great on MS Windows with spacing = 0.
       Should not the same value be the fallback under X? - kkm */
    fb = Fcons (Fcons (list1 (Qx), make_int (2)), fb);
#endif
#ifdef HAVE_MS_WINDOWS
    fb = Fcons (Fcons (list1 (Qmswindows), Qzero), fb);
#endif
    set_specifier_fallback (Vvertical_divider_spacing, fb);
  }
  set_specifier_caching (Vvertical_divider_spacing,
			 slot_offset (struct window,
				      vertical_divider_spacing),
			 vertical_divider_changed_in_window,
			 0, 0);
}
