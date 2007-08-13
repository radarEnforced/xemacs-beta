/* Buffer insertion/deletion and gap motion for XEmacs.
   Copyright (C) 1985, 1986, 1991, 1992, 1993, 1994, 1995
   Free Software Foundation, Inc.
   Copyright (C) 1995 Sun Microsystems, Inc.

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

/* Synched up with: Mule 2.0, FSF 19.30.  Diverges significantly. */

/* This file has been Mule-ized. */

/* Overhauled by Ben Wing, December 1994, for Mule implementation. */

/*
   There are three possible ways to specify positions in a buffer.  All
   of these are one-based: the beginning of the buffer is position or
   index 1, and 0 is not a valid position.
   
   As a "buffer position" (typedef Bufpos):

      This is an index specifying an offset in characters from the
      beginning of the buffer.  Note that buffer positions are
      logically *between* characters, not on a character.  The
      difference between two buffer positions specifies the number of
      characters between those positions.  Buffer positions are the
      only kind of position externally visible to the user.

   As a "byte index" (typedef Bytind):

      This is an index over the bytes used to represent the characters
      in the buffer.  If there is no Mule support, this is identical
      to a buffer position, because each character is represented
      using one byte.  However, with Mule support, many characters
      require two or more bytes for their representation, and so a
      byte index may be greater than the corresponding buffer
      position.

   As a "memory index" (typedef Memind):

      This is the byte index adjusted for the gap.  For positions
      before the gap, this is identical to the byte index.  For
      positions after the gap, this is the byte index plus the gap
      size.  There are two possible memory indices for the gap
      position; the memory index at the beginning of the gap should
      always be used, except in code that deals with manipulating the
      gap, where both indices may be seen.  The address of the
      character "at" (i.e. following) a particular position can be
      obtained from the formula

        buffer_start_address + memory_index(position) - 1

      except in the case of characters at the gap position.

   Other typedefs:
   ===============

      Emchar:
      -------
        This typedef represents a single Emacs character, which can be
	ASCII, ISO-8859, or some extended character, as would typically
	be used for Kanji.  Note that the representation of a character
	as an Emchar is *not* the same as the representation of that
	same character in a string; thus, you cannot do the standard
	C trick of passing a pointer to a character to a function that
	expects a string.

	An Emchar takes up 19 bits of representation and (for code
	compatibility and such) is compatible with an int.  This
	representation is visible on the Lisp level.  The important
	characteristics	of the Emchar representation are

	  -- values 0x00 - 0x7f represent ASCII.
	  -- values 0x80 - 0xff represent the right half of ISO-8859-1.
	  -- values 0x100 and up represent all other characters.

	This means that Emchar values are upwardly compatible with
	the standard 8-bit representation of ASCII/ISO-8859-1.

      Bufbyte:
      --------
        The data in a buffer or string is logically made up of Bufbyte
	objects, where a Bufbyte takes up the same amount of space as a
	char. (It is declared differently, though, to catch invalid
	usages.) Strings stored using Bufbytes are said to be in
	"internal format".  The important characteristics of internal
	format are

	  -- ASCII characters are represented as a single Bufbyte,
	     in the range 0 - 0x7f.
	  -- All other characters are represented as a Bufbyte in
	     the range 0x80 - 0x9f followed by one or more Bufbytes
	     in the range 0xa0 to 0xff.

	This leads to a number of desirable properties:

	  -- Given the position of the beginning of a character,
	     you can find the beginning of the next or previous
	     character in constant time.
	  -- When searching for a substring or an ASCII character
	     within the string, you need merely use standard
	     searching routines.

      array of char:
      --------------
        Strings that go in or out of Emacs are in "external format",
	typedef'ed as an array of char or a char *.  There is more
	than one external format (JIS, EUC, etc.) but they all
	have similar properties.  They are modal encodings,
	which is to say that the meaning of particular bytes is
	not fixed but depends on what "mode" the string is currently
	in (e.g. bytes in the range 0 - 0x7f might be
	interpreted as ASCII, or as Hiragana, or as 2-byte Kanji,
	depending on the current mode).  The mode starts out in
	ASCII/ISO-8859-1 and is switched using escape sequences --
	for example, in the JIS encoding, 'ESC $ B' switches to a
	mode where pairs of bytes in the range 0 - 0x7f
	are interpreted as Kanji characters.

	External-formatted data is generally desirable for passing
	data between programs because it is upwardly compatible
	with standard ASCII/ISO-8859-1 strings and may require
	less space than internal encodings such as the one
	described above.  In addition, some encodings (e.g. JIS)
	keep all characters (except the ESC used to switch modes)
	in the printing ASCII range 0x20 - 0x7e, which results in
	a much higher probability that the data will avoid being
	garbled in transmission.  Externally-formatted data is
	generally not very convenient to work with, however, and
	for this reason is usually converted to internal format
	before any work is done on the string.

	NOTE: filenames need to be in external format so that
	ISO-8859-1 characters come out correctly.

      Charcount:
      ----------
        This typedef represents a count of characters, such as
	a character offset into a string or the number of
	characters between two positions in a buffer.  The
	difference between two Bufpos's is a Charcount, and
	character positions in a string are represented using
	a Charcount.

      Bytecount:
      ----------
        Similar to a Charcount but represents a count of bytes.
	The difference between two Bytind's is a Bytecount.

	
   Usage of the various representations:
   =====================================

   Memory indices are used in low-level functions in insdel.c and for
   extent endpoints and marker positions.  The reason for this is that
   this way, the extents and markers don't need to be updated for most
   insertions, which merely shrink the gap and don't move any
   characters around in memory.

   (The beginning-of-gap memory index simplifies insertions w.r.t.
   markers, because text usually gets inserted after markers.  For
   extents, it is merely for consistency, because text can get
   inserted either before or after an extent's endpoint depending on
   the open/closedness of the endpoint.)

   Byte indices are used in other code that needs to be fast,
   such as the searching, redisplay, and extent-manipulation code.

   Buffer positions are used in all other code.  This is because this
   representation is easiest to work with (especially since Lisp
   code always uses buffer positions), necessitates the fewest
   changes to existing code, and is the safest (e.g. if the text gets
   shifted underneath a buffer position, it will still point to a
   character; if text is shifted under a byte index, it might point
   to the middle of a character, which would be bad).

   Similarly, Charcounts are used in all code that deals with strings
   except for code that needs to be fast, which used Bytecounts.

   Strings are always passed around internally using internal format.
   Conversions between external format are performed at the time
   that the data goes in or out of Emacs.
   
   Working with the various representations:
   ========================================= */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "device.h"
#include "frame.h"
#include "extents.h"
#include "insdel.h"
#include "lstream.h"
#include "redisplay.h"

/* We write things this way because it's very important the
   MAX_BYTIND_GAP_SIZE_3 is a multiple of 3. (As it happens,
   65535 is a multiple of 3, but this may not always be the
   case.) */

#define MAX_BUFPOS_GAP_SIZE_3 (65535/3)
#define MAX_BYTIND_GAP_SIZE_3 (3 * MAX_BUFPOS_GAP_SIZE_3)

short three_to_one_table[1 + MAX_BYTIND_GAP_SIZE_3];

/* Various macros modelled along the lines of those in buffer.h.
   Purposefully omitted from buffer.h because files other than this
   one should not be using them. */

/* Address of beginning of buffer.  This is an lvalue because
   BUFFER_ALLOC needs it to be. */
#define BUF_BEG_ADDR(buf) ((buf)->text->beg)

/* Set the address of beginning of buffer. */
#define SET_BUF_BEG_ADDR(buf, addr) do { (buf)->text->beg = (addr); } while (0)

/* Gap size.  */
#define BUF_GAP_SIZE(buf) ((buf)->text->gap_size + 0)

/* Set gap size.  */
#define SET_BUF_GAP_SIZE(buf, value) \
  do { (buf)->text->gap_size = (value); } while (0)

/* Gap location.  */ 
#define BI_BUF_GPT(buf) ((buf)->text->gpt + 0)
#define BUF_GPT_ADDR(buf) (BUF_BEG_ADDR (buf) + BI_BUF_GPT (buf) - 1)

/* Set gap location.  */
#define SET_BI_BUF_GPT(buf, value) do { (buf)->text->gpt = (value); } while (0)

/* Set end of buffer.  */ 
#define SET_BOTH_BUF_Z(buf, val, bival)		\
do						\
{						\
  (buf)->text->z = (bival);			\
  (buf)->text->bufz = (val);			\
} while (0)

# define GAP_CAN_HOLD_SIZE_P(buf, len) (BUF_GAP_SIZE (buf) >= (len))
# define SET_GAP_SENTINEL(buf)
# define BUF_END_SENTINEL_SIZE 0
# define SET_END_SENTINEL(buf)


