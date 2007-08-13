/* Sound functions.
   Copyright (C) 1992, 1993, 1994 Lucid Inc.
   Copyright (C) 1994, 1995 Free Software Foundation, Inc.

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

/* Originally written by Jamie Zawinski.
   Hacked on quite a bit by various others. */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#ifdef HAVE_X_WINDOWS
#include "console-x.h"
#endif

#ifdef HAVE_NEXTSTEP
#include "console-ns.h"
#endif

#include "commands.h"
#include "device.h"
#include "redisplay.h"
#include "sysdep.h"

#ifdef HAVE_NATIVE_SOUND
# include <netdb.h>
#endif

int bell_volume;
Lisp_Object Vsound_alist;
Lisp_Object Vsynchronous_sounds;
Lisp_Object Vnative_sound_only_on_console;
Lisp_Object Q_volume, Q_pitch, Q_duration, Q_sound;

/* These are defined in the appropriate file (sunplay.c, sgiplay.c,
   or hpplay.c). */

extern void play_sound_file (char *name, int volume);
extern void play_sound_data (unsigned char *data, int length, int volume);

#ifdef HAVE_NAS_SOUND
extern int nas_play_sound_file (char *name, int volume);
extern int nas_play_sound_data (unsigned char *data, int length, int volume);
extern int nas_wait_for_sounds (void);
extern char *nas_init_play (Display *);

Lisp_Object Qnas;
#endif

DEFUN ("play-sound-file", Fplay_sound_file, 1, 3, "fSound file name: ", /*
Play the named sound file on DEVICE's speaker at the specified volume
(0-100, default specified by the `bell-volume' variable).
The sound file must be in the Sun/NeXT U-LAW format except under Linux
where WAV files are also supported.
  DEVICE defaults to the selected device.
*/
       (file, volume, device))
{
  /* This function can call lisp */
  int vol;
#if defined (HAVE_NATIVE_SOUND) || defined (HAVE_NAS_SOUND)
  struct device *d = decode_device (device);
#endif
  struct gcpro gcpro1;

  CHECK_STRING (file);
  if (NILP (volume))
    vol = bell_volume;
  else
    {
      CHECK_INT (volume);
      vol = XINT (volume);
    }

  GCPRO1 (file);
  file = Fexpand_file_name (file, Qnil);
  if (NILP (Ffile_readable_p (file)))
    if (NILP (Ffile_exists_p (file)))
      error ("file does not exist.");
    else
      error ("file is unreadable.");
  UNGCPRO;

#ifdef HAVE_NAS_SOUND
  if (DEVICE_CONNECTED_TO_NAS_P (d))
    {
      char *fileext;

      GET_C_STRING_FILENAME_DATA_ALLOCA (file, fileext);
      /* #### NAS code should allow specification of a device. */
      if (nas_play_sound_file (fileext, vol))
	return Qnil;
    }
#endif /* HAVE_NAS_SOUND */

#ifdef HAVE_NATIVE_SOUND
  if (NILP (Vnative_sound_only_on_console) || DEVICE_ON_CONSOLE_P (d))
    {
      CONST char *fileext;

      GET_C_STRING_FILENAME_DATA_ALLOCA (file, fileext);
      /* The sound code doesn't like getting SIGIO interrupts.
	 Unix sucks! */
      stop_interrupts ();
      play_sound_file ((char *) fileext, vol);
      start_interrupts ();
      QUIT;
    }
#endif /* HAVE_NATIVE_SOUND */

  return Qnil;
}

