/* Functions to handle multilingual characters.
   Copyright (C) 1992, 1995 Free Software Foundation, Inc.
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

/* Synched up with: Mule 2.3.  Not in FSF. */

/* Rewritten by Ben Wing <wing@666.com>. */

#include <config.h>
#include "lisp.h"

#include "buffer.h"
#include "chartab.h"
#include "elhash.h"
#include "lstream.h"
#include "device.h"
#include "faces.h"

/* The various pre-defined charsets. */

Lisp_Object Vcharset_ascii;
Lisp_Object Vcharset_control_1;
Lisp_Object Vcharset_latin_iso8859_1;
Lisp_Object Vcharset_latin_iso8859_2;
Lisp_Object Vcharset_latin_iso8859_3;
Lisp_Object Vcharset_latin_iso8859_4;
Lisp_Object Vcharset_cyrillic_iso8859_5;
Lisp_Object Vcharset_arabic_iso8859_6;
Lisp_Object Vcharset_greek_iso8859_7;
Lisp_Object Vcharset_hebrew_iso8859_8;
Lisp_Object Vcharset_latin_iso8859_9;
Lisp_Object Vcharset_thai_tis620;
Lisp_Object Vcharset_katakana_jisx0201;
Lisp_Object Vcharset_latin_jisx0201;
Lisp_Object Vcharset_japanese_jisx0208_1978;
Lisp_Object Vcharset_japanese_jisx0208;
Lisp_Object Vcharset_japanese_jisx0212;
Lisp_Object Vcharset_chinese_gb2312;
Lisp_Object Vcharset_chinese_big5_1;
Lisp_Object Vcharset_chinese_big5_2;
Lisp_Object Vcharset_chinese_cns11643_1;
Lisp_Object Vcharset_chinese_cns11643_2;
Lisp_Object Vcharset_korean_ksc5601;
Lisp_Object Vcharset_composite;

/* Hashtables for composite chars.  One maps string representing
   composed chars to their equivalent chars; one goes the
   other way. */
Lisp_Object Vcomposite_char_char2string_hashtable;
Lisp_Object Vcomposite_char_string2char_hashtable;

/* Table of charsets indexed by leading byte. */
Lisp_Object charset_by_leading_byte[128];

/* Table of charsets indexed by type/final-byte/direction. */
Lisp_Object charset_by_attributes[4][128][2];

static int composite_char_row_next;
static int composite_char_col_next;

/* Table of number of bytes in the string representation of a character
   indexed by the first byte of that representation.

   This value can be derived other ways -- e.g. something like

   (BYTE_ASCII_P (first_byte) ? 1 :
    XCHARSET_REP_BYTES (CHARSET_BY_LEADING_BYTE (first_byte)))

   but it's faster this way. */

Bytecount rep_bytes_by_first_byte[0xA0] =
{ /* 16 x 8 ones for ASCII */
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  /* 1 x 8 for Dimension-1 official Mule charsets */
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  /* 0x90 - 0x9d are Dimension-2 official */
  /* 0x9e is Dimension-1 private */
  /* 0x9f is Dimension-2 private */
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4
};

Lisp_Object Qcharsetp;

/* Qdoc_string, Qdimension, Qchars defined in general.c */
Lisp_Object Qregistry, Qfinal, Qgraphic;
Lisp_Object Qdirection;
Lisp_Object Qreverse_direction_charset;
Lisp_Object Qccl_program;

Lisp_Object Qascii, Qcontrol_1,
  
  Qlatin_iso8859_1,
  Qlatin_iso8859_2,
  Qlatin_iso8859_3,
  Qlatin_iso8859_4,
  Qcyrillic_iso8859_5,
  Qarabic_iso8859_6,
  Qgreek_iso8859_7,
  Qhebrew_iso8859_8,
  Qlatin_iso8859_9,
  
  Qthai_tis620,
  
  Qkatakana_jisx0201, Qlatin_jisx0201,
  Qjapanese_jisx0208_1978,
  Qjapanese_jisx0208,
  Qjapanese_jisx0212,
  
  Qchinese_gb2312,
  Qchinese_big5_1, Qchinese_big5_2,
  Qchinese_cns11643_1, Qchinese_cns11643_2,
  
  Qkorean_ksc5601, Qcomposite;

Lisp_Object Ql2r, Qr2l;

Lisp_Object Vcharset_hashtable;

static Bufbyte next_allocated_1_byte_leading_byte;
static Bufbyte next_allocated_2_byte_leading_byte;

/* Composite characters are characters constructed by overstriking two
   or more regular characters.

   1) The old Mule implementation involves storing composite characters
      in a buffer as a tag followed by all of the actual characters
      used to make up the composite character.  I think this is a bad
      idea; it greatly complicates code that wants to handle strings
      one character at a time because it has to deal with the possibility
      of great big ungainly characters.  It's much more reasonable to
      simply store an index into a table of composite characters.

   2) The current implementation only allows for 16,384 separate
      composite characters over the lifetime of the XEmacs process.
      This could become a potential problem if the user
      edited lots of different files that use composite characters.
      Due to FSF bogosity, increasing the number of allowable
      composite characters under Mule would decrease the number
      of possible faces that can exist.  Mule already has shrunk
      this to 2048, and further shrinkage would become uncomfortable.
      No such problems exist in XEmacs.

      Composite characters could be represented as 0x80 C1 C2 C3,
      where each C[1-3] is in the range 0xA0 - 0xFF.  This allows
      for slightly under 2^20 (one million) composite characters
      over the XEmacs process lifetime, and you only need to
      increase the size of a Mule character from 19 to 21 bits.
      Or you could use 0x80 C1 C2 C3 C4, allowing for about
      85 million (slightly over 2^26) composite characters. */


/************************************************************************/
/*                       Basic Emchar functions                         */
/************************************************************************/

/* Convert a non-ASCII Mule character C into a one-character Mule-encoded
   string in STR.  Returns the number of bytes stored.
   Do not call this directly.  Use the macro set_charptr_emchar() instead.
 */

Bytecount
non_ascii_set_charptr_emchar (Bufbyte *str, Emchar c)
{
  Bufbyte *p;
  Bufbyte lb;
  int c1, c2;
  Lisp_Object charset;

  p = str;
  BREAKUP_CHAR (c, charset, c1, c2);
  lb = CHAR_LEADING_BYTE (c);
  if (LEADING_BYTE_PRIVATE_P (lb))
    *p++ = PRIVATE_LEADING_BYTE_PREFIX (lb);
  *p++ = lb;
  if (EQ (charset, Vcharset_control_1))
    c1 += 0x20;
  *p++ = c1 | 0x80;
  if (c2)
    *p++ = c2 | 0x80;

  return (p - str);
}