/************************************************************************/
/*                    Charcount/Bytecount conversion                    */
/************************************************************************/

/* Optimization.  Do it.  Live it.  Love it.  */

#ifdef ERROR_CHECK_BUFPOS

Bytind
bufpos_to_bytind (struct buffer *buf, Bufpos x)
{
  Bytind retval = real_bufpos_to_bytind (buf, x);
  ASSERT_VALID_BYTIND_UNSAFE (buf, retval);
  return retval;
}

Bufpos
bytind_to_bufpos (struct buffer *buf, Bytind x)
{
  ASSERT_VALID_BYTIND_UNSAFE (buf, x);
  return real_bytind_to_bufpos (buf, x);
}

#endif /* ERROR_CHECK_BUFPOS */


/************************************************************************/
/*                verifying buffer and string positions                 */
/************************************************************************/

/* Functions below are tagged with either _byte or _char indicating
   whether they return byte or character positions.  For a buffer,
   a character position is a "Bufpos" and a byte position is a "Bytind".
   For strings, these are sometimes typed using "Charcount" and
   "Bytecount". */

/* Flags for the functions below are:

   GB_ALLOW_PAST_ACCESSIBLE

     The allowable range for the position is the entire buffer
     (BEG and Z), rather than the accessible portion.  For strings,
     this flag has no effect.

   GB_COERCE_RANGE

     If the position is outside the allowable range, return
     the lower or upper bound of the range, whichever is closer
     to the specified position.

   GB_NO_ERROR_IF_BAD

     If the position is outside the allowable range, return -1.

   GB_NEGATIVE_FROM_END

     If a value is negative, treat it as an offset from the end.
     Only applies to strings.

   The following additional flags apply only to the functions
   that return ranges:

   GB_ALLOW_NIL

     Either or both positions can be nil.  If FROM is nil,
     FROM_OUT will contain the lower bound of the allowed range.
     If TO is nil, TO_OUT will contain the upper bound of the
     allowed range.

   GB_CHECK_ORDER

     FROM must contain the lower bound and TO the upper bound
     of the range.  If the positions are reversed, an error is
     signalled.

   The following is a combination flag:

   GB_HISTORICAL_STRING_BEHAVIOR

     Equivalent to (GB_NEGATIVE_FROM_END | GB_ALLOW_NIL).
 */

/* Return a buffer position stored in a Lisp_Object.  Full
   error-checking is done on the position.  Flags can be specified to
   control the behavior of out-of-range values.  The default behavior
   is to require that the position is within the accessible part of
   the buffer (BEGV and ZV), and to signal an error if the position is
   out of range.

*/

Bufpos
get_buffer_pos_char (struct buffer *b, Lisp_Object pos, unsigned int flags)
{
  Bufpos ind;
  Bufpos min_allowed, max_allowed;

  CHECK_INT_COERCE_MARKER (pos);
  ind = XINT (pos);
  min_allowed = (flags & GB_ALLOW_PAST_ACCESSIBLE) ?
    BUF_BEG (b) : BUF_BEGV (b);
  max_allowed = (flags & GB_ALLOW_PAST_ACCESSIBLE) ?
    BUF_Z (b) : BUF_ZV (b);
    
  if (ind < min_allowed || ind > max_allowed)
    {
      if (flags & GB_COERCE_RANGE)
	ind = ind < min_allowed ? min_allowed : max_allowed;
      else if (flags & GB_NO_ERROR_IF_BAD)
	ind = -1;
      else
	{
	  Lisp_Object buffer;
	  XSETBUFFER (buffer, b);
	  args_out_of_range (buffer, pos);
	}
    }

  return ind;
}

Bytind
get_buffer_pos_byte (struct buffer *b, Lisp_Object pos, unsigned int flags)
{
  Bufpos bpos = get_buffer_pos_char (b, pos, flags);
  if (bpos < 0) /* could happen with GB_NO_ERROR_IF_BAD */
    return -1;
  return bufpos_to_bytind (b, bpos);
}

/* Return a pair of buffer positions representing a range of text,
   taken from a pair of Lisp_Objects.  Full error-checking is
   done on the positions.  Flags can be specified to control the
   behavior of out-of-range values.  The default behavior is to
   allow the range bounds to be specified in either order
   (however, FROM_OUT will always be the lower bound of the range
   and TO_OUT the upper bound),to require that the positions
   are within the accessible part of the buffer (BEGV and ZV),
   and to signal an error if the positions are out of range.
*/

void
get_buffer_range_char (struct buffer *b, Lisp_Object from, Lisp_Object to,
		       Bufpos *from_out, Bufpos *to_out, unsigned int flags)
{
  Bufpos min_allowed, max_allowed;

  min_allowed = (flags & GB_ALLOW_PAST_ACCESSIBLE) ?
    BUF_BEG (b) : BUF_BEGV (b);
  max_allowed = (flags & GB_ALLOW_PAST_ACCESSIBLE) ?
    BUF_Z (b) : BUF_ZV (b);

  if (NILP (from) && (flags & GB_ALLOW_NIL))
    *from_out = min_allowed;
  else
    *from_out = get_buffer_pos_char (b, from, flags | GB_NO_ERROR_IF_BAD);

  if (NILP (to) && (flags & GB_ALLOW_NIL))
    *to_out = max_allowed;
  else
    *to_out = get_buffer_pos_char (b, to, flags | GB_NO_ERROR_IF_BAD);

  if ((*from_out < 0 || *to_out < 0) && !(flags & GB_NO_ERROR_IF_BAD))
    {
      Lisp_Object buffer;
      XSETBUFFER (buffer, b);
      args_out_of_range_3 (buffer, from, to);
    }

  if (*from_out >= 0 && *to_out >= 0 && *from_out > *to_out)
    {
      if (flags & GB_CHECK_ORDER)
	signal_simple_error_2 ("start greater than end", from, to);
      else
	{
	  Bufpos temp;

	  temp = *from_out;
	  *from_out = *to_out;
	  *to_out = temp;
	}
    }
}

void
get_buffer_range_byte (struct buffer *b, Lisp_Object from, Lisp_Object to,
		       Bytind *from_out, Bytind *to_out, unsigned int flags)
{
  Bufpos s, e;

  get_buffer_range_char (b, from, to, &s, &e, flags);
  if (s >= 0)
    *from_out = bufpos_to_bytind (b, s);
  else /* could happen with GB_NO_ERROR_IF_BAD */
    *from_out = -1;
  if (e >= 0)
    *to_out = bufpos_to_bytind (b, e);
  else
    *to_out = -1;
}

static Charcount
get_string_pos_char_1 (Lisp_Object string, Lisp_Object pos, unsigned int flags,
		       Charcount known_length)
{
  Charcount ccpos;
  Charcount min_allowed = 0;
  Charcount max_allowed = known_length;

  /* Computation of KNOWN_LENGTH is potentially expensive so we pass
     it in. */
  CHECK_INT (pos);
  ccpos = XINT (pos);
  if (ccpos < 0 && flags & GB_NEGATIVE_FROM_END)
    ccpos += max_allowed;

  if (ccpos < min_allowed || ccpos > max_allowed)
    {
      if (flags & GB_COERCE_RANGE)
	ccpos = ccpos < min_allowed ? min_allowed : max_allowed;
      else if (flags & GB_NO_ERROR_IF_BAD)
	ccpos = -1;
      else
	args_out_of_range (string, pos);
    }

  return ccpos;
}

Charcount
get_string_pos_char (Lisp_Object string, Lisp_Object pos, unsigned int flags)
{
  return get_string_pos_char_1 (string, pos, flags,
				string_char_length (XSTRING (string)));
}

Bytecount
get_string_pos_byte (Lisp_Object string, Lisp_Object pos, unsigned int flags)
{
  Charcount ccpos = get_string_pos_char (string, pos, flags);
  if (ccpos < 0) /* could happen with GB_NO_ERROR_IF_BAD */
    return -1;
  return charcount_to_bytecount (XSTRING_DATA (string), ccpos);
}

void
get_string_range_char (Lisp_Object string, Lisp_Object from, Lisp_Object to,
		       Charcount *from_out, Charcount *to_out,
		       unsigned int flags)
{
  Charcount min_allowed = 0;
  Charcount max_allowed = string_char_length (XSTRING (string));

  if (NILP (from) && (flags & GB_ALLOW_NIL))
    *from_out = min_allowed;
  else
    *from_out = get_string_pos_char_1 (string, from,
				       flags | GB_NO_ERROR_IF_BAD,
				       max_allowed);

  if (NILP (to) && (flags & GB_ALLOW_NIL))
    *to_out = max_allowed;
  else
    *to_out = get_string_pos_char_1 (string, to,
				     flags | GB_NO_ERROR_IF_BAD,
				     max_allowed);

  if ((*from_out < 0 || *to_out < 0) && !(flags & GB_NO_ERROR_IF_BAD))
    args_out_of_range_3 (string, from, to);

  if (*from_out >= 0 && *to_out >= 0 && *from_out > *to_out)
    {
      if (flags & GB_CHECK_ORDER)
	signal_simple_error_2 ("start greater than end", from, to);
      else
	{
	  Bufpos temp;

	  temp = *from_out;
	  *from_out = *to_out;
	  *to_out = temp;
	}
    }
}