static void
parse_sound_alist_elt (Lisp_Object elt,
		       Lisp_Object *volume,
		       Lisp_Object *pitch,
		       Lisp_Object *duration,
		       Lisp_Object *sound)
{
  *volume = Qnil;
  *pitch = Qnil;
  *duration = Qnil;
  *sound = Qnil;
  if (! CONSP (elt))
    return;

  /* The things we do for backward compatibility...
     I wish I had just forced this to be a plist to begin with.
   */

  if (SYMBOLP (elt) || STRINGP (elt))		/* ( name . <sound> ) */
    {
      *sound = elt;
    }
  else if (!CONSP (elt))
    {
      return;
    }
  else if (NILP (XCDR (elt)) &&		/* ( name <sound> ) */
	   (SYMBOLP (XCAR (elt)) ||
	    STRINGP (XCAR (elt))))
    {
      *sound = XCAR (elt);
    }
  else if (INT_OR_FLOATP (XCAR (elt)) &&	/* ( name <vol> . <sound> ) */
	   (SYMBOLP (XCDR (elt)) ||
	    STRINGP (XCDR (elt))))
    {
      *volume = XCAR (elt);
      *sound = XCDR (elt);
    }
  else if (INT_OR_FLOATP (XCAR (elt)) &&	/* ( name <vol> <sound> ) */
	   CONSP (XCDR (elt)) &&
	   NILP (XCDR (XCDR (elt))) &&
	   (SYMBOLP (XCAR (XCDR (elt))) ||
	    STRINGP (XCAR (XCDR (elt)))))
    {
      *volume = XCAR (elt);
      *sound = XCAR (XCDR (elt));
    }
  else if ((SYMBOLP (XCAR (elt)) ||	/* ( name <sound> . <vol> ) */
	    STRINGP (XCAR (elt))) &&
	   INT_OR_FLOATP (XCDR (elt)))
    {
      *sound = XCAR (elt);
      *volume = XCDR (elt);
    }
#if 0 /* this one is ambiguous with the plist form */
  else if ((SYMBOLP (XCAR (elt)) ||	/* ( name <sound> <vol> ) */
	    STRINGP (XCAR (elt))) &&
	   CONSP (XCDR (elt)) &&
	   NILP (XCDR (XCDR (elt))) &&
	   INT_OR_FLOATP (XCAR (XCDR (elt))))
    {
      *sound = XCAR (elt);
      *volume = XCAR (XCDR (elt));
    }
#endif /* 0 */
  else					/* ( name [ keyword <value> ]* ) */
    {
      while (CONSP (elt))
	{
	  Lisp_Object key, val;
	  key = XCAR (elt);
	  val = XCDR (elt);
	  if (!CONSP (val))
	    return;
	  elt = XCDR (val);
	  val = XCAR (val);
	  if (EQ (key, Q_volume))
	    {
	      if (INT_OR_FLOATP (val)) *volume = val;
	    }
	  else if (EQ (key, Q_pitch))
	    {
	      if (INT_OR_FLOATP (val)) *pitch = val;
	      if (NILP (*sound)) *sound = Qt;
	    }
	  else if (EQ (key, Q_duration))
	    {
	      if (INT_OR_FLOATP (val)) *duration = val;
	      if (NILP (*sound)) *sound = Qt;
	    }
	  else if (EQ (key, Q_sound))
	    {
	      if (SYMBOLP (val) || STRINGP (val)) *sound = val;
	    }
	}
    }
}

DEFUN ("play-sound", Fplay_sound, 1, 3, 0, /*
Play a sound of the provided type.
See the variable `sound-alist'.
*/
       (sound, volume, device))
{
  int looking_for_default = 0;
  /* variable `sound' is anything that can be a cdr in sound-alist */
  Lisp_Object new_volume, pitch, duration, data;
  int loop_count = 0;
  int vol, pit, dur;
  struct device *d = decode_device (device);

  /* NOTE!  You'd better not signal an error in here. */


 try_it_again:
  while (1)
    {
      if (SYMBOLP (sound))
	sound = Fcdr (Fassq (sound, Vsound_alist));
      parse_sound_alist_elt (sound, &new_volume, &pitch, &duration, &data);
      sound = data;
      if (NILP (volume)) volume = new_volume;
      if (EQ (sound, Qt) || EQ (sound, Qnil) || STRINGP (sound))
	break;
      if (loop_count++ > 500)	/* much bogosity has occurred */
	break;
    }

  if (NILP (sound) && !looking_for_default)
    {
      looking_for_default = 1;
      loop_count = 0;
      sound = Qdefault;
      goto try_it_again;
    }

  
  vol = (INT_OR_FLOATP (volume) ? XFLOATINT (volume) : bell_volume);
  pit = (INT_OR_FLOATP (pitch) ? XFLOATINT (pitch) : -1);
  dur = (INT_OR_FLOATP (duration) ? XFLOATINT (duration) : -1);

  /* If the sound is a string, and we're connected to Nas, do that.
     Else if the sound is a string, and we're on console, play it natively.
     Else just beep.
   */
#ifdef HAVE_NAS_SOUND
  if (DEVICE_CONNECTED_TO_NAS_P (d) && STRINGP (sound))
    {
      Extbyte *soundext;
      Extcount soundextlen;

      GET_STRING_BINARY_DATA_ALLOCA (sound, soundext, soundextlen);
      if (nas_play_sound_data (soundext, soundextlen, vol))
	return Qnil;
    }
#endif /* HAVE_NAS_SOUND */

#ifdef HAVE_NATIVE_SOUND
  if ((NILP (Vnative_sound_only_on_console) || DEVICE_ON_CONSOLE_P (d))
      && STRINGP (sound))
    {
      Extbyte *soundext;
      Extcount soundextlen;

      GET_STRING_BINARY_DATA_ALLOCA (sound, soundext, soundextlen);
      /* The sound code doesn't like getting SIGIO interrupts. Unix sucks! */
      stop_interrupts ();
      play_sound_data (soundext, soundextlen, vol);
      start_interrupts ();
      QUIT;
      return Qnil;
    }
#endif  /* HAVE_NATIVE_SOUND */

  DEVMETH (d, ring_bell, (d, vol, pit, dur));
  return Qnil;
}

