/* Device functions for X windows.
   Copyright (C) 1994, 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1994, 1995 Free Software Foundation, Inc.
   Copyright (C) 2002 Ben Wing.

This file is part of XEmacs.

XEmacs is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

XEmacs is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with XEmacs.  If not, see <http://www.gnu.org/licenses/>. */

/* Synched up with: Not in FSF. */

/* Original authors: Jamie Zawinski and the FSF */
/* Rewritten by Ben Wing and Chuck Thompson. */
/* Gtk flavor written by William Perry */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "device-impl.h"
#include "elhash.h"
#include "events.h"
#include "faces.h"
#include "frame-impl.h"
#include "redisplay.h"
#include "sysdep.h"
#include "window.h"
#include "select.h"

#include "console-gtk-impl.h"
#include "glyphs-gtk.h"
#include "fontcolor-gtk.h"
#include "gtk-xemacs.h"

#include "sysfile.h"
#include "systime.h"

#include <locale.h>

Lisp_Object Qmake_device_early_gtk_entry_point,
   Qmake_device_late_gtk_entry_point;

Lisp_Object Vgtk_version, Vgtk_major_version;
Lisp_Object Vgtk_minor_version, Vgtk_micro_version;
Lisp_Object Vgtk_binary_age, Vgtk_interface_age;

/* The application class of Emacs. */
Lisp_Object Vgtk_emacs_application_class;

Lisp_Object Vgtk_initial_argv_list; /* #### ugh! */
Lisp_Object Vgtk_initial_geometry;

static void gtk_device_init_x_specific_cruft (struct device *d);

static const struct memory_description gtk_device_data_description_1 [] = {
  { XD_LISP_OBJECT, offsetof (struct gtk_device, x_keysym_map_hashtable) },
  { XD_LISP_OBJECT, offsetof (struct gtk_device, WM_COMMAND_frame) },
  { XD_END }
};

#ifdef NEW_GC
DEFINE_DUMPABLE_INTERNAL_LISP_OBJECT ("gtk-device", gtk_device,
				      0, gtk_device_data_description_1,
				      Lisp_Gtk_Device);
#else /* not NEW_GC */
extern const struct sized_memory_description gtk_device_data_description;

const struct sized_memory_description gtk_device_data_description = {
  sizeof (struct gtk_device), gtk_device_data_description_1
};
#endif /* not NEW_GC */


/************************************************************************/
/*                          helper functions                            */
/************************************************************************/

struct device *
decode_gtk_device (Lisp_Object device)
{
  device = wrap_device (decode_device (device));
  CHECK_GTK_DEVICE (device);
  return XDEVICE (device);
}


/************************************************************************/
/*		      initializing a GTK connection			*/
/************************************************************************/
extern Lisp_Object
xemacs_gtk_convert_color(GdkColor *c, GtkWidget *w);

extern Lisp_Object __get_gtk_font_truename (PangoFont *font,
					    int expandp);

#define convert_font(f) __get_gtk_font_truename (f, 0)

static void
allocate_gtk_device_struct (struct device *d)
{
#ifdef NEW_GC
  d->device_data = XGTK_DEVICE (ALLOC_NORMAL_LISP_OBJECT (gtk_device));
#else /* not NEW_GC */
  d->device_data = xnew_and_zero (struct gtk_device);
#endif /* not NEW_GC */
  DEVICE_GTK_DATA (d)->x_keysym_map_hashtable = Qnil;
}

static void
gtk_init_device_class (struct device *d)
{
  if (DEVICE_GTK_DEPTH(d) > 2)
    {
#if GTK_CHECK_VERSION(2,22,1)
      GdkVisualType vtype = gdk_visual_get_visual_type (DEVICE_GTK_VISUAL(d));
#else
      GdkVisualType vtype = DEVICE_GTK_VISUAL(d)->type;
#endif

      switch (vtype)
	{
	case GDK_VISUAL_STATIC_GRAY:
	case GDK_VISUAL_GRAYSCALE:
	  DEVICE_CLASS (d) = Qgrayscale;
	  break;
	default:
	  DEVICE_CLASS (d) = Qcolor;
	}
    }
  else
    DEVICE_CLASS (d) = Qmono;
}

extern void emacs_gtk_selection_handle (GtkWidget *,
					GtkSelectionData *selection_data,
					guint info,
					guint time_stamp,
					gpointer data);
extern void emacs_gtk_selection_clear_event_handle (GtkWidget *widget,
                                                    GdkEventSelection *event,
                                                    gpointer data);
extern void emacs_gtk_selection_received (GtkWidget *widget,
					  GtkSelectionData *selection_data,
					  gpointer user_data);