/* Return the first character from a Mule-encoded string in STR,
   assuming it's non-ASCII.  Do not call this directly.
   Use the macro charptr_emchar() instead. */

Emchar
non_ascii_charptr_emchar (CONST Bufbyte *str)
{
  Bufbyte i0 = *str, i1, i2 = 0;
  Lisp_Object charset;

  if (i0 == LEADING_BYTE_CONTROL_1)
    return (Emchar) (*++str - 0x20);

  if (LEADING_BYTE_PREFIX_P (i0))
    i0 = *++str;

  i1 = *++str & 0x7F;

  charset = CHARSET_BY_LEADING_BYTE (i0);
  if (XCHARSET_DIMENSION (charset) == 2)
    i2 = *++str & 0x7F;

  return MAKE_CHAR (charset, i1, i2);
}

/* Return whether CH is a valid Emchar, assuming it's non-ASCII.
   Do not call this directly.  Use the macro valid_char_p() instead. */

int
non_ascii_valid_char_p (Emchar ch)
{
  int f1, f2, f3;

  /* Must have only lowest 19 bits set */
  if (ch & ~0x7FFFF)
    return 0;

  f1 = CHAR_FIELD1 (ch);
  f2 = CHAR_FIELD2 (ch);
  f3 = CHAR_FIELD3 (ch);

  if (f1 == 0)
    {
      Lisp_Object charset;

      if (f2 < MIN_CHAR_FIELD2_OFFICIAL ||
	  (f2 > MAX_CHAR_FIELD2_OFFICIAL && f2 < MIN_CHAR_FIELD2_PRIVATE) ||
	   f2 > MAX_CHAR_FIELD2_PRIVATE)
	return 0;
      if (f3 < 0x20)
	return 0;

      if (f3 != 0x20 && f3 != 0x7F)
	return 1;

      /*
	 NOTE: This takes advantage of the fact that
	 FIELD2_TO_OFFICIAL_LEADING_BYTE and
	 FIELD2_TO_PRIVATE_LEADING_BYTE are the same.
	 */
      charset = CHARSET_BY_LEADING_BYTE (f2 + FIELD2_TO_OFFICIAL_LEADING_BYTE);
      return (XCHARSET_CHARS (charset) == 96);
    }
  else
    {
      Lisp_Object charset;

      if (f1 < MIN_CHAR_FIELD1_OFFICIAL ||
	  (f1 > MAX_CHAR_FIELD1_OFFICIAL && f1 < MIN_CHAR_FIELD1_PRIVATE) ||
	  f1 > MAX_CHAR_FIELD1_PRIVATE)
	return 0;
      if (f2 < 0x20 || f3 < 0x20)
	return 0;

      if (f1 + FIELD1_TO_OFFICIAL_LEADING_BYTE == LEADING_BYTE_COMPOSITE)
	{
	  if (UNBOUNDP (Fgethash (make_int (ch),
				  Vcomposite_char_char2string_hashtable,
				  Qunbound)))
	    return 0;
	  return 1;
	}

      if (f2 != 0x20 && f2 != 0x7F && f3 != 0x20 && f3 != 0x7F)
	return 1;

      if (f1 <= MAX_CHAR_FIELD1_OFFICIAL)
	charset =
	  CHARSET_BY_LEADING_BYTE (f1 + FIELD1_TO_OFFICIAL_LEADING_BYTE);
      else
	charset =
	  CHARSET_BY_LEADING_BYTE (f1 + FIELD1_TO_PRIVATE_LEADING_BYTE);

      return (XCHARSET_CHARS (charset) == 96);
    }
}


/************************************************************************/
/*                       Basic string functions                         */
/************************************************************************/

/* Copy the character pointed to by PTR into STR, assuming it's
   non-ASCII.  Do not call this directly.  Use the macro
   charptr_copy_char() instead. */

Bytecount
non_ascii_charptr_copy_char (CONST Bufbyte *ptr, Bufbyte *str)
{
  Bufbyte *strptr = str;
  *strptr = *ptr++;
  switch (REP_BYTES_BY_FIRST_BYTE (*strptr))
    {
      /* Notice fallthrough. */
    case 4: *++strptr = *ptr++;
    case 3: *++strptr = *ptr++;
    case 2: *++strptr = *ptr;
      break;
    default:
      abort ();
    }
  return strptr + 1 - str;
}


/************************************************************************/
/*                        streams of Emchars                            */
/************************************************************************/

/* Treat a stream as a stream of Emchar's rather than a stream of bytes.
   The functions below are not meant to be called directly; use
   the macros in insdel.h. */

Emchar
Lstream_get_emchar_1 (Lstream *stream, int ch)
{
  Bufbyte str[MAX_EMCHAR_LEN];
  Bufbyte *strptr = str;

  str[0] = (Bufbyte) ch;
  switch (REP_BYTES_BY_FIRST_BYTE (ch))
    {
      /* Notice fallthrough. */
    case 4:
      ch = Lstream_getc (stream);
      assert (ch >= 0);
      *++strptr = (Bufbyte) ch;
    case 3:
      ch = Lstream_getc (stream);
      assert (ch >= 0);
      *++strptr = (Bufbyte) ch;
    case 2:
      ch = Lstream_getc (stream);
      assert (ch >= 0);
      *++strptr = (Bufbyte) ch;
      break;
    default:
      abort ();
    }
  return charptr_emchar (str);
}

int
Lstream_fput_emchar (Lstream *stream, Emchar ch)
{
  Bufbyte str[MAX_EMCHAR_LEN];
  Bytecount len = set_charptr_emchar (str, ch);
  return Lstream_write (stream, str, len);
}

void
Lstream_funget_emchar (Lstream *stream, Emchar ch)
{
  Bufbyte str[MAX_EMCHAR_LEN];
  Bytecount len = set_charptr_emchar (str, ch);
  Lstream_unread (stream, str, len);
}


/************************************************************************/
/*                            charset object                            */
/************************************************************************/

static Lisp_Object mark_charset (Lisp_Object, void (*) (Lisp_Object));
static void print_charset (Lisp_Object, Lisp_Object, int);
DEFINE_LRECORD_IMPLEMENTATION ("charset", charset,
                               mark_charset, print_charset, 0, 0, 0,
			       struct Lisp_Charset);

