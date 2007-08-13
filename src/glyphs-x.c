/* X-specific Lisp objects.
   Copyright (C) 1993, 1994 Free Software Foundation, Inc.
   Copyright (C) 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1995 Tinker Systems
   Copyright (C) 1995, 1996 Ben Wing
   Copyright (C) 1995 Sun Microsystems
   Copyright (C) 1999 Andy Piper

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

/* Original author: Jamie Zawinski for 19.8
   font-truename stuff added by Jamie Zawinski for 19.10
   subwindow support added by Chuck Thompson
   additional XPM support added by Chuck Thompson
   initial X-Face support added by Stig
   rewritten/restructured by Ben Wing for 19.12/19.13
   GIF/JPEG support added by Ben Wing for 19.14
   PNG support added by Bill Perry for 19.14
   Improved GIF/JPEG support added by Bill Perry for 19.14
   Cleanup/simplification of error handling by Ben Wing for 19.14
   Pointer/icon overhaul, more restructuring by Ben Wing for 19.14
   GIF support changed to external GIFlib 3.1 by Jareth Hein for 21.0
   Many changes for color work and optimizations by Jareth Hein for 21.0
   Switch of GIF/JPEG/PNG to new EImage intermediate code by Jareth Hein for 21.0
   TIFF code by Jareth Hein for 21.0
   GIF/JPEG/PNG/TIFF code moved to new glyph-eimage.c for 21.0

   TODO:
   Convert images.el to C and stick it in here?
 */

#include <config.h>
#include "lisp.h"
#include "lstream.h"
#include "console-x.h"
#include "glyphs-x.h"
#include "objects-x.h"
#include "gui-x.h"
#include "xmu.h"

#include "buffer.h"
#include "window.h"
#include "frame.h"
#include "insdel.h"
#include "opaque.h"
#include "gui.h"
#include "faces.h"

#include "imgproc.h"

#include "sysfile.h"

#include <setjmp.h>

#ifdef FILE_CODING
#include "file-coding.h"
#endif

#ifdef LWLIB_USES_MOTIF
#include <Xm/Xm.h>
#endif
#include <X11/IntrinsicP.h>

#if INTBITS == 32
# define FOUR_BYTE_TYPE unsigned int
#elif LONGBITS == 32
# define FOUR_BYTE_TYPE unsigned long
#elif SHORTBITS == 32
# define FOUR_BYTE_TYPE unsigned short
#else
#error What kind of strange-ass system are we running on?
#endif

#define LISP_DEVICE_TO_X_SCREEN(dev) XDefaultScreenOfDisplay (DEVICE_X_DISPLAY (XDEVICE (dev)))

DECLARE_IMAGE_INSTANTIATOR_FORMAT (nothing);
DECLARE_IMAGE_INSTANTIATOR_FORMAT (string);
DECLARE_IMAGE_INSTANTIATOR_FORMAT (formatted_string);
DECLARE_IMAGE_INSTANTIATOR_FORMAT (inherit);
#ifdef HAVE_JPEG
DECLARE_IMAGE_INSTANTIATOR_FORMAT (jpeg);
#endif
#ifdef HAVE_TIFF
DECLARE_IMAGE_INSTANTIATOR_FORMAT (tiff);
#endif  
#ifdef HAVE_PNG
DECLARE_IMAGE_INSTANTIATOR_FORMAT (png);
#endif  
#ifdef HAVE_GIF
DECLARE_IMAGE_INSTANTIATOR_FORMAT (gif);
#endif  
#ifdef HAVE_XPM
DEFINE_DEVICE_IIFORMAT (x, xpm);
#endif
DEFINE_DEVICE_IIFORMAT (x, xbm);
DEFINE_DEVICE_IIFORMAT (x, subwindow);
#ifdef HAVE_XFACE
DEFINE_DEVICE_IIFORMAT (x, xface);
#endif

DEFINE_IMAGE_INSTANTIATOR_FORMAT (cursor_font);
Lisp_Object Qcursor_font;

DEFINE_IMAGE_INSTANTIATOR_FORMAT (font);

DEFINE_IMAGE_INSTANTIATOR_FORMAT (autodetect);

DEFINE_DEVICE_IIFORMAT (x, widget);
DEFINE_DEVICE_IIFORMAT (x, button);
DEFINE_DEVICE_IIFORMAT (x, progress_gauge);
DEFINE_DEVICE_IIFORMAT (x, edit_field);
DEFINE_DEVICE_IIFORMAT (x, combo_box);

static void cursor_font_instantiate (Lisp_Object image_instance,
				     Lisp_Object instantiator,
				     Lisp_Object pointer_fg,
				     Lisp_Object pointer_bg,
				     int dest_mask,
				     Lisp_Object domain);

#include "bitmaps.h"


/************************************************************************/
/*                      image instance methods                          */
/************************************************************************/

/************************************************************************/
/* convert from a series of RGB triples to an XImage formated for the   */
/* proper display 							*/
/************************************************************************/
static XImage *
convert_EImage_to_XImage (Lisp_Object device, int width, int height,
			  unsigned char *pic, unsigned long **pixtbl,
			  int *npixels)
{
  Display *dpy;
  Colormap cmap;
  Visual *vis;
  XImage *outimg;
  int depth, bitmap_pad, bits_per_pixel, byte_cnt, i, j;
  int rd,gr,bl,q;
  unsigned char *data, *ip, *dp;
  quant_table *qtable = 0;
  union {
    FOUR_BYTE_TYPE val;
    char cp[4];
  } conv;

  dpy = DEVICE_X_DISPLAY (XDEVICE (device));
  cmap = DEVICE_X_COLORMAP (XDEVICE(device));
  vis = DEVICE_X_VISUAL (XDEVICE(device));
  depth = DEVICE_X_DEPTH(XDEVICE(device));

  if (vis->class == PseudoColor)
    {
      /* Quantize the image and get a histogram while we're at it.
	 Do this first to save memory */
      qtable = build_EImage_quantable(pic, width, height, 256);
      if (qtable == NULL) return NULL;
    }

  bitmap_pad = ((depth > 16) ? 32 :
		(depth >  8) ? 16 :
		8);

  outimg = XCreateImage (dpy, vis,
			 depth, ZPixmap, 0, 0, width, height,
			 bitmap_pad, 0);
  if (!outimg) return NULL;

  bits_per_pixel = outimg->bits_per_pixel;
  byte_cnt = bits_per_pixel >> 3;

  data = (unsigned char *) xmalloc (outimg->bytes_per_line * height);
  if (!data)
    {
      XDestroyImage (outimg);
      return NULL;
    }
  outimg->data = (char *) data;

  if (vis->class == PseudoColor)
    {
      unsigned long pixarray[256];
      int pixcount, n;
      /* use our quantize table to allocate the colors */
      pixcount = 32;
      *pixtbl = xnew_array (unsigned long, pixcount);
      *npixels = 0;

      /* ### should implement a sort by popularity to assure proper allocation */
      n = *npixels;
      for (i = 0; i < qtable->num_active_colors; i++)
	{
	  XColor color;
	  int res;

	  color.red = qtable->rm[i] ? qtable->rm[i] << 8 : 0;
	  color.green = qtable->gm[i] ? qtable->gm[i] << 8 : 0;
	  color.blue = qtable->bm[i] ? qtable->bm[i] << 8 : 0;
	  color.flags = DoRed | DoGreen | DoBlue;
	  res = allocate_nearest_color (dpy, cmap, vis, &color);
	  if (res > 0 && res < 3)
	    {
	      DO_REALLOC(*pixtbl, pixcount, n+1, unsigned long);
	      (*pixtbl)[n] = color.pixel;
	      n++;
	    }
	  pixarray[i] = color.pixel;
	}
      *npixels = n;
      ip = pic;
      for (i = 0; i < height; i++)
	{
	  dp = data + (i * outimg->bytes_per_line);
	  for (j = 0; j < width; j++)
	    {
	      rd = *ip++;
	      gr = *ip++;
	      bl = *ip++;
	      conv.val = pixarray[QUANT_GET_COLOR(qtable,rd,gr,bl)];
#if WORDS_BIGENDIAN
	      if (outimg->byte_order == MSBFirst)
		for (q = 4-byte_cnt; q < 4; q++) *dp++ = conv.cp[q];
	      else
		for (q = 3; q >= 4-byte_cnt; q--) *dp++ = conv.cp[q];
#else
	      if (outimg->byte_order == MSBFirst)
		for (q = byte_cnt-1; q >= 0; q--) *dp++ = conv.cp[q];
	      else
		for (q = 0; q < byte_cnt; q++) *dp++ = conv.cp[q];
#endif
	    }
	}
      xfree(qtable);
    } else {
      unsigned long rshift,gshift,bshift,rbits,gbits,bbits,junk;
      junk = vis->red_mask;
      rshift = 0;
      while ((junk & 0x1) == 0)
	{
	  junk = junk >> 1;
	  rshift ++;
	}
      rbits = 0;
      while (junk != 0)
	{
	  junk = junk >> 1;
	  rbits++;
	}
      junk = vis->green_mask;
      gshift = 0;
      while ((junk & 0x1) == 0)
	{
	  junk = junk >> 1;
	  gshift ++;
	}
      gbits = 0;
      while (junk != 0)
	{
	  junk = junk >> 1;
	  gbits++;
	}
      junk = vis->blue_mask;
      bshift = 0;
      while ((junk & 0x1) == 0)
	{
	  junk = junk >> 1;
	  bshift ++;
	}
      bbits = 0;
      while (junk != 0)
	{
	  junk = junk >> 1;
	  bbits++;
	}
      ip = pic;
      for (i = 0; i < height; i++)
	{
	  dp = data + (i * outimg->bytes_per_line);
	  for (j = 0; j < width; j++)
	    {
	      if (rbits > 8)
		rd = *ip++ << (rbits - 8);
	      else
		rd = *ip++ >> (8 - rbits);
	      if (gbits > 8)
		gr = *ip++ << (gbits - 8);
	      else
		gr = *ip++ >> (8 - gbits);
	      if (bbits > 8)
		bl = *ip++ << (bbits - 8);
	      else
		bl = *ip++ >> (8 - bbits);

	      conv.val = (rd << rshift) | (gr << gshift) | (bl << bshift);
#if WORDS_BIGENDIAN
	      if (outimg->byte_order == MSBFirst)
		for (q = 4-byte_cnt; q < 4; q++) *dp++ = conv.cp[q];
	      else
		for (q = 3; q >= 4-byte_cnt; q--) *dp++ = conv.cp[q];
#else
	      if (outimg->byte_order == MSBFirst)
		for (q = byte_cnt-1; q >= 0; q--) *dp++ = conv.cp[q];
	      else
		for (q = 0; q < byte_cnt; q++) *dp++ = conv.cp[q];
#endif
	    }
	}
    }
  return outimg;
}



static void
x_print_image_instance (struct Lisp_Image_Instance *p,
			Lisp_Object printcharfun,
			int escapeflag)
{
  char buf[100];

  switch (IMAGE_INSTANCE_TYPE (p))
    {
    case IMAGE_MONO_PIXMAP:
    case IMAGE_COLOR_PIXMAP:
    case IMAGE_POINTER:
      sprintf (buf, " (0x%lx", (unsigned long) IMAGE_INSTANCE_X_PIXMAP (p));
      write_c_string (buf, printcharfun);
      if (IMAGE_INSTANCE_X_MASK (p))
	{
	  sprintf (buf, "/0x%lx", (unsigned long) IMAGE_INSTANCE_X_MASK (p));
	  write_c_string (buf, printcharfun);
	}
      write_c_string (")", printcharfun);
      break;
    default:
      break;
    }
}