DEFUN ("gtk-init", Fgtk_init, 1, 1, 0, /*
Initialize the GTK subsystem.
ARGS is a standard list of command-line arguments.

No effect if called more than once.  Called automatically when
creating the first GTK device.  Must be called manually from batch
mode.
*/
       (args))
{
  int argc;
  char **argv;
  static int done;

  if (done)
    {
      return (Qt);
    }

  make_argc_argv (args, &argc, &argv);

  slow_down_interrupts ();
  /* Turn Ubuntu overlay scrollbars off.  They don't have per-line scrolling. */
  setenv("LIBOVERLAY_SCROLLBAR", "0", 0);
  /* Turn of Ubuntu Unity title bar menu.  We don't handle it properly. */
  setenv("UBUNTU_MENUPROXY", "0", 0);

  gtk_init (&argc, &argv);

  /* Sigh, gtk_init stomped on LC_NUMERIC, which we need to be C. Otherwise
     the Lisp reader doesn't necessarily understand the radix character for
     floats, which is a problem. */
  setlocale (LC_NUMERIC, "C");

  speed_up_interrupts ();

  free_argc_argv (argv);
  done = 1;
  return (Qt);
}

static void
gtk_init_device (struct device *d, Lisp_Object UNUSED (props))
{
  Lisp_Object display;
  GtkWidget *app_shell = NULL;
  GdkVisual *visual = NULL;

  /* Run the early elisp side of the GTK device initialization. */
  call0 (Qmake_device_early_gtk_entry_point);

  /* gtk_init() and even gtk_check_init() are so brain dead that
     getting an empty argv array causes them to abort. */
  if (NILP (Vgtk_initial_argv_list))
    {
      invalid_operation ("gtk-initial-argv-list must be set before creating Gtk devices", Vgtk_initial_argv_list);
      return;
    }

  allocate_gtk_device_struct (d);
  display = DEVICE_CONNECTION (d);
  /* gtk_init loads these files in Gtk 3, I think -jsparkes */
#ifdef HAVE_GTK2
  /* Attempt to load a site-specific gtkrc */
  {
    Lisp_Object gtkrc = Fexpand_file_name (build_ascstring ("gtkrc"), Vdata_directory);
    gchar **default_files = gtk_rc_get_default_files ();
    gint num_files;

    if (STRINGP (gtkrc))
      {
	/* Found one, load it up! */
	gchar **new_rc_files = NULL;
	int ctr;

	for (num_files = 0; default_files[num_files]; num_files++);

	new_rc_files = xnew_array_and_zero (gchar *, num_files + 3);

	LISP_PATHNAME_CONVERT_OUT (gtkrc, new_rc_files[0]);

	for (ctr = 1; default_files[ctr-1]; ctr++)
	  new_rc_files[ctr] = g_strdup (default_files[ctr-1]);

	gtk_rc_set_default_files (new_rc_files);

	for (ctr = 1; new_rc_files[ctr]; ctr++)
	  free(new_rc_files[ctr]);

	xfree (new_rc_files);
      }
  }
#endif
  Fgtk_init (Vgtk_initial_argv_list);

#ifdef __FreeBSD__
  gdk_set_use_xshm (FALSE);
#endif

  if (NILP (DEVICE_NAME (d)))
    DEVICE_NAME (d) = display;

  /* Always search for the best visual */
#if GTK_CHECK_VERSION(2, 8, 0)
  visual = gdk_screen_get_rgba_visual (gdk_screen_get_default ());
#endif

  if (NULL == visual)
    {
      visual = gdk_visual_get_best();
    }


  DEVICE_GTK_VISUAL (d) = visual;
#if GTK_CHECK_VERSION(2,22,1)
  DEVICE_GTK_DEPTH (d) = gdk_visual_get_depth (visual);
#else
  DEVICE_GTK_DEPTH (d) = visual->depth;
#endif

  {
    GtkWidget *w = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name (w, "XEmacs");
    app_shell = gtk_xemacs_new (NULL);
    gtk_widget_set_name (app_shell, "shell");
    gtk_container_add (GTK_CONTAINER (w), app_shell);

    gtk_widget_realize (w);
    {
      PangoContext *context = 0;
      PangoFontMap *font_map = 0;
      Display *disp = GDK_DISPLAY_XDISPLAY (gtk_widget_get_display (w));
      int screen = GDK_SCREEN_XNUMBER (gtk_widget_get_screen (w));

      font_map = pango_xft_get_font_map (disp, screen);
      DEVICE_GTK_FONT_MAP (d) = font_map;
      context = pango_font_map_create_context (font_map);
      DEVICE_GTK_CONTEXT (d) = context;
    }
  }

  DEVICE_GTK_APP_SHELL (d) = app_shell;

  /* Realize the app_shell so that its window exists for GC creation
     purposes */
  gtk_widget_realize (GTK_WIDGET (app_shell));

  /* Set up the selection handlers. I attempted just to register the handler
     for TARGETS, and this works in that the requestor does see our offered
     list of targets (GTK gets out of the way for this), but then further
     attempts to transfer COMPOUND_TEXT and so on fail, because GTK
     interposes itself, and ignores that we've demonstrated we know what
     formats we can transfer by sending TARGETS.

     Note that under X11 the cars of selection-converter-out-alist can
     usefully be modified at runtime, making fewer or more selection types
     available; this isn't the case under GTK, the set is examined once at
     startup. */
  {
    Lisp_Object tcount
      = Flist_length (Vselection_converter_out_alist);
    guint target_count = NILP (tcount) ? 0 : XFIXNUM (tcount);
    GtkTargetEntry *targets = alloca_array (GtkTargetEntry, target_count);
    Lisp_Object tail = Vselection_converter_out_alist;
    DECLARE_EISTRING(ei_symname);
    guint ii;

    for (ii = 0; ii < target_count; ii++)
      {
        targets[ii].flags = 0;
        /* We don't use info at the moment. */
        targets[ii].info = ii;
        if (CONSP (Fcar (tail)) && SYMBOLP (XCAR (XCAR (tail))))
          {
            eicpy_lstr (ei_symname, XSYMBOL_NAME (XCAR (XCAR (tail))));
            /* GTK doesn't specify the encoding of their atom names. */
            eito_external (ei_symname, Qbinary);
            targets[ii].target = alloca_array (gchar, eiextlen (ei_symname)
                                               + 1);
            memcpy ((void *) (targets[ii].target),
                    (void *) eiextdata (ei_symname), eiextlen (ei_symname) + 1);
          }
        else
          {
            if (ii > 0xFF)
              {
                /* It was corrupt long before ii > 0xff, of course. */
                gui_error ("selection-converter-out-alist is corrupt",
                           Vselection_converter_out_alist);
              }
            targets[ii].target = alloca_array (gchar, sizeof ("TARGETFF"));
            sprintf (targets[ii].target, "TARGET%02X", ii);
          }
        tail = Fcdr (tail);
      }

    gtk_selection_add_targets (GTK_WIDGET (app_shell), GDK_SELECTION_PRIMARY,
                               targets, target_count);
    gtk_selection_add_targets (GTK_WIDGET (app_shell), GDK_SELECTION_SECONDARY,
                               targets, target_count);
    gtk_selection_add_targets (GTK_WIDGET (app_shell), GDK_SELECTION_CLIPBOARD,
                               targets, target_count);
  }
  
  g_signal_connect (G_OBJECT (app_shell), "selection_get",
                    G_CALLBACK (emacs_gtk_selection_handle), NULL);
  g_signal_connect (G_OBJECT (app_shell), "selection_clear_event",
                    G_CALLBACK (emacs_gtk_selection_clear_event_handle),
                    NULL);
  g_signal_connect (G_OBJECT (app_shell), "selection_received",
                    G_CALLBACK (emacs_gtk_selection_received), NULL);

  DEVICE_GTK_WM_COMMAND_FRAME (d) = Qnil;

  gtk_init_modifier_mapping (d);

  gtk_device_init_x_specific_cruft (d);

  init_baud_rate (d);
  init_one_device (d);

  gtk_init_device_class (d);
}