void
get_string_range_byte (Lisp_Object string, Lisp_Object from, Lisp_Object to,
		       Bytecount *from_out, Bytecount *to_out,
		       unsigned int flags)
{
  Charcount s, e;

  get_string_range_char (string, from, to, &s, &e, flags);
  if (s >= 0)
    *from_out = charcount_to_bytecount (XSTRING_DATA (string), s);
  else /* could happen with GB_NO_ERROR_IF_BAD */
    *from_out = -1;
  if (e >= 0)
    *to_out = charcount_to_bytecount (XSTRING_DATA (string), e);
  else
    *to_out = -1;

}

Bufpos
get_buffer_or_string_pos_char (Lisp_Object object, Lisp_Object pos,
			       unsigned int flags)
{
  if (STRINGP (object))
    return get_string_pos_char (object, pos, flags);
  else
    return get_buffer_pos_char (XBUFFER (object), pos, flags);
}

Bytind
get_buffer_or_string_pos_byte (Lisp_Object object, Lisp_Object pos,
			       unsigned int flags)
{
  if (STRINGP (object))
    return get_string_pos_byte (object, pos, flags);
  else
    return get_buffer_pos_byte (XBUFFER (object), pos, flags);
}

void
get_buffer_or_string_range_char (Lisp_Object object, Lisp_Object from,
				 Lisp_Object to, Bufpos *from_out,
				 Bufpos *to_out, unsigned int flags)
{
  if (STRINGP (object))
    get_string_range_char (object, from, to, from_out, to_out, flags);
  else
    get_buffer_range_char (XBUFFER (object), from, to, from_out, to_out,
			   flags);
}

void
get_buffer_or_string_range_byte (Lisp_Object object, Lisp_Object from,
				 Lisp_Object to, Bytind *from_out,
				 Bytind *to_out, unsigned int flags)
{
  if (STRINGP (object))
    get_string_range_byte (object, from, to, from_out, to_out, flags);
  else
    get_buffer_range_byte (XBUFFER (object), from, to, from_out, to_out,
			   flags);
}

Bufpos
buffer_or_string_accessible_begin_char (Lisp_Object object)
{
  if (STRINGP (object))
    return 0;
  return BUF_BEGV (XBUFFER (object));
}

Bufpos
buffer_or_string_accessible_end_char (Lisp_Object object)
{
  if (STRINGP (object))
    return string_char_length (XSTRING (object));
  return BUF_ZV (XBUFFER (object));
}

Bytind
buffer_or_string_accessible_begin_byte (Lisp_Object object)
{
  if (STRINGP (object))
    return 0;
  return BI_BUF_BEGV (XBUFFER (object));
}

Bytind
buffer_or_string_accessible_end_byte (Lisp_Object object)
{
  if (STRINGP (object))
    return XSTRING_LENGTH (object);
  return BI_BUF_ZV (XBUFFER (object));
}

Bufpos
buffer_or_string_absolute_begin_char (Lisp_Object object)
{
  if (STRINGP (object))
    return 0;
  return BUF_BEG (XBUFFER (object));
}

Bufpos
buffer_or_string_absolute_end_char (Lisp_Object object)
{
  if (STRINGP (object))
    return string_char_length (XSTRING (object));
  return BUF_Z (XBUFFER (object));
}

Bytind
buffer_or_string_absolute_begin_byte (Lisp_Object object)
{
  if (STRINGP (object))
    return 0;
  return BI_BUF_BEG (XBUFFER (object));
}

Bytind
buffer_or_string_absolute_end_byte (Lisp_Object object)
{
  if (STRINGP (object))
    return XSTRING_LENGTH (object);
  return BI_BUF_Z (XBUFFER (object));
}


/************************************************************************/
/*                     point and marker adjustment                      */
/************************************************************************/

/* just_set_point() is the only place `PT' is an lvalue in all of emacs.
   This function is called from set_buffer_point(), which is the function
   that the SET_PT and BUF_SET_PT macros expand into, and from the
   routines below that insert and delete text. (This is in cases where
   the point marker logically doesn't move but PT (being a byte index)
   needs to get adjusted.) */

/* Set point to a specified value.  This is used only when the value
   of point changes due to an insert or delete; it does not represent
   a conceptual change in point as a marker.  In particular, point is
   not crossing any interval boundaries, so there's no need to use the
   usual SET_PT macro.  In fact it would be incorrect to do so, because
   either the old or the new value of point is out of synch with the
   current set of intervals.  */

/* This gets called more than enough to make the function call
   overhead a significant factor so we've turned it into a macro. */
#define JUST_SET_POINT(buf, bufpos, ind)	\
do						\
{						\
  buf->bufpt = (bufpos);			\
  buf->pt = (ind);				\
} while (0)

/* Set a buffer's point. */

void
set_buffer_point (struct buffer *buf, Bufpos bufpos, Bytind bytpos)
{
  assert (bytpos >= BI_BUF_BEGV (buf) && bytpos <= BI_BUF_ZV (buf));
  if (bytpos == BI_BUF_PT (buf))
    return;
  JUST_SET_POINT (buf, bufpos, bytpos);
  MARK_POINT_CHANGED;
  assert (MARKERP (buf->point_marker));
  XMARKER (buf->point_marker)->memind =
    bytind_to_memind (buf, bytpos);

  /* FSF makes sure that PT is not being set within invisible text.
     However, this is the wrong place for that check.  The check
     should happen only at the next redisplay. */

  /* Some old coder said:

     "If there were to be hooks which were run when point entered/left an
     extent, this would be the place to put them.

     However, it's probably the case that such hooks should be implemented
     using a post-command-hook instead, to avoid running the hooks as a
     result of intermediate motion inside of save-excursions, for example."

     I definitely agree with this.  PT gets moved all over the place
     and it would be a Bad Thing for any hooks to get called, both for
     the reason above and because many callers are not prepared for
     a GC within this function. --ben
   */
}

/* Do the correct marker-like adjustment on MPOS (see below).  FROM, TO,
   and AMOUNT are as in adjust_markers().  If MPOS doesn't need to be
   adjusted, nothing will happen. */
Memind
do_marker_adjustment (Memind mpos, Memind from,
		      Memind to, Bytecount amount)
{
  if (amount > 0)
    {
      if (mpos > to && mpos < to + amount)
	mpos = to + amount;
    }
  else
    {
      if (mpos > from + amount && mpos <= from)
	mpos = from + amount;
    }
  if (mpos > from && mpos <= to)
    mpos += amount;
  return mpos;
}  

/* Do the following:

   (1) Add `amount' to the position of every marker in the current buffer
   whose current position is between `from' (exclusive) and `to' (inclusive).

   (2) Also, any markers past the outside of that interval, in the direction
   of adjustment, are first moved back to the near end of the interval
   and then adjusted by `amount'.

   This function is called in two different cases: when a region of
   characters adjacent to the gap is moved, causing the gap to shift
   to the other side of the region (in this case, `from' and `to'
   point to the old position of the region and there should be no
   markers affected by (2) because they would be inside the gap),
   or when a region of characters adjacent to the gap is wiped out,
   causing the gap to increase to include the region (in this case,
   `from' and `to' are the same, both pointing to the boundary
   between the gap and the deleted region, and there are no markers
   affected by (1)).
   
   The reason for the use of exclusive and inclusive is that markers at
   the gap always sit at the beginning, not at the end.
*/

static void
adjust_markers (struct buffer *buf, Memind from, Memind to,
		Bytecount amount)
{
  struct Lisp_Marker *m;

  for (m = BUF_MARKERS (buf); m; m = marker_next (m))
    m->memind = do_marker_adjustment (m->memind, from, to, amount);
}

/* Adjust markers whose insertion-type is t
   for an insertion of AMOUNT characters at POS.  */

static void
adjust_markers_for_insert (struct buffer *buf, Memind ind, Bytecount amount)
{
  struct Lisp_Marker *m;

  for (m = BUF_MARKERS (buf); m; m = marker_next (m))
    {
      if (m->insertion_type && m->memind == ind)
	m->memind += amount;
    }
}


/************************************************************************/
/*                  Routines for dealing with the gap                   */
/************************************************************************/

/* XEmacs requires an ANSI C compiler, and it damn well better have a
   working memmove() */