static Lisp_Object
mark_charset (Lisp_Object obj, void (*markobj) (Lisp_Object))
{
  struct Lisp_Charset *cs = XCHARSET (obj);

  (markobj) (cs->doc_string);
  (markobj) (cs->registry);
  (markobj) (cs->ccl_program);
  return cs->name;
}

static void
print_charset (Lisp_Object obj, Lisp_Object printcharfun, int escapeflag)
{
  struct Lisp_Charset *cs = XCHARSET (obj);
  char buf[200];
  
  if (print_readably)
    error ("printing unreadable object #<charset %s 0x%x>",
           string_data (XSYMBOL (CHARSET_NAME (cs))->name),
	   cs->header.uid);

  write_c_string ("#<charset ", printcharfun);
  print_internal (CHARSET_NAME (cs), printcharfun, 0);
  write_c_string (" ", printcharfun);
  print_internal (CHARSET_DOC_STRING (cs), printcharfun, 1);
  sprintf (buf, " %s %s cols=%d g%d final='%c' reg=",
	   CHARSET_TYPE (cs) == CHARSET_TYPE_94    ? "94" :
	   CHARSET_TYPE (cs) == CHARSET_TYPE_96    ? "96" :
	   CHARSET_TYPE (cs) == CHARSET_TYPE_94X94 ? "94x94" :
	   "96x96",
	   CHARSET_DIRECTION (cs) == CHARSET_LEFT_TO_RIGHT ? "l2r" : "r2l",
	   CHARSET_COLUMNS (cs),
	   CHARSET_GRAPHIC (cs),
	   CHARSET_FINAL (cs));
  write_c_string (buf, printcharfun);
  print_internal (CHARSET_REGISTRY (cs), printcharfun, 0);
  sprintf (buf, " 0x%x>", cs->header.uid);
  write_c_string (buf, printcharfun);
}

/* Make a new charset. */

static Lisp_Object
make_charset (Lisp_Object name, Bufbyte leading_byte, unsigned char rep_bytes,
	      unsigned char type, unsigned char columns, unsigned char graphic,
	      Bufbyte final, unsigned char direction, Lisp_Object doc,
	      Lisp_Object reg)
{
  struct Lisp_Charset *cs;
  Lisp_Object obj = Qnil;

  cs = (struct Lisp_Charset *) alloc_lcrecord (sizeof (struct Lisp_Charset),
					       lrecord_charset);
  XSETCHARSET (obj, cs);

  CHARSET_NAME		(cs) = name;
  CHARSET_LEADING_BYTE	(cs) = leading_byte;
  CHARSET_REP_BYTES	(cs) = rep_bytes;
  CHARSET_DIRECTION	(cs) = direction;
  CHARSET_TYPE		(cs) = type;
  CHARSET_COLUMNS	(cs) = columns;
  CHARSET_GRAPHIC	(cs) = graphic;
  CHARSET_FINAL		(cs) = final;
  CHARSET_DOC_STRING	(cs) = doc;
  CHARSET_REGISTRY	(cs) = reg;
  CHARSET_CCL_PROGRAM	(cs) = Qnil;
  CHARSET_REVERSE_DIRECTION_CHARSET (cs) = Qnil;

  if (final)
    {
      /* some charsets do not have final characters.  This includes
	 ASCII, Control-1, Composite, and the two faux private
	 charsets. */
      assert (NILP (charset_by_attributes[type][final][direction]));
      charset_by_attributes[type][final][direction] = obj;
    }

  assert (NILP (charset_by_leading_byte[leading_byte - 128]));
  charset_by_leading_byte[leading_byte - 128] = obj;
  if (leading_byte < 0xA0)
    /* official leading byte */
    rep_bytes_by_first_byte[leading_byte] = rep_bytes;
  
  /* Some charsets are "faux" and don't have names or really exist at
     all except in the leading-byte table. */
  if (!NILP (name))
    Fputhash (name, obj, Vcharset_hashtable);
  return obj;
}

static int
get_unallocated_leading_byte (int dimension)
{
  int lb;

  if (dimension == 1)
    {
      if (next_allocated_1_byte_leading_byte > MAX_LEADING_BYTE_PRIVATE_1)
	lb = 0;
      else
	lb = next_allocated_1_byte_leading_byte++;
    }
  else
    {
      if (next_allocated_2_byte_leading_byte > MAX_LEADING_BYTE_PRIVATE_2)
	lb = 0;
      else
	lb = next_allocated_2_byte_leading_byte++;
    }

  if (!lb)
    signal_simple_error
      ("No more character sets free for this dimension",
       make_int (dimension));

  return lb;
}


/************************************************************************/
/*                      Basic charset Lisp functions                    */
/************************************************************************/

DEFUN ("charsetp", Fcharsetp, 1, 1, 0, /*
Return non-nil if OBJECT is a charset.
*/
       (object))
{
  return (CHARSETP (object) ? Qt : Qnil);
}

DEFUN ("find-charset", Ffind_charset, 1, 1, 0, /*
Retrieve the charset of the given name.
If CHARSET-OR-NAME is a charset object, it is simply returned.
Otherwise, CHARSET-OR-NAME should be a symbol.  If there is no such charset,
nil is returned.  Otherwise the associated charset object is returned.
*/
       (charset_or_name))
{
  if (CHARSETP (charset_or_name))
    return charset_or_name;
  CHECK_SYMBOL (charset_or_name);

  return Fgethash (charset_or_name, Vcharset_hashtable, Qnil);
}

DEFUN ("get-charset", Fget_charset, 1, 1, 0, /*
Retrieve the charset of the given name.
Same as `find-charset' except an error is signalled if there is no such
charset instead of returning nil.
*/
       (name))
{
  Lisp_Object charset = Ffind_charset (name);

  if (NILP (charset))
    signal_simple_error ("No such charset", name);
  return charset;
}

/* We store the charsets in hash tables with the names as the key and the
   actual charset object as the value.  Occasionally we need to use them
   in a list format.  These routines provide us with that. */
struct charset_list_closure
{
  Lisp_Object *charset_list;
};

static void
add_charset_to_list_mapper (CONST void *hash_key, void *hash_contents,
			    void *charset_list_closure)
{
  /* This function can GC */
  Lisp_Object key, contents;
  Lisp_Object *charset_list;
  struct charset_list_closure *chcl = charset_list_closure;
  CVOID_TO_LISP (key, hash_key);
  VOID_TO_LISP (contents, hash_contents);
  charset_list = chcl->charset_list;

  *charset_list = Fcons (XCHARSET_NAME (contents), *charset_list);
}