static void
gtk_finish_init_device (struct device *d, Lisp_Object UNUSED (props))
{
  call1 (Qmake_device_late_gtk_entry_point, wrap_device(d));
}

static void
gtk_mark_device (struct device *d)
{
  mark_object (DEVICE_GTK_WM_COMMAND_FRAME (d));
  mark_object (DEVICE_GTK_DATA (d)->x_keysym_map_hashtable);
}


/************************************************************************/
/*                       closing an X connection	                */
/************************************************************************/

#ifndef NEW_GC
static void
free_gtk_device_struct (struct device *d)
{
  //xfree (DEVICE_GTK_DATA (d));
  xfree (d->device_data);
  d->device_data = 0;
}
#endif /* not NEW_GC */

static void
gtk_delete_device (struct device *d)
{
#ifdef FREE_CHECKING
  extern void (*__free_hook)();
  int checking_free;
#endif

  if (1)
    {
#ifdef FREE_CHECKING
      checking_free = (__free_hook != 0);

      /* Disable strict free checking, to avoid bug in X library */
      if (checking_free)
	disable_strict_free_check ();
#endif

#ifdef FREE_CHECKING
      if (checking_free)
	enable_strict_free_check ();
#endif
    }
  /* g_free(DEVICE_GTK_CONTEXT (d)); */
#ifndef NEW_GC
  free_gtk_device_struct (d);
#endif
}


/************************************************************************/
/*				handle X errors				*/
/************************************************************************/