static void
x_finalize_image_instance (struct Lisp_Image_Instance *p)
{
  if (!p->data)
    return;

  if (DEVICE_LIVE_P (XDEVICE (p->device)))
    {
      Display *dpy = DEVICE_X_DISPLAY (XDEVICE (p->device));

      if (IMAGE_INSTANCE_TYPE (p) == IMAGE_WIDGET)
	{
	  if (IMAGE_INSTANCE_SUBWINDOW_ID (p))
	    {
	      XtUnmanageChild (IMAGE_INSTANCE_X_WIDGET_ID (p));
	      XtDestroyWidget (IMAGE_INSTANCE_X_WIDGET_ID (p));
	      IMAGE_INSTANCE_SUBWINDOW_ID (p) = 0;
	    }
	}
      else if (IMAGE_INSTANCE_TYPE (p) == IMAGE_SUBWINDOW)
	{
	  if (IMAGE_INSTANCE_SUBWINDOW_ID (p))
	    XDestroyWindow (dpy, IMAGE_INSTANCE_X_SUBWINDOW_ID (p));
	  IMAGE_INSTANCE_SUBWINDOW_ID (p) = 0;
	}
      else
	{
	  if (IMAGE_INSTANCE_X_PIXMAP (p))
	    XFreePixmap (dpy, IMAGE_INSTANCE_X_PIXMAP (p));
	  if (IMAGE_INSTANCE_X_MASK (p) &&
	      IMAGE_INSTANCE_X_MASK (p) != IMAGE_INSTANCE_X_PIXMAP (p))
	    XFreePixmap (dpy, IMAGE_INSTANCE_X_MASK (p));
	  IMAGE_INSTANCE_X_PIXMAP (p) = 0;
	  IMAGE_INSTANCE_X_MASK (p) = 0;
	  
	  if (IMAGE_INSTANCE_X_CURSOR (p))
	    {
	      XFreeCursor (dpy, IMAGE_INSTANCE_X_CURSOR (p));
	      IMAGE_INSTANCE_X_CURSOR (p) = 0;
	    }
	  
	  if (IMAGE_INSTANCE_X_NPIXELS (p) != 0)
	    {
	      XFreeColors (dpy,
			   IMAGE_INSTANCE_X_COLORMAP (p),
			   IMAGE_INSTANCE_X_PIXELS (p),
			   IMAGE_INSTANCE_X_NPIXELS (p), 0);
	      IMAGE_INSTANCE_X_NPIXELS (p) = 0;
	    }
	}
    }
  if (IMAGE_INSTANCE_X_PIXELS (p))
    {
      xfree (IMAGE_INSTANCE_X_PIXELS (p));
      IMAGE_INSTANCE_X_PIXELS (p) = 0;
    }

  xfree (p->data);
  p->data = 0;
}

static int
x_image_instance_equal (struct Lisp_Image_Instance *p1,
			struct Lisp_Image_Instance *p2, int depth)
{
  switch (IMAGE_INSTANCE_TYPE (p1))
    {
    case IMAGE_MONO_PIXMAP:
    case IMAGE_COLOR_PIXMAP:
    case IMAGE_POINTER:
      if (IMAGE_INSTANCE_X_COLORMAP (p1) != IMAGE_INSTANCE_X_COLORMAP (p2) ||
	  IMAGE_INSTANCE_X_NPIXELS (p1) != IMAGE_INSTANCE_X_NPIXELS (p2))
	return 0;
      break;
    default:
      break;
    }

  return 1;
}

static unsigned long
x_image_instance_hash (struct Lisp_Image_Instance *p, int depth)
{
  switch (IMAGE_INSTANCE_TYPE (p))
    {
    case IMAGE_MONO_PIXMAP:
    case IMAGE_COLOR_PIXMAP:
    case IMAGE_POINTER:
      return IMAGE_INSTANCE_X_NPIXELS (p);
    default:
      return 0;
    }
}

/* Set all the slots in an image instance structure to reasonable
   default values.  This is used somewhere within an instantiate
   method.  It is assumed that the device slot within the image
   instance is already set -- this is the case when instantiate
   methods are called. */

static void
x_initialize_pixmap_image_instance (struct Lisp_Image_Instance *ii,
				    enum image_instance_type type)
{
  ii->data = xnew_and_zero (struct x_image_instance_data);
  IMAGE_INSTANCE_TYPE (ii) = type;
  IMAGE_INSTANCE_PIXMAP_FILENAME (ii) = Qnil;
  IMAGE_INSTANCE_PIXMAP_MASK_FILENAME (ii) = Qnil;
  IMAGE_INSTANCE_PIXMAP_HOTSPOT_X (ii) = Qnil;
  IMAGE_INSTANCE_PIXMAP_HOTSPOT_Y (ii) = Qnil;
  IMAGE_INSTANCE_PIXMAP_FG (ii) = Qnil;
  IMAGE_INSTANCE_PIXMAP_BG (ii) = Qnil;
}


/************************************************************************/
/*                        pixmap file functions                         */
/************************************************************************/

/* Where bitmaps are; initialized from resource database */
Lisp_Object Vx_bitmap_file_path;

#ifndef BITMAPDIR
#define BITMAPDIR "/usr/include/X11/bitmaps"
#endif

#define USE_XBMLANGPATH

/* Given a pixmap filename, look through all of the "standard" places
   where the file might be located.  Return a full pathname if found;
   otherwise, return Qnil. */

static Lisp_Object
x_locate_pixmap_file (Lisp_Object name)
{
  /* This function can GC if IN_REDISPLAY is false */
  Display *display;

  /* Check non-absolute pathnames with a directory component relative to
     the search path; that's the way Xt does it. */
  /* #### Unix-specific */
  if (XSTRING_BYTE (name, 0) == '/' ||
      (XSTRING_BYTE (name, 0) == '.' &&
       (XSTRING_BYTE (name, 1) == '/' ||
	(XSTRING_BYTE (name, 1) == '.' &&
	 (XSTRING_BYTE (name, 2) == '/')))))
    {
      if (!NILP (Ffile_readable_p (name)))
	return name;
      else
	return Qnil;
    }

  if (NILP (Vdefault_x_device))
    /* This may occur during initialization. */
    return Qnil;
  else
    /* We only check the bitmapFilePath resource on the original X device. */
    display = DEVICE_X_DISPLAY (XDEVICE (Vdefault_x_device));

#ifdef USE_XBMLANGPATH
  {
    char *path = egetenv ("XBMLANGPATH");
    SubstitutionRec subs[1];
    subs[0].match = 'B';
    subs[0].substitution = (char *) XSTRING_DATA (name);
    /* #### Motif uses a big hairy default if $XBMLANGPATH isn't set.
       We don't.  If you want it used, set it. */
    if (path &&
	(path = XtResolvePathname (display, "bitmaps", 0, 0, path,
				   subs, XtNumber (subs), 0)))
      {
	name = build_string (path);
	XtFree (path);
        return (name);
      }
  }
#endif

  if (NILP (Vx_bitmap_file_path))
    {
      char *type = 0;
      XrmValue value;
      if (XrmGetResource (XtDatabase (display),
			  "bitmapFilePath", "BitmapFilePath", &type, &value)
	  && !strcmp (type, "String"))
	Vx_bitmap_file_path = decode_env_path (0, (char *) value.addr);
      Vx_bitmap_file_path = nconc2 (Vx_bitmap_file_path,
				    (decode_path (BITMAPDIR)));
    }

  {
    Lisp_Object found;
    if (locate_file (Vx_bitmap_file_path, name, Qnil, &found, R_OK) < 0)
      {
	Lisp_Object temp = list1 (Vdata_directory);
	struct gcpro gcpro1;

	GCPRO1 (temp);
	locate_file (temp, name, Qnil, &found, R_OK);
	UNGCPRO;
      }

    return found;
  }
}

static Lisp_Object
locate_pixmap_file (Lisp_Object name)
{
  return x_locate_pixmap_file (name);
}

#if 0
static void
write_lisp_string_to_temp_file (Lisp_Object string, char *filename_out)
{
  Lisp_Object instream, outstream;
  Lstream *istr, *ostr;
  char tempbuf[1024]; /* some random amount */
  int fubar = 0;
  FILE *tmpfil;
  static Extbyte_dynarr *conversion_out_dynarr;
  Bytecount bstart, bend;
  struct gcpro gcpro1, gcpro2;
#ifdef FILE_CODING
  Lisp_Object conv_out_stream;
  Lstream *costr;
  struct gcpro gcpro3;
#endif

  /* This function can GC */
  if (!conversion_out_dynarr)
    conversion_out_dynarr = Dynarr_new (Extbyte);
  else
    Dynarr_reset (conversion_out_dynarr);

  /* Create the temporary file ... */
  sprintf (filename_out, "/tmp/emacs%d.XXXXXX", (int) getpid ());
  mktemp (filename_out);
  tmpfil = fopen (filename_out, "w");
  if (!tmpfil)
    {
      if (tmpfil)
	{
	  int old_errno = errno;
	  fclose (tmpfil);
	  unlink (filename_out);
	  errno = old_errno;
	}
      report_file_error ("Creating temp file",
			 list1 (build_string (filename_out)));
    }

  CHECK_STRING (string);
  get_string_range_byte (string, Qnil, Qnil, &bstart, &bend,
			 GB_HISTORICAL_STRING_BEHAVIOR);
  instream = make_lisp_string_input_stream (string, bstart, bend);
  istr = XLSTREAM (instream);
  /* setup the out stream */
  outstream = make_dynarr_output_stream((unsigned_char_dynarr *)conversion_out_dynarr);
  ostr = XLSTREAM (outstream);
#ifdef FILE_CODING
  /* setup the conversion stream */
  conv_out_stream = make_encoding_output_stream (ostr, Fget_coding_system(Qbinary));
  costr = XLSTREAM (conv_out_stream);
  GCPRO3 (instream, outstream, conv_out_stream);
#else
  GCPRO2 (instream, outstream);
#endif

  /* Get the data while doing the conversion */
  while (1)
    {
      int size_in_bytes = Lstream_read (istr, tempbuf, sizeof (tempbuf));
      if (!size_in_bytes)
	break;
      /* It does seem the flushes are necessary... */
#ifdef FILE_CODING
      Lstream_write (costr, tempbuf, size_in_bytes);
      Lstream_flush (costr);
#else
      Lstream_write (ostr, tempbuf, size_in_bytes);
#endif
      Lstream_flush (ostr);
      if (fwrite ((unsigned char *)Dynarr_atp(conversion_out_dynarr, 0),
		  Dynarr_length(conversion_out_dynarr), 1, tmpfil) != 1)
	{
	  fubar = 1;
	  break;
	}
      /* reset the dynarr */
      Lstream_rewind(ostr);
    }

  if (fclose (tmpfil) != 0)
    fubar = 1;
  Lstream_close (istr);
#ifdef FILE_CODING
  Lstream_close (costr);
#endif
  Lstream_close (ostr);

  UNGCPRO;
  Lstream_delete (istr);
  Lstream_delete (ostr);
#ifdef FILE_CODING
  Lstream_delete (costr);
#endif

  if (fubar)
    report_file_error ("Writing temp file",
		       list1 (build_string (filename_out)));
}
#endif /* 0 */


/************************************************************************/
/*                           cursor functions                           */
/************************************************************************/

/* Check that this server supports cursors of size WIDTH * HEIGHT.  If
   not, signal an error.  INSTANTIATOR is only used in the error
   message. */

static void
check_pointer_sizes (Screen *xs, unsigned int width, unsigned int height,
		     Lisp_Object instantiator)
{
  unsigned int best_width, best_height;
  if (! XQueryBestCursor (DisplayOfScreen (xs), RootWindowOfScreen (xs),
			  width, height, &best_width, &best_height))
    /* this means that an X error of some sort occurred (we trap
       these so they're not fatal). */
    signal_simple_error ("XQueryBestCursor() failed?", instantiator);

  if (width > best_width || height > best_height)
    error_with_frob (instantiator,
		     "pointer too large (%dx%d): "
		     "server requires %dx%d or smaller",
		     width, height, best_width, best_height);
}


