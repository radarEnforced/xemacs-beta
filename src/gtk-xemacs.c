/* gtk-xemacs.c
**
** Description: A widget to encapsulate a XEmacs 'text widget'
**
** Created by: William M. Perry
** Copyright (c) 2000 William M. Perry <wmperry@gnu.org>
** Copyright (C) 2010 Ben Wing.
**
** This file is part of XEmacs.
**
** XEmacs is free software: you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation, either version 3 of the License, or (at your
** option) any later version.
** 
** XEmacs is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
** FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
** for more details.
** 
** You should have received a copy of the GNU General Public License
** along with XEmacs.  If not, see <http://www.gnu.org/licenses/>. */

#include <config.h>

#include "lisp.h"

#include "device.h"
#include "faces.h"
#include "glyphs.h"
#include "window.h"

#include "frame-impl.h"
#include "console-gtk-impl.h"
#include "device-impl.h"
#include "gtk-xemacs.h"
#include "fontcolor-gtk.h"

extern Lisp_Object Vmodeline_face;
extern Lisp_Object Vscrollbar_on_left_p;

EXFUN (Fmake_image_instance, 4);

static void gtk_xemacs_class_init	(GtkXEmacsClass *klass);
static void gtk_xemacs_init		(GtkXEmacs *xemacs, GtkXEmacsClass *klass);
static void gtk_xemacs_size_allocate	(GtkWidget *widget, GtkAllocation *allocaction);
static void gtk_xemacs_size_request	(GtkWidget *widget, GtkRequisition *requisition);
static void gtk_xemacs_realize		(GtkWidget *widget);
static void gtk_xemacs_style_set        (GtkWidget *widget, GtkStyle *previous_style);
#ifdef HAVE_GTK3
static void gtk_xemacs_paint (GtkWidget *widget, cairo_rectangle_int_t *cr);
static void gtk_xemacs_get_preferred_height (GtkWidget *, gint *minimal_height,
					     gint *natural_height);
static void gtk_xemacs_get_preferred_width (GtkWidget *, gint *minimal_width,
						gint *natural_width);
static gboolean gtk_xemacs_draw		(GtkWidget *widget, cairo_t *cr);
#endif
#ifdef HAVE_GTK2
static void gtk_xemacs_paint		(GtkWidget *widget, GdkRectangle *area);
static gint gtk_xemacs_expose		(GtkWidget *widget, GdkEventExpose *event);
#endif

GType
gtk_xemacs_get_type (void)
{
  static GType xemacs_type = G_TYPE_INVALID;

  if (xemacs_type == G_TYPE_INVALID)
    {
      xemacs_type =
        g_type_register_static_simple (GTK_TYPE_FIXED,
                                       "GtkXEmacs",
                                       sizeof (GtkXEmacsClass),
                                       (GClassInitFunc) gtk_xemacs_class_init,
                                       sizeof (GtkXEmacs),
                                       (GInstanceInitFunc) gtk_xemacs_init,
                                       (GTypeFlags) 0);
    }

  assert (xemacs_type != G_TYPE_INVALID);
  return xemacs_type;
}

static GtkWidgetClass *parent_class;

static void
gtk_xemacs_class_init (GtkXEmacsClass *class_)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass *) class_;
  parent_class = (GtkWidgetClass *) g_type_class_peek (GTK_TYPE_FIXED);

  widget_class->size_allocate = gtk_xemacs_size_allocate;
#ifdef HAVE_GTK2
  widget_class->size_request = gtk_xemacs_size_request;
  widget_class->expose_event = gtk_xemacs_expose;
  widget_class->style_set = gtk_xemacs_style_set;
#endif
#ifdef HAVE_GTK3
  widget_class->get_preferred_height = gtk_xemacs_get_preferred_height;
  widget_class->get_preferred_width  = gtk_xemacs_get_preferred_width;
  widget_class->draw = gtk_xemacs_draw;