DEFUN ("charset-list", Fcharset_list, 0, 0, 0, /*
Return a list of the names of all defined charsets.
*/
       ())
{
  Lisp_Object charset_list = Qnil;
  struct gcpro gcpro1;
  struct charset_list_closure charset_list_closure;

  GCPRO1 (charset_list);
  charset_list_closure.charset_list = &charset_list;
  elisp_maphash (add_charset_to_list_mapper, Vcharset_hashtable,
		 &charset_list_closure);
  UNGCPRO;

  return charset_list;
}

DEFUN ("charset-name", Fcharset_name, 1, 1, 0, /*
Return the name of the given charset.
*/
       (charset))
{
  return (XCHARSET_NAME (Fget_charset (charset)));
}

DEFUN ("make-charset", Fmake_charset, 3, 3, 0, /*
Define a new character set.
This function is for use with Mule support.
NAME is a symbol, the name by which the character set is normally referred.
DOC-STRING is a string describing the character set.
PROPS is a property list, describing the specific nature of the
character set.  Recognized properties are:

'registry	A regular expression matching the font registry field for
		this character set.
'dimension	Number of octets used to index a character in this charset.
		Either 1 or 2.  Defaults to 1.
'columns	Number of columns used to display a character in this charset.
		Only used in TTY mode. (Under X, the actual width of a
		character can be derived from the font used to display the
		characters.) If unspecified, defaults to the dimension
		(this is almost	always the correct value).
'chars		Number of characters in each dimension (94 or 96).
		Defaults to 94.  Note that if the dimension is 2, the
		character set thus described is 94x94 or 96x96.
'final		Final byte of ISO 2022 escape sequence.  Must be
		supplied.  Each combination of (DIMENSION, CHARS) defines a
		separate namespace for final bytes.  Note that ISO
		2022 restricts the final byte to the range
		0x30 - 0x7E if dimension == 1, and 0x30 - 0x5F if
		dimension == 2.  Note also that final bytes in the range
		0x30 - 0x3F are reserved for user-defined (not official)
		character sets.
'graphic	0 (use left half of font on output) or 1 (use right half
		of font on output).  Defaults to 0.  For example, for
		a font whose registry is ISO8859-1, the left half
		(octets 0x20 - 0x7F) is the `ascii' character set, while
		the right half (octets 0xA0 - 0xFF) is the `latin-1'
		character set.  With 'graphic set to 0, the octets
		will have their high bit cleared; with it set to 1,
		the octets will have their high bit set.
'direction	'l2r (left-to-right) or 'r2l (right-to-left).
		Defaults to 'l2r.
'ccl-program	A compiled CCL program used to convert a character in
		this charset into an index into the font.  This is in
		addition to the 'graphic property.  The CCL program
		is passed the octets of the character, with the high
		bit cleared and set depending upon whether the value
		of the 'graphic property is 0 or 1.
*/
       (name, doc_string, props))
{
  int lb, dimension = 1, chars = 94, graphic = 0, final = 0, columns = -1;
  int direction = CHARSET_LEFT_TO_RIGHT;
  int type;
  Lisp_Object registry = Qnil;
  Lisp_Object charset;
  Lisp_Object rest, keyword, value;
  Lisp_Object ccl_program = Qnil;

  CHECK_SYMBOL (name);
  if (!NILP (doc_string))
    CHECK_STRING (doc_string);

  charset = Ffind_charset (name);
  if (!NILP (charset))
    signal_simple_error ("Cannot redefine existing charset", name);

  EXTERNAL_PROPERTY_LIST_LOOP (rest, keyword, value, props)
    {
      if (EQ (keyword, Qdimension))
	{
	  CHECK_INT (value);
	  dimension = XINT (value);
	  if (dimension < 1 || dimension > 2)
	    signal_simple_error ("Invalid value for 'dimension", value);
	}

      else if (EQ (keyword, Qchars))
	{
	  CHECK_INT (value);
	  chars = XINT (value);
	  if (chars != 94 && chars != 96)
	    signal_simple_error ("Invalid value for 'chars", value);
	}

      else if (EQ (keyword, Qcolumns))
	{
	  CHECK_INT (value);
	  columns = XINT (value);
	  if (columns != 1 && columns != 2)
	    signal_simple_error ("Invalid value for 'columns", value);
	}

      else if (EQ (keyword, Qgraphic))
	{
	  CHECK_INT (value);
	  graphic = XINT (value);
	  if (graphic < 0 || graphic > 1)
	    signal_simple_error ("Invalid value for 'graphic", value);
	}

      else if (EQ (keyword, Qregistry))
	{
	  CHECK_STRING (value);
	  registry = value;
	}

      else if (EQ (keyword, Qdirection))
	{
	  if (EQ (value, Ql2r))
	    direction = CHARSET_LEFT_TO_RIGHT;
	  else if (EQ (value, Qr2l))
	    direction = CHARSET_RIGHT_TO_LEFT;
	  else
	    signal_simple_error ("Invalid value for 'direction", value);
	}

      else if (EQ (keyword, Qfinal))
	{
	  CHECK_CHAR_COERCE_INT (value);
	  final = XCHAR (value);
	  if (final < '0' || final > '~')
	    signal_simple_error ("Invalid value for 'final", value);
	}

      else if (EQ (keyword, Qccl_program))
	{
	  CHECK_VECTOR (value);
	  ccl_program = value;
	}

      else
	signal_simple_error ("Unrecognized property", keyword);
    }

  if (!final)
    error ("'final must be specified");
  if (dimension == 2 && final > 0x5F)
    signal_simple_error
      ("Final must be in the range 0x30 - 0x5F for dimension == 2",
       make_char (final));

  if (dimension == 1)
    type = (chars == 94) ? CHARSET_TYPE_94    : CHARSET_TYPE_96;
  else
    type = (chars == 94) ? CHARSET_TYPE_94X94 : CHARSET_TYPE_96X96;

  if (!NILP (CHARSET_BY_ATTRIBUTES (type, final, CHARSET_LEFT_TO_RIGHT)) ||
      !NILP (CHARSET_BY_ATTRIBUTES (type, final, CHARSET_RIGHT_TO_LEFT)))
    error
      ("Character set already defined for this DIMENSION/CHARS/FINAL combo");

  lb = get_unallocated_leading_byte (dimension);

  if (NILP (doc_string))
    doc_string = build_string ("");

  if (NILP (registry))
    registry = build_string ("");

  if (columns == -1)
    columns = dimension;
  charset = make_charset (name, lb, dimension + 2, type, columns, graphic,
			  final, direction, doc_string, registry);
  if (!NILP (ccl_program))
    XCHARSET_CCL_PROGRAM (charset) = ccl_program;
  return charset;
}