#define GAP_USE_BCOPY
#ifdef BCOPY_UPWARD_SAFE
# undef BCOPY_UPWARD_SAFE
#endif
#ifdef BCOPY_DOWNWARD_SAFE
# undef BCOPY_DOWNWARD_SAFE
#endif
#define BCOPY_UPWARD_SAFE 1
#define BCOPY_DOWNWARD_SAFE 1

/* maximum amount of memory moved in a single chunk.  Increasing this
   value improves gap-motion efficiency but decreases QUIT responsiveness
   time.  Was 32000 but today's processors are faster and files are
   bigger.  --ben */
#define GAP_MOVE_CHUNK 300000

/* Move the gap to POS, which is less than the current GPT. */

static void
gap_left (struct buffer *buf, Bytind pos)
{
  Bufbyte *to, *from;
  Bytecount i;
  Bytind new_s1;

  from = BUF_GPT_ADDR (buf);
  to = from + BUF_GAP_SIZE (buf);
  new_s1 = BI_BUF_GPT (buf);

  /* Now copy the characters.  To move the gap down,
     copy characters up.  */

  while (1)
    {
      /* I gets number of characters left to copy.  */
      i = new_s1 - pos;
      if (i == 0)
	break;
      /* If a quit is requested, stop copying now.
	 Change POS to be where we have actually moved the gap to.  */
      if (QUITP)
	{
	  pos = new_s1;
	  break;
	}
      /* Move at most GAP_MOVE_CHUNK chars before checking again for a quit. */
      if (i > GAP_MOVE_CHUNK)
	i = GAP_MOVE_CHUNK;
#ifdef GAP_USE_BCOPY
      if (i >= 128
	  /* bcopy is safe if the two areas of memory do not overlap
	     or on systems where bcopy is always safe for moving upward.  */
	  && (BCOPY_UPWARD_SAFE
	      || to - from >= 128))
	{
	  /* If overlap is not safe, avoid it by not moving too many
	     characters at once.  */
	  if (!BCOPY_UPWARD_SAFE && i > to - from)
	    i = to - from;
	  new_s1 -= i;
	  from -= i, to -= i;
	  memmove (to, from, i);
	}
      else
#endif
	{
	  new_s1 -= i;
	  while (--i >= 0)
	    *--to = *--from;
	}
    }

  /* Adjust markers, and buffer data structure, to put the gap at POS.
     POS is where the loop above stopped, which may be what was specified
     or may be where a quit was detected.  */
  adjust_markers (buf, pos, BI_BUF_GPT (buf), BUF_GAP_SIZE (buf));
  adjust_extents (make_buffer (buf), pos, BI_BUF_GPT (buf),
		  BUF_GAP_SIZE (buf));
  SET_BI_BUF_GPT (buf, pos);
  SET_GAP_SENTINEL (buf);
#ifdef ERROR_CHECK_EXTENTS
  sledgehammer_extent_check (make_buffer (buf));
#endif
  QUIT;
}

static void
gap_right (struct buffer *buf, Bytind pos)
{
  Bufbyte *to, *from;
  Bytecount i;
  Bytind new_s1;

  to = BUF_GPT_ADDR (buf);
  from = to + BUF_GAP_SIZE (buf);
  new_s1 = BI_BUF_GPT (buf);

  /* Now copy the characters.  To move the gap up,
     copy characters down.  */

  while (1)
    {
      /* I gets number of characters left to copy.  */
      i = pos - new_s1;
      if (i == 0)
	break;
      /* If a quit is requested, stop copying now.
	 Change POS to be where we have actually moved the gap to.  */
      if (QUITP)
	{
	  pos = new_s1;
	  break;
	}
      /* Move at most GAP_MOVE_CHUNK chars before checking again for a quit. */
      if (i > GAP_MOVE_CHUNK)
	i = GAP_MOVE_CHUNK;
#ifdef GAP_USE_BCOPY
      if (i >= 128
	  /* bcopy is safe if the two areas of memory do not overlap
	     or on systems where bcopy is always safe for moving downward. */
	  && (BCOPY_DOWNWARD_SAFE
	      || from - to >= 128))
	{
	  /* If overlap is not safe, avoid it by not moving too many
	     characters at once.  */
	  if (!BCOPY_DOWNWARD_SAFE && i > from - to)
	    i = from - to;
	  new_s1 += i;
	  memmove (to, from, i);
	  from += i, to += i;
	}
      else
#endif
	{
	  new_s1 += i;
	  while (--i >= 0)
	    *to++ = *from++;
	}
    }

  {
    int gsize = BUF_GAP_SIZE (buf);
    adjust_markers (buf, BI_BUF_GPT (buf) + gsize, pos + gsize, - gsize);
    adjust_extents (make_buffer (buf), BI_BUF_GPT (buf) + gsize, pos + gsize,
		    - gsize);
    SET_BI_BUF_GPT (buf, pos);
    SET_GAP_SENTINEL (buf);
#ifdef ERROR_CHECK_EXTENTS
    sledgehammer_extent_check (make_buffer (buf));
#endif
  }
  QUIT;
}

/* Move gap to position `pos'.
   Note that this can quit!  */

static void
move_gap (struct buffer *buf, Bytind pos)
{
  if (! BUF_BEG_ADDR (buf))
    abort ();
  if (pos < BI_BUF_GPT (buf))
    gap_left (buf, pos);
  else if (pos > BI_BUF_GPT (buf))
    gap_right (buf, pos);
}

/* Make the gap INCREMENT bytes longer.  */

static void
make_gap (struct buffer *buf, Bytecount increment)
{
  Bufbyte *result;
  Lisp_Object tem;
  Bytind real_gap_loc;
  Bytecount old_gap_size;

  /* If we have to get more space, get enough to last a while.  We use
     a geometric progession that saves on realloc space. */
  increment += 2000 + ((BI_BUF_Z (buf) - BI_BUF_BEG (buf)) / 8);

  /* Don't allow a buffer size that won't fit in an int
     even if it will fit in a Lisp integer.
     That won't work because so many places use `int'.  */
     
  if (BUF_Z (buf) - BUF_BEG (buf) + BUF_GAP_SIZE (buf) + increment
      >= ((unsigned) 1 << (min (INTBITS, VALBITS) - 1)))
    error ("Buffer exceeds maximum size");

  result = BUFFER_REALLOC (buf->text->beg,
			   BI_BUF_Z (buf) - BI_BUF_BEG (buf) +
			   BUF_GAP_SIZE (buf) + increment +
			   BUF_END_SENTINEL_SIZE);
  if (result == 0)
    memory_full ();
  SET_BUF_BEG_ADDR (buf, result);

  /* Prevent quitting in move_gap.  */
  tem = Vinhibit_quit;
  Vinhibit_quit = Qt;

  real_gap_loc = BI_BUF_GPT (buf);
  old_gap_size = BUF_GAP_SIZE (buf);

  /* Call the newly allocated space a gap at the end of the whole space.  */
  SET_BI_BUF_GPT (buf, BI_BUF_Z (buf) + BUF_GAP_SIZE (buf));
  SET_BUF_GAP_SIZE (buf, increment);

  /* Move the new gap down to be consecutive with the end of the old one.
     This adjusts the markers properly too.  */
  gap_left (buf, real_gap_loc + old_gap_size);

  /* Now combine the two into one large gap.  */
  SET_BUF_GAP_SIZE (buf, BUF_GAP_SIZE (buf) + old_gap_size);
  SET_BI_BUF_GPT (buf, real_gap_loc);
  SET_GAP_SENTINEL (buf);

  /* We changed the total size of the buffer (including gap),
     so we need to fix up the end sentinel. */
  SET_END_SENTINEL (buf);

  Vinhibit_quit = tem;
}


/************************************************************************/
/*                     Before/after-change processing                   */
/************************************************************************/

/* Those magic changes ... */

static void
buffer_signal_changed_region (struct buffer *buf, Bufpos start,
			      Bufpos end)
{
  /* The changed region is recorded as the number of unchanged
     characters from the beginning and from the end of the
     buffer.  This obviates much of the need of shifting the
     region around to compensate for insertions and deletions.
     */
  if (buf->changes->begin_unchanged < 0 ||
      buf->changes->begin_unchanged > start - BUF_BEG (buf))
    buf->changes->begin_unchanged = start - BUF_BEG (buf);
  if (buf->changes->end_unchanged < 0 ||
      buf->changes->end_unchanged > BUF_Z (buf) - end)
    buf->changes->end_unchanged = BUF_Z (buf) - end;
}

void
buffer_extent_signal_changed_region (struct buffer *buf, Bufpos start,
				     Bufpos end)
{
  if (buf->changes->begin_extent_unchanged < 0 ||
      buf->changes->begin_extent_unchanged > start - BUF_BEG (buf))
    buf->changes->begin_extent_unchanged = start - BUF_BEG (buf);
  if (buf->changes->end_extent_unchanged < 0 ||
      buf->changes->end_extent_unchanged > BUF_Z (buf) - end)
    buf->changes->end_extent_unchanged = BUF_Z (buf) - end;
}