#endif
  widget_class->realize = gtk_xemacs_realize;
  widget_class->button_press_event = emacs_gtk_button_event_handler;
  widget_class->button_release_event = emacs_gtk_button_event_handler;
  widget_class->key_press_event = emacs_gtk_key_event_handler;
  widget_class->key_release_event = emacs_gtk_key_event_handler;
  widget_class->motion_notify_event = emacs_gtk_motion_event_handler;
}

static void
gtk_xemacs_init (GtkXEmacs *xemacs, GtkXEmacsClass * UNUSED (klass))
{
  gtk_widget_set_can_focus (GTK_WIDGET (xemacs), TRUE);
}

GtkWidget*
gtk_xemacs_new (struct frame *f)
{
  GtkXEmacs *xemacs;

  xemacs = GTK_XEMACS (g_object_new (GTK_TYPE_XEMACS, NULL));
  gtk_widget_set_name (GTK_WIDGET (xemacs), "xemacs");
  gtk_widget_set_has_window (GTK_WIDGET (xemacs), TRUE);
  xemacs->f = f;
  gtk_widget_add_events (GTK_WIDGET (xemacs), GDK_BUTTON_RELEASE);

  return GTK_WIDGET (xemacs);
}

static void
__nuke_background_items (GtkWidget
#ifdef HAVE_GTK2
			 *widget
#endif
#ifdef HAVE_GTK3
			 * UNUSED (widget)
#endif
			 )
{
  /* This bit of voodoo is here to get around the annoying flicker
     when GDK tries to futz with our background pixmap as well as
     XEmacs doing it

     We do NOT set the background of this widget window, that way
     there is NO flickering, etc.  The downside is the XEmacs frame
     appears as 'seethru' when XEmacs is too busy to redraw the
     frame.

     Well, wait, we do... otherwise there sre weird 'seethru' areas
     even when XEmacs does a full redisplay.  Most noticeable in some
     areas of the modeline, or in the right-hand-side of the window
     between the scrollbar and the edge of the window.
  */
#ifdef HAVE_GTK2
  if (widget->window)
    {
      gdk_window_set_back_pixmap (widget->window, NULL, 0);
      gdk_window_set_back_pixmap (widget->parent->window, NULL, 0);
      gdk_window_set_background (widget->parent->window,
				 &widget->style->bg[GTK_STATE_NORMAL]);
      gdk_window_set_background (widget->window,
				 &widget->style->bg[GTK_STATE_NORMAL]);
    }
#endif
}

extern Lisp_Object xemacs_gtk_convert_color(GdkColor *c, GtkWidget *w);

/* From fontcolor-gtk.c */
extern Lisp_Object __get_gtk_font_truename (PangoFont *gdk_font, int expandp);

#define convert_font(f) __get_gtk_font_truename (f, 0)

#ifdef SMASH_FACE_FALLBACKS
static void
smash_face_fallbacks (struct frame *f, GtkStyle *style)
{
#define FROB(face,prop,slot) do { 							\
				Lisp_Object fallback = Qnil;				\
				Lisp_Object specifier = Fget (face, prop, Qnil);	\
   				struct Lisp_Specifier *sp = NULL;			\
 				if (NILP (specifier)) continue;				\
 				sp = XSPECIFIER (specifier);				\
 				fallback = sp->fallback;				\
 				if (EQ (Fcar (Fcar (Fcar (fallback))), Qgtk))		\
 					fallback = XCDR (fallback);			\
 				if (! NILP (slot))					\
 					fallback = acons (list1 (Qgtk),			\
 								  slot,			\
 								  fallback);		\
 				set_specifier_fallback (specifier, fallback);		\
			     } while (0);