DEFUN ("make-reverse-direction-charset",
       Fmake_reverse_direction_charset, 2, 2, 0, /*
Make a charset equivalent to CHARSET but which goes in the opposite direction.
NEW-NAME is the name of the new charset.  Return the new charset.
*/
       (charset, new_name))
{
  Lisp_Object new_charset = Qnil;
  int lb, dimension, columns, graphic, final;
  int direction, type;
  Lisp_Object registry, doc_string;
  struct Lisp_Charset *cs;

  charset = Fget_charset (charset);
  if (!NILP (XCHARSET_REVERSE_DIRECTION_CHARSET (charset)))
    signal_simple_error ("Charset already has reverse-direction charset",
			 charset);

  CHECK_SYMBOL (new_name);
  if (!NILP (Ffind_charset (new_name)))
    signal_simple_error ("Cannot redefine existing charset", new_name);

  cs = XCHARSET (charset);
  
  type      = CHARSET_TYPE      (cs);
  columns   = CHARSET_COLUMNS   (cs);
  dimension = CHARSET_DIMENSION (cs);
  lb = get_unallocated_leading_byte (dimension);

  graphic = CHARSET_GRAPHIC (cs);
  final = CHARSET_FINAL (cs);
  direction = CHARSET_RIGHT_TO_LEFT;
  if (CHARSET_DIRECTION (cs) == CHARSET_RIGHT_TO_LEFT)
    direction = CHARSET_LEFT_TO_RIGHT;
  doc_string = CHARSET_DOC_STRING (cs);
  registry = CHARSET_REGISTRY (cs);

  new_charset = make_charset (new_name, lb, dimension + 2, type, columns,
			      graphic, final, direction, doc_string, registry);

  CHARSET_REVERSE_DIRECTION_CHARSET (cs) = new_charset;
  XCHARSET_REVERSE_DIRECTION_CHARSET (new_charset) = charset;

  return new_charset;
}

/* #### The defsubr for this is commented out at the moment but no
   reason why is given. */
#if 0
DEFUN ("charset-reverse-direction-charset",
       Fcharset_reverse_direction_charset, 1, 1, 0, /*
Return the reverse-direction charset parallel to CHARSET, if any.
This is the charset with the same properties (in particular, the same
dimension, number of characters per dimension, and final byte) as
CHARSET but whose characters are displayed in the opposite direction.
*/
       (charset))
{
  charset = Fget_charset (charset);
  return XCHARSET_REVERSE_DIRECTION_CHARSET (charset);
}
#endif

DEFUN ("charset-from-attributes", Fcharset_from_attributes, 3, 4, 0, /*
Return a charset with the given DIMENSION, CHARS, FINAL, and DIRECTION.
If DIRECTION is omitted, both directions will be checked (left-to-right
will be returned if character sets exist for both directions).
*/
       (dimension, chars, final, direction))
{
  int dm, ch, fi, di = -1;
  int type;
  Lisp_Object obj = Qnil;

  CHECK_INT (dimension);
  dm = XINT (dimension);
  if (dm < 1 || dm > 2)
    signal_simple_error ("Invalid value for DIMENSION", dimension);

  CHECK_INT (chars);
  ch = XINT (chars);
  if (ch != 94 && ch != 96)
    signal_simple_error ("Invalid value for CHARS", chars);

  CHECK_CHAR_COERCE_INT (final);
  fi = XCHAR (final);
  if (fi < '0' || fi > '~')
    signal_simple_error ("Invalid value for FINAL", final);

  if (EQ (direction, Ql2r))
    di = CHARSET_LEFT_TO_RIGHT;
  else if (EQ (direction, Qr2l))
    di = CHARSET_RIGHT_TO_LEFT;
  else if (!NILP (direction))
    signal_simple_error ("Invalid value for DIRECTION", direction);

  if (dm == 2 && fi > 0x5F)
    signal_simple_error
      ("Final must be in the range 0x30 - 0x5F for dimension == 2", final);

  if (dm == 1)
    type = (ch == 94) ? CHARSET_TYPE_94    : CHARSET_TYPE_96;
  else
    type = (ch == 94) ? CHARSET_TYPE_94X94 : CHARSET_TYPE_96X96;

  if (di == -1)
    {
      obj = CHARSET_BY_ATTRIBUTES (type, fi, CHARSET_LEFT_TO_RIGHT);
      if (NILP (obj))
	obj = CHARSET_BY_ATTRIBUTES (type, fi, CHARSET_RIGHT_TO_LEFT);
    }
  else
    obj = CHARSET_BY_ATTRIBUTES (type, fi, di);

  if (CHARSETP (obj))
    return XCHARSET_NAME (obj);
  return obj;
}

DEFUN ("charset-doc-string", Fcharset_doc_string, 1, 1, 0, /*
Return doc string of CHARSET.
*/
       (charset))
{
  return XCHARSET_DOC_STRING (Fget_charset (charset));
}

DEFUN ("charset-dimension", Fcharset_dimension, 1, 1, 0, /*
Return dimension of CHARSET.
*/
       (charset))
{
  return make_int (XCHARSET_DIMENSION (Fget_charset (charset)));
}

DEFUN ("charset-property", Fcharset_property, 2, 2, 0, /*
Return property PROP of CHARSET.
Recognized properties are those listed in `make-charset', as well as
'name and 'doc-string.
*/
       (charset, prop))
{
  struct Lisp_Charset *cs;

  charset = Fget_charset (charset);
  cs = XCHARSET (charset);

  CHECK_SYMBOL (prop);
  if (EQ (prop, Qname))        return CHARSET_NAME (cs);
  if (EQ (prop, Qdoc_string))  return CHARSET_DOC_STRING (cs);
  if (EQ (prop, Qdimension))   return make_int (CHARSET_DIMENSION (cs));
  if (EQ (prop, Qcolumns))     return make_int (CHARSET_COLUMNS (cs));
  if (EQ (prop, Qgraphic))     return make_int (CHARSET_GRAPHIC (cs));
  if (EQ (prop, Qfinal))       return make_char (CHARSET_FINAL (cs));
  if (EQ (prop, Qchars))       return make_int (CHARSET_CHARS (cs));
  if (EQ (prop, Qregistry))    return CHARSET_REGISTRY (cs);
  if (EQ (prop, Qccl_program)) return CHARSET_CCL_PROGRAM (cs);
  if (EQ (prop, Qdirection))
    return (CHARSET_DIRECTION (cs) == CHARSET_LEFT_TO_RIGHT ? Ql2r : Qr2l);
  if (EQ (prop, Qreverse_direction_charset))
    {
      Lisp_Object obj = CHARSET_REVERSE_DIRECTION_CHARSET (cs);
      if (NILP (obj))
	return Qnil;
      else
	return XCHARSET_NAME (obj);
    }
  signal_simple_error ("Unrecognized charset property name", prop);
  return Qnil; /* not reached */
}