void
buffer_reset_changes (struct buffer *buf)
{
  buf->changes->begin_unchanged = -1;
  buf->changes->end_unchanged = -1;
  buf->changes->begin_extent_unchanged = -1;
  buf->changes->end_extent_unchanged = -1;
  buf->changes->newline_was_deleted = 0;
}

static void
signal_after_change (struct buffer *buf, Bufpos start, Bufpos orig_end,
		     Bufpos new_end);

/* Call the after-change-functions according to the changes made so far
   and treat all further changes as single until the outermost
   multiple change exits.  This is called when the outermost multiple
   change exits and when someone is trying to make a change that violates
   the constraints specified in begin_multiple_change(), typically
   when nested multiple-change sessions occur. (There are smarter ways of
   dealing with nested multiple changes, but these rarely occur so there's
   probably no point in it.) */

/* #### This needs to keep track of what actually changed and only
   call the after-change functions on that region. */

static void
cancel_multiple_change (struct buffer *buf)
{
  /* This function can GC */
  /* Call the after-change-functions except when they've already been
     called or when there were no changes made to the buffer at all. */
  if (buf->text->changes->mc_begin != 0 &&
      buf->text->changes->mc_begin_signaled)
    {
      Bufpos real_mc_begin = buf->text->changes->mc_begin;
      buf->text->changes->mc_begin = 0;

      signal_after_change (buf, real_mc_begin, buf->text->changes->mc_orig_end,
			   buf->text->changes->mc_new_end);
    }
  else
    {
      buf->text->changes->mc_begin = 0;
    }
}

/* this is an unwind_protect, to ensure that the after-change-functions
   get called even in a non-local exit. */

static Lisp_Object
multiple_change_finish_up (Lisp_Object buffer)
{
  struct buffer *buf = XBUFFER (buffer);

  /* #### I don't know whether or not it should even be possible to
     get here with a dead buffer (though given how it is called I can
     see how it might be).  In any case, there isn't time before 19.14
     to find out. */
  if (!BUFFER_LIVE_P (buf))
    return Qnil;

  /* This function can GC */
  buf->text->changes->in_multiple_change = 0; /* do this first so that
						 errors in the after-change
						 functions don't mess things
						 up. */
  cancel_multiple_change (buf);
  return Qnil;
}

/* Call this function when you're about to make a number of buffer changes
   that should be considered a single change. (e.g. `replace-match' calls
   this.) You need to specify the START and END of the region that is
   going to be changed so that the before-change-functions are called
   with the correct arguments.  The after-change region is calculated
   automatically, however, and if changes somehow or other happen outside
   of the specified region, that will also be handled correctly.

   begin_multiple_change() returns a number (actually a specpdl depth)
   that you must pass to end_multiple_change() when you are done. */
   
int
begin_multiple_change (struct buffer *buf, Bufpos start, Bufpos end)
{
  /* This function can GC */
  int count = -1;
  if (buf->text->changes->in_multiple_change)
    {
      if (buf->text->changes->mc_begin != 0 &&
	  (start < buf->text->changes->mc_begin ||
	   end > buf->text->changes->mc_new_end))
	cancel_multiple_change (buf);
    }
  else
    {
      Lisp_Object buffer;

      buf->text->changes->mc_begin = start;
      buf->text->changes->mc_orig_end = buf->text->changes->mc_new_end = end;
      buf->text->changes->mc_begin_signaled = 0;
      count = specpdl_depth ();
      XSETBUFFER (buffer, buf);
      record_unwind_protect (multiple_change_finish_up, buffer);
    }
  buf->text->changes->in_multiple_change++;
  /* We don't call before-change-functions until signal_before_change()
     is called, in case there is a read-only or other error. */
  return count;
}

void
end_multiple_change (struct buffer *buf, int count)
{
  assert (buf->text->changes->in_multiple_change > 0);
  buf->text->changes->in_multiple_change--;
  if (!buf->text->changes->in_multiple_change)
    unbind_to (count, Qnil);
}

static int inside_change_hook;

static Lisp_Object
change_function_restore (Lisp_Object buffer)
{
  Fset_buffer (buffer);
  inside_change_hook = 0;
  return Qnil;
}

static int in_first_change;

static Lisp_Object
first_change_hook_restore (Lisp_Object buffer)
{
  Fset_buffer (buffer);
  in_first_change = 0;
  return Qnil;
}

/* Signal an initial modification to the buffer.  */

static void
signal_first_change (struct buffer *buf)
{
  /* This function can GC */
  Lisp_Object buffer;
  XSETBUFFER (buffer, buf);

  if (!in_first_change)
    {
      if (!preparing_for_armageddon &&
	  !NILP (symbol_value_in_buffer (Qfirst_change_hook, buffer)))
	{
	  int speccount = specpdl_depth ();
	  record_unwind_protect (first_change_hook_restore, buffer);
	  set_buffer_internal (buf);
	  in_first_change = 1;
	  run_hook (Qfirst_change_hook);
	  unbind_to (speccount, Qnil);
	}
    }
}

/* Signal a change to the buffer immediately before it happens.
   START and END are the bounds of the text to be changed. */

static void
signal_before_change (struct buffer *buf, Bufpos start, Bufpos end)
{
  /* This function can GC */
  Lisp_Object buffer;
  XSETBUFFER (buffer, buf);

  if (!inside_change_hook)
    {
      /* Are we in a multiple-change session? */
      if (buf->text->changes->in_multiple_change &&
	  buf->text->changes->mc_begin != 0)
	{
	  /* If we're violating the constraints of the session,
	     call the after-change-functions as necessary for the
	     changes already made and treat further changes as
	     single. */
	  if (start < buf->text->changes->mc_begin ||
	      end > buf->text->changes->mc_new_end)
	    cancel_multiple_change (buf);
	  /* Do nothing if this is not the first change in the session. */
	  else if (buf->text->changes->mc_begin_signaled)
	    return;
	  else
	    {
	      /* First time through; call the before-change-functions
		 specifying the entire region to be changed. (Note that
		 we didn't call before-change-functions in
		 begin_multiple_change() because the buffer might be
		 read-only, etc.) */
	      start = buf->text->changes->mc_begin;
	      end = buf->text->changes->mc_new_end;
	    }
	}

      /* If buffer is unmodified, run a special hook for that case.  */
      if (BUF_SAVE_MODIFF (buf) >= BUF_MODIFF (buf))
	signal_first_change (buf);

      /* Now in any case run the before-change-functions if any.  */

      if (!preparing_for_armageddon &&
	  (!NILP (symbol_value_in_buffer (Qbefore_change_functions, buffer)) ||
	   /* Obsolete, for compatibility */
	   !NILP (symbol_value_in_buffer (Qbefore_change_function, buffer))))
	{
	  int speccount = specpdl_depth ();
	  record_unwind_protect (change_function_restore, Fcurrent_buffer ());
	  set_buffer_internal (buf);
	  inside_change_hook = 1;
	  va_run_hook_with_args (Qbefore_change_functions, 2,
				 make_int (start), make_int (end));
	  /* Obsolete, for compatibility */
	  va_run_hook_with_args (Qbefore_change_function, 2,
				 make_int (start), make_int (end));
 	  unbind_to (speccount, Qnil);
	}

      /* Only now do we indicate that the before-change-functions have
	 been called, in case some function throws out. */
      buf->text->changes->mc_begin_signaled = 1;
    }

  /* #### At this point we should map over extents calling
     modification-hooks, insert-before-hooks and insert-after-hooks
     of relevant extents */
}

/* Signal a change immediately after it happens.
   START is the bufpos of the start of the changed text.
   ORIG_END is the bufpos of the end of the before-changed text.
   NEW_END is the bufpos of the end of the after-changed text.
 */

static void
signal_after_change (struct buffer *buf, Bufpos start, Bufpos orig_end,
		     Bufpos new_end)
{
  /* This function can GC */
  Lisp_Object buffer;
  XSETBUFFER (buffer, buf);

  /* always do this. */
  buffer_signal_changed_region (buf, start, new_end);
  font_lock_maybe_update_syntactic_caches (buf, start, orig_end, new_end);

  if (!inside_change_hook)
    {
      if (buf->text->changes->in_multiple_change &&
	  buf->text->changes->mc_begin != 0)
	{
	  assert (start >= buf->text->changes->mc_begin &&
		  start <= buf->text->changes->mc_new_end);
	  assert (orig_end >= buf->text->changes->mc_begin &&
		  orig_end <= buf->text->changes->mc_new_end);
	  buf->text->changes->mc_new_end += new_end - orig_end;
	  return; /* after-change-functions signalled when all changes done */
	}

      if (!preparing_for_armageddon &&
	  (!NILP (symbol_value_in_buffer (Qafter_change_functions, buffer)) ||
	   /* Obsolete, for compatibility */
	   !NILP (symbol_value_in_buffer (Qafter_change_function, buffer))))
	{
	  int speccount = specpdl_depth ();
	  record_unwind_protect (change_function_restore, Fcurrent_buffer ());
	  set_buffer_internal (buf);
	  inside_change_hook = 1;
	  /* The actual after-change functions take slightly
	     different arguments than what we were passed. */
	  va_run_hook_with_args (Qafter_change_functions, 3,
				 make_int (start), make_int (new_end),
				 make_int (orig_end - start));
	  /* Obsolete, for compatibility */
	  va_run_hook_with_args (Qafter_change_function, 3,
				 make_int (start), make_int (new_end),
				 make_int (orig_end - start));
 	  unbind_to (speccount, Qnil);
	}
    }

  /* #### At this point we should map over extents calling
     some sort of modification hooks of relevant extents */
}