DEFUN ("device-sound-enabled-p", Fdevice_sound_enabled_p, 0, 1, 0, /*
Return T iff DEVICE is able to play sound.  Defaults to selected device.
*/
       (device))
{
  struct device *d = decode_device(device);

#ifdef HAVE_NAS_SOUND
  if ( DEVICE_CONNECTED_TO_NAS_P (d) )
    return (Qt);
#endif
#ifdef HAVE_NATIVE_SOUND
  if ( DEVICE_ON_CONSOLE_P (d) )
    return (Qt);
#endif
  return Qnil;
}

DEFUN ("ding", Fding, 0, 3, 0, /*
Beep, or flash the frame.
Also, unless an argument is given,
terminate any keyboard macro currently executing.
When called from lisp, the second argument is what sound to make, and
the third argument is the device to make it in (defaults to the selected
device).
*/
       (arg, sound, device))
{
  struct device *d = decode_device (device);

  XSETDEVICE (device, d);
  if (NILP (arg) && !NILP (Vexecuting_macro))
    /* Stop executing a keyboard macro. */
    error ("Keyboard macro terminated by a command ringing the bell");
  else if (visible_bell && DEVMETH (d, flash, (d)))
    ;
  else
    Fplay_sound (sound, Qnil, device);

  return Qnil;
}

DEFUN ("wait-for-sounds", Fwait_for_sounds, 0, 1, 0, /*
Wait for all sounds to finish playing on DEVICE.
*/
       (device))
{
#ifdef HAVE_NAS_SOUND
  struct device *d = decode_device (device);
  if (DEVICE_CONNECTED_TO_NAS_P (d))
    {
      /* #### somebody fix this to be device-dependent. */
      nas_wait_for_sounds ();
    }
#endif
  return Qnil;
}

DEFUN ("connected-to-nas-p", Fconnected_to_nas_p, 0, 1, 0, /*
t if connected to NAS server for sounds on DEVICE.
*/
       (device))
{
#ifdef HAVE_NAS_SOUND
  struct device *d = decode_device (device);
  if (DEVICE_CONNECTED_TO_NAS_P (d))
    return Qt;
  else
    return Qnil;
#else
  return Qnil;
#endif
}
#ifdef HAVE_NAS_SOUND

static void
init_nas_sound (struct device *d)
{
  char *error;

#ifdef HAVE_X_WINDOWS
  if (DEVICE_X_P (d))
    {
      error = nas_init_play (DEVICE_X_DISPLAY (d));
      DEVICE_CONNECTED_TO_NAS_P (d) = !error;
      /* Print out the message? */
    }
#endif /* HAVE_X_WINDOWS */
}

#endif /* HAVE_NAS_SOUND */

#ifdef HAVE_NATIVE_SOUND

static void
init_native_sound (struct device *d)
{
  if (DEVICE_TTY_P (d) || DEVICE_STREAM_P (d))
    DEVICE_ON_CONSOLE_P (d) = 1;
#ifdef HAVE_X_WINDOWS
  else
    {
      /* When running on a machine with native sound support, we cannot use
	 digitized sounds as beeps unless emacs is running on the same machine
	 that $DISPLAY points to, and $DISPLAY points to frame 0 of that
	 machine.
	 */

      Display *display = DEVICE_X_DISPLAY (d);
      char *dpy = DisplayString (display);
      char *tail = (char *) strchr (dpy, ':');
      if (! tail ||
	  strncmp (tail, ":0", 2))
	DEVICE_ON_CONSOLE_P (d) = 0;
      else
	{
	  char dpyname[255], localname[255];

	  /* some systems can't handle SIGIO or SIGALARM in gethostbyname. */
	  stop_interrupts ();
	  strncpy (dpyname, dpy, tail-dpy);
	  dpyname [tail-dpy] = 0;
	  if (!*dpyname ||
	      !strcmp (dpyname, "unix") ||
	      !strcmp (dpyname, "localhost"))
	    DEVICE_ON_CONSOLE_P (d) = 1;
	  else if (gethostname (localname, sizeof (localname)))
	    DEVICE_ON_CONSOLE_P (d) = 0;	/* can't find hostname? */
	  else
	    {
	      /* We have to call gethostbyname() on the result of gethostname()
		 because the two aren't guarenteed to be the same name for the
		 same host: on some losing systems, one is a FQDN and the other
		 is not.  Here in the wide wonderful world of Unix it's rocket
		 science to obtain the local hostname in a portable fashion.
		 
		 And don't forget, gethostbyname() reuses the structure it
		 returns, so we have to copy the fucker before calling it
		 again.

		 Thank you master, may I have another.
		 */
	      struct hostent *h = gethostbyname (dpyname);
	      if (!h)
		DEVICE_ON_CONSOLE_P (d) = 0;
	      else
		{
		  char hn [255];
		  struct hostent *l;
		  strcpy (hn, h->h_name);
		  l = gethostbyname (localname);
		  DEVICE_ON_CONSOLE_P (d) = (l && !(strcmp (l->h_name, hn)));
		}
	    }
	  start_interrupts ();
	}
    }
#endif /* HAVE_X_WINDOWS */
}