#define FROB_FACE(face,fg_slot,bg_slot) \
do {											\
	FROB (face, Qforeground, xemacs_gtk_convert_color (&style->fg_slot[GTK_STATE_NORMAL], FRAME_GTK_SHELL_WIDGET (f)));	\
	FROB (face, Qbackground, xemacs_gtk_convert_color (&style->bg_slot[GTK_STATE_NORMAL], FRAME_GTK_SHELL_WIDGET (f)));	\
	if (style->rc_style && style->rc_style->bg_pixmap_name[GTK_STATE_NORMAL])	\
	{										\
		FROB (Vdefault_face, Qbackground_pixmap,				\
			Fmake_image_instance (build_cistring (style->rc_style->bg_pixmap_name[GTK_STATE_NORMAL]), \
					  f->device, Qnil, make_fixnum (5)));			\
	}										\
	else										\
	{										\
		FROB (Vdefault_face, Qbackground_pixmap, Qnil);				\
	}										\
} while (0)

  FROB (Vdefault_face, Qfont, convert_font (style->font));
  FROB_FACE (Vdefault_face, fg, bg);
  FROB_FACE (Vgui_element_face, text, mid);

#undef FROB
#undef FROB_FACE
}
#endif /* SMASH_FACE_FALLBACKS */

#ifdef HAVE_SCROLLBARS
static void
smash_scrollbar_specifiers (struct frame *f, GtkStyle *style)
{
  Lisp_Object frame;
  int slider_size = 10;
  int hsize, vsize;

  frame = wrap_frame (f);

#ifdef HAVE_GTK2
  // Note: where do I get this in Gtk 2.X?  -- jsparkes
  //slider_size = style->slider_width;
  slider_size = 10;
  hsize = slider_size + (style->ythickness * 2);
  vsize = slider_size + (style->xthickness * 2);

  style = gtk_style_attach (style,
			    GTK_WIDGET (DEVICE_GTK_APP_SHELL (XDEVICE (FRAME_DEVICE (f))))->window);
#endif
#ifdef HAVE_GTK3
  /* No properties for this in Gtk 3.X? */
  hsize = vsize = slider_size;
#endif

  Fadd_spec_to_specifier (Vscrollbar_width, make_fixnum (vsize), frame, Qnil, Qnil);
  Fadd_spec_to_specifier (Vscrollbar_height, make_fixnum (hsize), frame, Qnil, Qnil);
}
#endif /* HAVE_SCROLLBARS */

#ifdef HAVE_TOOLBARS
extern Lisp_Object Vtoolbar_shadow_thickness;

static void
smash_toolbar_specifiers(struct frame *f, GtkStyle *style)
{
  Lisp_Object frame;

  frame = wrap_frame (f);

  Fadd_spec_to_specifier (Vtoolbar_shadow_thickness,
			  make_fixnum (style->xthickness), Qnil, 
			  list2 (Qgtk, Qdefault), Qprepend);
}
#endif /* HAVE_TOOLBARS */

static void
gtk_xemacs_realize (GtkWidget *widget)
{
  parent_class->realize (widget);
  gtk_xemacs_style_set (widget, gtk_widget_get_style (widget));
}

static void
gtk_xemacs_style_set (GtkWidget *widget, GtkStyle *previous_style)
{
  GtkStyle *new_style = gtk_widget_get_style (widget);
  GtkXEmacs *x = GTK_XEMACS (widget);

  parent_class->style_set (widget, previous_style);

  if (x->f)
    {
      __nuke_background_items (widget);
#ifdef SMASH_FACE_FALLBACKS
      smash_face_fallbacks (x->f, new_style);
#endif
#ifdef HAVE_SCROLLBARS
      smash_scrollbar_specifiers (x->f, new_style);
#endif
#ifdef HAVE_TOOLBARS
      smash_toolbar_specifiers (x->f, new_style);
#endif
    }
}