static void
generate_cursor_fg_bg (Lisp_Object device, Lisp_Object *foreground,
		       Lisp_Object *background, XColor *xfg, XColor *xbg)
{
  if (!NILP (*foreground) && !COLOR_INSTANCEP (*foreground))
    *foreground =
      Fmake_color_instance (*foreground, device,
			    encode_error_behavior_flag (ERROR_ME));
  if (COLOR_INSTANCEP (*foreground))
    *xfg = COLOR_INSTANCE_X_COLOR (XCOLOR_INSTANCE (*foreground));
  else
    {
      xfg->pixel = 0;
      xfg->red = xfg->green = xfg->blue = 0;
    }

  if (!NILP (*background) && !COLOR_INSTANCEP (*background))
    *background =
      Fmake_color_instance (*background, device,
			    encode_error_behavior_flag (ERROR_ME));
  if (COLOR_INSTANCEP (*background))
    *xbg = COLOR_INSTANCE_X_COLOR (XCOLOR_INSTANCE (*background));
  else
    {
      xbg->pixel = 0;
      xbg->red = xbg->green = xbg->blue = ~0;
    }
}

static void
maybe_recolor_cursor (Lisp_Object image_instance, Lisp_Object foreground,
		      Lisp_Object background)
{
  Lisp_Object device = XIMAGE_INSTANCE_DEVICE (image_instance);
  XColor xfg, xbg;

  generate_cursor_fg_bg (device, &foreground, &background, &xfg, &xbg);
  if (!NILP (foreground) || !NILP (background))
    {
      XRecolorCursor (DEVICE_X_DISPLAY (XDEVICE (device)),
		      XIMAGE_INSTANCE_X_CURSOR (image_instance),
		      &xfg, &xbg);
      XIMAGE_INSTANCE_PIXMAP_FG (image_instance) = foreground;
      XIMAGE_INSTANCE_PIXMAP_BG (image_instance) = background;
    }
}


/************************************************************************/
/*                        color pixmap functions                        */
/************************************************************************/

/* Initialize an image instance from an XImage.

   DEST_MASK specifies the mask of allowed image types.

   PIXELS and NPIXELS specify an array of pixels that are used in
   the image.  These need to be kept around for the duration of the
   image.  When the image instance is freed, XFreeColors() will
   automatically be called on all the pixels specified here; thus,
   you should have allocated the pixels yourself using XAllocColor()
   or the like.  The array passed in is used directly without
   being copied, so it should be heap data created with xmalloc().
   It will be freed using xfree() when the image instance is
   destroyed.

   If this fails, signal an error.  INSTANTIATOR is only used
   in the error message.

   #### This should be able to handle conversion into `pointer'.
   Use the same code as for `xpm'. */

static void
init_image_instance_from_x_image (struct Lisp_Image_Instance *ii,
				  XImage *ximage,
				  int dest_mask,
				  Colormap cmap,
				  unsigned long *pixels,
				  int npixels,
				  Lisp_Object instantiator)
{
  Lisp_Object device = IMAGE_INSTANCE_DEVICE (ii);
  Display *dpy;
  GC gc;
  Drawable d;
  Pixmap pixmap;

  if (!DEVICE_X_P (XDEVICE (device)))
    signal_simple_error ("Not an X device", device);

  dpy = DEVICE_X_DISPLAY (XDEVICE (device));
  d = XtWindow(DEVICE_XT_APP_SHELL (XDEVICE (device)));

  if (!(dest_mask & IMAGE_COLOR_PIXMAP_MASK))
    incompatible_image_types (instantiator, dest_mask,
			      IMAGE_COLOR_PIXMAP_MASK);

  pixmap = XCreatePixmap (dpy, d, ximage->width,
			  ximage->height, ximage->depth);
  if (!pixmap)
    signal_simple_error ("Unable to create pixmap", instantiator);

  gc = XCreateGC (dpy, pixmap, 0, NULL);
  if (!gc)
    {
      XFreePixmap (dpy, pixmap);
      signal_simple_error ("Unable to create GC", instantiator);
    }

  XPutImage (dpy, pixmap, gc, ximage, 0, 0, 0, 0,
	     ximage->width, ximage->height);

  XFreeGC (dpy, gc);

  x_initialize_pixmap_image_instance (ii, IMAGE_COLOR_PIXMAP);

  IMAGE_INSTANCE_PIXMAP_FILENAME (ii) =
    find_keyword_in_vector (instantiator, Q_file);

  IMAGE_INSTANCE_X_PIXMAP (ii) = pixmap;
  IMAGE_INSTANCE_X_MASK (ii) = 0;
  IMAGE_INSTANCE_PIXMAP_WIDTH (ii) = ximage->width;
  IMAGE_INSTANCE_PIXMAP_HEIGHT (ii) = ximage->height;
  IMAGE_INSTANCE_PIXMAP_DEPTH (ii) = ximage->depth;
  IMAGE_INSTANCE_X_COLORMAP (ii) = cmap;
  IMAGE_INSTANCE_X_PIXELS (ii) = pixels;
  IMAGE_INSTANCE_X_NPIXELS (ii) = npixels;
}

static void
x_init_image_instance_from_eimage (struct Lisp_Image_Instance *ii,
				   int width, int height,
				   unsigned char *eimage,
				   int dest_mask,
				   Lisp_Object instantiator,
				   Lisp_Object domain)
{
  Lisp_Object device = IMAGE_INSTANCE_DEVICE (ii);
  Colormap cmap = DEVICE_X_COLORMAP (XDEVICE(device));
  unsigned long *pixtbl = NULL;
  int npixels = 0;
  XImage* ximage;

  ximage = convert_EImage_to_XImage (device, width, height, eimage,
				     &pixtbl, &npixels);
  if (!ximage)
    {
      if (pixtbl) xfree (pixtbl);
      signal_image_error("EImage to XImage conversion failed", instantiator);
    }

  /* Now create the pixmap and set up the image instance */
  init_image_instance_from_x_image (ii, ximage, dest_mask,
				    cmap, pixtbl, npixels,
				    instantiator);

  if (ximage)
    {
      if (ximage->data)
        {
	  xfree (ximage->data);
          ximage->data = 0;
        }
      XDestroyImage (ximage);
    }
}

int read_bitmap_data_from_file (CONST char *filename, unsigned int *width,
				unsigned int *height, unsigned char **datap,
				int *x_hot, int *y_hot)
{
  return XmuReadBitmapDataFromFile (filename, width, height,
				    datap, x_hot, y_hot);
}

/* Given inline data for a mono pixmap, create and return the
   corresponding X object. */

static Pixmap
pixmap_from_xbm_inline (Lisp_Object device, int width, int height,
			/* Note that data is in ext-format! */
			CONST Extbyte *bits)
{
  return XCreatePixmapFromBitmapData (DEVICE_X_DISPLAY (XDEVICE(device)),
				      XtWindow (DEVICE_XT_APP_SHELL (XDEVICE (device))),
				      (char *) bits, width, height,
				      1, 0, 1);
}

/* Given inline data for a mono pixmap, initialize the given
   image instance accordingly. */

static void
init_image_instance_from_xbm_inline (struct Lisp_Image_Instance *ii,
				     int width, int height,
				     /* Note that data is in ext-format! */
				     CONST char *bits,
				     Lisp_Object instantiator,
				     Lisp_Object pointer_fg,
				     Lisp_Object pointer_bg,
				     int dest_mask,
				     Pixmap mask,
				     Lisp_Object mask_filename)
{
  Lisp_Object device = IMAGE_INSTANCE_DEVICE (ii);
  Lisp_Object foreground = find_keyword_in_vector (instantiator, Q_foreground);
  Lisp_Object background = find_keyword_in_vector (instantiator, Q_background);
  Display *dpy;
  Screen *scr;
  Drawable draw;
  enum image_instance_type type;

  if (!DEVICE_X_P (XDEVICE (device)))
    signal_simple_error ("Not an X device", device);

  dpy = DEVICE_X_DISPLAY (XDEVICE (device));
  draw = XtWindow(DEVICE_XT_APP_SHELL (XDEVICE (device)));
  scr = DefaultScreenOfDisplay (dpy);

  if ((dest_mask & IMAGE_MONO_PIXMAP_MASK) &&
      (dest_mask & IMAGE_COLOR_PIXMAP_MASK))
    {
      if (!NILP (foreground) || !NILP (background))
	type = IMAGE_COLOR_PIXMAP;
      else
	type = IMAGE_MONO_PIXMAP;
    }
  else if (dest_mask & IMAGE_MONO_PIXMAP_MASK)
    type = IMAGE_MONO_PIXMAP;
  else if (dest_mask & IMAGE_COLOR_PIXMAP_MASK)
    type = IMAGE_COLOR_PIXMAP;
  else if (dest_mask & IMAGE_POINTER_MASK)
    type = IMAGE_POINTER;
  else
    incompatible_image_types (instantiator, dest_mask,
			      IMAGE_MONO_PIXMAP_MASK | IMAGE_COLOR_PIXMAP_MASK
			      | IMAGE_POINTER_MASK);

  x_initialize_pixmap_image_instance (ii, type);
  IMAGE_INSTANCE_PIXMAP_WIDTH (ii) = width;
  IMAGE_INSTANCE_PIXMAP_HEIGHT (ii) = height;
  IMAGE_INSTANCE_PIXMAP_FILENAME (ii) =
    find_keyword_in_vector (instantiator, Q_file);

  switch (type)
    {
    case IMAGE_MONO_PIXMAP:
      {
	IMAGE_INSTANCE_X_PIXMAP (ii) =
	  pixmap_from_xbm_inline (device, width, height, (Extbyte *) bits);
      }
      break;

    case IMAGE_COLOR_PIXMAP:
      {
	Dimension d = DEVICE_X_DEPTH (XDEVICE(device));
	unsigned long fg = BlackPixelOfScreen (scr);
	unsigned long bg = WhitePixelOfScreen (scr);

	if (!NILP (foreground) && !COLOR_INSTANCEP (foreground))
	  foreground =
	    Fmake_color_instance (foreground, device,
				  encode_error_behavior_flag (ERROR_ME));

	if (COLOR_INSTANCEP (foreground))
	  fg = COLOR_INSTANCE_X_COLOR (XCOLOR_INSTANCE (foreground)).pixel;

	if (!NILP (background) && !COLOR_INSTANCEP (background))
	  background =
	    Fmake_color_instance (background, device,
				  encode_error_behavior_flag (ERROR_ME));

	if (COLOR_INSTANCEP (background))
	  bg = COLOR_INSTANCE_X_COLOR (XCOLOR_INSTANCE (background)).pixel;

	/* We used to duplicate the pixels using XAllocColor(), to protect
	   against their getting freed.  Just as easy to just store the
	   color instances here and GC-protect them, so this doesn't
	   happen. */
	IMAGE_INSTANCE_PIXMAP_FG (ii) = foreground;
	IMAGE_INSTANCE_PIXMAP_BG (ii) = background;
	IMAGE_INSTANCE_X_PIXMAP (ii) =
	  XCreatePixmapFromBitmapData (dpy, draw,
				       (char *) bits, width, height,
				       fg, bg, d);
	IMAGE_INSTANCE_PIXMAP_DEPTH (ii) = d;
      }
      break;

    case IMAGE_POINTER:
    {
	XColor fg_color, bg_color;
	Pixmap source;

	check_pointer_sizes (scr, width, height, instantiator);

	source =
	  XCreatePixmapFromBitmapData (dpy, draw,
				       (char *) bits, width, height,
				       1, 0, 1);

	if (NILP (foreground))
	  foreground = pointer_fg;
	if (NILP (background))
	  background = pointer_bg;
	generate_cursor_fg_bg (device, &foreground, &background,
			       &fg_color, &bg_color);

	IMAGE_INSTANCE_PIXMAP_FG (ii) = foreground;
	IMAGE_INSTANCE_PIXMAP_BG (ii) = background;
	IMAGE_INSTANCE_PIXMAP_HOTSPOT_X (ii) =
	  find_keyword_in_vector (instantiator, Q_hotspot_x);
	IMAGE_INSTANCE_PIXMAP_HOTSPOT_Y (ii) =
	  find_keyword_in_vector (instantiator, Q_hotspot_y);
	IMAGE_INSTANCE_X_CURSOR (ii) =
	  XCreatePixmapCursor
	    (dpy, source, mask, &fg_color, &bg_color,
	     !NILP (IMAGE_INSTANCE_PIXMAP_HOTSPOT_X (ii)) ?
	     XINT (IMAGE_INSTANCE_PIXMAP_HOTSPOT_X (ii)) : 0,
	     !NILP (IMAGE_INSTANCE_PIXMAP_HOTSPOT_Y (ii)) ?
	     XINT (IMAGE_INSTANCE_PIXMAP_HOTSPOT_Y (ii)) : 0);
      }
      break;

    default:
      abort ();
    }
}