const char *
gtk_event_name (GdkEventType event_type)
{

#define GET_EVENT_NAME(ev) case ev: return #ev;

  switch (event_type)
  {
    GET_EVENT_NAME (GDK_NOTHING);
    GET_EVENT_NAME (GDK_DELETE);
    GET_EVENT_NAME (GDK_DESTROY);
    GET_EVENT_NAME (GDK_EXPOSE);
    GET_EVENT_NAME (GDK_MOTION_NOTIFY);
    GET_EVENT_NAME (GDK_BUTTON_PRESS);
    GET_EVENT_NAME (GDK_2BUTTON_PRESS);
    GET_EVENT_NAME (GDK_3BUTTON_PRESS);
    GET_EVENT_NAME (GDK_BUTTON_RELEASE);
    GET_EVENT_NAME (GDK_KEY_PRESS);
    GET_EVENT_NAME (GDK_KEY_RELEASE);
    GET_EVENT_NAME (GDK_ENTER_NOTIFY);
    GET_EVENT_NAME (GDK_LEAVE_NOTIFY);
    GET_EVENT_NAME (GDK_FOCUS_CHANGE);
    GET_EVENT_NAME (GDK_CONFIGURE);
    GET_EVENT_NAME (GDK_MAP);
    GET_EVENT_NAME (GDK_UNMAP);
    GET_EVENT_NAME (GDK_PROPERTY_NOTIFY);
    GET_EVENT_NAME (GDK_SELECTION_CLEAR);
    GET_EVENT_NAME (GDK_SELECTION_REQUEST);
    GET_EVENT_NAME (GDK_SELECTION_NOTIFY);
    GET_EVENT_NAME (GDK_PROXIMITY_IN);
    GET_EVENT_NAME (GDK_PROXIMITY_OUT);
    GET_EVENT_NAME (GDK_DRAG_ENTER);
    GET_EVENT_NAME (GDK_DRAG_LEAVE);
    GET_EVENT_NAME (GDK_DRAG_MOTION);
    GET_EVENT_NAME (GDK_DRAG_STATUS);
    GET_EVENT_NAME (GDK_DROP_START);
    GET_EVENT_NAME (GDK_DROP_FINISHED);
    GET_EVENT_NAME (GDK_CLIENT_EVENT);
    GET_EVENT_NAME (GDK_VISIBILITY_NOTIFY);
#ifdef HAVE_GTK2
    GET_EVENT_NAME (GDK_NO_EXPOSE);
#endif
    GET_EVENT_NAME (GDK_SCROLL);
    GET_EVENT_NAME (GDK_WINDOW_STATE);
    GET_EVENT_NAME (GDK_SETTING);
    GET_EVENT_NAME (GDK_OWNER_CHANGE);
    GET_EVENT_NAME (GDK_GRAB_BROKEN);
    GET_EVENT_NAME (GDK_DAMAGE);
    /* Not useful, but clang warns about missing enumeration value. */
    GET_EVENT_NAME (GDK_EVENT_LAST);
  }
#undef GET_EVENT_NAME
  return "Unknown GdkEventType";
}


/************************************************************************/
/*                   display information functions                      */
/************************************************************************/

DEFUN ("gtk-display-visual-class", Fgtk_display_visual_class, 0, 1, 0, /*
Return the visual class of the GTK display DEVICE is using.
The returned value will be one of the symbols `static-gray', `gray-scale',
`static-color', `pseudo-color', `true-color', or `direct-color'.
*/
       (device))
{
  GdkVisual *vis = DEVICE_GTK_VISUAL (decode_gtk_device (device));
 #if GTK_CHECK_VERSION(2,22,1)
   GdkVisualType type = gdk_visual_get_visual_type (vis);
 #else
   GdkVisualType type = vis->type;
 #endif
  switch (type)
    {
    case GDK_VISUAL_STATIC_GRAY:  return intern ("static-gray");
    case GDK_VISUAL_GRAYSCALE:    return intern ("gray-scale");
    case GDK_VISUAL_STATIC_COLOR: return intern ("static-color");
    case GDK_VISUAL_PSEUDO_COLOR: return intern ("pseudo-color");
    case GDK_VISUAL_TRUE_COLOR:   return intern ("true-color");
    case GDK_VISUAL_DIRECT_COLOR: return intern ("direct-color");
    default:
      invalid_state ("display has an unknown visual class", Qunbound);
      return Qnil;	/* suppress compiler warning */
    }
}

DEFUN ("gtk-display-visual-depth", Fgtk_display_visual_depth, 0, 1, 0, /*
Return the bitplane depth of the visual the GTK display DEVICE is using.
*/
       (device))
{
   return make_fixnum (DEVICE_GTK_DEPTH (decode_gtk_device (device)));
}

static Lisp_Object
gtk_device_system_metrics (struct device *d,
			   enum device_metrics m)
{
#if 0
  GtkStyle *style = gtk_widget_get_style (GTK_WIDGET (DEVICE_GTK_APP_SHELL (d)));