/* Call this if you're about to change the region of BUFFER from START
   to END.  This checks the read-only properties of the region, calls
   the necessary modification hooks, and warns the next redisplay that
   it should pay attention to that area.  */

static void
prepare_to_modify_buffer (struct buffer *buf, Bufpos start, Bufpos end,
			  int lockit)
{
  /* This function can GC */
  barf_if_buffer_read_only (buf, start, end);

  /* if this is the first modification, see about locking the buffer's
     file */
  if (!NILP (buf->filename) && lockit &&
      BUF_SAVE_MODIFF (buf) >= BUF_MODIFF (buf))
    {
#ifdef CLASH_DETECTION
      if (!NILP (buf->file_truename))
	/* Make binding buffer-file-name to nil effective.  */
	lock_file (buf->file_truename);
#else
      Lisp_Object buffer;
      XSETBUFFER (buffer, buf);
      /* At least warn if this file has changed on disk since it was visited.*/
      if (NILP (Fverify_visited_file_modtime (buffer))
	  && !NILP (Ffile_exists_p (buf->filename)))
	call1_in_buffer (buf, intern ("ask-user-about-supersession-threat"),
			 buf->filename);
#endif /* not CLASH_DETECTION */
    }

  signal_before_change (buf, start, end);

#ifdef REGION_CACHE_NEEDS_WORK
  if (buf->newline_cache)
    invalidate_region_cache (buf,
                             buf->newline_cache,
                             start - BUF_BEG (buf), BUF_Z (buf) - end);
  if (buf->width_run_cache)
    invalidate_region_cache (buf,
                             buf->width_run_cache,
                             start - BUF_BEG (buf), BUF_Z (buf) - end);
#endif

#if 0 /* FSFmacs */
  Vdeactivate_mark = Qt;
#endif

  buf->point_before_scroll = Qnil;

  /* BUF_MODIFF (buf)++; -- should be done by callers (insert, delete range)
     else record_first_change isn't called */
}


/************************************************************************/
/*                        Insertion of strings                          */
/************************************************************************/

void
fixup_internal_substring (CONST Bufbyte *nonreloc, Lisp_Object reloc,
			  Bytecount offset, Bytecount *len)
{
  assert ((nonreloc && NILP (reloc)) || (!nonreloc && STRINGP (reloc)));

  if (*len < 0)
    {
      if (nonreloc)
	*len = strlen ((CONST char *) nonreloc) - offset;
      else
	*len = XSTRING_LENGTH (reloc) - offset;
    }
  assert (*len >= 0);
  if (STRINGP (reloc))
    {
      assert (offset >= 0 && offset <= XSTRING_LENGTH (reloc));
      assert (offset + *len <= XSTRING_LENGTH (reloc));
    }
}

/* Insert a string into BUF at Bufpos POS.  The string data comes
   from one of two sources: constant, non-relocatable data (specified
   in NONRELOC), or a Lisp string object (specified in RELOC), which
   is relocatable and may have extent data that needs to be copied
   into the buffer.  OFFSET and LENGTH specify the substring of the
   data that is actually to be inserted.  As a special case, if POS
   is -1, insert the string at point and move point to the end of the
   string.

   Normally, markers at the insertion point end up before the
   inserted string.  If INSDEL_BEFORE_MARKERS is set in flags, however,
   they end up after the string.

   INSDEL_NO_LOCKING is kludgy and is used when insert-file-contents is
   visiting a new file; it inhibits the locking checks normally done
   before modifying a buffer.  Similar checks were already done
   in the higher-level Lisp functions calling insert-file-contents. */

Charcount
buffer_insert_string_1 (struct buffer *buf, Bufpos pos,
			CONST Bufbyte *nonreloc, Lisp_Object reloc,
			Bytecount offset, Bytecount length,
			int flags)
{
  /* This function can GC */
  struct gcpro gcpro1;
  Bytind ind;
  Charcount cclen;
  int move_point = 0;

  /* Defensive steps just in case a buffer gets deleted and a calling
     function doesn't notice it. */
  if (!BUFFER_LIVE_P (buf))
    return 0;

  fixup_internal_substring (nonreloc, reloc, offset, &length);

  if (pos == -1)
    {
      pos = BUF_PT (buf);
      move_point = 1;
    }

#ifdef I18N3
  /* #### See the comment in print_internal().  If this buffer is marked
     as translatable, then Fgettext() should be called on obj if it
     is a string. */
#endif

  /* Make sure that point-max won't exceed the size of an emacs int. */
  {
    Lisp_Object temp;
  
    XSETINT (temp, (int) (length + BUF_Z (buf)));
    if ((int) (length + BUF_Z (buf)) != XINT (temp))
      error ("maximum buffer size exceeded");
  }

  /* theoretically not necessary -- caller should GCPRO */
  GCPRO1 (reloc);

  prepare_to_modify_buffer (buf, pos, pos, !(flags & INSDEL_NO_LOCKING));

  /* Defensive steps in case the before-change-functions fuck around */
  if (!BUFFER_LIVE_P (buf))
    {
      UNGCPRO;
      /* Bad bad pre-change function. */
      return 0;
    }

  /* Make args be valid again.  prepare_to_modify_buffer() might have
     modified the buffer. */
  if (pos < BUF_BEGV (buf))
    pos = BUF_BEGV (buf);
  if (pos > BUF_ZV (buf))
    pos = BUF_ZV (buf);

  /* string may have been relocated up to this point */
  if (STRINGP (reloc))
    nonreloc = XSTRING_DATA (reloc);

  ind = bufpos_to_bytind (buf, pos);
  cclen = bytecount_to_charcount (nonreloc + offset, length);

  if (ind != BI_BUF_GPT (buf))
    /* #### if debug-on-quit is invoked and the user changes the
       buffer, bad things can happen.  This is a rampant problem
       in Emacs. */
    move_gap (buf, ind); /* may QUIT */
  if (! GAP_CAN_HOLD_SIZE_P (buf, length))
    make_gap (buf, length - BUF_GAP_SIZE (buf));

  record_insert (buf, pos, cclen);
  BUF_MODIFF (buf)++;
  MARK_BUFFERS_CHANGED;

  /* string may have been relocated up to this point */
  if (STRINGP (reloc))
    nonreloc = XSTRING_DATA (reloc);

  memcpy (BUF_GPT_ADDR (buf), nonreloc + offset, length);

  SET_BUF_GAP_SIZE (buf, BUF_GAP_SIZE (buf) - length);
  SET_BI_BUF_GPT (buf, BI_BUF_GPT (buf) + length);
  SET_BOTH_BUF_ZV (buf, BUF_ZV (buf) + cclen, BI_BUF_ZV (buf) + length);
  SET_BOTH_BUF_Z (buf, BUF_Z (buf) + cclen, BI_BUF_Z (buf) + length);
  SET_GAP_SENTINEL (buf);

  process_extents_for_insertion (make_buffer (buf), ind, length);
  /* We know the gap is at IND so the cast is OK. */
  adjust_markers_for_insert (buf, (Memind) ind, length);

  /* Point logically doesn't move, but may need to be adjusted because
     it's a byte index.  point-marker doesn't change because it's a
     memory index. */
  if (BI_BUF_PT (buf) > ind)
    JUST_SET_POINT (buf, BUF_PT (buf) + cclen, BI_BUF_PT (buf) + length);

  /* Well, point might move. */
  if (move_point)
    BI_BUF_SET_PT (buf, ind + length);

  if (STRINGP (reloc))
    splice_in_string_extents (reloc, buf, ind, length, offset);

  if (flags & INSDEL_BEFORE_MARKERS)
    {
      /* ind - 1 is correct because the FROM argument is exclusive.
	 I formerly used DEC_BYTIND() but that caused problems at the
	 beginning of the buffer. */
      adjust_markers (buf, ind - 1, ind, length);
    }

  signal_after_change (buf, pos, pos, pos + cclen);

  UNGCPRO;

  return cclen;
}