static void
gtk_xemacs_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
    GtkXEmacs *x = GTK_XEMACS (widget);
    struct frame *f = GTK_XEMACS_FRAME (x);
    int width, height, unused;

    if (f)
      {
	frame_unit_to_pixel_size (f, FRAME_WIDTH (f), FRAME_HEIGHT (f),
				  &width, &height);
	requisition->width = width;
	requisition->height = height;
      }
    else
      {
#ifdef HAVE_GTK2
	parent_class->size_request (widget, requisition);
#endif
#ifdef HAVE_GTK3
	GTK_WIDGET_GET_CLASS (widget)->get_preferred_height (widget, &unused,
							     &height);
	requisition->height = height;
	GTK_WIDGET_GET_CLASS (widget)->get_preferred_width  (widget, &unused,
							     &width);
	requisition->width  = width;
#endif
      }
}

#ifdef HAVE_GTK3
static void
gtk_xemacs_get_preferred_height (GtkWidget *widget,
				 gint *minimal_height,
				 gint *natural_height)
{
  GtkRequisition req;

  gtk_xemacs_size_request (widget, &req);
  *minimal_height = *natural_height = req.height;
}

static void
gtk_xemacs_get_preferred_width (GtkWidget *widget,
				gint *minimal_width,
				gint *natural_width)
{
  GtkRequisition req;

  gtk_xemacs_size_request (widget, &req);
  *minimal_width = *natural_width = req.width;
}
#endif

static void
gtk_xemacs_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
    GtkXEmacs *x = GTK_XEMACS (widget);
    struct frame *f = GTK_XEMACS_FRAME (x);
    int columns, rows;

    /* Now that the toolbar is a widget, all of the children are
       widgets.  This means that the standard size allocation function
       is appropriate, no need for custom.  This fixes visibility of
       both toolbar and scrollbars. */
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

    if (f)
      {
	f->pixwidth = allocation->width;
	f->pixheight = allocation->height;

	pixel_to_frame_unit_size (f,
			    allocation->width,
			    allocation->height, &columns, &rows);

	change_frame_size (f, columns, rows, 1);
      }
}

#ifdef HAVE_GTK2
static void
gtk_xemacs_paint (GtkWidget *widget, GdkRectangle *area)
{
    GtkXEmacs *x = GTK_XEMACS (widget);
    struct frame *f = GTK_XEMACS_FRAME (x);

    if (gtk_widget_is_drawable (widget))
      redisplay_redraw_exposed_area (f, area->x, area->y, area->width,
				     area->height);
}
#endif

#ifdef HAVE_GTK3
static void
gtk_xemacs_paint (GtkWidget *widget, cairo_rectangle_int_t *area)
{
  GtkXEmacs *x = GTK_XEMACS (widget);
  struct frame *f = GTK_XEMACS_FRAME (x);

  if (gtk_widget_is_drawable (widget))
    redisplay_redraw_exposed_area (f, area->x, area->y, area->width,
				   area->height);
}
#endif

#ifdef HAVE_GTK2
static gboolean
gtk_xemacs_draw (GtkWidget *widget, GdkRectangle *area)
{
    GtkFixed *fixed = GTK_FIXED (widget);
    GtkFixedChild *child;
    GdkRectangle child_area;
    GList *children;

    /* I need to manually iterate over the children instead of just
       chaining to parent_class->draw() because it calls
       gtk_fixed_paint() directly, which clears the background window,
       which causes A LOT of flashing. */

  if (gtk_widget_is_drawable (widget))
    {
      gtk_xemacs_paint (widget, area);

      children = gtk_container_get_children (GTK_CONTAINER (fixed));

      while (children)
	{
	  child = (GtkFixedChild*) children->data;
	  children = children->next;
	  /* #### This is what causes the scrollbar flickering!
	     Evidently the scrollbars pretty much take care of drawing
	     themselves in most cases.  Then we come along and tell them
	     to redraw again!

	     But if we just leave it out, then they do not get drawn
	     correctly the first time!

	     Scrollbar flickering has been greatly helped by the
	     optimizations in scrollbar-gtk.c /
	     gtk_update_scrollbar_instance_status (), so this is not that
	     big a deal anymore.
	  */
	  if (gtk_widget_intersect (child->widget, area, &child_area))
	    {
	      gtk_widget_draw (child->widget, &child_area);
	    }
	}
    }
  return TRUE;
}
#endif