  style = gtk_style_attach (style, w);
#endif

  switch (m)
    {
    case DM_size_device:
      return Fcons (make_fixnum (gdk_screen_width ()),
		    make_fixnum (gdk_screen_height ()));
    case DM_size_device_mm:
      return Fcons (make_fixnum (gdk_screen_width_mm ()),
		    make_fixnum (gdk_screen_height_mm ()));
    case DM_num_color_cells:
#if GTK_CHECK_VERSION(2,22,1)
      return make_fixnum (gdk_visual_get_colormap_size (DEVICE_GTK_VISUAL (d)));
#else
      return make_fixnum (gdk_colormap_get_system_size ());
#endif
    case DM_num_bit_planes:
      return make_fixnum (DEVICE_GTK_DEPTH (d));

#if 0
    case DM_color_default:
    case DM_color_select:
    case DM_color_balloon:
    case DM_color_3d_face:
    case DM_color_3d_light:
    case DM_color_3d_dark:
    case DM_color_menu:
    case DM_color_menu_highlight:
    case DM_color_menu_button:
    case DM_color_menu_disabled:
    case DM_color_toolbar:
    case DM_color_scrollbar:
    case DM_color_desktop:
    case DM_color_workspace:
    case DM_font_default:
    case DM_font_menubar:
    case DM_font_dialog:
    case DM_size_cursor:
    case DM_size_scrollbar:
    case DM_size_menu:
    case DM_size_toolbar:
    case DM_size_toolbar_button:
    case DM_size_toolbar_border:
    case DM_size_icon:
    case DM_size_icon_small:
    case DM_size_workspace:
    case DM_device_dpi:
    case DM_mouse_buttons:
    case DM_swap_buttons:
    case DM_show_sounds:
    case DM_slow_device:
    case DM_security:
#endif
    default: /* No such device metric property for GTK devices  */
      return Qunbound;
    }
}

DEFUN ("gtk-keysym-on-keyboard-p", Fgtk_keysym_on_keyboard_p, 1, 2, 0, /*
Return true if KEYSYM names a key on the keyboard of DEVICE.
More precisely, return true if some keystroke (possibly including modifiers)
on the keyboard of DEVICE keys generates KEYSYM.
Valid keysyms are listed in the files /usr/include/X11/keysymdef.h and in
/usr/lib/X11/XKeysymDB, or whatever the equivalents are on your system.
The keysym name can be provided in two forms:
- if keysym is a string, it must be the name as known to X windows.
- if keysym is a symbol, it must be the name as known to XEmacs.
The two names differ in capitalization and underscoring.
*/
       (keysym, device))
{
  struct device *d = decode_device (device);

  if (!DEVICE_GTK_P (d))
    gui_error ("Not a GTK device", device);

  return (NILP (Fgethash (keysym, DEVICE_GTK_DATA (d)->x_keysym_map_hashtable, Qnil)) ?
	  Qnil : Qt);
}


/************************************************************************/
/*                          grabs and ungrabs                           */
/************************************************************************/

DEFUN ("gtk-grab-pointer", Fgtk_grab_pointer, 0, 3, 0, /*
Grab the pointer and restrict it to its current window.
If optional DEVICE argument is nil, the default device will be used.
If optional CURSOR argument is non-nil, change the pointer shape to that
 until `gtk-ungrab-pointer' is called (it should be an object returned by the
 `make-cursor-glyph' function).
If the second optional argument IGNORE-KEYBOARD is non-nil, ignore all
  keyboard events during the grab.
Returns t if the grab is successful, nil otherwise.
*/
       (device, cursor, UNUSED (ignore_keyboard)))
{
  GdkWindow *w;
  int result = -1;
  struct device *d = decode_gtk_device (device);

  if (!NILP (cursor))
    {
      CHECK_POINTER_GLYPH (cursor);
      cursor = glyph_image_instance (cursor, device, ERROR_ME, 0);
    }

  /* We should call gdk_pointer_grab() and (possibly) gdk_keyboard_grab() here instead */
  w = gtk_widget_get_window (FRAME_GTK_TEXT_WIDGET (device_selected_frame (d)));
  assert (w);

#ifdef HAVE_GTK2
  result = gdk_pointer_grab (w, FALSE,
			     (GdkEventMask) (GDK_POINTER_MOTION_MASK |
					     GDK_POINTER_MOTION_HINT_MASK |
					     GDK_BUTTON1_MOTION_MASK |
					     GDK_BUTTON2_MOTION_MASK |
					     GDK_BUTTON3_MOTION_MASK |
					     GDK_BUTTON_PRESS_MASK |
					     GDK_BUTTON_RELEASE_MASK),
			     w,
			     NULL, /* #### BILL!!! Need to create a GdkCursor * as necessary! */
			     GDK_CURRENT_TIME);
#endif
#ifdef HAVE_GTK3
  {
    GtkWidget *widget = FRAME_GTK_TEXT_WIDGET (device_selected_frame (d));
    GdkDevice *gdk_device = gtk_widget_get_device (widget);

    assert (gdk_device);
    result = gdk_device_grab (gdk_device, w,
			      GDK_OWNERSHIP_APPLICATION, FALSE,
			      (GdkEventMask) (GDK_POINTER_MOTION_MASK |
					      GDK_POINTER_MOTION_HINT_MASK |
					      GDK_BUTTON1_MOTION_MASK |
					      GDK_BUTTON2_MOTION_MASK |
					      GDK_BUTTON3_MOTION_MASK |
					      GDK_BUTTON_PRESS_MASK |
					      GDK_BUTTON_RELEASE_MASK),
			      NULL, /* #### BILL!!! Need to create a GdkCursor * as necessary! */
			      GDK_CURRENT_TIME);
  }
#endif

  return (result == GDK_GRAB_SUCCESS) ? Qt : Qnil;
}