/* #### We need to figure out which properties we really want to
   allow to be set. */

DEFUN ("set-charset-ccl-program", Fset_charset_ccl_program, 2, 2, 0, /*
Set the 'ccl-program property of CHARSET to CCL-PROGRAM.
*/
       (charset, ccl_program))
{
  charset = Fget_charset (charset);
  CHECK_VECTOR (ccl_program);
  XCHARSET_CCL_PROGRAM (charset) = ccl_program;
  return Qnil;
}

static void
invalidate_charset_font_caches (Lisp_Object charset)
{
  /* Invalidate font cache entries for charset on all devices. */
  Lisp_Object devcons, concons, hashtab;
  DEVICE_LOOP_NO_BREAK (devcons, concons)
    {
      struct device *d = XDEVICE (XCAR (devcons));
      hashtab = Fgethash (charset, d->charset_font_cache, Qunbound);
      if (!UNBOUNDP (hashtab))
        Fclrhash (hashtab);
    }
}

/* Japanese folks may want to (set-charset-registry 'ascii "jisx0201") */
DEFUN ("set-charset-registry", Fset_charset_registry, 2, 2, 0, /*
Set the 'registry property of CHARSET to REGISTRY.
*/
       (charset, registry))
{
  charset = Fget_charset (charset);
  CHECK_STRING (registry);
  XCHARSET_REGISTRY (charset) = registry;
  invalidate_charset_font_caches (charset);
  face_property_was_changed (Vdefault_face, Qfont, Qglobal);
  return Qnil;
}
  

/************************************************************************/
/*              Lisp primitives for working with characters             */
/************************************************************************/

DEFUN ("make-char", Fmake_char, 2, 3, 0, /*
Make a multi-byte character from CHARSET and octets ARG1 and ARG2.
*/
       (charset, arg1, arg2))
{
  struct Lisp_Charset *cs;
  int a1, a2;
  int lowlim, highlim;

  charset = Fget_charset (charset);
  cs = XCHARSET (charset);

  if      (EQ (charset, Vcharset_ascii))     lowlim =  0, highlim = 127;
  else if (EQ (charset, Vcharset_control_1)) lowlim =  0, highlim =  31;
  else if (CHARSET_CHARS (cs) == 94)         lowlim = 33, highlim = 126;
  else	/* CHARSET_CHARS (cs) == 96) */	     lowlim = 32, highlim = 127;

  CHECK_INT (arg1);
  a1 = XINT (arg1);
  if (a1 < lowlim || a1 > highlim)
    args_out_of_range_3 (arg1, make_int (lowlim), make_int (highlim));

  if (CHARSET_DIMENSION (cs) == 1)
    {
      if (!NILP (arg2))
        signal_simple_error
          ("Charset is of dimension one; second octet must be nil", arg2);
      return make_char (MAKE_CHAR (charset, a1, 0));
    }

  CHECK_INT (arg2);
  a2 = XINT (arg2);
  if (a2 < lowlim || a2 > highlim)
    args_out_of_range_3 (arg2, make_int (lowlim), make_int (highlim));

  return make_char (MAKE_CHAR (charset, a1, a2));
}

DEFUN ("char-charset", Fchar_charset, 1, 1, 0, /*
Return the character set of char CH.
*/
       (ch))
{
  CHECK_CHAR_COERCE_INT (ch);

  return XCHARSET_NAME (CHARSET_BY_LEADING_BYTE
			(CHAR_LEADING_BYTE (XCHAR (ch))));
}

DEFUN ("char-octet", Fchar_octet, 1, 2, 0, /*
Return the octet numbered N (should be 0 or 1) of char CH.
N defaults to 0 if omitted.
*/
       (ch, n))
{
  Lisp_Object charset;
  int c1, c2;

  CHECK_CHAR_COERCE_INT (ch);
  if (NILP (n))
    n = Qzero;
  else
    {
      CHECK_INT (n);
      if (XINT (n) != 0 && XINT (n) != 1)
	signal_simple_error ("Octet number must be 0 or 1", n);
    }
  BREAKUP_CHAR (XCHAR (ch), charset, c1, c2);
  if (XINT (n) == 0)
    return make_int (c1);
  else
    return make_int (c2);
}


/************************************************************************/
/*                     composite character functions                    */
/************************************************************************/

Emchar
lookup_composite_char (Bufbyte *str, int len)
{
  Lisp_Object lispstr = make_string (str, len);
  Lisp_Object ch = Fgethash (lispstr,
			     Vcomposite_char_string2char_hashtable,
			     Qunbound);
  Emchar emch;

  if (UNBOUNDP (ch))
    {
      if (composite_char_row_next >= 128)
	signal_simple_error ("No more composite chars available", lispstr);
      emch = MAKE_CHAR (Vcharset_composite, composite_char_row_next,
			composite_char_col_next);
      Fputhash (make_char (emch), lispstr,
	        Vcomposite_char_char2string_hashtable);
      Fputhash (lispstr, make_char (emch),
		Vcomposite_char_string2char_hashtable);
      composite_char_col_next++;
      if (composite_char_col_next >= 128)
	{
	  composite_char_col_next = 32;
	  composite_char_row_next++;
	}
    }
  else
    emch = XCHAR (ch);
  return emch;
}

Lisp_Object
composite_char_string (Emchar ch)
{
  Lisp_Object str = Fgethash (make_char (ch),
			      Vcomposite_char_char2string_hashtable,
			      Qunbound);
  assert (!UNBOUNDP (str));
  return str;
}

DEFUN ("make-composite-char", Fmake_composite_char, 1, 1, 0, /*
Convert a string into a single composite character.
The character is the result of overstriking all the characters in
the string.
*/
       (string))
{
  CHECK_STRING (string);
  return make_char (lookup_composite_char (XSTRING_DATA (string),
					   XSTRING_LENGTH (string)));
}

DEFUN ("composite-char-string", Fcomposite_char_string, 1, 1, 0, /*
Return a string of the characters comprising a composite character.
*/
       (ch))
{
  Emchar emch;

  CHECK_CHAR (ch);
  emch = XCHAR (ch);
  if (CHAR_LEADING_BYTE (emch) != LEADING_BYTE_COMPOSITE)
    signal_simple_error ("Must be composite char", ch);
  return composite_char_string (emch);
}


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