static void
xbm_instantiate_1 (Lisp_Object image_instance, Lisp_Object instantiator,
		   Lisp_Object pointer_fg, Lisp_Object pointer_bg,
		   int dest_mask, int width, int height,
		   /* Note that data is in ext-format! */
		   CONST char *bits)
{
  Lisp_Object mask_data = find_keyword_in_vector (instantiator, Q_mask_data);
  Lisp_Object mask_file = find_keyword_in_vector (instantiator, Q_mask_file);
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  Pixmap mask = 0;
  CONST char *gcc_may_you_rot_in_hell;

  if (!NILP (mask_data))
    {
      GET_C_STRING_BINARY_DATA_ALLOCA (XCAR (XCDR (XCDR (mask_data))),
				       gcc_may_you_rot_in_hell);
      mask =
	pixmap_from_xbm_inline (IMAGE_INSTANCE_DEVICE (ii),
				XINT (XCAR (mask_data)),
				XINT (XCAR (XCDR (mask_data))),
				(CONST unsigned char *)
				gcc_may_you_rot_in_hell);
    }

  init_image_instance_from_xbm_inline (ii, width, height, bits,
				       instantiator, pointer_fg, pointer_bg,
				       dest_mask, mask, mask_file);
}

/* Instantiate method for XBM's. */

static void
x_xbm_instantiate (Lisp_Object image_instance, Lisp_Object instantiator,
		   Lisp_Object pointer_fg, Lisp_Object pointer_bg,
		   int dest_mask, Lisp_Object domain)
{
  Lisp_Object data = find_keyword_in_vector (instantiator, Q_data);
  CONST char *gcc_go_home;

  assert (!NILP (data));

  GET_C_STRING_BINARY_DATA_ALLOCA (XCAR (XCDR (XCDR (data))),
				   gcc_go_home);

  xbm_instantiate_1 (image_instance, instantiator, pointer_fg,
		     pointer_bg, dest_mask, XINT (XCAR (data)),
		     XINT (XCAR (XCDR (data))), gcc_go_home);
}


#ifdef HAVE_XPM

/**********************************************************************
 *                             XPM                                    *
 **********************************************************************/
 /* xpm 3.2g and better has XpmCreatePixmapFromBuffer()...
    There was no version number in xpm.h before 3.3, but this should do.
  */
#if (XpmVersion >= 3) || defined(XpmExactColors)
# define XPM_DOES_BUFFERS
#endif

#ifndef XPM_DOES_BUFFERS
Your version of XPM is too old.  You cannot compile with it.
Upgrade to version 3.2g or better or compile with --with-xpm=no.
#endif /* !XPM_DOES_BUFFERS */

static XpmColorSymbol *
extract_xpm_color_names (XpmAttributes *xpmattrs, Lisp_Object device,
			 Lisp_Object domain,
			 Lisp_Object color_symbol_alist)
{
  /* This function can GC */
  Display *dpy =  DEVICE_X_DISPLAY (XDEVICE(device));
  Colormap cmap = DEVICE_X_COLORMAP (XDEVICE(device));
  XColor color;
  Lisp_Object rest;
  Lisp_Object results = Qnil;
  int i;
  XpmColorSymbol *symbols;
  struct gcpro gcpro1, gcpro2;

  GCPRO2 (results, device);

  /* We built up results to be (("name" . #<color>) ...) so that if an
     error happens we don't lose any malloc()ed data, or more importantly,
     leave any pixels allocated in the server. */
  i = 0;
  LIST_LOOP (rest, color_symbol_alist)
    {
      Lisp_Object cons = XCAR (rest);
      Lisp_Object name = XCAR (cons);
      Lisp_Object value = XCDR (cons);
      if (NILP (value))
	continue;
      if (STRINGP (value))
	value =
	  Fmake_color_instance
	    (value, device, encode_error_behavior_flag (ERROR_ME_NOT));
      else
        {
          assert (COLOR_SPECIFIERP (value));
          value = Fspecifier_instance (value, domain, Qnil, Qnil);
        }
      if (NILP (value))
        continue;
      results = noseeum_cons (noseeum_cons (name, value), results);
      i++;
    }
  UNGCPRO;			/* no more evaluation */

  if (i == 0) return 0;

  symbols = xnew_array (XpmColorSymbol, i);
  xpmattrs->valuemask |= XpmColorSymbols;
  xpmattrs->colorsymbols = symbols;
  xpmattrs->numsymbols = i;

  while (--i >= 0)
    {
      Lisp_Object cons = XCAR (results);
      color = COLOR_INSTANCE_X_COLOR (XCOLOR_INSTANCE (XCDR (cons)));
      /* Duplicate the pixel value so that we still have a lock on it if
	 the pixel we were passed is later freed. */
      if (! XAllocColor (dpy, cmap, &color))
	abort ();  /* it must be allocable since we're just duplicating it */

      symbols [i].name = (char *) XSTRING_DATA (XCAR (cons));
      symbols [i].pixel = color.pixel;
      symbols [i].value = 0;
      free_cons (XCONS (cons));
      cons = results;
      results = XCDR (results);
      free_cons (XCONS (cons));
    }
  return symbols;
}

static void
xpm_free (XpmAttributes *xpmattrs)
{
  /* Could conceivably lose if XpmXXX returned an error without first
     initializing this structure, if we didn't know that initializing it
     to all zeros was ok (and also that it's ok to call XpmFreeAttributes()
     multiple times, since it zeros slots as it frees them...) */
  XpmFreeAttributes (xpmattrs);
}