DEFUN ("gtk-ungrab-pointer", Fgtk_ungrab_pointer, 0, 1, 0, /*
Release a pointer grab made with `gtk-grab-pointer'.
If optional first arg DEVICE is nil the default device is used.
If it is t the pointer will be released on all GTK devices.
*/
       (device))
{
  if (!EQ (device, Qt))
    {
#ifdef HAVE_GTK2
      gdk_pointer_ungrab (GDK_CURRENT_TIME);
#endif
#ifdef HAVE_GTK3
      struct device *d = decode_gtk_device (device);
      /* struct device *d = XDEVICE (XCAR (device)); */
      GtkWidget *widget = FRAME_GTK_TEXT_WIDGET (device_selected_frame (d));
      GdkDevice *gdk_device = gtk_widget_get_device (widget);

      gdk_device_ungrab (gdk_device, GDK_CURRENT_TIME);
#endif
    }
  else
    {
      Lisp_Object devcons, concons;

      DEVICE_LOOP_NO_BREAK (devcons, concons)
	{
	  struct device *d = XDEVICE (XCAR (devcons));

	  if (DEVICE_GTK_P (d))
	    {
#ifdef HAVE_GTK2
	      gdk_pointer_ungrab (GDK_CURRENT_TIME);
#endif
#ifdef HAVE_GTK3
	      GtkWidget *widget = FRAME_GTK_TEXT_WIDGET (device_selected_frame (d));
	      GdkDevice *gdk_device = gtk_widget_get_device (widget);
	      gdk_device_ungrab (gdk_device, GDK_CURRENT_TIME);
#endif
	    }
	}
    }
  return Qnil;
}

DEFUN ("gtk-grab-keyboard", Fgtk_grab_keyboard, 0, 1, 0, /*
Grab the keyboard on the given device (defaulting to the selected one).
So long as the keyboard is grabbed, all keyboard events will be delivered
to emacs -- it is not possible for other clients to eavesdrop on them.
Ungrab the keyboard with `gtk-ungrab-keyboard' (use an unwind-protect).
Returns t if the grab is successful, nil otherwise.
*/
       (device))
{
  struct device *d = decode_gtk_device (device);
  GdkWindow *w = gtk_widget_get_window (FRAME_GTK_TEXT_WIDGET (device_selected_frame (d)));

#ifdef HAVE_GTK2
  gdk_keyboard_grab (w, FALSE, GDK_CURRENT_TIME );
#endif

#ifdef HAVE_GTK3
  {
    GtkWidget *widget = FRAME_GTK_TEXT_WIDGET (device_selected_frame (d));
    GdkDevice *gdk_device = gtk_widget_get_device (widget);

    if (gdk_device)
      gdk_device_grab (gdk_device, w, GDK_OWNERSHIP_APPLICATION, FALSE,
		       (GdkEventMask) (GDK_POINTER_MOTION_MASK |
				       GDK_POINTER_MOTION_HINT_MASK |
				       GDK_BUTTON1_MOTION_MASK |
				       GDK_BUTTON2_MOTION_MASK |
				       GDK_BUTTON3_MOTION_MASK |
				       GDK_BUTTON_PRESS_MASK |
				       GDK_BUTTON_RELEASE_MASK |
				       GDK_KEY_PRESS),
		       NULL, /* #### BILL!!! Need to create a GdkCursor * as necessary! */
		       GDK_CURRENT_TIME);
  }
#endif
  return Qt;
}