void
syms_of_mule_charset (void)
{
  DEFSUBR (Fcharsetp);
  DEFSUBR (Ffind_charset);
  DEFSUBR (Fget_charset);
  DEFSUBR (Fcharset_list);
  DEFSUBR (Fcharset_name);
  DEFSUBR (Fmake_charset);
  DEFSUBR (Fmake_reverse_direction_charset);
  /*  DEFSUBR (Freverse_direction_charset); */
  DEFSUBR (Fcharset_from_attributes);
  DEFSUBR (Fcharset_doc_string);
  DEFSUBR (Fcharset_dimension);
  DEFSUBR (Fcharset_property);
  DEFSUBR (Fset_charset_ccl_program);
  DEFSUBR (Fset_charset_registry);

  DEFSUBR (Fmake_char);
  DEFSUBR (Fchar_charset);
  DEFSUBR (Fchar_octet);

  DEFSUBR (Fmake_composite_char);
  DEFSUBR (Fcomposite_char_string);

  defsymbol (&Qcharsetp, "charsetp");
  defsymbol (&Qregistry, "registry");
  defsymbol (&Qfinal, "final");
  defsymbol (&Qgraphic, "graphic");
  defsymbol (&Qdirection, "direction");
  defsymbol (&Qreverse_direction_charset, "reverse-direction-charset");
  defsymbol (&Qccl_program, "ccl-program");

  defsymbol (&Ql2r, "l2r");
  defsymbol (&Qr2l, "r2l");

  /* Charsets, compatible with Emacs/Mule 19.33-delta
     Naming convention is Script-Charset[-Edition] */
  defsymbol (&Qascii,			"ascii");
  defsymbol (&Qcontrol_1,		"control-1");
  defsymbol (&Qlatin_iso8859_1,		"latin-iso8859-1");
  defsymbol (&Qlatin_iso8859_2,		"latin-iso8859-2");
  defsymbol (&Qlatin_iso8859_3,		"latin-iso8859-3");
  defsymbol (&Qlatin_iso8859_4,		"latin-iso8859-4");
  defsymbol (&Qcyrillic_iso8859_5, 	"cyrillic-iso8859-5");
  defsymbol (&Qarabic_iso8859_6,	"arabic-iso8859-6");
  defsymbol (&Qgreek_iso8859_7,		"greek-iso8859-7");
  defsymbol (&Qhebrew_iso8859_8,	"hebrew-iso8859-8");
  defsymbol (&Qlatin_iso8859_9,		"latin-iso8859-9");
  defsymbol (&Qthai_tis620,		"thai-tis620");
  
  defsymbol (&Qkatakana_jisx0201,	"katakana-jisx0201");
  defsymbol (&Qlatin_jisx0201,		"latin-jisx0201");
  defsymbol (&Qjapanese_jisx0208_1978,	"japanese-jisx0208-1978");
  defsymbol (&Qjapanese_jisx0208, 	"japanese-jisx0208");
  defsymbol (&Qjapanese_jisx0212,	"japanese-jisx0212");
  
  defsymbol (&Qchinese_gb2312,		"chinese-gb2312");
  defsymbol (&Qchinese_big5_1,		"chinese-big5-1");
  defsymbol (&Qchinese_big5_2,		"chinese-big5-2");
  defsymbol (&Qchinese_cns11643_1,	"chinese-cns11643-1");
  defsymbol (&Qchinese_cns11643_2,	"chinese-cns11643-2");
  
  defsymbol (&Qkorean_ksc5601,		"korean-ksc5601");
  defsymbol (&Qcomposite,		"composite");
}

void
vars_of_mule_charset (void)
{
  int i, j, k;

  for (i = 0; i < 128; i++)
    charset_by_leading_byte[i] = Qnil;
  
  for (i = 0; i < 4; i++)
    for (j = 0; j < 128; j++)
      for (k = 0; k < 2; k ++)
	charset_by_attributes[i][j][k] = Qnil;

  /* Now done at compile time
  for (i = 0; i < 128; i++)
    rep_bytes_by_first_byte[i] = 1;
    */

  next_allocated_1_byte_leading_byte = MIN_LEADING_BYTE_PRIVATE_1;
  next_allocated_2_byte_leading_byte = MIN_LEADING_BYTE_PRIVATE_2;
}