#endif /* HAVE_NATIVE_SOUND */

void
init_device_sound (struct device *d)
{
#ifdef HAVE_NAS_SOUND
  init_nas_sound (d);
#endif

#ifdef HAVE_NATIVE_SOUND
  init_native_sound (d);
#endif
}

void
syms_of_sound (void)
{
  defkeyword (&Q_volume,   ":volume");
  defkeyword (&Q_pitch,    ":pitch");
  defkeyword (&Q_duration, ":duration");
  defkeyword (&Q_sound,    ":sound");

#ifdef HAVE_NAS_SOUND
  defsymbol (&Qnas, "nas");
#endif

  DEFSUBR (Fplay_sound_file);
  DEFSUBR (Fplay_sound);
  DEFSUBR (Fding);
  DEFSUBR (Fwait_for_sounds);
  DEFSUBR (Fconnected_to_nas_p);
  DEFSUBR (Fdevice_sound_enabled_p);
}


void
vars_of_sound (void)
{
#ifdef HAVE_NATIVE_SOUND
  Fprovide (intern ("native-sound"));
#endif
#ifdef HAVE_NAS_SOUND
  Fprovide (intern ("nas-sound"));
#endif

  DEFVAR_INT ("bell-volume", &bell_volume /*
*How loud to be, from 0 to 100.
*/ );
  bell_volume = 50;

  DEFVAR_LISP ("sound-alist", &Vsound_alist /*
An alist associating names with sounds.
When `beep' or `ding' is called with one of the name symbols, the associated
sound will be generated instead of the standard beep.

Each element of `sound-alist' is a list describing a sound.
The first element of the list is the name of the sound being defined.
Subsequent elements of the list are alternating keyword/value pairs:

   Keyword:	Value:
   -------	-----
   sound	A string of raw sound data, or the name of another sound to
		play.   The symbol `t' here means use the default X beep.
   volume	An integer from 0-100, defaulting to `bell-volume'
   pitch	If using the default X beep, the pitch (Hz) to generate.
   duration	If using the default X beep, the duration (milliseconds).

For compatibility, elements of `sound-alist' may also be:

   ( sound-name . <sound> )
   ( sound-name <volume> <sound> )

You should probably add things to this list by calling the function
load-sound-file.

Caveats:
 - You can only play audio data if running on the console screen of a
   Sun SparcStation, SGI, or HP9000s700.

 - The pitch, duration, and volume options are available everywhere, but
   many X servers ignore the `pitch' option.

The following beep-types are used by emacs itself:

    auto-save-error	when an auto-save does not succeed
    command-error	when the emacs command loop catches an error
    undefined-key	when you type a key that is undefined
    undefined-click	when you use an undefined mouse-click combination
    no-completion	during completing-read
    y-or-n-p		when you type something other than 'y' or 'n'
    yes-or-no-p  	when you type something other than 'yes' or 'no'
    default		used when nothing else is appropriate.

Other lisp packages may use other beep types, but these are the ones that
the C kernel of Emacs uses.
*/ );
  Vsound_alist = Qnil;

  DEFVAR_LISP ("synchronous-sounds", &Vsynchronous_sounds /*
Play sounds synchronously, if non-nil.
Only applies if NAS is used and supports asynchronous playing
of sounds.  Otherwise, sounds are always played synchronously.
*/ );
  Vsynchronous_sounds = Qnil;

  DEFVAR_LISP ("native-sound-only-on-console", &Vnative_sound_only_on_console /*
Non-nil value means play sounds only if XEmacs is running
on the system console.
Nil means always always play sounds, even if running on a non-console tty
or a secondary X display.

This variable only applies to native sound support.
*/ );
  Vnative_sound_only_on_console = Qt;

#if defined (HAVE_NATIVE_SOUND) && defined (hp9000s800)
  vars_of_hpplay ();
#endif
}