static void
x_xpm_instantiate (Lisp_Object image_instance, Lisp_Object instantiator,
				   Lisp_Object pointer_fg, Lisp_Object pointer_bg,
				   int dest_mask, Lisp_Object domain)
{
  /* This function can GC */
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  Lisp_Object device = IMAGE_INSTANCE_DEVICE (ii);
  Lisp_Object data = find_keyword_in_vector (instantiator, Q_data);
  Display *dpy;
  Screen *xs;
  Colormap cmap;
  int depth;
  Visual *visual;
  Pixmap pixmap;
  Pixmap mask = 0;
  XpmAttributes xpmattrs;
  int result;
  XpmColorSymbol *color_symbols;
  Lisp_Object color_symbol_alist = find_keyword_in_vector (instantiator,
							   Q_color_symbols);
  enum image_instance_type type;
  int force_mono;
  unsigned int w, h;

  if (!DEVICE_X_P (XDEVICE (device)))
    signal_simple_error ("Not an X device", device);

  dpy = DEVICE_X_DISPLAY (XDEVICE (device));
  xs = DefaultScreenOfDisplay (dpy);

  if (dest_mask & IMAGE_COLOR_PIXMAP_MASK)
    type = IMAGE_COLOR_PIXMAP;
  else if (dest_mask & IMAGE_MONO_PIXMAP_MASK)
    type = IMAGE_MONO_PIXMAP;
  else if (dest_mask & IMAGE_POINTER_MASK)
    type = IMAGE_POINTER;
  else
    incompatible_image_types (instantiator, dest_mask,
			      IMAGE_MONO_PIXMAP_MASK | IMAGE_COLOR_PIXMAP_MASK
			      | IMAGE_POINTER_MASK);
  force_mono = (type != IMAGE_COLOR_PIXMAP);

#if 1
  /* Although I haven't found it documented yet, it appears that pointers are
     always colored via the default window colormap... Sigh. */
  if (type == IMAGE_POINTER)
    {
      cmap = DefaultColormap(dpy, DefaultScreen(dpy));
      depth = DefaultDepthOfScreen (xs);
      visual = DefaultVisualOfScreen (xs);
    }
  else
    {
      cmap = DEVICE_X_COLORMAP (XDEVICE(device));
      depth = DEVICE_X_DEPTH (XDEVICE(device));
      visual = DEVICE_X_VISUAL (XDEVICE(device));
    }
#else
  cmap = DEVICE_X_COLORMAP (XDEVICE(device));
  depth = DEVICE_X_DEPTH (XDEVICE(device));
  visual = DEVICE_X_VISUAL (XDEVICE(device));
#endif

  x_initialize_pixmap_image_instance (ii, type);

  assert (!NILP (data));

 retry:

  xzero (xpmattrs); /* want XpmInitAttributes() */
  xpmattrs.valuemask = XpmReturnPixels;
  if (force_mono)
    {
      /* Without this, we get a 1-bit version of the color image, which
	 isn't quite right.  With this, we get the mono image, which might
	 be very different looking. */
      xpmattrs.valuemask |= XpmColorKey;
      xpmattrs.color_key = XPM_MONO;
      xpmattrs.depth = 1;
      xpmattrs.valuemask |= XpmDepth;
    }
  else
    {
      xpmattrs.closeness = 65535;
      xpmattrs.valuemask |= XpmCloseness;
      xpmattrs.depth = depth;
      xpmattrs.valuemask |= XpmDepth;
      xpmattrs.visual = visual;
      xpmattrs.valuemask |= XpmVisual;
      xpmattrs.colormap = cmap;
      xpmattrs.valuemask |= XpmColormap;
    }

  color_symbols = extract_xpm_color_names (&xpmattrs, device, domain,
					   color_symbol_alist);

  result = XpmCreatePixmapFromBuffer (dpy,
				      XtWindow(DEVICE_XT_APP_SHELL (XDEVICE(device))),
				      (char *) XSTRING_DATA (data),
				      &pixmap, &mask, &xpmattrs);

  if (color_symbols)
    {
      xfree (color_symbols);
      xpmattrs.colorsymbols = 0; /* in case XpmFreeAttr is too smart... */
      xpmattrs.numsymbols = 0;
    }

  switch (result)
    {
    case XpmSuccess:
      break;
    case XpmFileInvalid:
      {
	xpm_free (&xpmattrs);
	signal_image_error ("invalid XPM data", data);
      }
    case XpmColorFailed:
    case XpmColorError:
      {
	xpm_free (&xpmattrs);
	if (force_mono)
	  {
	    /* second time; blow out. */
	    signal_double_file_error ("Reading pixmap data",
				      "color allocation failed",
				      data);
	  }
	else
	  {
	    if (! (dest_mask & IMAGE_MONO_PIXMAP_MASK))
	      {
		/* second time; blow out. */
		signal_double_file_error ("Reading pixmap data",
					  "color allocation failed",
					  data);
	      }
	    force_mono = 1;
	    IMAGE_INSTANCE_TYPE (ii) = IMAGE_MONO_PIXMAP;
	    goto retry;
	  }
      }
    case XpmNoMemory:
      {
	xpm_free (&xpmattrs);
	signal_double_file_error ("Parsing pixmap data",
				  "out of memory", data);
      }
    default:
      {
	xpm_free (&xpmattrs);
	signal_double_file_error_2 ("Parsing pixmap data",
				    "unknown error code",
				    make_int (result), data);
      }
    }

  w = xpmattrs.width;
  h = xpmattrs.height;

  {
    int npixels = xpmattrs.npixels;
    Pixel *pixels;

    if (npixels != 0)
      {
	pixels = xnew_array (Pixel, npixels);
	memcpy (pixels, xpmattrs.pixels, npixels * sizeof (Pixel));
      }
    else
      pixels = NULL;

    IMAGE_INSTANCE_X_PIXMAP (ii) = pixmap;
    IMAGE_INSTANCE_X_MASK (ii) = mask;
    IMAGE_INSTANCE_X_COLORMAP (ii) = cmap;
    IMAGE_INSTANCE_X_PIXELS (ii) = pixels;
    IMAGE_INSTANCE_X_NPIXELS (ii) = npixels;
    IMAGE_INSTANCE_PIXMAP_WIDTH (ii) = w;
    IMAGE_INSTANCE_PIXMAP_HEIGHT (ii) = h;
    IMAGE_INSTANCE_PIXMAP_FILENAME (ii) =
      find_keyword_in_vector (instantiator, Q_file);
  }

  switch (type)
    {
    case IMAGE_MONO_PIXMAP:
      break;

    case IMAGE_COLOR_PIXMAP:
      {
	IMAGE_INSTANCE_PIXMAP_DEPTH (ii) = depth;
      }
      break;

    case IMAGE_POINTER:
      {
	int npixels = xpmattrs.npixels;
	Pixel *pixels = xpmattrs.pixels;
	XColor fg, bg;
	int i;
	int xhot = 0, yhot = 0;

	if (xpmattrs.valuemask & XpmHotspot)
	  {
	    xhot = xpmattrs.x_hotspot;
	    XSETINT (IMAGE_INSTANCE_PIXMAP_HOTSPOT_X (ii), xpmattrs.x_hotspot);
	  }
	if (xpmattrs.valuemask & XpmHotspot)
	  {
	    yhot = xpmattrs.y_hotspot;
	    XSETINT (IMAGE_INSTANCE_PIXMAP_HOTSPOT_Y (ii), xpmattrs.y_hotspot);
	  }
	check_pointer_sizes (xs, w, h, instantiator);

	/* If the loaded pixmap has colors allocated (meaning it came from an
	   XPM file), then use those as the default colors for the cursor we
	   create.  Otherwise, default to pointer_fg and pointer_bg.
	   */
	if (npixels >= 2)
	  {
	    /* With an XBM file, it's obvious which bit is foreground
	       and which is background, or rather, it's implicit: in
	       an XBM file, a 1 bit is foreground, and a 0 bit is
	       background.

	       XCreatePixmapCursor() assumes this property of the
	       pixmap it is called with as well; the `foreground'
	       color argument is used for the 1 bits.

	       With an XPM file, it's tricker, since the elements of
	       the pixmap don't represent FG and BG, but are actual
	       pixel values.  So we need to figure out which of those
	       pixels is the foreground color and which is the
	       background.  We do it by comparing RGB and assuming
	       that the darker color is the foreground.  This works
	       with the result of xbmtopbm|ppmtoxpm, at least.

	       It might be nice if there was some way to tag the
	       colors in the XPM file with whether they are the
	       foreground - perhaps with logical color names somehow?

	       Once we have decided which color is the foreground, we
	       need to ensure that that color corresponds to a `1' bit
	       in the Pixmap.  The XPM library wrote into the (1-bit)
	       pixmap with XPutPixel, which will ignore all but the
	       least significant bit.

	       This means that a 1 bit in the image corresponds to
	       `fg' only if `fg.pixel' is odd.

	       (This also means that the image will be all the same
	       color if both `fg' and `bg' are odd or even, but we can
	       safely assume that that won't happen if the XPM file is
	       sensible I think.)

	       The desired result is that the image use `1' to
	       represent the foreground color, and `0' to represent
	       the background color.  So, we may need to invert the
	       image to accomplish this; we invert if fg is
	       odd. (Remember that WhitePixel and BlackPixel are not
	       necessarily 1 and 0 respectively, though I think it
	       might be safe to assume that one of them is always 1
	       and the other is always 0.  We also pretty much need to
	       assume that one is even and the other is odd.)
	       */

	    fg.pixel = pixels[0];	/* pick a pixel at random. */
	    bg.pixel = fg.pixel;
	    for (i = 1; i < npixels; i++) /* Look for an "other" pixel value.*/
	      {
		bg.pixel = pixels[i];
		if (fg.pixel != bg.pixel)
		  break;
	      }

	    /* If (fg.pixel == bg.pixel) then probably something has
	       gone wrong, but I don't think signalling an error would
	       be appropriate. */

	    XQueryColor (dpy, cmap, &fg);
	    XQueryColor (dpy, cmap, &bg);

	    /* If the foreground is lighter than the background, swap them.
	       (This occurs semi-randomly, depending on the ordering of the
	       color list in the XPM file.)
	       */
	    {
	      unsigned short fg_total = ((fg.red / 3) + (fg.green / 3)
					 + (fg.blue / 3));
	      unsigned short bg_total = ((bg.red / 3) + (bg.green / 3)
					 + (bg.blue / 3));
	      if (fg_total > bg_total)
		{
		  XColor swap;
		  swap = fg;
		  fg = bg;
		  bg = swap;
		}
	    }

	    /* If the fg pixel corresponds to a `0' in the bitmap, invert it.
	       (This occurs (only?) on servers with Black=0, White=1.)
	       */
	    if ((fg.pixel & 1) == 0)
	      {
		XGCValues gcv;
		GC gc;
		gcv.function = GXxor;
		gcv.foreground = 1;
		gc = XCreateGC (dpy, pixmap, (GCFunction | GCForeground),
				&gcv);
		XFillRectangle (dpy, pixmap, gc, 0, 0, w, h);
		XFreeGC (dpy, gc);
	      }
	  }
	else
	  {
	    generate_cursor_fg_bg (device, &pointer_fg, &pointer_bg,
				   &fg, &bg);
	    IMAGE_INSTANCE_PIXMAP_FG (ii) = pointer_fg;
	    IMAGE_INSTANCE_PIXMAP_BG (ii) = pointer_bg;
	  }

	IMAGE_INSTANCE_X_CURSOR (ii) =
	  XCreatePixmapCursor
	    (dpy, pixmap, mask, &fg, &bg, xhot, yhot);
      }

      break;

    default:
      abort ();
    }

  xpm_free (&xpmattrs);	/* after we've read pixels and hotspot */
}

#endif /* HAVE_XPM */


#ifdef HAVE_XFACE

/**********************************************************************
 *                             X-Face                                 *
 **********************************************************************/
#if defined(EXTERN)
/* This is about to get redefined! */
#undef EXTERN
#endif
/* We have to define SYSV32 so that compface.h includes string.h
   instead of strings.h. */
#define SYSV32
#ifdef __cplusplus
extern "C" {
#endif
#include <compface.h>
#ifdef __cplusplus
}
#endif
/* JMP_BUF cannot be used here because if it doesn't get defined
   to jmp_buf we end up with a conflicting type error with the
   definition in compface.h */
extern jmp_buf comp_env;
#undef SYSV32

static void
x_xface_instantiate (Lisp_Object image_instance, Lisp_Object instantiator,
		     Lisp_Object pointer_fg, Lisp_Object pointer_bg,
		     int dest_mask, Lisp_Object domain)
{
  Lisp_Object data = find_keyword_in_vector (instantiator, Q_data);
  int i, stattis;
  char *p, *bits, *bp;
  CONST char * volatile emsg = 0;
  CONST char * volatile dstring;

  assert (!NILP (data));

  GET_C_STRING_BINARY_DATA_ALLOCA (data, dstring);

  if ((p = strchr (dstring, ':')))
    {
      dstring = p + 1;
    }

  /* Must use setjmp not SETJMP because we used jmp_buf above not JMP_BUF */
  if (!(stattis = setjmp (comp_env)))
    {
      UnCompAll ((char *) dstring);
      UnGenFace ();
    }

  switch (stattis)
    {
    case -2:
      emsg = "uncompface: internal error";
      break;
    case -1:
      emsg = "uncompface: insufficient or invalid data";
      break;
    case 1:
      emsg = "uncompface: excess data ignored";
      break;
    }

  if (emsg)
    signal_simple_error_2 (emsg, data, Qimage);

  bp = bits = (char *) alloca (PIXELS / 8);

  /* the compface library exports char F[], which uses a single byte per
     pixel to represent a 48x48 bitmap.  Yuck. */
  for (i = 0, p = F; i < (PIXELS / 8); ++i)
    {
      int n, b;
      /* reverse the bit order of each byte... */
      for (b = n = 0; b < 8; ++b)
	{
	  n |= ((*p++) << b);
	}
      *bp++ = (char) n;
    }

  xbm_instantiate_1 (image_instance, instantiator, pointer_fg,
		     pointer_bg, dest_mask, 48, 48, bits);
}

#endif /* HAVE_XFACE */


/**********************************************************************
 *			 Autodetect		                         *
 **********************************************************************/

static void
autodetect_validate (Lisp_Object instantiator)
{
  data_must_be_present (instantiator);
}

static Lisp_Object
autodetect_normalize (Lisp_Object instantiator,
				Lisp_Object console_type)
{
  Lisp_Object file = find_keyword_in_vector (instantiator, Q_data);
  Lisp_Object filename = Qnil;
  Lisp_Object data = Qnil;
  struct gcpro gcpro1, gcpro2, gcpro3;
  Lisp_Object alist = Qnil;

  GCPRO3 (filename, data, alist);

  if (NILP (file)) /* no conversion necessary */
    RETURN_UNGCPRO (instantiator);

  alist = tagged_vector_to_alist (instantiator);

  filename = locate_pixmap_file (file);
  if (!NILP (filename))
    {
      int xhot, yhot;
      /* #### Apparently some versions of XpmReadFileToData, which is
	 called by pixmap_to_lisp_data, don't return an error value
	 if the given file is not a valid XPM file.  Instead, they
	 just seg fault.  It is definitely caused by passing a
	 bitmap.  To try and avoid this we check for bitmaps first.  */

      data = bitmap_to_lisp_data (filename, &xhot, &yhot, 1);

      if (!EQ (data, Qt))
	{
	  alist = remassq_no_quit (Q_data, alist);
	  alist = Fcons (Fcons (Q_file, filename),
			 Fcons (Fcons (Q_data, data), alist));
	  if (xhot != -1)
	    alist = Fcons (Fcons (Q_hotspot_x, make_int (xhot)),
			   alist);
	  if (yhot != -1)
	    alist = Fcons (Fcons (Q_hotspot_y, make_int (yhot)),
			   alist);

	  alist = xbm_mask_file_munging (alist, filename, Qnil, console_type);

	  {
	    Lisp_Object result = alist_to_tagged_vector (Qxbm, alist);
	    free_alist (alist);
	    RETURN_UNGCPRO (result);
	  }
	}

#ifdef HAVE_XPM
      data = pixmap_to_lisp_data (filename, 1);

      if (!EQ (data, Qt))
	{
	  alist = remassq_no_quit (Q_data, alist);
	  alist = Fcons (Fcons (Q_file, filename),
			 Fcons (Fcons (Q_data, data), alist));
	  alist = Fcons (Fcons (Q_color_symbols,
				evaluate_xpm_color_symbols ()),
			 alist);
	  {
	    Lisp_Object result = alist_to_tagged_vector (Qxpm, alist);
	    free_alist (alist);
	    RETURN_UNGCPRO (result);
	  }
	}
#endif
    }

  /* If we couldn't convert it, just put it back as it is.
     We might try to further frob it later as a cursor-font
     specification. (We can't do that now because we don't know
     what dest-types it's going to be instantiated into.) */
  {
    Lisp_Object result = alist_to_tagged_vector (Qautodetect, alist);
    free_alist (alist);
    RETURN_UNGCPRO (result);
  }
}