#ifdef HAVE_GTK3
static gboolean
gtk_xemacs_draw (GtkWidget *widget, cairo_t *cr)
{
  GtkFixed *fixed = GTK_FIXED (widget);
  GtkAllocation allocation = { 0 };
  GtkStyleContext *context;
  GtkStateFlags state;
  GtkFixedChild *child;
  GdkRectangle child_area;
  GList *children;

  gtk_widget_get_allocation (widget, &allocation);
  context = gtk_widget_get_style_context (widget);
  state = gtk_widget_get_state_flags (widget);

  gtk_render_background (context, cr, 0, 0,
			 allocation.width, allocation.height);

  /* I need to manually iterate over the children instead of just
     chaining to parent_class->draw() because it calls
     gtk_fixed_paint() directly, which clears the background window,
     which causes A LOT of flashing. */

  if (gtk_widget_is_drawable (widget))
    {
      cairo_rectangle_list_t *rects = cairo_copy_clip_rectangle_list (cr);
      if (rects->status == CAIRO_STATUS_SUCCESS)
	{
	  int i;
	  for (i=0; i < rects->num_rectangles; i++)
	    {
	      cairo_rectangle_int_t r;

	      r.x = floor (rects->rectangles[i].x);
	      r.y = floor (rects->rectangles[i].y);
	      r.width = ceil (rects->rectangles[i].width);
	      r.height = ceil (rects->rectangles[i].height);
	      gtk_xemacs_paint (widget, &r);
	    }
	}
      cairo_rectangle_list_destroy (rects);
      rects = 0;

      children = gtk_container_get_children (GTK_CONTAINER (fixed));
#if 0
      while (children)
	{
	  child = (GtkFixedChild*) children->data;
	  children = children->next;
	  /* #### This is what causes the scrollbar flickering!
	     Evidently the scrollbars pretty much take care of drawing
	     themselves in most cases.  Then we come along and tell them
	     to redraw again!

	     But if we just leave it out, then they do not get drawn
	     correctly the first time!

	     Scrollbar flickering has been greatly helped by the
	     optimizations in scrollbar-gtk.c /
	     gtk_update_scrollbar_instance_status (), so this is not that
	     big a deal anymore.
	  */
	  if (gtk_widget_intersect (child->widget, area, &child_area))
	    {
	      gtk_widget_draw (child->widget, &child_area);
	    }
	}
#endif
    }
  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);
  return TRUE;
}
#endif

#ifdef HAVE_GTK2
static gint
gtk_xemacs_expose (GtkWidget *widget, GdkEventExpose *event)
{
    GtkXEmacs *x = GTK_XEMACS (widget);
    struct frame *f = GTK_XEMACS_FRAME (x);
    GdkRectangle *a = &event->area;

  if (gtk_widget_is_drawable (widget))
    {
      /* This takes care of drawing the scrollbars, etc */
      parent_class->expose_event (widget, event);

      /* Now draw the actual frame data */
      if (!check_for_ignored_expose (f, a->x, a->y, a->width, a->height) &&
	  !find_matching_subwindow (f, a->x, a->y, a->width, a->height))
	redisplay_redraw_exposed_area (f, a->x, a->y, a->width, a->height);
      return (TRUE);
    }

  return FALSE;
}
#endif

Lisp_Object
xemacs_gtk_convert_color(GdkColor *c, GtkWidget *UNUSED (w))
{
  char color_buf[255];

  sprintf (color_buf, "#%04x%04x%04x", c->red, c->green, c->blue);

  return (build_cistring (color_buf));
}