/* The following functions are interfaces onto the above function,
   for inserting particular sorts of data.  In all the functions,
   BUF and POS specify the buffer and location where the insertion is
   to take place. (If POS is -1, text is inserted at point and point
   moves forward past the text.) FLAGS is as above. */

Charcount
buffer_insert_raw_string_1 (struct buffer *buf, Bufpos pos,
			    CONST Bufbyte *nonreloc, Bytecount length,
			    int flags)
{
  /* This function can GC */
  return buffer_insert_string_1 (buf, pos, nonreloc, Qnil, 0, length,
				 flags);
}

Charcount
buffer_insert_lisp_string_1 (struct buffer *buf, Bufpos pos, Lisp_Object str,
			     int flags)
{
  /* This function can GC */
  assert (STRINGP (str));
  return buffer_insert_string_1 (buf, pos, 0, str, 0,
				 XSTRING_LENGTH (str),
				 flags);
}

/* Insert the null-terminated string S (in external format). */

Charcount
buffer_insert_c_string_1 (struct buffer *buf, Bufpos pos, CONST char *s,
			  int flags)
{
  /* This function can GC */
     
  CONST char *translated = GETTEXT (s);
  return buffer_insert_string_1 (buf, pos, (CONST Bufbyte *) translated, Qnil,
				 0, strlen (translated), flags);
}

Charcount
buffer_insert_emacs_char_1 (struct buffer *buf, Bufpos pos, Emchar ch,
			    int flags)
{
  /* This function can GC */
  Bufbyte str[MAX_EMCHAR_LEN];
  Bytecount len;

  len = set_charptr_emchar (str, ch);
  return buffer_insert_string_1 (buf, pos, str, Qnil, 0, len, flags);
}

Charcount
buffer_insert_c_char_1 (struct buffer *buf, Bufpos pos, char c,
			int flags)
{
  /* This function can GC */
  return buffer_insert_emacs_char_1 (buf, pos, (Emchar) (unsigned char) c,
				     flags);
}
  
Charcount
buffer_insert_from_buffer_1 (struct buffer *buf, Bufpos pos,
			     struct buffer *buf2, Bufpos pos2,
			     Charcount length, int flags)
{
  /* This function can GC */
  Lisp_Object str = make_string_from_buffer (buf2, pos2, length);
  return buffer_insert_string_1 (buf, pos, 0, str, 0,
				 XSTRING_LENGTH (str), flags);
}


/************************************************************************/
/*                        Deletion of ranges                            */
/************************************************************************/

/* Delete characters in buffer from FROM up to (but not including) TO.  */

void
buffer_delete_range (struct buffer *buf, Bufpos from, Bufpos to, int flags)
{
  /* This function can GC */
  Charcount numdel;
  Bytind bi_from, bi_to;
  Bytecount bc_numdel;
  int shortage;
  Lisp_Object bufobj = Qnil;

  /* Defensive steps just in case a buffer gets deleted and a calling
     function doesn't notice it. */
  if (!BUFFER_LIVE_P (buf))
    return;

  /* Make args be valid */
  if (from < BUF_BEGV (buf))
    from = BUF_BEGV (buf);
  if (to > BUF_ZV (buf))
    to = BUF_ZV (buf);
  if ((numdel = to - from) <= 0)
    return;

  prepare_to_modify_buffer (buf, from, to, !(flags & INSDEL_NO_LOCKING));

  /* Defensive steps in case the before-change-functions fuck around */
  if (!BUFFER_LIVE_P (buf))
    /* Bad bad pre-change function. */
    return;

  /* Make args be valid again.  prepare_to_modify_buffer() might have
     modified the buffer. */
  if (from < BUF_BEGV (buf))
    from = BUF_BEGV (buf);
  if (to > BUF_ZV (buf))
    to = BUF_ZV (buf);
  if ((numdel = to - from) <= 0)
    return;

  XSETBUFFER (bufobj, buf);

  /* Redisplay needs to know if a newline was in the deleted region.
     If we've already marked the changed region as having a deleted
     newline there is no use in performing the check. */
  if (!buf->changes->newline_was_deleted)
    {
      scan_buffer (buf, '\n', from, to, 1, &shortage, 1);
      if (!shortage)
	buf->changes->newline_was_deleted = 1;
    }

  bi_from = bufpos_to_bytind (buf, from);
  bi_to = bufpos_to_bytind (buf, to);
  bc_numdel = bi_to - bi_from;

  /* Make sure the gap is somewhere in or next to what we are deleting.  */
  if (bi_to < BI_BUF_GPT (buf))
    gap_left (buf, bi_to);
  if (bi_from > BI_BUF_GPT (buf))
    gap_right (buf, bi_from);

  record_delete (buf, from, numdel);
  BUF_MODIFF (buf)++;
  MARK_BUFFERS_CHANGED;

  /* Relocate point as if it were a marker.  */
  if (bi_from < BI_BUF_PT (buf))
    {
      if (BI_BUF_PT (buf) < bi_to)
	JUST_SET_POINT (buf, from, bi_from);
      else
	JUST_SET_POINT (buf, BUF_PT (buf) - numdel,
			BI_BUF_PT (buf) - bc_numdel);
    }

  /* Detach any extents that are completely within the range [FROM, TO],
     if the extents are detachable.

     This must come AFTER record_delete(), so that the appropriate extents
     will be present to be recorded, and BEFORE the gap size is increased,
     as otherwise we will be confused about where the extents end. */
  process_extents_for_deletion (bufobj, bi_from, bi_to, 0);

  /* Relocate all markers pointing into the new, larger gap
     to point at the end of the text before the gap.  */
  adjust_markers (buf,
		  (bi_to + BUF_GAP_SIZE (buf)),
		  (bi_to + BUF_GAP_SIZE (buf)),
                  (- bc_numdel - BUF_GAP_SIZE (buf)));

  /* Relocate any extent endpoints just like markers. */
  adjust_extents_for_deletion (bufobj, bi_from, bi_to, BUF_GAP_SIZE (buf),
			       bc_numdel);

  SET_BUF_GAP_SIZE (buf, BUF_GAP_SIZE (buf) + bc_numdel);
  SET_BOTH_BUF_ZV (buf, BUF_ZV (buf) - numdel, BI_BUF_ZV (buf) - bc_numdel);
  SET_BOTH_BUF_Z (buf, BUF_Z (buf) - numdel, BI_BUF_Z (buf) - bc_numdel);
  SET_BI_BUF_GPT (buf, bi_from);
  SET_GAP_SENTINEL (buf);

#ifdef ERROR_CHECK_EXTENTS
  sledgehammer_extent_check (bufobj);
#endif

  signal_after_change (buf, from, to, from);
}


/************************************************************************/
/*                    Replacement of characters                         */
/************************************************************************/

/* Replace the character at POS in buffer B with CH. */

void
buffer_replace_char (struct buffer *b, Bufpos pos, Emchar ch,
		     int not_real_change, int force_lock_check)
{
  /* This function can GC */
  Bufbyte curstr[MAX_EMCHAR_LEN];
  Bufbyte newstr[MAX_EMCHAR_LEN];
  Bytecount curlen, newlen;

  /* Defensive steps just in case a buffer gets deleted and a calling
     function doesn't notice it. */
  if (!BUFFER_LIVE_P (b))
    return;

  curlen = BUF_CHARPTR_COPY_CHAR (b, pos, curstr);
  newlen = set_charptr_emchar (newstr, ch);

  if (curlen == newlen)
    {
      /* then we can just replace the text. */
      prepare_to_modify_buffer (b, pos, pos + 1,
				!not_real_change || force_lock_check);
      /* Defensive steps in case the before-change-functions fuck around */
      if (!BUFFER_LIVE_P (b))
	/* Bad bad pre-change function. */
	return;

      /* Make args be valid again.  prepare_to_modify_buffer() might have
	 modified the buffer. */
      if (pos < BUF_BEGV (b))
	pos = BUF_BEGV (b);
      if (pos >= BUF_ZV (b))
	pos = BUF_ZV (b) - 1;
      if (pos < BUF_BEGV (b))
	/* no more characters in buffer! */
	return;

      if (BUF_FETCH_CHAR (b, pos) == '\n')
	b->changes->newline_was_deleted = 1;
      MARK_BUFFERS_CHANGED;
      if (!not_real_change)
	{
	  record_change (b, pos, 1);
	  BUF_MODIFF (b)++;
	}
      memcpy (BUF_BYTE_ADDRESS (b, pos), newstr, newlen);
      signal_after_change (b, pos, pos + 1, pos + 1);
    }
  else
    {
      /* must implement as deletion followed by insertion. */
      buffer_delete_range (b, pos, pos + 1, 0);
      /* Defensive steps in case the before-change-functions fuck around */
      if (!BUFFER_LIVE_P (b))
	/* Bad bad pre-change function. */
	return;

      /* Make args be valid again.  prepare_to_modify_buffer() might have
	 modified the buffer. */
      if (pos < BUF_BEGV (b))
	pos = BUF_BEGV (b);
      if (pos >= BUF_ZV (b))
	pos = BUF_ZV (b) - 1;
      if (pos < BUF_BEGV (b))
	/* no more characters in buffer! */
	return;
      buffer_insert_string_1 (b, pos, newstr, Qnil, 0, newlen, 0);
    }
}