DEFUN ("gtk-ungrab-keyboard", Fgtk_ungrab_keyboard, 0, 1, 0, /*
Release a keyboard grab made with `gtk-grab-keyboard'.
*/
       (device))
{
#ifdef HAVE_GTK2
  gdk_keyboard_ungrab (GDK_CURRENT_TIME);
#endif
#ifdef HAVE_GTK3
  struct device *d = decode_gtk_device (device);
  GtkWidget *widget = FRAME_GTK_TEXT_WIDGET (device_selected_frame (d));
  GdkDevice *gdk_device = gtk_widget_get_device (widget);

  gdk_device_ungrab (gdk_device, GDK_CURRENT_TIME);
#endif
  return Qnil;
}


/************************************************************************/
/*                              Style Info                              */
/************************************************************************/
DEFUN ("gtk-style-info", Fgtk_style_info, 0, 1, 0, /*
Get the style information for a Gtk device.
*/
       (device))
{
  struct device *d = decode_device (device);
  Lisp_Object result = Qnil;
#ifdef HAVE_GTK2
  GtkStyle *style = NULL;
  GtkWidget *app_shell = GTK_WIDGET (DEVICE_GTK_APP_SHELL (d));
  GdkWindow *w = gtk_widget_get_window (app_shell);

  if (!DEVICE_GTK_P (d))
    return (Qnil);

  style = gtk_widget_get_style (app_shell);
  style = gtk_style_attach (style, w);

  if (!style) return (Qnil);

#define FROB_COLOR(slot, name) \
 result = nconc2 (result, \
		list2 (intern (name), \
		list5 (xemacs_gtk_convert_color (&style->slot[GTK_STATE_NORMAL], app_shell),\
			xemacs_gtk_convert_color (&style->slot[GTK_STATE_ACTIVE], app_shell),\
			xemacs_gtk_convert_color (&style->slot[GTK_STATE_PRELIGHT], app_shell),\
			xemacs_gtk_convert_color (&style->slot[GTK_STATE_SELECTED], app_shell),\
			xemacs_gtk_convert_color (&style->slot[GTK_STATE_INSENSITIVE], app_shell))))

  FROB_COLOR (fg, "foreground");
  FROB_COLOR (bg, "background");
  FROB_COLOR (light, "light");
  FROB_COLOR (dark, "dark");
  FROB_COLOR (mid, "mid");
  FROB_COLOR (text, "text");
  FROB_COLOR (base, "base");
#undef FROB_COLOR

#ifdef USE_PANGO
  result = nconc2 (result, list2 (Qfont,
                                  build_cistring (pango_font_description_to_string (style->font_desc))
                                  /* convert_font (style->font_desc) */
                                  ));
#endif

#define FROB_PIXMAP(state) (style->rc_style->bg_pixmap_name[state] ? build_cistring (style->rc_style->bg_pixmap_name[state]) : Qnil)

  if (style->rc_style)
    result = nconc2 (result, list2 (Qbackground,
				    list5 ( FROB_PIXMAP (GTK_STATE_NORMAL),
					    FROB_PIXMAP (GTK_STATE_ACTIVE),
					    FROB_PIXMAP (GTK_STATE_PRELIGHT),
					    FROB_PIXMAP (GTK_STATE_SELECTED),
					    FROB_PIXMAP (GTK_STATE_INSENSITIVE))));
#undef FROB_PIXMAP
#endif

  return (result);
}