static int
autodetect_possible_dest_types (void)
{
  return
    IMAGE_MONO_PIXMAP_MASK  |
    IMAGE_COLOR_PIXMAP_MASK |
    IMAGE_POINTER_MASK      |
    IMAGE_TEXT_MASK;
}

static void
autodetect_instantiate (Lisp_Object image_instance,
				  Lisp_Object instantiator,
				  Lisp_Object pointer_fg,
				  Lisp_Object pointer_bg,
				  int dest_mask, Lisp_Object domain)
{
  Lisp_Object data = find_keyword_in_vector (instantiator, Q_data);
  struct gcpro gcpro1, gcpro2, gcpro3;
  Lisp_Object alist = Qnil;
  Lisp_Object result = Qnil;
  int is_cursor_font = 0;

  GCPRO3 (data, alist, result);

  alist = tagged_vector_to_alist (instantiator);
  if (dest_mask & IMAGE_POINTER_MASK)
    {
      CONST char *name_ext;
      GET_C_STRING_FILENAME_DATA_ALLOCA (data, name_ext);
      if (XmuCursorNameToIndex (name_ext) != -1)
        {
          result = alist_to_tagged_vector (Qcursor_font, alist);
          is_cursor_font = 1;
        }
    }

  if (!is_cursor_font)
    result = alist_to_tagged_vector (Qstring, alist);
  free_alist (alist);

  if (is_cursor_font)
    cursor_font_instantiate (image_instance, result, pointer_fg,
			     pointer_bg, dest_mask, domain);
  else
    string_instantiate (image_instance, result, pointer_fg,
			pointer_bg, dest_mask, domain);

  UNGCPRO;
}


/**********************************************************************
 *                              Font                                  *
 **********************************************************************/

static void
font_validate (Lisp_Object instantiator)
{
  data_must_be_present (instantiator);
}

/* XmuCvtStringToCursor is bogus in the following ways:

   - When it can't convert the given string to a real cursor, it will
     sometimes return a "success" value, after triggering a BadPixmap
     error.  It then gives you a cursor that will itself generate BadCursor
     errors.  So we install this error handler to catch/notice the X error
     and take that as meaning "couldn't convert."

   - When you tell it to find a cursor file that doesn't exist, it prints
     an error message on stderr.  You can't make it not do that.

   - Also, using Xmu means we can't properly hack Lisp_Image_Instance
     objects, or XPM files, or $XBMLANGPATH.
 */

/* Duplicate the behavior of XmuCvtStringToCursor() to bypass its bogusness. */

static int XLoadFont_got_error;

static int
XLoadFont_error_handler (Display *dpy, XErrorEvent *xerror)
{
  XLoadFont_got_error = 1;
  return 0;
}

static Font
safe_XLoadFont (Display *dpy, char *name)
{
  Font font;
  int (*old_handler) (Display *, XErrorEvent *);
  XLoadFont_got_error = 0;
  XSync (dpy, 0);
  old_handler = XSetErrorHandler (XLoadFont_error_handler);
  font = XLoadFont (dpy, name);
  XSync (dpy, 0);
  XSetErrorHandler (old_handler);
  if (XLoadFont_got_error) return 0;
  return font;
}

static int
font_possible_dest_types (void)
{
  return IMAGE_POINTER_MASK;
}

static void
font_instantiate (Lisp_Object image_instance, Lisp_Object instantiator,
		  Lisp_Object pointer_fg, Lisp_Object pointer_bg,
		  int dest_mask, Lisp_Object domain)
{
  /* This function can GC */
  Lisp_Object data = find_keyword_in_vector (instantiator, Q_data);
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  Lisp_Object device = IMAGE_INSTANCE_DEVICE (ii);
  Display *dpy;
  XColor fg, bg;
  Font source, mask;
  char source_name[MAXPATHLEN], mask_name[MAXPATHLEN], dummy;
  int source_char, mask_char;
  int count;
  Lisp_Object foreground, background;

  if (!DEVICE_X_P (XDEVICE (device)))
    signal_simple_error ("Not an X device", device);

  dpy = DEVICE_X_DISPLAY (XDEVICE (device));

  if (!STRINGP (data) ||
      strncmp ("FONT ", (char *) XSTRING_DATA (data), 5))
    signal_simple_error ("Invalid font-glyph instantiator",
			 instantiator);

  if (!(dest_mask & IMAGE_POINTER_MASK))
    incompatible_image_types (instantiator, dest_mask, IMAGE_POINTER_MASK);

  foreground = find_keyword_in_vector (instantiator, Q_foreground);
  if (NILP (foreground))
    foreground = pointer_fg;
  background = find_keyword_in_vector (instantiator, Q_background);
  if (NILP (background))
    background = pointer_bg;

  generate_cursor_fg_bg (device, &foreground, &background, &fg, &bg);

  count = sscanf ((char *) XSTRING_DATA (data),
		  "FONT %s %d %s %d %c",
		  source_name, &source_char,
		  mask_name, &mask_char, &dummy);
  /* Allow "%s %d %d" as well... */
  if (count == 3 && (1 == sscanf (mask_name, "%d %c", &mask_char, &dummy)))
    count = 4, mask_name[0] = 0;

  if (count != 2 && count != 4)
    signal_simple_error ("invalid cursor specification", data);
  source = safe_XLoadFont (dpy, source_name);
  if (! source)
    signal_simple_error_2 ("couldn't load font",
			   build_string (source_name),
			   data);
  if (count == 2)
    mask = 0;
  else if (!mask_name[0])
    mask = source;
  else
    {
      mask = safe_XLoadFont (dpy, mask_name);
      if (!mask)
	/* continuable */
	Fsignal (Qerror, list3 (build_string ("couldn't load font"),
				build_string (mask_name), data));
    }
  if (!mask)
    mask_char = 0;

  /* #### call XQueryTextExtents() and check_pointer_sizes() here. */

  x_initialize_pixmap_image_instance (ii, IMAGE_POINTER);
  IMAGE_INSTANCE_X_CURSOR (ii) =
    XCreateGlyphCursor (dpy, source, mask, source_char, mask_char,
			&fg, &bg);
  XIMAGE_INSTANCE_PIXMAP_FG (image_instance) = foreground;
  XIMAGE_INSTANCE_PIXMAP_BG (image_instance) = background;
  XUnloadFont (dpy, source);
  if (mask && mask != source) XUnloadFont (dpy, mask);
}


/**********************************************************************
 *                           Cursor-Font                              *
 **********************************************************************/

static void
cursor_font_validate (Lisp_Object instantiator)
{
  data_must_be_present (instantiator);
}

static int
cursor_font_possible_dest_types (void)
{
  return IMAGE_POINTER_MASK;
}

static void
cursor_font_instantiate (Lisp_Object image_instance, Lisp_Object instantiator,
			 Lisp_Object pointer_fg, Lisp_Object pointer_bg,
			 int dest_mask, Lisp_Object domain)
{
  /* This function can GC */
  Lisp_Object data = find_keyword_in_vector (instantiator, Q_data);
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  Lisp_Object device = IMAGE_INSTANCE_DEVICE (ii);
  Display *dpy;
  int i;
  CONST char *name_ext;
  Lisp_Object foreground, background;

  if (!DEVICE_X_P (XDEVICE (device)))
    signal_simple_error ("Not an X device", device);

  dpy = DEVICE_X_DISPLAY (XDEVICE (device));

  if (!(dest_mask & IMAGE_POINTER_MASK))
    incompatible_image_types (instantiator, dest_mask, IMAGE_POINTER_MASK);

  GET_C_STRING_FILENAME_DATA_ALLOCA (data, name_ext);
  if ((i = XmuCursorNameToIndex (name_ext)) == -1)
    signal_simple_error ("Unrecognized cursor-font name", data);

  x_initialize_pixmap_image_instance (ii, IMAGE_POINTER);
  IMAGE_INSTANCE_X_CURSOR (ii) = XCreateFontCursor (dpy, i);
  foreground = find_keyword_in_vector (instantiator, Q_foreground);
  if (NILP (foreground))
    foreground = pointer_fg;
  background = find_keyword_in_vector (instantiator, Q_background);
  if (NILP (background))
    background = pointer_bg;
  maybe_recolor_cursor (image_instance, foreground, background);
}

static int
x_colorize_image_instance (Lisp_Object image_instance,
			   Lisp_Object foreground, Lisp_Object background)
{
  struct Lisp_Image_Instance *p;

  p = XIMAGE_INSTANCE (image_instance);

  switch (IMAGE_INSTANCE_TYPE (p))
    {
    case IMAGE_MONO_PIXMAP:
      IMAGE_INSTANCE_TYPE (p) = IMAGE_COLOR_PIXMAP;
      /* Make sure there aren't two pointers to the same mask, causing
	 it to get freed twice. */
      IMAGE_INSTANCE_X_MASK (p) = 0;
      break;

    default:
      return 0;
    }

  {
    Display *dpy = DEVICE_X_DISPLAY (XDEVICE (IMAGE_INSTANCE_DEVICE (p)));
    Drawable draw = XtWindow(DEVICE_XT_APP_SHELL (XDEVICE (IMAGE_INSTANCE_DEVICE (p))));
    Dimension d = DEVICE_X_DEPTH (XDEVICE (IMAGE_INSTANCE_DEVICE (p)));
    Pixmap new = XCreatePixmap (dpy, draw,
				IMAGE_INSTANCE_PIXMAP_WIDTH (p),
				IMAGE_INSTANCE_PIXMAP_HEIGHT (p), d);
    XColor color;
    XGCValues gcv;
    GC gc;
    color = COLOR_INSTANCE_X_COLOR (XCOLOR_INSTANCE (foreground));
    gcv.foreground = color.pixel;
    color = COLOR_INSTANCE_X_COLOR (XCOLOR_INSTANCE (background));
    gcv.background = color.pixel;
    gc = XCreateGC (dpy, new, GCBackground|GCForeground, &gcv);
    XCopyPlane (dpy, IMAGE_INSTANCE_X_PIXMAP (p), new, gc, 0, 0,
		IMAGE_INSTANCE_PIXMAP_WIDTH (p),
		IMAGE_INSTANCE_PIXMAP_HEIGHT (p),
		0, 0, 1);
    XFreeGC (dpy, gc);
    IMAGE_INSTANCE_X_PIXMAP (p) = new;
    IMAGE_INSTANCE_PIXMAP_DEPTH (p) = d;
    IMAGE_INSTANCE_PIXMAP_FG (p) = foreground;
    IMAGE_INSTANCE_PIXMAP_BG (p) = background;
    return 1;
  }
}


/************************************************************************/
/*                      subwindow and widget support                      */
/************************************************************************/

/* unmap the image if it is a widget. This is used by redisplay via
   redisplay_unmap_subwindows */
static void
x_unmap_subwindow (struct Lisp_Image_Instance *p)
{
  if (IMAGE_INSTANCE_TYPE (p) == IMAGE_SUBWINDOW)
    {
      XUnmapWindow 
	(DisplayOfScreen (IMAGE_INSTANCE_X_SUBWINDOW_SCREEN (p)), 
	 IMAGE_INSTANCE_X_SUBWINDOW_ID (p));
    }
  else				/* must be a widget */
    {
      XtUnmapWidget (IMAGE_INSTANCE_X_WIDGET_ID (p));
    }
}