/************************************************************************/
/*                            Other functions                           */
/************************************************************************/

/* Make a string from a buffer.  This needs to take into account the gap,
   and add any necessary extents from the buffer. */

Lisp_Object
make_string_from_buffer (struct buffer *buf, Bufpos pos, Charcount length)
{
  /* This function can GC */
  Lisp_Object val;
  struct gcpro gcpro1;
  Bytind bi_ind;
  Bytecount bi_len;

  bi_ind = bufpos_to_bytind (buf, pos);
  bi_len = bufpos_to_bytind (buf, pos + length) - bi_ind;

  val = make_uninit_string (bi_len);
  GCPRO1 (val);

  add_string_extents (val, buf, bi_ind, bi_len);

  {
    Bytecount len1 = BI_BUF_GPT (buf) - bi_ind;
    Bufbyte *start1 = BI_BUF_BYTE_ADDRESS (buf, bi_ind);
    Bufbyte *dest = XSTRING_DATA (val);

    if (len1 < 0)
      {
	/* Completely after gap */
	memcpy (dest, start1, bi_len);
      }
    else if (bi_len <= len1)
      {
	/* Completely before gap */
	memcpy (dest, start1, bi_len);
      }
    else
      {
	/* Spans gap */
	Bytind pos2 = bi_ind + len1;
	Bufbyte *start2 = BI_BUF_BYTE_ADDRESS (buf, pos2);

	memcpy (dest, start1, len1);
	memcpy (dest + len1, start2, bi_len - len1);
      }
  }

  UNGCPRO;
  return val;
}

void
barf_if_buffer_read_only (struct buffer *buf, Bufpos from, Bufpos to)
{
  Lisp_Object buffer = Qnil;
  Lisp_Object iro;

  XSETBUFFER (buffer, buf);
 back:
  iro = (buf == current_buffer ? Vinhibit_read_only :
	 symbol_value_in_buffer (Qinhibit_read_only, buffer));
  if (!NILP (iro) && !CONSP (iro))
    return;
  if (NILP (iro) && !NILP (buf->read_only))
    {
      Fsignal (Qbuffer_read_only, (list1 (buffer)));
      goto back;
    }
  if (from > 0)
    {
      if (to < 0)
	to = from;
      verify_extent_modification (buffer,
				  bufpos_to_bytind (buf, from),
				  bufpos_to_bytind (buf, to),
				  iro);
    }
}

void
find_charsets_in_bufbyte_string (unsigned char *charsets, CONST Bufbyte *str,
				 Bytecount len)
{
  /* Telescope this. */
  charsets[0] = 1;
}

void
find_charsets_in_emchar_string (unsigned char *charsets, CONST Emchar *str,
				Charcount len)
{
  /* Telescope this. */
  charsets[0] = 1;
}

int
bufbyte_string_displayed_columns (CONST Bufbyte *str, Bytecount len)
{
  int cols = 0;
  CONST Bufbyte *end = str + len;

  while (str < end)
    {
      cols++;
      INC_CHARPTR (str);
    }

  return cols;
}

int
emchar_string_displayed_columns (CONST Emchar *str, Charcount len)
{
  int cols = 0;
  int i;

  for (i = 0; i < len; i++)
    cols += XCHARSET_COLUMNS (CHAR_CHARSET (str[i]));

  return cols;
}

/* NOTE: Does not reset the Dynarr. */

void
convert_bufbyte_string_into_emchar_dynarr (CONST Bufbyte *str, Bytecount len,
					   emchar_dynarr *dyn)
{
  CONST Bufbyte *strend = str + len;

  while (str < strend)
    {
      Emchar ch = charptr_emchar (str);
      Dynarr_add (dyn, ch);
      INC_CHARPTR (str);
    }
}

int
convert_bufbyte_string_into_emchar_string (CONST Bufbyte *str, Bytecount len,
					   Emchar *arr)
{
  CONST Bufbyte *strend = str + len;
  Charcount newlen = 0;
  while (str < strend)
    {
      Emchar ch = charptr_emchar (str);
      arr[newlen++] = ch;
      INC_CHARPTR (str);
    }
  return newlen;
}

/* Convert an array of Emchars into the equivalent string representation.
   Store into the given Bufbyte dynarr.  Does not reset the dynarr.
   Does not add a terminating zero. */

void
convert_emchar_string_into_bufbyte_dynarr (Emchar *arr, int nels,
					  bufbyte_dynarr *dyn)
{
  Bufbyte str[MAX_EMCHAR_LEN];
  Bytecount len;
  int i;

  for (i = 0; i < nels; i++)
    {
      len = set_charptr_emchar (str, arr[i]);
      Dynarr_add_many (dyn, str, len);
    }
}

/* Convert an array of Emchars into the equivalent string representation.
   Malloc the space needed for this and return it.  If LEN_OUT is not a
   NULL pointer, store into LEN_OUT the number of Bufbytes in the
   malloc()ed string.  Note that the actual number of Bufbytes allocated
   is one more than this: the returned string is zero-terminated. */

Bufbyte *
convert_emchar_string_into_malloced_string (Emchar *arr, int nels,
					   Bytecount *len_out)
{
  /* Damn zero-termination. */
  Bufbyte *str = (Bufbyte *) alloca (nels * MAX_EMCHAR_LEN + 1);
  Bufbyte *strorig = str;
  Bytecount len;
  
  int i;

  for (i = 0; i < nels; i++)
    str += set_charptr_emchar (str, arr[i]);
  *str = '\0';
  len = str - strorig;
  str = xmalloc (1 + len);
  memcpy (str, strorig, 1 + len);
  if (len_out)
    *len_out = len;
  return str;
}


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
vars_of_insdel (void)
{
  int i;

  inside_change_hook = 0;
  in_first_change = 0;

  for (i = 0; i <= MAX_BYTIND_GAP_SIZE_3; i++)
    three_to_one_table[i] = i / 3;
}

void
init_buffer_text (struct buffer *b, int indirect_p)
{
  if (!indirect_p)
    {
      SET_BUF_GAP_SIZE (b, 20);
      (void) BUFFER_ALLOC (b->text->beg,
			   BUF_GAP_SIZE (b) + BUF_END_SENTINEL_SIZE);
      if (! BUF_BEG_ADDR (b))
	memory_full ();
      
      SET_BI_BUF_GPT (b, 1);
      SET_BOTH_BUF_Z (b, 1, 1);
      SET_GAP_SENTINEL (b);
      SET_END_SENTINEL (b);

      BUF_MODIFF (b) = 1;
      BUF_SAVE_MODIFF (b) = 1;

      JUST_SET_POINT (b, 1, 1);
      SET_BOTH_BUF_BEGV (b, 1, 1);
      SET_BOTH_BUF_ZV (b, 1, 1);

      b->text->changes =
	(struct buffer_text_change_data *)
	  xmalloc (sizeof (*b->text->changes));
      memset (b->text->changes, 0, sizeof (*b->text->changes));
    }
  else
    {
      JUST_SET_POINT (b, BUF_PT (b->base_buffer), BI_BUF_PT (b->base_buffer));
      SET_BOTH_BUF_BEGV (b, BUF_BEGV (b->base_buffer),
			 BI_BUF_BEGV (b->base_buffer));
      SET_BOTH_BUF_ZV (b, BUF_ZV (b->base_buffer),
			 BI_BUF_ZV (b->base_buffer));
    }

  b->changes =
    (struct each_buffer_change_data *) xmalloc (sizeof (*b->changes));
  memset (b->changes, 0, sizeof (*b->changes));
  BUF_FACECHANGE (b) = 1;

#ifdef REGION_CACHE_NEEDS_WORK
  b->newline_cache = 0;
  b->width_run_cache = 0;
  b->width_table = Qnil;
#endif
}

void
uninit_buffer_text (struct buffer *b, int indirect_p)
{
  if (!indirect_p)
    {
      BUFFER_FREE (b->text->beg);
      xfree (b->text->changes);
    }
  xfree (b->changes);

#ifdef REGION_CACHE_NEEDS_WORK
  if (b->newline_cache)
    {
      free_region_cache (b->newline_cache);
      b->newline_cache = 0;
    }
  if (b->width_run_cache)
    {
      free_region_cache (b->width_run_cache);
      b->width_run_cache = 0;
    }
  b->width_table = Qnil;
#endif
}