void
complex_vars_of_mule_charset (void)
{
  staticpro (&Vcharset_hashtable);
  Vcharset_hashtable = make_lisp_hashtable (50, HASHTABLE_NONWEAK,
					    HASHTABLE_EQ);
  
  /* Predefined character sets.  We store them into variables for
     ease of access. */

  Vcharset_ascii =
    make_charset (Qascii, LEADING_BYTE_ASCII, 1,
		  CHARSET_TYPE_94, 1, 0, 'B',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("ASCII"),
		  build_string ("ISO8859-1"));
  Vcharset_control_1 =
    make_charset (Qcontrol_1, LEADING_BYTE_CONTROL_1, 2,
		  CHARSET_TYPE_94, 1, 0, 0,
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Control characters"),
		  build_string (""));
  Vcharset_latin_iso8859_1 =
    make_charset (Qlatin_iso8859_1, LEADING_BYTE_LATIN_ISO8859_1, 2,
		  CHARSET_TYPE_96, 1, 1, 'A',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Latin-1"),
		  build_string ("ISO8859-1"));
  Vcharset_latin_iso8859_2 =
    make_charset (Qlatin_iso8859_2, LEADING_BYTE_LATIN_ISO8859_2, 2,
		  CHARSET_TYPE_96, 1, 1, 'B',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Latin-2"),
		  build_string ("ISO8859-2"));
  Vcharset_latin_iso8859_3 =
    make_charset (Qlatin_iso8859_3, LEADING_BYTE_LATIN_ISO8859_3, 2,
		  CHARSET_TYPE_96, 1, 1, 'C',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Latin-3"),
		  build_string ("ISO8859-3"));
  Vcharset_latin_iso8859_4 =
    make_charset (Qlatin_iso8859_4, LEADING_BYTE_LATIN_ISO8859_4, 2,
		  CHARSET_TYPE_96, 1, 1, 'D',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Latin-4"),
		  build_string ("ISO8859-4"));
  Vcharset_cyrillic_iso8859_5 =
    make_charset (Qcyrillic_iso8859_5, LEADING_BYTE_CYRILLIC_ISO8859_5, 2,
		  CHARSET_TYPE_96, 1, 1, 'L',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Cyrillic"),
		  build_string ("ISO8859-5"));
  Vcharset_arabic_iso8859_6 =
    make_charset (Qarabic_iso8859_6, LEADING_BYTE_ARABIC_ISO8859_6, 2,
		  CHARSET_TYPE_96, 1, 1, 'G',
		  CHARSET_RIGHT_TO_LEFT,
		  build_string ("Arabic"),
		  build_string ("ISO8859-6"));
  Vcharset_greek_iso8859_7 =
    make_charset (Qgreek_iso8859_7, LEADING_BYTE_GREEK_ISO8859_7, 2,
		  CHARSET_TYPE_96, 1, 1, 'F',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Greek"),
		  build_string ("ISO8859-7"));
  Vcharset_hebrew_iso8859_8 =
    make_charset (Qhebrew_iso8859_8, LEADING_BYTE_HEBREW_ISO8859_8, 2,
		  CHARSET_TYPE_96, 1, 1, 'H',
		  CHARSET_RIGHT_TO_LEFT,
		  build_string ("Hebrew"),
		  build_string ("ISO8859-8"));
  Vcharset_latin_iso8859_9 =
    make_charset (Qlatin_iso8859_9, LEADING_BYTE_LATIN_ISO8859_9, 2,
		  CHARSET_TYPE_96, 1, 1, 'M',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Latin-5"),
		  build_string ("ISO8859-9"));
  Vcharset_thai_tis620 =
    make_charset (Qthai_tis620, LEADING_BYTE_THAI_TIS620, 2,
		  CHARSET_TYPE_96, 1, 1, 'T',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Thai"),
		  build_string ("TIS620"));

  /* Japanese */
  Vcharset_katakana_jisx0201 =
    make_charset (Qkatakana_jisx0201,
		  LEADING_BYTE_KATAKANA_JISX0201, 2,
		  CHARSET_TYPE_94, 1, 1, 'I',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Japanese Katakana"),
		  build_string ("JISX0201.1976"));
  Vcharset_latin_jisx0201 =
    make_charset (Qlatin_jisx0201,
		  LEADING_BYTE_LATIN_JISX0201, 2,
		  CHARSET_TYPE_94, 1, 0, 'J',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Japanese Roman"),
		  build_string ("JISX0201.1976"));
  Vcharset_japanese_jisx0208_1978 =
    make_charset (Qjapanese_jisx0208_1978,
		  LEADING_BYTE_JAPANESE_JISX0208_1978, 3,
		  CHARSET_TYPE_94X94, 2, 0, '@',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Japanese Old"),
		  build_string ("\\(JISX0208\\|jisc6226\\).1978"));
  Vcharset_japanese_jisx0208 =
    make_charset (Qjapanese_jisx0208,
		  LEADING_BYTE_JAPANESE_JISX0208, 3,
		  CHARSET_TYPE_94X94, 2, 0, 'B',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Japanese"),
		  build_string ("JISX0208.\\(1983\\|1990\\)"));
  Vcharset_japanese_jisx0212 =
    make_charset (Qjapanese_jisx0212,
		  LEADING_BYTE_JAPANESE_JISX0212, 3,
		  CHARSET_TYPE_94X94, 2, 0, 'D',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Japanese Supplement"),
		  build_string ("JISX0212"));

  /* Chinese */
  Vcharset_chinese_gb2312 =
    make_charset (Qchinese_gb2312, LEADING_BYTE_CHINESE_GB2312, 3,
		  CHARSET_TYPE_94X94, 2, 0, 'A',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Chinese GB2312"),
		  build_string ("GB2312"));
#define CHINESE_CNS_PLANE_RE(n) "CNS11643[.-]\\(.*[.-]\\)?" n "$"
  Vcharset_chinese_cns11643_1 =
    make_charset (Qchinese_cns11643_1,
		  LEADING_BYTE_CHINESE_CNS11643_1, 3,
		  CHARSET_TYPE_94X94, 2, 0, 'G',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Chinese CNS Plane 1"),
		  build_string (CHINESE_CNS_PLANE_RE("1")));
  Vcharset_chinese_cns11643_2 =
    make_charset (Qchinese_cns11643_2,
		  LEADING_BYTE_CHINESE_CNS11643_2, 3,
		  CHARSET_TYPE_94X94, 2, 0, 'H',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Chinese CNS Plane 2"),
		  build_string (CHINESE_CNS_PLANE_RE("2")));
  Vcharset_chinese_big5_1 =
    make_charset (Qchinese_big5_1, LEADING_BYTE_CHINESE_BIG5_1, 3,
		  CHARSET_TYPE_94X94, 2, 0, '0',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Chinese Big5 Level 1"),
		  build_string ("Big5"));
  Vcharset_chinese_big5_2 =
    make_charset (Qchinese_big5_2, LEADING_BYTE_CHINESE_BIG5_2, 3,
		  CHARSET_TYPE_94X94, 2, 0, '1',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Chinese Big5 Level 2"),
		  build_string ("Big5"));

  Vcharset_korean_ksc5601 =
    make_charset (Qkorean_ksc5601, LEADING_BYTE_KOREAN_KSC5601, 3,
		  CHARSET_TYPE_94X94, 2, 0, 'C',
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Korean"),
		  build_string ("KSC5601"));
  /* #### For simplicity, we put composite chars into a 96x96 charset.
     This is going to lead to problems because you can run out of
     room, esp. as we don't yet recycle numbers. */
  Vcharset_composite =
    make_charset (Qcomposite, LEADING_BYTE_COMPOSITE, 3,
		  CHARSET_TYPE_96X96, 2, 0, 0,
		  CHARSET_LEFT_TO_RIGHT,
		  build_string ("Composite characters"),
		  build_string (""));

  composite_char_row_next = 32;
  composite_char_col_next = 32;

  Vcomposite_char_string2char_hashtable =
    make_lisp_hashtable (500, HASHTABLE_NONWEAK,
			 HASHTABLE_EQUAL);
  Vcomposite_char_char2string_hashtable =
    make_lisp_hashtable (500, HASHTABLE_NONWEAK,
			 HASHTABLE_EQ);
  staticpro (&Vcomposite_char_string2char_hashtable);
  staticpro (&Vcomposite_char_char2string_hashtable);

  /* Faux charsets used only for convenience in retrieving the
     number of rep bytes associated with a leading byte. */

  make_charset (Qnil, PRE_LEADING_BYTE_PRIVATE_1, 3, 0, 0, 0, 0,
		0, Qnil, Qnil);
  make_charset (Qnil, PRE_LEADING_BYTE_PRIVATE_2, 4, 0, 0, 0, 0,
		0, Qnil, Qnil);
}