/* map the subwindow. This is used by redisplay via
   redisplay_output_subwindow */
static void
x_map_subwindow (struct Lisp_Image_Instance *p, int x, int y)
{
  if (IMAGE_INSTANCE_TYPE (p) == IMAGE_SUBWINDOW)
    {
      Window subwindow = IMAGE_INSTANCE_X_SUBWINDOW_ID (p);
      Screen* screen = IMAGE_INSTANCE_X_SUBWINDOW_SCREEN (p);
      XMapWindow (DisplayOfScreen (screen), subwindow);
      XMoveWindow (DisplayOfScreen (screen), subwindow, x, y);
    }
  else				/* must be a widget */
    {
      XtMoveWidget (IMAGE_INSTANCE_X_WIDGET_ID (p), 
		    x + IMAGE_INSTANCE_X_WIDGET_XOFFSET (p),
		    y + IMAGE_INSTANCE_X_WIDGET_YOFFSET (p));
      XtMapWidget (IMAGE_INSTANCE_X_WIDGET_ID (p));
    }
}

/* when you click on a widget you may activate another widget this
   needs to be checked and all appropriate widgets updated */
static void
x_update_subwindow (struct Lisp_Image_Instance *p)
{
  if (IMAGE_INSTANCE_TYPE (p) == IMAGE_WIDGET)
    {
      widget_value* wv = xmalloc_widget_value ();
      button_item_to_widget_value (IMAGE_INSTANCE_WIDGET_SINGLE_ITEM (p),
				   wv, 1, 1);
      lw_modify_all_widgets (IMAGE_INSTANCE_X_WIDGET_LWID (p), 
			     wv, 1);
    }
}

/* instantiate and x type subwindow */
static void
x_subwindow_instantiate (Lisp_Object image_instance, Lisp_Object instantiator,
			Lisp_Object pointer_fg, Lisp_Object pointer_bg,
			int dest_mask, Lisp_Object domain)
{
  /* This function can GC */
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  Lisp_Object device = IMAGE_INSTANCE_DEVICE (ii);
  Lisp_Object frame = FW_FRAME (domain);
  struct frame* f = XFRAME (frame);
  Display *dpy;
  Screen *xs;
  Window pw, win;
  XSetWindowAttributes xswa;
  Mask valueMask = 0;
  unsigned int w = IMAGE_INSTANCE_SUBWINDOW_WIDTH (ii), 
    h = IMAGE_INSTANCE_SUBWINDOW_HEIGHT (ii);

  if (!DEVICE_X_P (XDEVICE (device)))
    signal_simple_error ("Not an X device", device);

  dpy = DEVICE_X_DISPLAY (XDEVICE (device));
  xs = DefaultScreenOfDisplay (dpy);

  IMAGE_INSTANCE_TYPE (ii) = IMAGE_SUBWINDOW;

  pw = XtWindow (FRAME_X_TEXT_WIDGET (f));

  ii->data = xnew_and_zero (struct x_subwindow_data);

  IMAGE_INSTANCE_X_SUBWINDOW_PARENT (ii) = pw;
  IMAGE_INSTANCE_X_SUBWINDOW_SCREEN (ii) = xs;

  xswa.backing_store = Always;
  valueMask |= CWBackingStore;
  xswa.colormap = DefaultColormapOfScreen (xs);
  valueMask |= CWColormap;
  
  win = XCreateWindow (dpy, pw, 0, 0, w, h, 0, CopyFromParent,
		       InputOutput, CopyFromParent, valueMask,
		       &xswa);
  
  IMAGE_INSTANCE_SUBWINDOW_ID (ii) = (void*)win;
}

#if 0
/* #### Should this function exist? If there's any doubt I'm not implementing it --andyp */
DEFUN ("change-subwindow-property", Fchange_subwindow_property, 3, 3, 0, /*
For the given SUBWINDOW, set PROPERTY to DATA, which is a string.
Subwindows are not currently implemented.
*/
       (subwindow, property, data))
{
  Atom property_atom;
  struct Lisp_Subwindow *sw;
  Display *dpy;

  CHECK_SUBWINDOW (subwindow);
  CHECK_STRING (property);
  CHECK_STRING (data);

  sw = XSUBWINDOW (subwindow);
  dpy = DisplayOfScreen (LISP_DEVICE_TO_X_SCREEN
			 (FRAME_DEVICE (XFRAME (sw->frame))));

  property_atom = XInternAtom (dpy, (char *) XSTRING_DATA (property), False);
  XChangeProperty (dpy, sw->subwindow, property_atom, XA_STRING, 8,
		   PropModeReplace,
		   XSTRING_DATA   (data),
		   XSTRING_LENGTH (data));

  return property;
}
#endif

static void 
x_resize_subwindow (struct Lisp_Image_Instance* ii, int w, int h)
{
  if (IMAGE_INSTANCE_TYPE (ii) == IMAGE_SUBWINDOW)
    {
      XResizeWindow (DisplayOfScreen (IMAGE_INSTANCE_X_SUBWINDOW_SCREEN (ii)),
		     IMAGE_INSTANCE_X_SUBWINDOW_ID (ii),
		     w, h);
    }
  else				/* must be a widget */
    {
      Arg al[2];
      XtSetArg (al [0], XtNwidth, (Dimension)w);
      XtSetArg (al [1], XtNheight, (Dimension)h);
      XtSetValues (IMAGE_INSTANCE_X_WIDGET_ID (ii), al, 2);
    }
}

/************************************************************************/
/*                            widgets                            */
/************************************************************************/

static void
x_widget_instantiate (Lisp_Object image_instance, Lisp_Object instantiator,
		      Lisp_Object pointer_fg, Lisp_Object pointer_bg,
		      int dest_mask, Lisp_Object domain,
		      CONST char* type, widget_value* wv)
{
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  Lisp_Object device = IMAGE_INSTANCE_DEVICE (ii), pixel;
  struct device* d = XDEVICE (device);
  Lisp_Object frame = FW_FRAME (domain);
  struct frame* f = XFRAME (frame);
  XColor fcolor, bcolor;
  Extbyte* nm=0;
  Widget wid;
  Arg al [32];
  int ac = 0;
  int id = new_lwlib_id ();
#ifdef LWLIB_USES_MOTIF
  XmFontList fontList;
#endif

  if (!DEVICE_X_P (d))
    signal_simple_error ("Not an mswindows device", device);

  /* have to set the type this late in case there is no device
     instantiation for a widget. But we can go ahead and do it without
     checking because there is always a generic instantiator. */
  IMAGE_INSTANCE_TYPE (ii) = IMAGE_WIDGET;

  if (!NILP (IMAGE_INSTANCE_WIDGET_TEXT (ii)))
    GET_C_STRING_OS_DATA_ALLOCA (IMAGE_INSTANCE_WIDGET_TEXT (ii), nm);

  ii->data = xnew_and_zero (struct x_subwindow_data);

  /* copy any args we were given */
  if (wv->nargs)
    lw_add_value_args_to_args (wv, al, &ac);

  /* add our own arguments */
  pixel = FACE_FOREGROUND 
    (IMAGE_INSTANCE_WIDGET_FACE (ii),
     IMAGE_INSTANCE_SUBWINDOW_FRAME (ii));
  fcolor = COLOR_INSTANCE_X_COLOR (XCOLOR_INSTANCE (pixel));
  pixel = FACE_BACKGROUND
    (IMAGE_INSTANCE_WIDGET_FACE (ii),
     IMAGE_INSTANCE_SUBWINDOW_FRAME (ii));
  bcolor = COLOR_INSTANCE_X_COLOR (XCOLOR_INSTANCE (pixel));

  XtSetArg (al [ac], XtNbackground, bcolor.pixel);		ac++;
  XtSetArg (al [ac], XtNforeground, fcolor.pixel);		ac++;
#ifdef LWLIB_USES_MOTIF
  fontList = XmFontListCreate 
    ((void*)FONT_INSTANCE_X_FONT 
     (XFONT_INSTANCE (widget_face_font_info 
		      (domain, IMAGE_INSTANCE_WIDGET_FACE (ii),
		       0, 0))), XmSTRING_DEFAULT_CHARSET);
  XtSetArg (al [ac], XmNfontList, fontList );				ac++;
#else
  XtSetArg (al [ac], XtNfont, (void*)FONT_INSTANCE_X_FONT 
	    (XFONT_INSTANCE (widget_face_font_info 
			     (domain, 
			      IMAGE_INSTANCE_WIDGET_FACE (ii),
			      0, 0))));			ac++;
#endif

  wv->nargs = ac;
  wv->args = al;
  
  wid = lw_create_widget (type, wv->name, id, wv, FRAME_X_CONTAINER_WIDGET (f),
			  False, 0, popup_selection_callback, 0);

  IMAGE_INSTANCE_X_WIDGET_LWID (ii) = id;
#ifdef LWLIB_USES_MOTIF
  XmFontListFree (fontList);
#endif
  /* because the EmacsManager is the widgets parent we have to
     offset the redisplay of the widget by the amount the text
     widget is inside the manager. */
  ac = 0;
  XtSetArg (al [ac], XtNwidth, 
	    (Dimension)IMAGE_INSTANCE_SUBWINDOW_WIDTH (ii)); ac++;
  XtSetArg (al [ac], XtNheight, 
	    (Dimension)IMAGE_INSTANCE_SUBWINDOW_HEIGHT (ii)); ac++;
  XtSetValues (wid, al, ac);
  /* finally get offsets in the frame */
  ac = 0;
  XtSetArg (al [ac], XtNx, &IMAGE_INSTANCE_X_WIDGET_XOFFSET (ii)); ac++;
  XtSetArg (al [ac], XtNy, &IMAGE_INSTANCE_X_WIDGET_YOFFSET (ii)); ac++;
  XtGetValues (FRAME_X_TEXT_WIDGET (f), al, ac);

  IMAGE_INSTANCE_SUBWINDOW_ID (ii) = (void*)wid; 

  free_widget_value (wv);
}

static Lisp_Object
x_widget_set_property (Lisp_Object image_instance, Lisp_Object prop,
		       Lisp_Object val)
{
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);

  if (EQ (prop, Q_text))
    {
      Extbyte* str=0;
      widget_value* wv = lw_get_all_values (IMAGE_INSTANCE_X_WIDGET_LWID (ii));
      CHECK_STRING (val);
      GET_C_STRING_OS_DATA_ALLOCA (val, str);
      wv->value = str;
      lw_modify_all_widgets (IMAGE_INSTANCE_X_WIDGET_LWID (ii), wv, False);
      return Qt;
    }
  return Qunbound;
}

/* get properties of a control */
static Lisp_Object
x_widget_property (Lisp_Object image_instance, Lisp_Object prop)
{
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  /* get the text from a control */
  if (EQ (prop, Q_text))
    {
      widget_value* wv = lw_get_all_values (IMAGE_INSTANCE_X_WIDGET_LWID (ii));
      return build_ext_string (wv->value, FORMAT_OS);
    }
  return Qunbound;
}

/* Instantiate a button widget. Unfortunately instantiated widgets are
   particular to a frame since they need to have a parent. It's not
   like images where you just select the image into the context you
   want to display it in and BitBlt it. So images instances can have a
   many-to-one relationship with things you see, whereas widgets can
   only be one-to-one (i.e. per frame) */