DEFUN ("gtk-load-css", Fgtk_load_css, 1, 1, 0, /*
Load a CSS FILE for styling widgets.
*/
       (file))
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GtkCssProvider *css_prov = gtk_css_provider_new ();
  GError *error = NULL;
  Extbyte *path = NULL;

  CHECK_STRING (file);

  path = LISP_STRING_TO_EXTERNAL (file, Qfile_name);
  gtk_css_provider_load_from_path (css_prov, path, &error);

  if (error == NULL)
    {
      gtk_style_context_add_provider_for_screen(gdk_screen_get_default (),
                                                GTK_STYLE_PROVIDER (css_prov),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
  else
    {
      if (css_prov != NULL)
        g_object_unref (css_prov);
      /* TODO put error message in here. */
      gui_error ("Error loading CSS file", file);
    }

  if (css_prov != NULL)
    g_object_unref (css_prov);
  return Qt;
#else
  return Qnil;
#endif
}


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_device_gtk (void)
{
#ifdef NEW_GC
  INIT_LISP_OBJECT (gtk_device);
#endif /* NEW_GC */

  DEFSUBR (Fgtk_keysym_on_keyboard_p);
  DEFSUBR (Fgtk_display_visual_class);
  DEFSUBR (Fgtk_display_visual_depth);
  DEFSUBR (Fgtk_style_info);
  DEFSUBR (Fgtk_grab_pointer);
  DEFSUBR (Fgtk_ungrab_pointer);
  DEFSUBR (Fgtk_grab_keyboard);
  DEFSUBR (Fgtk_ungrab_keyboard);
  DEFSUBR (Fgtk_init);
  DEFSUBR (Fgtk_load_css);

  DEFSYMBOL (Qmake_device_early_gtk_entry_point);
  DEFSYMBOL (Qmake_device_late_gtk_entry_point);
}

void
console_type_create_device_gtk (void)
{
  CONSOLE_HAS_METHOD (gtk, init_device);
  CONSOLE_HAS_METHOD (gtk, finish_init_device);
  CONSOLE_HAS_METHOD (gtk, mark_device);
  CONSOLE_HAS_METHOD (gtk, delete_device);
  CONSOLE_HAS_METHOD (gtk, device_system_metrics);
  /* CONSOLE_IMPLEMENTATION_FLAGS (gtk, XDEVIMPF_PIXEL_GEOMETRY); */
  /* I inserted the above commented out statement, as the original
     implementation of gtk_device_implementation_flags(), which I
     deleted, contained commented out XDEVIMPF_PIXEL_GEOMETRY - kkm*/
}

void
vars_of_device_gtk (void)
{
  Ibyte *version = alloca_ibytes (128);

  Fprovide (Qgtk);

  DEFVAR_LISP ("gtk-version", &Vgtk_version /*
GTK version string.
*/ );
#ifdef HAVE_GTK2
  qxesprintf (version, "%d.%d.%d", GTK_MAJOR_VERSION, GTK_MINOR_VERSION,
	      GTK_MICRO_VERSION);
#endif
#ifdef HAVE_GTK3
  qxesprintf (version, "%d.%d.%d", gtk_get_major_version(), gtk_get_minor_version(),
	      gtk_get_micro_version());
#endif
  Vgtk_version = build_istring (version);

  DEFVAR_LISP ("gtk-major-version", &Vgtk_major_version /*
GTK major version as integer.
*/ );
#ifdef HAVE_GTK2
  Vgtk_major_version = make_unsigned_integer (GTK_MAJOR_VERSION);
#endif
#ifdef HAVE_GTK3
  Vgtk_major_version = make_unsigned_integer (gtk_get_major_version());
#endif

  DEFVAR_LISP ("gtk-minor-version", &Vgtk_minor_version /*
GTK minor version as integer.
*/ );
#ifdef HAVE_GTK2
  Vgtk_minor_version = make_unsigned_integer (GTK_MINOR_VERSION);
#endif
#ifdef HAVE_GTK3
  Vgtk_minor_version = make_unsigned_integer (gtk_get_minor_version());
#endif

  DEFVAR_LISP ("gtk-micro-version", &Vgtk_micro_version /*
GTK micro version as integer.
*/ );
#ifdef HAVE_GTK2
  Vgtk_micro_version = make_unsigned_integer (GTK_MICRO_VERSION);
#endif
#ifdef HAVE_GTK3
  Vgtk_micro_version = make_unsigned_integer (gtk_get_micro_version());
#endif

  DEFVAR_LISP ("gtk-binary-age", &Vgtk_binary_age /*
GTK binary age as integer.
*/ );
#ifdef HAVE_GTK2
  Vgtk_binary_age = make_unsigned_integer (GTK_BINARY_AGE);
#endif
#ifdef HAVE_GTK3
  Vgtk_binary_age = make_unsigned_integer (gtk_get_binary_age());
#endif

  DEFVAR_LISP ("gtk-interface-age", &Vgtk_interface_age /*
GTK interface age as integer.
*/ );
#ifdef HAVE_GTK2
  Vgtk_interface_age = make_unsigned_integer (GTK_INTERFACE_AGE);
#endif
#ifdef HAVE_GTK3
  Vgtk_interface_age = make_unsigned_integer (gtk_get_interface_age());
#endif

  DEFVAR_LISP ("gtk-initial-argv-list", &Vgtk_initial_argv_list /*
You don't want to know.
This is used during startup to communicate the remaining arguments in
`command-line-args-left' to the C code, which passes the args to
the GTK initialization code, which removes some args, and then the
args are placed back into `gtk-initial-arg-list' and thence into
`command-line-args-left'.  Perhaps `command-line-args-left' should
just reside in C.
*/ );

  DEFVAR_LISP ("gtk-initial-geometry", &Vgtk_initial_geometry /*
You don't want to know.
This is used during startup to communicate the default geometry to GTK.
*/ );

  Vgtk_initial_geometry = Qnil;
  Vgtk_initial_argv_list = Qnil;
}

#include "sysgdkx.h"

static void
gtk_device_init_x_specific_cruft (struct device *d)
{
  DEVICE_INFD (d) = DEVICE_OUTFD (d)
    = ConnectionNumber (GDK_WINDOW_XDISPLAY (gtk_widget_get_window
                                             (DEVICE_GTK_APP_SHELL (d))));
}