static void
x_button_instantiate (Lisp_Object image_instance, Lisp_Object instantiator,
		      Lisp_Object pointer_fg, Lisp_Object pointer_bg,
		      int dest_mask, Lisp_Object domain)
{
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  Lisp_Object gui = IMAGE_INSTANCE_WIDGET_ITEM (ii);
  Lisp_Object glyph = find_keyword_in_vector (instantiator, Q_image);
  widget_value* wv = xmalloc_widget_value ();

  button_item_to_widget_value (gui, wv, 1, 1);

  if (!NILP (glyph))
    {
      if (!IMAGE_INSTANCEP (glyph))
	glyph = glyph_image_instance (glyph, domain, ERROR_ME, 1);
    }

  x_widget_instantiate (image_instance, instantiator, pointer_fg,
			pointer_bg, dest_mask, domain, "button", wv);

  /* add the image if one was given */
  if (!NILP (glyph) && IMAGE_INSTANCEP (glyph))
    {
      Arg al [2];
      int ac =0;
#ifdef LWLIB_USES_MOTIF
      XtSetArg (al [ac], XmNlabelType, XmPIXMAP);	ac++;
      XtSetArg (al [ac], XmNlabelPixmap, XIMAGE_INSTANCE_X_PIXMAP (glyph));ac++;
#else
      XtSetArg (al [ac], XtNpixmap, XIMAGE_INSTANCE_X_PIXMAP (glyph));	ac++;
#endif
      XtSetValues (IMAGE_INSTANCE_X_WIDGET_ID (ii), al, ac);
    }
}

/* get properties of a button */
static Lisp_Object
x_button_property (Lisp_Object image_instance, Lisp_Object prop)
{
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  /* check the state of a button */
  if (EQ (prop, Q_selected))
    {
      widget_value* wv = lw_get_all_values (IMAGE_INSTANCE_X_WIDGET_LWID (ii));

      if (wv->selected)
	return Qt;
      else
	return Qnil;
    }
  return Qunbound;
}

/* instantiate a progress gauge */
static void
x_progress_gauge_instantiate (Lisp_Object image_instance, Lisp_Object instantiator,
			Lisp_Object pointer_fg, Lisp_Object pointer_bg,
			int dest_mask, Lisp_Object domain)
{
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  Lisp_Object gui = IMAGE_INSTANCE_WIDGET_ITEM (ii);
  widget_value* wv = xmalloc_widget_value ();

  button_item_to_widget_value (gui, wv, 1, 1);

  x_widget_instantiate (image_instance, instantiator, pointer_fg,
			pointer_bg, dest_mask, domain, "progress", wv);
}

/* set the properties of a progres guage */
static Lisp_Object
x_progress_gauge_set_property (Lisp_Object image_instance, Lisp_Object prop,
			 Lisp_Object val)
{
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);

  if (EQ (prop, Q_percent))
    {
      Arg al [1];
      CHECK_INT (val);
      XtSetArg (al[0], XtNvalue, XINT (val));
      XtSetValues (IMAGE_INSTANCE_X_WIDGET_ID (ii), al, 1);
      return Qt;
    }
  return Qunbound;
}

/* instantiate an edit control */
static void
x_edit_field_instantiate (Lisp_Object image_instance, Lisp_Object instantiator,
		    Lisp_Object pointer_fg, Lisp_Object pointer_bg,
		    int dest_mask, Lisp_Object domain)
{
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  Lisp_Object gui = IMAGE_INSTANCE_WIDGET_ITEM (ii);
  widget_value* wv = xmalloc_widget_value ();
  
  button_item_to_widget_value (gui, wv, 1, 1);
  
  x_widget_instantiate (image_instance, instantiator, pointer_fg,
			pointer_bg, dest_mask, domain, "text-field", wv);
}

/* instantiate a combo control */
static void
x_combo_box_instantiate (Lisp_Object image_instance, Lisp_Object instantiator,
		     Lisp_Object pointer_fg, Lisp_Object pointer_bg,
		     int dest_mask, Lisp_Object domain)
{
  struct Lisp_Image_Instance *ii = XIMAGE_INSTANCE (image_instance);
  Lisp_Object rest;
  Lisp_Object gui = IMAGE_INSTANCE_WIDGET_ITEM (ii);
  widget_value* wv = xmalloc_widget_value ();

  button_item_to_widget_value (gui, wv, 1, 1);
  
  x_widget_instantiate (image_instance, instantiator, pointer_fg,
			pointer_bg, dest_mask, domain, "combo-box", wv);
  /* add items to the combo box */
  LIST_LOOP (rest, Fplist_get (IMAGE_INSTANCE_WIDGET_PROPS (ii), Q_items, Qnil))
    {
#if 0
      Extbyte* str;
      XmString xmstr;
      GET_C_STRING_OS_DATA_ALLOCA (XCAR (rest), str);
      xmstr = XmStringCreate (str, XmSTRING_DEFAULT_CHARSET);
      XmListAddItem (IMAGE_INSTANCE_X_WIDGET_ID (ii), xmstr, 0);
      XmStringFree (xmstr);
#endif
    }
}


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_glyphs_x (void)
{
#if 0
  DEFSUBR (Fchange_subwindow_property);
#endif
}

void
console_type_create_glyphs_x (void)
{
  /* image methods */

  CONSOLE_HAS_METHOD (x, print_image_instance);
  CONSOLE_HAS_METHOD (x, finalize_image_instance);
  CONSOLE_HAS_METHOD (x, image_instance_equal);
  CONSOLE_HAS_METHOD (x, image_instance_hash);
  CONSOLE_HAS_METHOD (x, colorize_image_instance);
  CONSOLE_HAS_METHOD (x, init_image_instance_from_eimage);
  CONSOLE_HAS_METHOD (x, locate_pixmap_file);
  CONSOLE_HAS_METHOD (x, unmap_subwindow);
  CONSOLE_HAS_METHOD (x, map_subwindow);
  CONSOLE_HAS_METHOD (x, resize_subwindow);
  CONSOLE_HAS_METHOD (x, update_subwindow);
}

void
image_instantiator_format_create_glyphs_x (void)
{
  IIFORMAT_VALID_CONSOLE (x, nothing);
  IIFORMAT_VALID_CONSOLE (x, string);
  IIFORMAT_VALID_CONSOLE (x, formatted_string);
  IIFORMAT_VALID_CONSOLE (x, inherit);
#ifdef HAVE_XPM
  INITIALIZE_DEVICE_IIFORMAT (x, xpm);
  IIFORMAT_HAS_DEVMETHOD (x, xpm, instantiate);
#endif
#ifdef HAVE_JPEG
  IIFORMAT_VALID_CONSOLE (x, jpeg);
#endif
#ifdef HAVE_TIFF
  IIFORMAT_VALID_CONSOLE (x, tiff);
#endif  
#ifdef HAVE_PNG
  IIFORMAT_VALID_CONSOLE (x, png);
#endif  
#ifdef HAVE_GIF
  IIFORMAT_VALID_CONSOLE (x, gif);
#endif  
  INITIALIZE_DEVICE_IIFORMAT (x, xbm);
  IIFORMAT_HAS_DEVMETHOD (x, xbm, instantiate);

  INITIALIZE_DEVICE_IIFORMAT (x, subwindow);
  IIFORMAT_HAS_DEVMETHOD (x, subwindow, instantiate);
#ifdef LWLIB_USES_MOTIF
  /* button widget */
  INITIALIZE_DEVICE_IIFORMAT (x, button);
  IIFORMAT_HAS_DEVMETHOD (x, button, property);
  IIFORMAT_HAS_DEVMETHOD (x, button, instantiate);

  INITIALIZE_DEVICE_IIFORMAT (x, widget);
  IIFORMAT_HAS_DEVMETHOD (x, widget, property);
  IIFORMAT_HAS_DEVMETHOD (x, widget, set_property);
  /* progress gauge */
  INITIALIZE_DEVICE_IIFORMAT (x, progress_gauge);
  IIFORMAT_HAS_DEVMETHOD (x, progress_gauge, set_property);
  IIFORMAT_HAS_DEVMETHOD (x, progress_gauge, instantiate);
  /* text field */
  INITIALIZE_DEVICE_IIFORMAT (x, edit_field);
  IIFORMAT_HAS_DEVMETHOD (x, edit_field, instantiate);
#if 0 /* XmVERSION > 1*/
  /* combo box */
  INITIALIZE_DEVICE_IIFORMAT (x, combo_box);
  IIFORMAT_HAS_DEVMETHOD (x, combo_box, instantiate);
#endif
#endif
  INITIALIZE_IMAGE_INSTANTIATOR_FORMAT (cursor_font, "cursor-font");
  IIFORMAT_VALID_CONSOLE (x, cursor_font);

  IIFORMAT_HAS_METHOD (cursor_font, validate);
  IIFORMAT_HAS_METHOD (cursor_font, possible_dest_types);
  IIFORMAT_HAS_METHOD (cursor_font, instantiate);

  IIFORMAT_VALID_KEYWORD (cursor_font, Q_data, check_valid_string);
  IIFORMAT_VALID_KEYWORD (cursor_font, Q_foreground, check_valid_string);
  IIFORMAT_VALID_KEYWORD (cursor_font, Q_background, check_valid_string);

  INITIALIZE_IMAGE_INSTANTIATOR_FORMAT (font, "font");

  IIFORMAT_HAS_METHOD (font, validate);
  IIFORMAT_HAS_METHOD (font, possible_dest_types);
  IIFORMAT_HAS_METHOD (font, instantiate);
  IIFORMAT_VALID_CONSOLE (x, font);

  IIFORMAT_VALID_KEYWORD (font, Q_data, check_valid_string);
  IIFORMAT_VALID_KEYWORD (font, Q_foreground, check_valid_string);
  IIFORMAT_VALID_KEYWORD (font, Q_background, check_valid_string);

#ifdef HAVE_XFACE
  INITIALIZE_DEVICE_IIFORMAT (x, xface);
  IIFORMAT_HAS_DEVMETHOD (x, xface, instantiate);
#endif

  INITIALIZE_IMAGE_INSTANTIATOR_FORMAT (autodetect,
					"autodetect");

  IIFORMAT_HAS_METHOD (autodetect, validate);
  IIFORMAT_HAS_METHOD (autodetect, normalize);
  IIFORMAT_HAS_METHOD (autodetect, possible_dest_types);
  IIFORMAT_HAS_METHOD (autodetect, instantiate);
  IIFORMAT_VALID_CONSOLE (x, autodetect);

  IIFORMAT_VALID_KEYWORD (autodetect, Q_data, check_valid_string);
}

void
vars_of_glyphs_x (void)
{
  DEFVAR_LISP ("x-bitmap-file-path", &Vx_bitmap_file_path /*
A list of the directories in which X bitmap files may be found.
If nil, this is initialized from the "*bitmapFilePath" resource.
This is used by the `make-image-instance' function (however, note that if
the environment variable XBMLANGPATH is set, it is consulted first).
*/ );
  Vx_bitmap_file_path = Qnil;
}

void
complex_vars_of_glyphs_x (void)
{
#define BUILD_GLYPH_INST(variable, name)			\
  Fadd_spec_to_specifier					\
    (GLYPH_IMAGE (XGLYPH (variable)),				\
     vector3 (Qxbm, Q_data,					\
	      list3 (make_int (name##_width),			\
		     make_int (name##_height),			\
		     make_ext_string (name##_bits,		\
				      sizeof (name##_bits),	\
				      FORMAT_BINARY))),		\
     Qglobal, Qx, Qnil)

  BUILD_GLYPH_INST (Vtruncation_glyph, truncator);
  BUILD_GLYPH_INST (Vcontinuation_glyph, continuer);
  BUILD_GLYPH_INST (Vxemacs_logo, xemacs);
  BUILD_GLYPH_INST (Vhscroll_glyph, hscroll);

#undef BUILD_GLYPH_INST
}
