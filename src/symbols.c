/* "intern" and friends -- moved here from lread.c and data.c
   Copyright (C) 1985-1989, 1992-1994 Free Software Foundation, Inc.
   Copyright (C) 1995 Ben Wing.

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

/* This file has been Mule-ized. */

/* NOTE:

   The value cell of a symbol can contain a simple value or one of
   various symbol-value-magic objects.  Some of these objects can
   chain into other kinds of objects.  Here is a table of possibilities:

   1a) simple value
   1b) Qunbound
   1c) symbol-value-forward, excluding Qunbound
   2) symbol-value-buffer-local -> 1a or 1b or 1c
   3) symbol-value-lisp-magic -> 1a or 1b or 1c
   4) symbol-value-lisp-magic -> symbol-value-buffer-local -> 1a or 1b or 1c
   5) symbol-value-varalias
   6) symbol-value-lisp-magic -> symbol-value-varalias

   The "chain" of a symbol-value-buffer-local is its current_value slot.

   The "chain" of a symbol-value-lisp-magic is its shadowed slot, which
   applies for handler types without associated handlers.

   All other fields in all the structures (including the "shadowed" slot
   in a symbol-value-varalias) can *only* contain a simple value or Qunbound.

*/

/* #### Ugh, though, this file does awful things with symbol-value-magic
   objects.  This ought to be cleaned up. */

#include <config.h>
#include "lisp.h"

#include "buffer.h"		/* for Vbuffer_defaults */
#include "console.h"

Lisp_Object Qad_advice_info, Qad_activate;

Lisp_Object Qget_value, Qset_value, Qbound_predicate, Qmake_unbound;
Lisp_Object Qlocal_predicate, Qmake_local;

Lisp_Object Qboundp, Qfboundp, Qglobally_boundp, Qmakunbound;
Lisp_Object Qsymbol_value, Qset, Qdefault_boundp, Qdefault_value;
Lisp_Object Qset_default, Qmake_variable_buffer_local, Qmake_local_variable;
Lisp_Object Qkill_local_variable, Qkill_console_local_variable;
Lisp_Object Qsymbol_value_in_buffer, Qsymbol_value_in_console;
Lisp_Object Qlocal_variable_p;

Lisp_Object Qconst_integer, Qconst_boolean, Qconst_object;
Lisp_Object Qconst_specifier;
Lisp_Object Qdefault_buffer, Qcurrent_buffer, Qconst_current_buffer;
Lisp_Object Qdefault_console, Qselected_console, Qconst_selected_console;

static Lisp_Object maybe_call_magic_handler (Lisp_Object sym,
					     Lisp_Object funsym,
					     int nargs, ...);
static Lisp_Object fetch_value_maybe_past_magic (Lisp_Object sym,
						 Lisp_Object
						 follow_past_lisp_magic);
static Lisp_Object *value_slot_past_magic (Lisp_Object sym);
static Lisp_Object follow_varalias_pointers (Lisp_Object object,
					     Lisp_Object
					     follow_past_lisp_magic);


#ifdef LRECORD_SYMBOL

static Lisp_Object mark_symbol (Lisp_Object, void (*) (Lisp_Object));
extern void print_symbol (Lisp_Object, Lisp_Object, int);
DEFINE_BASIC_LRECORD_IMPLEMENTATION ("symbol", symbol,
				     mark_symbol, print_symbol, 0, 0, 0,
				     struct Lisp_Symbol);

static Lisp_Object
mark_symbol (Lisp_Object obj, void (*markobj) (Lisp_Object))
{
  struct Lisp_Symbol *sym = XSYMBOL (obj);
  Lisp_Object pname;

  ((markobj) (sym->value));
  ((markobj) (sym->function));
  XSETSTRING (pname, sym->name);
  ((markobj) (pname));
  if (!symbol_next (sym))
    return (sym->plist);
  else
  {
    ((markobj) (sym->plist));
    /* Mark the rest of the symbols in the obarray hash-chain */
    sym = symbol_next (sym);
    XSETSYMBOL (obj, sym);
    return (obj);
  }
}

#endif /* LRECORD_SYMBOL */


/**********************************************************************/
/*                              Intern				      */
/**********************************************************************/

/* #### using a vector here is way bogus.  Use a hash table instead. */

Lisp_Object Vobarray;

static Lisp_Object initial_obarray;

/* oblookup stores the bucket number here, for the sake of Funintern.  */

static int oblookup_last_bucket_number;

static Lisp_Object
check_obarray (Lisp_Object obarray)
{
  while (!VECTORP (obarray) || vector_length (XVECTOR (obarray)) == 0)
    {
      /* If Vobarray is now invalid, force it to be valid.  */
      if (EQ (Vobarray, obarray)) Vobarray = initial_obarray;

      obarray = wrong_type_argument (Qvectorp, obarray);
    }
  return obarray;
}

Lisp_Object
intern (CONST char *str)
{
  Lisp_Object tem;
  Bytecount len = strlen (str);
  Lisp_Object obarray = Vobarray;
  if (!VECTORP (obarray) || vector_length (XVECTOR (obarray)) == 0)
    obarray = check_obarray (obarray);
  tem = oblookup (obarray, (CONST Bufbyte *) str, len);
    
  if (SYMBOLP (tem))
    return tem;
  return Fintern (((purify_flag)
		   ? make_pure_pname ((CONST Bufbyte *) str, len, 0)
		   : make_string ((CONST Bufbyte *) str, len)),
		  obarray);
}

DEFUN ("intern", Fintern, 1, 2, 0, /*
Return the canonical symbol whose name is STRING.
If there is none, one is created by this function and returned.
A second optional argument specifies the obarray to use;
it defaults to the value of `obarray'.
*/
       (str, obarray))
{
  Lisp_Object sym, *ptr;
  Bytecount len;

  if (NILP (obarray)) obarray = Vobarray;
  obarray = check_obarray (obarray);

  CHECK_STRING (str);

  len = XSTRING_LENGTH (str);
  sym = oblookup (obarray, XSTRING_DATA (str), len);
  if (!INTP (sym))
    /* Found it */
    return sym;

  ptr = &vector_data (XVECTOR (obarray))[XINT (sym)];

  if (purify_flag && ! purified (str))
    str = make_pure_pname (XSTRING_DATA (str), len, 0);
  sym = Fmake_symbol (str);

  if (SYMBOLP (*ptr))
    symbol_next (XSYMBOL (sym)) = XSYMBOL (*ptr);
  else
    symbol_next (XSYMBOL (sym)) = 0;
  *ptr = sym;
  return sym;
}

DEFUN ("intern-soft", Fintern_soft, 1, 2, 0, /*
Return the canonical symbol whose name is STRING, or nil if none exists.
A second optional argument specifies the obarray to use;
it defaults to the value of `obarray'.
*/
       (str, obarray))
{
  Lisp_Object tem;

  if (NILP (obarray)) obarray = Vobarray;
  obarray = check_obarray (obarray);

  CHECK_STRING (str);

  tem = oblookup (obarray, XSTRING_DATA (str), XSTRING_LENGTH (str));
  if (!INTP (tem))
    return tem;
  return Qnil;
}

DEFUN ("unintern", Funintern, 1, 2, 0, /*
Delete the symbol named NAME, if any, from OBARRAY.
The value is t if a symbol was found and deleted, nil otherwise.
NAME may be a string or a symbol.  If it is a symbol, that symbol
is deleted, if it belongs to OBARRAY--no other symbol is deleted.
OBARRAY defaults to the value of the variable `obarray'
*/
       (name, obarray))
{
  Lisp_Object string, tem;
  int hash;

  if (NILP (obarray)) obarray = Vobarray;
  obarray = check_obarray (obarray);

  if (SYMBOLP (name))
    XSETSTRING (string, XSYMBOL (name)->name);
  else
    {
      CHECK_STRING (name);
      string = name;
    }

  tem = oblookup (obarray, XSTRING_DATA (string), XSTRING_LENGTH (string));
  if (INTP (tem))
    return Qnil;
  /* If arg was a symbol, don't delete anything but that symbol itself.  */
  if (SYMBOLP (name) && !EQ (name, tem))
    return Qnil;

  hash = oblookup_last_bucket_number;

  if (EQ (XVECTOR (obarray)->contents[hash], tem))
    {
      if (XSYMBOL (tem)->next)
	XSETSYMBOL (XVECTOR (obarray)->contents[hash], XSYMBOL (tem)->next);
      else
	XVECTOR (obarray)->contents[hash] = Qzero;
    }
  else
    {
      Lisp_Object tail, following;

      for (tail = XVECTOR (obarray)->contents[hash];
	   XSYMBOL (tail)->next;
	   tail = following)
	{
	  XSETSYMBOL (following, XSYMBOL (tail)->next);
	  if (EQ (following, tem))
	    {
	      XSYMBOL (tail)->next = XSYMBOL (following)->next;
	      break;
	    }
	}
    }

  return Qt;
}

/* Return the symbol in OBARRAY whose names matches the string
   of SIZE characters at PTR.  If there is no such symbol in OBARRAY,
   return nil.

   Also store the bucket number in oblookup_last_bucket_number.  */

Lisp_Object
oblookup (Lisp_Object obarray, CONST Bufbyte *ptr, Bytecount size)
{
  int hash, obsize;
  struct Lisp_Symbol *tail;
  Lisp_Object bucket;

  if (!VECTORP (obarray) ||
      (obsize = vector_length (XVECTOR (obarray))) == 0)
    {
      obarray = check_obarray (obarray);
      obsize = vector_length (XVECTOR (obarray));
    }
#if 0 /* FSFmacs */
  /* #### Huh? */
  /* This is sometimes needed in the middle of GC.  */
  obsize &= ~ARRAY_MARK_FLAG;
#endif
  /* Combining next two lines breaks VMS C 2.3.	 */
  hash = hash_string (ptr, size);
  hash %= obsize;
  bucket = vector_data (XVECTOR (obarray))[hash];
  oblookup_last_bucket_number = hash;
  if (ZEROP (bucket))
    ;
  else if (!SYMBOLP (bucket))
    error ("Bad data in guts of obarray"); /* Like CADR error message */
  else
    for (tail = XSYMBOL (bucket); ;)
      {
	if (string_length (tail->name) == size &&
	    !memcmp (string_data (tail->name), ptr, size))
	  {
	    XSETSYMBOL (bucket, tail);
	    return (bucket);
	  }
	tail = symbol_next (tail);
	if (!tail)
	  break;
      }
  return (make_int (hash));
}

#if 0 /* Emacs 19.34 */
int
hash_string (CONST Bufbyte *ptr, Bytecount len)
{
  CONST Bufbyte *p = ptr;
  CONST Bufbyte *end = p + len;
  Bufbyte c;
  int hash = 0;

  while (p != end)
    {
      c = *p++;
      if (c >= 0140) c -= 40;
      hash = ((hash<<3) + (hash>>28) + c);
    }
  return hash & 07777777777;
}
#endif

/* derived from hashpjw, Dragon Book P436. */
int
hash_string (CONST Bufbyte *ptr, Bytecount len)
{
  CONST Bufbyte *p = ptr;
  int hash = 0, g;
  Bytecount count = len;

  while (count-- > 0)
    {
      hash = (hash << 4) + *p++;
      if (g = (hash & 0xf0000000)) {
	hash = hash ^ (g >> 24);
	hash = hash ^ g;
      }
    }
  return hash & 07777777777;
}

void
map_obarray (Lisp_Object obarray,
	     void (*fn) (Lisp_Object sym, Lisp_Object arg),
	     Lisp_Object arg)
{
  REGISTER int i;
  REGISTER Lisp_Object tail;
  CHECK_VECTOR (obarray);
  for (i = vector_length (XVECTOR (obarray)) - 1; i >= 0; i--)
    {
      tail = vector_data (XVECTOR (obarray))[i];
      if (SYMBOLP (tail))
	while (1)
	  {
	    struct Lisp_Symbol *next;
	    (*fn) (tail, arg);
	    next = symbol_next (XSYMBOL (tail));
	    if (!next)
	      break;
	    XSETSYMBOL (tail, next);
	  }
    }
}

static void
mapatoms_1 (Lisp_Object sym, Lisp_Object function)
{
  call1 (function, sym);
}

DEFUN ("mapatoms", Fmapatoms, 1, 2, 0, /*
Call FUNCTION on every symbol in OBARRAY.
OBARRAY defaults to the value of `obarray'.
*/
       (function, obarray))
{
  if (NILP (obarray))
    obarray = Vobarray;
  obarray = check_obarray (obarray);

  map_obarray (obarray, mapatoms_1, function);
  return Qnil;
}


/**********************************************************************/
/*                              Apropos				      */
/**********************************************************************/

static void
apropos_accum (Lisp_Object symbol, Lisp_Object arg)
{
  Lisp_Object tem;
  Lisp_Object string = XCAR (arg);
  Lisp_Object predicate = XCAR (XCDR (arg));
  Lisp_Object *accumulation = &(XCDR (XCDR (arg)));

  tem = Fstring_match (string, Fsymbol_name (symbol), Qnil,
		       /* #### current-buffer dependence is bogus. */
		       Fcurrent_buffer ());
  if (!NILP (tem) && !NILP (predicate))
    tem = call1 (predicate, symbol);
  if (!NILP (tem))
    *accumulation = Fcons (symbol, *accumulation);
}

DEFUN ("apropos-internal", Fapropos_internal, 1, 2, 0, /*
Show all symbols whose names contain match for REGEXP.
If optional 2nd arg PRED is non-nil, (funcall PRED SYM) is done
for each symbol and a symbol is mentioned only if that returns non-nil.
Return list of symbols found.
*/
       (string, pred))
{
  struct gcpro gcpro1;
  Lisp_Object accumulation;

  CHECK_STRING (string);
  accumulation = Fcons (string, Fcons (pred, Qnil));
  GCPRO1 (accumulation);
  map_obarray (Vobarray, apropos_accum, accumulation);
  accumulation = Fsort (Fcdr (Fcdr (accumulation)), Qstring_lessp);
  UNGCPRO;
  return (accumulation);
}


/* Extract and set components of symbols */

static void set_up_buffer_local_cache (Lisp_Object sym, 
				       struct symbol_value_buffer_local *bfwd,
				       struct buffer *buf,
				       Lisp_Object new_alist_el,
				       int set_it_p);

DEFUN ("boundp", Fboundp, 1, 1, 0, /*
T if SYMBOL's value is not void.
*/
       (sym))
{
  CHECK_SYMBOL (sym);
  return (UNBOUNDP (find_symbol_value (sym)) ? Qnil : Qt);
}

DEFUN ("globally-boundp", Fglobally_boundp, 1, 1, 0, /*
T if SYMBOL has a global (non-bound) value.
This is for the byte-compiler; you really shouldn't be using this.
*/
       (sym))
{
  CHECK_SYMBOL (sym);
  return (UNBOUNDP (top_level_value (sym)) ? Qnil : Qt);
}

DEFUN ("fboundp", Ffboundp, 1, 1, 0, /*
T if SYMBOL's function definition is not void.
*/
       (sym))
{
  CHECK_SYMBOL (sym);
  return ((UNBOUNDP (XSYMBOL (sym)->function)) ? Qnil : Qt);
}

/* Return non-zero if SYM's value or function (the current contents of
   which should be passed in as VAL) is constant, i.e. unsettable. */

static int
symbol_is_constant (Lisp_Object sym, Lisp_Object val)
{
  /* #### - I wonder if it would be better to just have a new magic value
     type and make nil, t, and all keywords have that same magic
     constant_symbol value.  This test is awfully specific about what is
     constant and what isn't.  --Stig */
  return
    NILP (sym) ||
    EQ (sym, Qt) ||
    (SYMBOL_VALUE_MAGIC_P (val) &&
     (XSYMBOL_VALUE_MAGIC_TYPE (val) == SYMVAL_CONST_OBJECT_FORWARD ||
      XSYMBOL_VALUE_MAGIC_TYPE (val) == SYMVAL_CONST_SPECIFIER_FORWARD ||
      XSYMBOL_VALUE_MAGIC_TYPE (val) == SYMVAL_CONST_FIXNUM_FORWARD ||
      XSYMBOL_VALUE_MAGIC_TYPE (val) == SYMVAL_CONST_BOOLEAN_FORWARD ||
      XSYMBOL_VALUE_MAGIC_TYPE (val) == SYMVAL_CONST_CURRENT_BUFFER_FORWARD ||
      XSYMBOL_VALUE_MAGIC_TYPE (val) == SYMVAL_CONST_SELECTED_CONSOLE_FORWARD))
#if 0
    /* #### - This is disabled until a new magic symbol_value for
       constants is added */
    || SYMBOL_IS_KEYWORD (sym)
#endif
    ;
}

/* We are setting SYM's value slot (or function slot, if FUNCTION_P is
   non-zero) to NEWVAL.  Make sure this is allowed.  NEWVAL is only
   used in the error message.  FOLLOW_PAST_LISP_MAGIC specifies
   whether we delve past symbol-value-lisp-magic objects.
   */

static void
reject_constant_symbols (Lisp_Object sym, Lisp_Object newval, int function_p,
			 Lisp_Object follow_past_lisp_magic)
{
  Lisp_Object val =
    (function_p ? XSYMBOL (sym)->function
     : fetch_value_maybe_past_magic (sym, follow_past_lisp_magic));

  if (SYMBOL_VALUE_MAGIC_P (val) &&
      XSYMBOL_VALUE_MAGIC_TYPE (val) == SYMVAL_CONST_SPECIFIER_FORWARD)
    signal_simple_error ("Use `set-specifier' to change a specifier's value",
			 sym);

  if (symbol_is_constant (sym, val))
    signal_error (Qsetting_constant,
		  UNBOUNDP (newval) ? list1 (sym) : list2 (sym, newval));
}

/* Verify that it's ok to make SYM buffer-local.  This rejects
   constants and default-buffer-local variables.  FOLLOW_PAST_LISP_MAGIC
   specifies whether we delve into symbol-value-lisp-magic objects.
   (Should be a symbol indicating what action is being taken; that way,
   we don't delve if there's a handler for that action, but do otherwise.) */

static void
verify_ok_for_buffer_local (Lisp_Object sym,
			    Lisp_Object follow_past_lisp_magic)
{
  Lisp_Object val = fetch_value_maybe_past_magic (sym, follow_past_lisp_magic);

  if (symbol_is_constant (sym, val) ||
      (SYMBOL_VALUE_MAGIC_P (val) &&
       XSYMBOL_VALUE_MAGIC_TYPE (val) ==
       SYMVAL_DEFAULT_BUFFER_FORWARD) ||
      (SYMBOL_VALUE_MAGIC_P (val) &&
       XSYMBOL_VALUE_MAGIC_TYPE (val) ==
       SYMVAL_DEFAULT_CONSOLE_FORWARD) ||
      /* #### It's theoretically possible for it to be reasonable
	 to have both console-local and buffer-local variables,
	 but I don't want to consider that right now. */
      (SYMBOL_VALUE_MAGIC_P (val) &&
       XSYMBOL_VALUE_MAGIC_TYPE (val) ==
       SYMVAL_SELECTED_CONSOLE_FORWARD)
      )
    signal_error (Qerror, 
		  list2 (build_string ("Symbol may not be buffer-local"),
			 sym));
}

DEFUN ("makunbound", Fmakunbound, 1, 1, 0, /*
Make SYMBOL's value be void.
*/
       (sym))
{
  Fset (sym, Qunbound);
  return sym;
}

DEFUN ("fmakunbound", Ffmakunbound, 1, 1, 0, /*
Make SYMBOL's function definition be void.
*/
       (sym))
{
  CHECK_SYMBOL (sym);
  reject_constant_symbols (sym, Qunbound, 1, Qt);
  XSYMBOL (sym)->function = Qunbound;
  return sym;
}

DEFUN ("symbol-function", Fsymbol_function, 1, 1, 0, /*
Return SYMBOL's function definition.	 Error if that is void.
*/
       (symbol))
{
  CHECK_SYMBOL (symbol);
  if (UNBOUNDP (XSYMBOL (symbol)->function))
    return Fsignal (Qvoid_function, list1 (symbol));
  return XSYMBOL (symbol)->function;
}

DEFUN ("symbol-plist", Fsymbol_plist, 1, 1, 0, /*
Return SYMBOL's property list.
*/
       (sym))
{
  CHECK_SYMBOL (sym);
  return XSYMBOL (sym)->plist;
}

DEFUN ("symbol-name", Fsymbol_name, 1, 1, 0, /*
Return SYMBOL's name, a string.
*/
       (sym))
{
  Lisp_Object name;

  CHECK_SYMBOL (sym);
  XSETSTRING (name, XSYMBOL (sym)->name);
  return name;
}

DEFUN ("fset", Ffset, 2, 2, 0, /*
Set SYMBOL's function definition to NEWVAL, and return NEWVAL.
*/
       (sym, newdef))
{
  /* This function can GC */
  CHECK_SYMBOL (sym);
  reject_constant_symbols (sym, newdef, 1, Qt);
  if (!NILP (Vautoload_queue) && !UNBOUNDP (XSYMBOL (sym)->function))
    Vautoload_queue = Fcons (Fcons (sym, XSYMBOL (sym)->function),
			     Vautoload_queue);
  XSYMBOL (sym)->function = newdef;
  /* Handle automatic advice activation */
  if (CONSP (XSYMBOL (sym)->plist) && !NILP (Fget (sym, Qad_advice_info,
  						   Qnil)))
    {
      call2 (Qad_activate, sym, Qnil);
      newdef = XSYMBOL (sym)->function;
    }
  return newdef;
}

/* FSFmacs */
DEFUN ("define-function", Fdefine_function, 2, 2, 0, /*
Set SYMBOL's function definition to NEWVAL, and return NEWVAL.
Associates the function with the current load file, if any.
*/
       (sym, newdef))
{
  /* This function can GC */
  CHECK_SYMBOL (sym);
  Ffset (sym, newdef);
  LOADHIST_ATTACH (sym);
  return newdef;
}


DEFUN ("setplist", Fsetplist, 2, 2, 0, /*
Set SYMBOL's property list to NEWVAL, and return NEWVAL.
*/
       (sym, newplist))
{
  CHECK_SYMBOL (sym);
  XSYMBOL (sym)->plist = newplist;
  return newplist;
}


/**********************************************************************/
/*                           symbol-value			      */
/**********************************************************************/

/* If the contents of the value cell of a symbol is one of the following
   three types of objects, then the symbol is "magic" in that setting
   and retrieving its value doesn't just set or retrieve the raw
   contents of the value cell.  None of these objects can escape to
   the user level, so there is no loss of generality.

   If a symbol is "unbound", then the contents of its value cell is
   Qunbound.  Despite appearances, this is *not* a symbol, but is
   a symbol-value-forward object.

   Logically all of the following objects are "symbol-value-magic"
   objects, and there are some games played w.r.t. this (#### this
   should be cleaned up).  SYMBOL_VALUE_MAGIC_P is true for all of
   the object types.  XSYMBOL_VALUE_MAGIC_TYPE returns the type of
   symbol-value-magic object.  There are more than three types
   returned by this macro: in particular, symbol-value-forward
   has eight subtypes, and symbol-value-buffer-local has two.  See
   symeval.h.

   1. symbol-value-forward

   symbol-value-forward is used for variables whose actual contents
   are stored in a C variable of some sort, and for Qunbound.  The
   lcheader.next field (which is only used to chain together free
   lcrecords) holds a pointer to the actual C variable.  Included
   in this type are "buffer-local" variables that are actually
   stored in the buffer object itself; in this case, the "pointer"
   is an offset into the struct buffer structure.

   The subtypes are as follows:

   SYMVAL_OBJECT_FORWARD:
      (declare with DEFVAR_LISP)
      The value of this variable is stored in a C variable of type
      "Lisp_Object".  Setting this variable sets the C variable.
      Accessing this variable retrieves a value from the C variable.
      These variables can be buffer-local -- in this case, the
      raw symbol-value field gets converted into a
      symbol-value-buffer-local, whose "current_value" slot contains
      the symbol-value-forward. (See below.)

   SYMVAL_FIXNUM_FORWARD:
   SYMVAL_BOOLEAN_FORWARD:
      (declare with DEFVAR_INT or DEFVAR_BOOL)
      Similar to SYMVAL_OBJECT_FORWARD except that the C variable
      is of type "int" and is an integer or boolean, respectively.

   SYMVAL_CONST_OBJECT_FORWARD:
   SYMVAL_CONST_FIXNUM_FORWARD:
   SYMVAL_CONST_BOOLEAN_FORWARD:
      (declare with DEFVAR_CONST_LISP, DEFVAR_CONST_INT, or
       DEFVAR_CONST_BOOL)
      Similar to SYMVAL_OBJECT_FORWARD, SYMVAL_FIXNUM_FORWARD, or
      SYMVAL_BOOLEAN_FORWARD, respectively, except that the value cannot
      be changed.

   SYMVAL_CONST_SPECIFIER_FORWARD:
      (declare with DEFVAR_SPECIFIER)
      Exactly like SYMVAL_CONST_OBJECT_FORWARD except that error message
      you get when attempting to set the value says to use
      `set-specifier' instead.

   SYMVAL_CURRENT_BUFFER_FORWARD:
      (declare with DEFVAR_BUFFER_LOCAL)
      This is used for built-in buffer-local variables -- i.e.
      Lisp variables whose value is stored in the "struct buffer".
      Variables of this sort always forward into C "Lisp_Object"
      fields (although there's no reason in principle that other
      types for ints and booleans couldn't be added).  Note that
      some of these variables are automatically local in each
      buffer, while some are only local when they become set
      (similar to `make-variable-buffer-local').  In these latter
      cases, of course, the default value shows through in all
      buffers in which the variable doesn't have a local value.
      This is implemented by making sure the "struct buffer" field
      always contains the correct value (whether it's local or
      a default) and maintaining a mask in the "struct buffer"
      indicating which fields are local.  When `set-default' is
      called on a variable that's not always local to all buffers,
      it loops through each buffer and sets the corresponding
      field in each buffer without a local value for the field,
      according to the mask.

      Calling `make-local-variable' on a variable of this sort
      only has the effect of maybe changing the current buffer's mask.
      Calling `make-variable-buffer-local' on a variable of this
      sort has no effect at all.

   SYMVAL_CONST_CURRENT_BUFFER_FORWARD:
      (declare with DEFVAR_CONST_BUFFER_LOCAL)
      Same as SYMVAL_CURRENT_BUFFER_FORWARD except that the
      value cannot be set.

   SYMVAL_DEFAULT_BUFFER_FORWARD:
      (declare with DEFVAR_BUFFER_DEFAULTS)
      This is used for the Lisp variables that contain the
      default values of built-in buffer-local variables.  Setting
      or referencing one of these variables forwards into a slot
      in the special struct buffer Vbuffer_defaults.

   SYMVAL_UNBOUND_MARKER:
      This is used for only one object, Qunbound.

   SYMVAL_SELECTED_CONSOLE_FORWARD:
      (declare with DEFVAR_CONSOLE_LOCAL)
      This is used for built-in console-local variables -- i.e.
      Lisp variables whose value is stored in the "struct console".
      These work just like built-in buffer-local variables.
      However, calling `make-local-variable' or
      `make-variable-buffer-local' on one of these variables
      is currently disallowed because that would entail having
      both console-local and buffer-local variables, which is
      trickier to implement.
      
   SYMVAL_CONST_SELECTED_CONSOLE_FORWARD:
      (declare with DEFVAR_CONST_CONSOLE_LOCAL)
      Same as SYMVAL_SELECTED_CONSOLE_FORWARD except that the
      value cannot be set.

   SYMVAL_DEFAULT_CONSOLE_FORWARD:
      (declare with DEFVAR_CONSOLE_DEFAULTS)
      This is used for the Lisp variables that contain the
      default values of built-in console-local variables.  Setting
      or referencing one of these variables forwards into a slot
      in the special struct console Vconsole_defaults.


   2. symbol-value-buffer-local

   symbol-value-buffer-local is used for variables that have had
   `make-local-variable' or `make-variable-buffer-local' applied
   to them.  This object contains an alist mapping buffers to
   values.  In addition, the object contains a "current value",
   which is the value in some buffer.  Whenever you access the
   variable with `symbol-value' or set it with `set' or `setq',
   things are switched around so that the "current value"
   refers to the current buffer, if it wasn't already.  This
   way, repeated references to a variable in the same buffer
   are almost as efficient as if the variable weren't buffer
   local.  Note that the alist may not be up-to-date w.r.t.
   the buffer whose value is current, as the "current value"
   cache is normally only flushed into the alist when the
   buffer it refers to changes.

   Note also that it is possible for `make-local-variable'
   or `make-variable-buffer-local' to be called on a variable
   that forwards into a C variable (i.e. a variable whose
   value cell is a symbol-value-forward).  In this case,
   the value cell becomes a symbol-value-buffer-local (as
   always), and the symbol-value-forward moves into
   the "current value" cell in this object.  Also, in
   this case the "current value" *always* refers to the
   current buffer, so that the values of the C variable
   always is the correct value for the current buffer.
   set_buffer_internal() automatically updates the current-value
   cells of all buffer-local variables that forward into C
   variables. (There is a list of all buffer-local variables
   that is maintained for this and other purposes.)

   Note that only certain types of `symbol-value-forward' objects
   can find their way into the "current value" cell of a
   `symbol-value-buffer-local' object: SYMVAL_OBJECT_FORWARD,
   SYMVAL_FIXNUM_FORWARD, SYMVAL_BOOLEAN_FORWARD, and
   SYMVAL_UNBOUND_MARKER.  The SYMVAL_CONST_*_FORWARD cannot
   be buffer-local because they are unsettable;
   SYMVAL_DEFAULT_*_FORWARD cannot be buffer-local because that
   makes no sense; making SYMVAL_CURRENT_BUFFER_FORWARD buffer-local
   does not have much of an effect (it's already buffer-local); and
   SYMVAL_SELECTED_CONSOLE_FORWARD cannot be buffer-local because
   that's not currently implemented.


   3. symbol-value-varalias

   A symbol-value-varalias object is used for variables that
   are aliases for other variables.  This object contains
   the symbol that this variable is aliased to.
   symbol-value-varalias objects cannot occur anywhere within
   a symbol-value-buffer-local object, and most of the
   low-level functions below do not accept them; you need
   to call follow_varalias_pointers to get the actual
   symbol to operate on.
   */

static Lisp_Object mark_symbol_value_buffer_local (Lisp_Object,
						   void (*) (Lisp_Object));
static Lisp_Object mark_symbol_value_lisp_magic (Lisp_Object,
						 void (*) (Lisp_Object));
static Lisp_Object mark_symbol_value_varalias (Lisp_Object,
					       void (*) (Lisp_Object));

DEFINE_LRECORD_IMPLEMENTATION ("symbol-value-forward",
			       symbol_value_forward,
			       this_one_is_unmarkable,
			       print_symbol_value_magic, 0, 0, 0,
			       struct symbol_value_forward);

DEFINE_LRECORD_IMPLEMENTATION ("symbol-value-buffer-local",
			       symbol_value_buffer_local, 
			       mark_symbol_value_buffer_local,
			       print_symbol_value_magic, 
			       0, 0, 0,
			       struct symbol_value_buffer_local);

DEFINE_LRECORD_IMPLEMENTATION ("symbol-value-lisp-magic",
			       symbol_value_lisp_magic, 
			       mark_symbol_value_lisp_magic,
			       print_symbol_value_magic, 
			       0, 0, 0,
			       struct symbol_value_lisp_magic);

DEFINE_LRECORD_IMPLEMENTATION ("symbol-value-varalias",
			       symbol_value_varalias, 
			       mark_symbol_value_varalias,
			       print_symbol_value_magic, 
			       0, 0, 0,
			       struct symbol_value_varalias);

static Lisp_Object
mark_symbol_value_buffer_local (Lisp_Object obj,
				void (*markobj) (Lisp_Object))
{
  struct symbol_value_buffer_local *bfwd;

  assert (XSYMBOL_VALUE_MAGIC_TYPE (obj) == SYMVAL_BUFFER_LOCAL ||
	  XSYMBOL_VALUE_MAGIC_TYPE (obj) == SYMVAL_SOME_BUFFER_LOCAL);

  bfwd = XSYMBOL_VALUE_BUFFER_LOCAL (obj);
  ((markobj) (bfwd->default_value));
  ((markobj) (bfwd->current_value));
  ((markobj) (bfwd->current_buffer));
  return (bfwd->current_alist_element);
}

static Lisp_Object
mark_symbol_value_lisp_magic (Lisp_Object obj,
			      void (*markobj) (Lisp_Object))
{
  struct symbol_value_lisp_magic *bfwd;
  int i;

  assert (XSYMBOL_VALUE_MAGIC_TYPE (obj) == SYMVAL_LISP_MAGIC);

  bfwd = XSYMBOL_VALUE_LISP_MAGIC (obj);
  for (i = 0; i < MAGIC_HANDLER_MAX; i++)
    {
      ((markobj) (bfwd->handler[i]));
      ((markobj) (bfwd->harg[i]));
    }
  return (bfwd->shadowed);
}

static Lisp_Object
mark_symbol_value_varalias (Lisp_Object obj,
			    void (*markobj) (Lisp_Object))
{
  struct symbol_value_varalias *bfwd;

  assert (XSYMBOL_VALUE_MAGIC_TYPE (obj) == SYMVAL_VARALIAS);

  bfwd = XSYMBOL_VALUE_VARALIAS (obj);
  ((markobj) (bfwd->shadowed));
  return (bfwd->aliasee);
}

/* Should never, ever be called. (except by an external debugger) */
void
print_symbol_value_magic (Lisp_Object obj, 
			  Lisp_Object printcharfun, int escapeflag)
{
  char buf[200];
  sprintf (buf, "#<INTERNAL EMACS BUG (symfwd %d) 0x%x>",
	   (EMACS_INT) XSYMBOL_VALUE_MAGIC_TYPE (obj),
	   (EMACS_INT) XPNTR (obj));
  write_c_string (buf, printcharfun);
}


/* Getting and setting values of symbols */

/* Given the raw contents of a symbol value cell, return the Lisp value of
   the symbol.  However, VALCONTENTS cannot be a symbol-value-buffer-local,
   symbol-value-lisp-magic, or symbol-value-varalias.

   BUFFER specifies a buffer, and is used for built-in buffer-local
   variables, which are of type SYMVAL_CURRENT_BUFFER_FORWARD.
   Note that such variables are never encapsulated in a
   symbol-value-buffer-local structure.

   CONSOLE specifies a console, and is used for built-in console-local
   variables, which are of type SYMVAL_SELECTED_CONSOLE_FORWARD.
   Note that such variables are (currently) never encapsulated in a
   symbol-value-buffer-local structure.
 */

static Lisp_Object
do_symval_forwarding (Lisp_Object valcontents, struct buffer *buffer,
		      struct console *console)
{
  CONST struct symbol_value_forward *fwd;

  if (!SYMBOL_VALUE_MAGIC_P (valcontents))
    return (valcontents);

  fwd = XSYMBOL_VALUE_FORWARD (valcontents);
  switch (fwd->magic.type)
    {
    case SYMVAL_FIXNUM_FORWARD:
    case SYMVAL_CONST_FIXNUM_FORWARD:
      return (make_int (*((int *)symbol_value_forward_forward (fwd))));

    case SYMVAL_BOOLEAN_FORWARD:
    case SYMVAL_CONST_BOOLEAN_FORWARD:
      {
	if (*((int *)symbol_value_forward_forward (fwd)))
	  return (Qt);
	else
	  return (Qnil);
      }

    case SYMVAL_OBJECT_FORWARD:
    case SYMVAL_CONST_OBJECT_FORWARD:
    case SYMVAL_CONST_SPECIFIER_FORWARD:
      return (*((Lisp_Object *)symbol_value_forward_forward (fwd)));

    case SYMVAL_DEFAULT_BUFFER_FORWARD:
      return (*((Lisp_Object *)((char *) XBUFFER (Vbuffer_defaults)
				+ ((char *)symbol_value_forward_forward (fwd)
				   - (char *)&buffer_local_flags))));


    case SYMVAL_CURRENT_BUFFER_FORWARD:
    case SYMVAL_CONST_CURRENT_BUFFER_FORWARD:
      assert (buffer);
      return (*((Lisp_Object *)((char *)buffer
				+ ((char *)symbol_value_forward_forward (fwd)
				   - (char *)&buffer_local_flags))));

    case SYMVAL_DEFAULT_CONSOLE_FORWARD:
      return (*((Lisp_Object *)((char *) XCONSOLE (Vconsole_defaults)
				+ ((char *)symbol_value_forward_forward (fwd)
				   - (char *)&console_local_flags))));

    case SYMVAL_SELECTED_CONSOLE_FORWARD:
    case SYMVAL_CONST_SELECTED_CONSOLE_FORWARD:
      assert (console);
      return (*((Lisp_Object *)((char *)console
				+ ((char *)symbol_value_forward_forward (fwd)
				   - (char *)&console_local_flags))));

    case SYMVAL_UNBOUND_MARKER:
      return (valcontents);

    default:
      abort ();
    }
  return Qnil;	/* suppress compiler warning */
}

/* Set the value of default-buffer-local variable SYM to VALUE. */

static void
set_default_buffer_slot_variable (Lisp_Object sym,
				  Lisp_Object value)
{
  /* Handle variables like case-fold-search that have special slots in
     the buffer. Make them work apparently like buffer_local variables.
     */
  /* At this point, the value cell may not contain a symbol-value-varalias
     or symbol-value-buffer-local, and if there's a handler, we should
     have already called it. */
  Lisp_Object valcontents = fetch_value_maybe_past_magic (sym, Qt);
  CONST struct symbol_value_forward *fwd 
    = XSYMBOL_VALUE_FORWARD (valcontents);
  int offset = ((char *) symbol_value_forward_forward (fwd) 
		- (char *) &buffer_local_flags);
  int mask = XINT (*((Lisp_Object *) symbol_value_forward_forward (fwd)));
  int (*magicfun) (Lisp_Object simm, Lisp_Object *val, Lisp_Object in_object,
		   int flags) = symbol_value_forward_magicfun (fwd);
  
  *((Lisp_Object *) (offset + (char *) XBUFFER (Vbuffer_defaults)))
    = value;
  
  if (mask > 0)		/* Not always per-buffer */
    {
      Lisp_Object tail;
      
      /* Set value in each buffer which hasn't shadowed the default */
      LIST_LOOP (tail, Vbuffer_alist)
	{
	  struct buffer *b = XBUFFER (XCDR (XCAR (tail)));
	  if (!(b->local_var_flags & mask))
	    {
	      if (magicfun)
		(magicfun) (sym, &value, make_buffer (b), 0);
	      *((Lisp_Object *) (offset + (char *) b)) = value; 
	    }
	}
    }
}

/* Set the value of default-console-local variable SYM to VALUE. */

static void
set_default_console_slot_variable (Lisp_Object sym,
				   Lisp_Object value)
{
  /* Handle variables like case-fold-search that have special slots in
     the console. Make them work apparently like console_local variables.
     */
  /* At this point, the value cell may not contain a symbol-value-varalias
     or symbol-value-buffer-local, and if there's a handler, we should
     have already called it. */
  Lisp_Object valcontents = fetch_value_maybe_past_magic (sym, Qt);
  CONST struct symbol_value_forward *fwd 
    = XSYMBOL_VALUE_FORWARD (valcontents);
  int offset = ((char *) symbol_value_forward_forward (fwd) 
		- (char *) &console_local_flags);
  int mask = XINT (*((Lisp_Object *) symbol_value_forward_forward (fwd)));
  int (*magicfun) (Lisp_Object simm, Lisp_Object *val, Lisp_Object in_object,
		   int flags) = symbol_value_forward_magicfun (fwd);
  
  *((Lisp_Object *) (offset + (char *) XCONSOLE (Vconsole_defaults)))
    = value;
  
  if (mask > 0)		/* Not always per-console */
    {
      Lisp_Object tail;
      
      /* Set value in each console which hasn't shadowed the default */
      LIST_LOOP (tail, Vconsole_list)
	{
	  Lisp_Object dev = XCAR (tail);
	  struct console *d = XCONSOLE (dev);
	  if (!(d->local_var_flags & mask))
	    {
	      if (magicfun)
		(magicfun) (sym, &value, dev, 0);
	      *((Lisp_Object *) (offset + (char *) d)) = value; 
	    }
	}
    }
}

/* Store NEWVAL into SYM.

   SYM's value slot may *not* be types (5) or (6) above,
   i.e. no symbol-value-varalias objects. (You should have
   forwarded past all of these.)

   SYM should not be an unsettable symbol or a symbol with
   a magic `set-value' handler (unless you want to explicitly
   ignore this handler).

   OVALUE is the current value of SYM, but forwarded past any
   symbol-value-buffer-local and symbol-value-lisp-magic objects.
   (i.e. if SYM is a symbol-value-buffer-local, OVALUE should be
   the contents of its current-value cell.) NEWVAL may only be
   a simple value or Qunbound.  If SYM is a symbol-value-buffer-local,
   this function will only modify its current-value cell, which should
   already be set up to point to the current buffer.
  */

static void
store_symval_forwarding (Lisp_Object sym, Lisp_Object ovalue,
			 Lisp_Object newval)
{
  if (!SYMBOL_VALUE_MAGIC_P (ovalue) || UNBOUNDP (ovalue))
    {
      Lisp_Object *store_pointer = value_slot_past_magic (sym);

      if (SYMBOL_VALUE_BUFFER_LOCAL_P (*store_pointer))
	store_pointer =
	  &XSYMBOL_VALUE_BUFFER_LOCAL (*store_pointer)->current_value;

      assert (UNBOUNDP (*store_pointer)
	      || !SYMBOL_VALUE_MAGIC_P (*store_pointer));
      *store_pointer = newval;
    }

  else
    {
      CONST struct symbol_value_forward *fwd
	= XSYMBOL_VALUE_FORWARD (ovalue);
      int type = XSYMBOL_VALUE_MAGIC_TYPE (ovalue);
      int (*magicfun) (Lisp_Object simm, Lisp_Object *val,
		       Lisp_Object in_object, int flags) =
			 symbol_value_forward_magicfun (fwd);

      switch (type)
	{
	case SYMVAL_FIXNUM_FORWARD:
	  {
	    CHECK_INT (newval);
	    if (magicfun)
	      (magicfun) (sym, &newval, Qnil, 0);
	    *((int *) symbol_value_forward_forward (fwd)) = XINT (newval);
	    return;
	  }

	case SYMVAL_BOOLEAN_FORWARD:
	  {
	    if (magicfun)
	      (magicfun) (sym, &newval, Qnil, 0);
	    *((int *) symbol_value_forward_forward (fwd))
	      = ((NILP (newval)) ? 0 : 1);
	    return;
	  }

	case SYMVAL_OBJECT_FORWARD:
	  {
	    if (magicfun)
	      (magicfun) (sym, &newval, Qnil, 0);
	    *((Lisp_Object *) symbol_value_forward_forward (fwd)) = newval;
	    return;
	  }
	  
	case SYMVAL_DEFAULT_BUFFER_FORWARD:
	  {
	    set_default_buffer_slot_variable (sym, newval);
	    return;
	  }

	case SYMVAL_CURRENT_BUFFER_FORWARD:
	  {
	    if (magicfun)
	      (magicfun) (sym, &newval, make_buffer (current_buffer), 0);
	    *((Lisp_Object *) ((char *) current_buffer
			       + ((char *) symbol_value_forward_forward (fwd)
				  - (char *) &buffer_local_flags))) 
	      = newval;
	    return;
	  }

	case SYMVAL_DEFAULT_CONSOLE_FORWARD:
	  {
	    set_default_console_slot_variable (sym, newval);
	    return;
	  }

	case SYMVAL_SELECTED_CONSOLE_FORWARD:
	  {
	    if (magicfun)
	      (magicfun) (sym, &newval, Vselected_console, 0);
	    *((Lisp_Object *) ((char *) XCONSOLE (Vselected_console)
			       + ((char *) symbol_value_forward_forward (fwd)
				  - (char *) &console_local_flags))) 
	      = newval;
	    return;
	  }

	default:
	  abort ();
	}
    }
}

/* Given a per-buffer variable SYMBOL and its raw value-cell contents
   BFWD, locate and return a pointer to the element in BUFFER's
   local_var_alist for SYMBOL.  The return value will be Qnil if
   BUFFER does not have its own value for SYMBOL (i.e. the default
   value is seen in that buffer).
   */

static Lisp_Object
buffer_local_alist_element (struct buffer *buffer, Lisp_Object symbol,
			    struct symbol_value_buffer_local *bfwd)
{
  if (!NILP (bfwd->current_buffer) &&
      XBUFFER (bfwd->current_buffer) == buffer)
    /* This is just an optimization of the below. */
    return (bfwd->current_alist_element);
  else
    return (assq_no_quit (symbol, buffer->local_var_alist));
}

/* [Remember that the slot that mirrors CURRENT-VALUE in the
   symbol-value-buffer-local of a per-buffer variable -- i.e. the
   slot in CURRENT-BUFFER's local_var_alist, or the DEFAULT-VALUE
   slot -- may be out of date.]

   Write out any cached value in buffer-local variable SYMBOL's
   buffer-local structure, which is passed in as BFWD.
*/

static void
write_out_buffer_local_cache (Lisp_Object symbol,
			      struct symbol_value_buffer_local *bfwd)
{
  if (!NILP (bfwd->current_buffer))
    {
      /* We pass 0 for BUFFER because only SYMVAL_CURRENT_BUFFER_FORWARD
	 uses it, and that type cannot be inside a symbol-value-buffer-local */
      Lisp_Object cval = do_symval_forwarding (bfwd->current_value, 0, 0);
      if (NILP (bfwd->current_alist_element))
	/* current_value may be updated more recently than default_value */
	bfwd->default_value = cval;
      else
	Fsetcdr (bfwd->current_alist_element, cval);
    }
}

/* SYM is a buffer-local variable, and BFWD is its buffer-local structure.
   Set up BFWD's cache for validity in buffer BUF.  This assumes that
   the cache is currently in a consistent state (this can include
   not having any value cached, if BFWD->CURRENT_BUFFER is nil).

   If the cache is already set up for BUF, this function does nothing
   at all.

   Otherwise, if SYM forwards out to a C variable, this also forwards
   SYM's value in BUF out to the variable.  Therefore, you generally
   only want to call this when BUF is, or is about to become, the
   current buffer.

   (Otherwise, you can just retrieve the value without changing the
   cache, at the expense of slower retrieval.)
*/

static void
set_up_buffer_local_cache (Lisp_Object sym, 
			   struct symbol_value_buffer_local *bfwd,
			   struct buffer *buf,
			   Lisp_Object new_alist_el,
			   int set_it_p)
{
  Lisp_Object new_val;

  if (!NILP (bfwd->current_buffer)
      && buf == XBUFFER (bfwd->current_buffer))
    /* Cache is already set up. */
    return;

  /* Flush out the old cache. */
  write_out_buffer_local_cache (sym, bfwd);

  /* Retrieve the new alist element and new value. */
  if (NILP (new_alist_el)
      && set_it_p)
  new_alist_el = buffer_local_alist_element (buf, sym, bfwd);

  if (NILP (new_alist_el))
    new_val = bfwd->default_value;
  else
    new_val = Fcdr (new_alist_el);

  bfwd->current_alist_element = new_alist_el;
  XSETBUFFER (bfwd->current_buffer, buf);

  /* Now store the value into the current-value slot.
     We don't simply write it there, because the current-value
     slot might be a forwarding pointer, in which case we need
     to instead write the value into the C variable.

     We might also want to call a magic function.

     So instead, we call this function. */
  store_symval_forwarding (sym, bfwd->current_value, new_val);
}


void
kill_buffer_local_variables (struct buffer *buf)
{
  Lisp_Object prev = Qnil;
  Lisp_Object alist;

  /* Any which are supposed to be permanent,
     make local again, with the same values they had.  */
     
  for (alist = buf->local_var_alist; !NILP (alist); alist = XCDR (alist))
    {
      Lisp_Object sym = XCAR (XCAR (alist));
      struct symbol_value_buffer_local *bfwd;
      /* Variables with a symbol-value-varalias should not be here
	 (we should have forwarded past them) and there must be a
	 symbol-value-buffer-local.  If there's a symbol-value-lisp-magic,
	 just forward past it; if the variable has a handler, it was
	 already called. */
      Lisp_Object value = fetch_value_maybe_past_magic (sym, Qt);

      assert (SYMBOL_VALUE_BUFFER_LOCAL_P (value));
      bfwd = XSYMBOL_VALUE_BUFFER_LOCAL (value);

      if (!NILP (Fget (sym, Qpermanent_local, Qnil)))
	/* prev points to the last alist element that is still
	   staying around, so *only* update it now.  This didn't
	   used to be the case; this bug has been around since
	   mly's rewrite two years ago! */
	prev = alist;
      else
	{
	  /* Really truly kill it. */
	  if (!NILP (prev))
	    XCDR (prev) = XCDR (alist);
	  else
	    buf->local_var_alist = XCDR (alist);

	  /* We just effectively changed the value for this variable
	     in BUF. So: */

	  /* (1) If the cache is caching BUF, invalidate the cache. */
	  if (!NILP (bfwd->current_buffer) &&
	      buf == XBUFFER (bfwd->current_buffer))
	    bfwd->current_buffer = Qnil;

	  /* (2) If we changed the value in current_buffer and this
	     variable forwards to a C variable, we need to change the
	     value of the C variable.  set_up_buffer_local_cache()
	     will do this.  It doesn't hurt to do it whenever
	     BUF == current_buffer, so just go ahead and do that. */
	  if (buf == current_buffer)
	    set_up_buffer_local_cache (sym, bfwd, buf, Qnil, 0);
	}
    }
}

static Lisp_Object
find_symbol_value_1 (Lisp_Object sym, struct buffer *buf,
		     struct console *con, int swap_it_in,
		     Lisp_Object symcons, int set_it_p)
{
  Lisp_Object valcontents;

 retry:
  valcontents = XSYMBOL (sym)->value;

 retry_2:
  if (!SYMBOL_VALUE_MAGIC_P (valcontents))
    return (valcontents);

  switch (XSYMBOL_VALUE_MAGIC_TYPE (valcontents))
    {
    case SYMVAL_LISP_MAGIC:
      /* #### kludge */
      valcontents = XSYMBOL_VALUE_LISP_MAGIC (valcontents)->shadowed;
      /* semi-change-o */
      goto retry_2;

    case SYMVAL_VARALIAS:
      sym = follow_varalias_pointers (sym, Qt /* #### kludge */);
      symcons = Qnil;
      /* presto change-o! */
      goto retry;

    case SYMVAL_BUFFER_LOCAL:
    case SYMVAL_SOME_BUFFER_LOCAL:
      {
	struct symbol_value_buffer_local *bfwd
	  = XSYMBOL_VALUE_BUFFER_LOCAL (valcontents);

	if (swap_it_in)
	  {
	    set_up_buffer_local_cache (sym, bfwd, buf, symcons, set_it_p);
	    valcontents = bfwd->current_value;
	  }
	else
	  {
	    if (!NILP (bfwd->current_buffer) &&
		buf == XBUFFER (bfwd->current_buffer))
	      valcontents = bfwd->current_value;
	    else if (NILP (symcons))
	      {
		if (set_it_p)
		valcontents = assq_no_quit (sym, buf->local_var_alist);
		if (NILP (valcontents))
		  valcontents = bfwd->default_value;
		else
		  valcontents = XCDR (valcontents);
	      }
	    else
	      valcontents = XCDR (symcons);
	  }
	break;
      }
      
    default:
      break;
    }
  return (do_symval_forwarding (valcontents, buf, con));
}


/* Find the value of a symbol in BUFFER, returning Qunbound if it's not
   bound.  Note that it must not be possible to QUIT within this
   function. */

Lisp_Object
symbol_value_in_buffer (Lisp_Object sym, Lisp_Object buffer)
{
  struct buffer *buf;

  CHECK_SYMBOL (sym);

  if (!NILP (buffer))
    {
      CHECK_BUFFER (buffer);
      buf = XBUFFER (buffer);
    }
  else
    buf = current_buffer;

  return find_symbol_value_1 (sym, buf,
			      /* If it bombs out at startup due to a
				 Lisp error, this may be nil. */
			      CONSOLEP (Vselected_console)
			      ?	XCONSOLE (Vselected_console) : 0, 0, Qnil, 1);
}

static Lisp_Object
symbol_value_in_console (Lisp_Object sym, Lisp_Object console)
{
  CHECK_SYMBOL (sym);

  if (!NILP (console))
    CHECK_CONSOLE (console);
  else
    console = Vselected_console;

  return find_symbol_value_1 (sym, current_buffer, XCONSOLE (console), 0,
			      Qnil, 1);
}

/* Return the current value of SYM.  The difference between this function
   and calling symbol_value_in_buffer with a BUFFER of Qnil is that
   this updates the CURRENT_VALUE slot of buffer-local variables to
   point to the current buffer, while symbol_value_in_buffer doesn't. */

Lisp_Object
find_symbol_value (Lisp_Object sym)
{
  /* WARNING: This function can be called when current_buffer is 0
     and Vselected_console is Qnil, early in initialization. */
  struct console *dev;

  CHECK_SYMBOL (sym);
  if (CONSOLEP (Vselected_console))
    dev = XCONSOLE (Vselected_console);
  else
    {
      /* This can also get called while we're preparing to shutdown.
         #### What should really happen in that case?  Should we
         actually fix things so we can't get here in that case? */
      assert (!initialized || preparing_for_armageddon);
      dev = 0;
    }

  return find_symbol_value_1 (sym, current_buffer, dev, 1, Qnil, 1);
}

/* This is an optimized function for quick lookup of buffer local symbols
   by avoiding O(n) search.  This will work when either:
     a) We have already found the symbol e.g. by traversing local_var_alist.
   or
     b) We know that the symbol will not be found in the current buffer's
        list of local variables.
   In the former case, find_it_p is 1 and symbol_cons is the element from
   local_var_alist.  In the latter case, find_it_p is 0 and symbol_cons
   is the symbol.

   This function is called from set_buffer_internal which does both of these
   things. */

Lisp_Object
find_symbol_value_quickly (Lisp_Object symbol_cons, int find_it_p)
{
  /* WARNING: This function can be called when current_buffer is 0
     and Vselected_console is Qnil, early in initialization. */
  struct console *dev;
  Lisp_Object sym = find_it_p ? XCAR (symbol_cons) : symbol_cons;
  
  CHECK_SYMBOL (sym);
  if (CONSOLEP (Vselected_console))
    dev = XCONSOLE (Vselected_console);
  else
    {
      /* This can also get called while we're preparing to shutdown.
         #### What should really happen in that case?  Should we
         actually fix things so we can't get here in that case? */
      assert (!initialized || preparing_for_armageddon);
      dev = 0;
    }

  return find_symbol_value_1 (sym, current_buffer, dev, 1,
			      find_it_p ? symbol_cons : Qnil,
			      find_it_p);
}

DEFUN ("symbol-value", Fsymbol_value, 1, 1, 0, /*
Return SYMBOL's value.  Error if that is void.
*/
       (sym))
{
  Lisp_Object val = find_symbol_value (sym);

  if (UNBOUNDP (val))
    return Fsignal (Qvoid_variable, list1 (sym));
  else
    return val;
}

DEFUN ("set", Fset, 2, 2, 0, /*
Set SYMBOL's value to NEWVAL, and return NEWVAL.
*/
       (sym, newval))
{
  REGISTER Lisp_Object valcontents;
  /* remember, we're called by Fmakunbound() as well */

  CHECK_SYMBOL (sym);

 retry:
  reject_constant_symbols (sym, newval, 0,
			   UNBOUNDP (newval) ? Qmakunbound : Qset);
  valcontents = XSYMBOL (sym)->value;
 retry_2:

  if (SYMBOL_VALUE_MAGIC_P (valcontents))
    {
      switch (XSYMBOL_VALUE_MAGIC_TYPE (valcontents))
	{
	case SYMVAL_LISP_MAGIC:
	  {
	    Lisp_Object retval;

	    if (UNBOUNDP (newval))
	      retval = maybe_call_magic_handler (sym, Qmakunbound, 0);
	    else
	      retval = maybe_call_magic_handler (sym, Qset, 1, newval);
	    if (!UNBOUNDP (retval))
	      return newval;
	    valcontents = XSYMBOL_VALUE_LISP_MAGIC (valcontents)->shadowed;
	    /* semi-change-o */
	    goto retry_2;
	  }
	  
	case SYMVAL_VARALIAS:
	  sym = follow_varalias_pointers (sym,
					  UNBOUNDP (newval)
					  ? Qmakunbound : Qset);
	  /* presto change-o! */
	  goto retry;

	case SYMVAL_FIXNUM_FORWARD:
	case SYMVAL_BOOLEAN_FORWARD:
	case SYMVAL_OBJECT_FORWARD:
	case SYMVAL_DEFAULT_BUFFER_FORWARD:
	case SYMVAL_DEFAULT_CONSOLE_FORWARD:
	  if (UNBOUNDP (newval))
	    signal_error (Qerror, 
			  list2 (build_string ("Cannot makunbound"), sym));
	  break;

	case SYMVAL_UNBOUND_MARKER:
	  break;

	case SYMVAL_CURRENT_BUFFER_FORWARD:
	  {
	    CONST struct symbol_value_forward *fwd
	      = XSYMBOL_VALUE_FORWARD (valcontents);
	    int mask = XINT (*((Lisp_Object *)
                               symbol_value_forward_forward (fwd)));
	    if (mask > 0)
	      /* Setting this variable makes it buffer-local */
	      current_buffer->local_var_flags |= mask;
	    break;
	  }

	case SYMVAL_SELECTED_CONSOLE_FORWARD:
	  {
	    CONST struct symbol_value_forward *fwd
	      = XSYMBOL_VALUE_FORWARD (valcontents);
	    int mask = XINT (*((Lisp_Object *)
                               symbol_value_forward_forward (fwd)));
	    if (mask > 0)
	      /* Setting this variable makes it console-local */
	      XCONSOLE (Vselected_console)->local_var_flags |= mask;
	    break;
	  }

	case SYMVAL_BUFFER_LOCAL:
	case SYMVAL_SOME_BUFFER_LOCAL:
	  {
	    /* If we want to examine or set the value and
	       CURRENT-BUFFER is current, we just examine or set
	       CURRENT-VALUE. If CURRENT-BUFFER is not current, we
	       store the current CURRENT-VALUE value into
	       CURRENT-ALIST- ELEMENT, then find the appropriate alist
	       element for the buffer now current and set up
	       CURRENT-ALIST-ELEMENT.  Then we set CURRENT-VALUE out
	       of that element, and store into CURRENT-BUFFER.
	       
	       If we are setting the variable and the current buffer does
	       not have an alist entry for this variable, an alist entry is
	       created.
	       
	       Note that CURRENT-VALUE can be a forwarding pointer.
	       Each time it is examined or set, forwarding must be
	       done. */
	    struct symbol_value_buffer_local *bfwd
	      = XSYMBOL_VALUE_BUFFER_LOCAL (valcontents);
	    int some_buffer_local_p =
	      (bfwd->magic.type == SYMVAL_SOME_BUFFER_LOCAL);
	    /* What value are we caching right now?  */
	    Lisp_Object aelt = bfwd->current_alist_element;

	    if (!NILP (bfwd->current_buffer) &&
		current_buffer == XBUFFER (bfwd->current_buffer)
		&& ((some_buffer_local_p)
		    ? 1		   /* doesn't automatically become local */
		    : !NILP (aelt) /* already local */
		    ))
	      {
		/* Cache is valid */
		valcontents = bfwd->current_value;
	      }
	    else
	      {
		/* If the current buffer is not the buffer whose binding is
		   currently cached, or if it's a SYMVAL_BUFFER_LOCAL and
		   we're looking at the default value, the cache is invalid; we
		   need to write it out, and find the new CURRENT-ALIST-ELEMENT
		   */
		
		/* Write out the cached value for the old buffer; copy it
		   back to its alist element.  This works if the current
		   buffer only sees the default value, too.  */
		write_out_buffer_local_cache (sym, bfwd);

		/* Find the new value for CURRENT-ALIST-ELEMENT.  */
		aelt = buffer_local_alist_element (current_buffer, sym, bfwd);
		if (NILP (aelt))
		  {
		    /* This buffer is still seeing the default value.  */
		    if (!some_buffer_local_p)
		      {
			/* If it's a SYMVAL_BUFFER_LOCAL, give this buffer a
			   new assoc for a local value and set
			   CURRENT-ALIST-ELEMENT to point to that.  */
			aelt =
			  do_symval_forwarding (bfwd->current_value,
						current_buffer,
						XCONSOLE (Vselected_console));
			aelt = Fcons (sym, aelt);
			current_buffer->local_var_alist
			  = Fcons (aelt, current_buffer->local_var_alist);
		      }
		    else
		      {
			/* If the variable is a SYMVAL_SOME_BUFFER_LOCAL,
			   we're currently seeing the default value. */
			;
		      }
		  }
		/* Cache the new buffer's assoc in CURRENT-ALIST-ELEMENT.  */
		bfwd->current_alist_element = aelt;
		/* Set BUFFER, now that CURRENT-ALIST-ELEMENT is accurate.  */
		XSETBUFFER (bfwd->current_buffer, current_buffer);
		valcontents = bfwd->current_value;
	      }
	    break;
	  }
	default:
	  abort ();
	}
    }
  store_symval_forwarding (sym, valcontents, newval);

  return (newval);
}


/* Access or set a buffer-local symbol's default value.	 */

/* Return the default value of SYM, but don't check for voidness.
   Return Qunbound if it is void.  */

static Lisp_Object
default_value (Lisp_Object sym)
{
  Lisp_Object valcontents;

  CHECK_SYMBOL (sym);

 retry:
  valcontents = XSYMBOL (sym)->value;

 retry_2:
  if (!SYMBOL_VALUE_MAGIC_P (valcontents))
    return (valcontents);
    
  switch (XSYMBOL_VALUE_MAGIC_TYPE (valcontents))
    {
    case SYMVAL_LISP_MAGIC:
      /* #### kludge */
      valcontents = XSYMBOL_VALUE_LISP_MAGIC (valcontents)->shadowed;
      /* semi-change-o */
      goto retry_2;

    case SYMVAL_VARALIAS:
      sym = follow_varalias_pointers (sym, Qt /* #### kludge */);
      /* presto change-o! */
      goto retry;

    case SYMVAL_UNBOUND_MARKER:
      return valcontents;

    case SYMVAL_CURRENT_BUFFER_FORWARD:
      {
	CONST struct symbol_value_forward *fwd
	  = XSYMBOL_VALUE_FORWARD (valcontents);
	return (*((Lisp_Object *)((char *) XBUFFER (Vbuffer_defaults)
				  + ((char *)symbol_value_forward_forward (fwd)
				     - (char *)&buffer_local_flags))));
      }

    case SYMVAL_SELECTED_CONSOLE_FORWARD:
      {
	CONST struct symbol_value_forward *fwd
	  = XSYMBOL_VALUE_FORWARD (valcontents);
	return (*((Lisp_Object *)((char *) XCONSOLE (Vconsole_defaults)
				  + ((char *)symbol_value_forward_forward (fwd)
				     - (char *)&console_local_flags))));
      }

    case SYMVAL_BUFFER_LOCAL:
    case SYMVAL_SOME_BUFFER_LOCAL:
      {
	struct symbol_value_buffer_local *bfwd = 
	  XSYMBOL_VALUE_BUFFER_LOCAL (valcontents);

	/* Handle user-created local variables.	 */
	/* If var is set up for a buffer that lacks a local value for it,
	   the current value is nominally the default value.
	   But the current value slot may be more up to date, since
	   ordinary setq stores just that slot.	 So use that.  */
	if (NILP (bfwd->current_alist_element))
	  return (do_symval_forwarding (bfwd->current_value, current_buffer,
					XCONSOLE (Vselected_console)));
	else
	  return (bfwd->default_value);
      }
    default:
      /* For other variables, get the current value.	*/
      return (do_symval_forwarding (valcontents, current_buffer,
				    XCONSOLE (Vselected_console)));
    }
  
  RETURN_NOT_REACHED(Qnil)	/* suppress compiler warning */
}

DEFUN ("default-boundp", Fdefault_boundp, 1, 1, 0, /*
Return T if SYMBOL has a non-void default value.
This is the value that is seen in buffers that do not have their own values
for this variable.
*/
       (sym))
{
  Lisp_Object value;

  value = default_value (sym);
  return (UNBOUNDP (value) ? Qnil : Qt);
}

DEFUN ("default-value", Fdefault_value, 1, 1, 0, /*
Return SYMBOL's default value.
This is the value that is seen in buffers that do not have their own values
for this variable.  The default value is meaningful for variables with
local bindings in certain buffers.
*/
       (sym))
{
  Lisp_Object value;

  value = default_value (sym);
  if (UNBOUNDP (value))
    return Fsignal (Qvoid_variable, list1 (sym));
  return value;
}

DEFUN ("set-default", Fset_default, 2, 2, 0, /*
Set SYMBOL's default value to VAL.  SYMBOL and VAL are evaluated.
The default value is seen in buffers that do not have their own values
for this variable.
*/
       (sym, value))
{
  Lisp_Object valcontents;

  CHECK_SYMBOL (sym);

 retry:
  valcontents = XSYMBOL (sym)->value;

 retry_2:
  if (!SYMBOL_VALUE_MAGIC_P (valcontents))
    return Fset (sym, value);

  switch (XSYMBOL_VALUE_MAGIC_TYPE (valcontents))
    {
    case SYMVAL_LISP_MAGIC:
      RETURN_IF_NOT_UNBOUND (maybe_call_magic_handler (sym, Qset_default, 1,
						       value));
      valcontents = XSYMBOL_VALUE_LISP_MAGIC (valcontents)->shadowed;
      /* semi-change-o */
      goto retry_2;
	  
    case SYMVAL_VARALIAS:
      sym = follow_varalias_pointers (sym, Qset_default);
      /* presto change-o! */
      goto retry;

    case SYMVAL_CURRENT_BUFFER_FORWARD:
      set_default_buffer_slot_variable (sym, value);
      return (value);

    case SYMVAL_SELECTED_CONSOLE_FORWARD:
      set_default_console_slot_variable (sym, value);
      return (value);

    case SYMVAL_BUFFER_LOCAL:
    case SYMVAL_SOME_BUFFER_LOCAL:
      {
	/* Store new value into the DEFAULT-VALUE slot */
	struct symbol_value_buffer_local *bfwd
	  = XSYMBOL_VALUE_BUFFER_LOCAL (valcontents);

	bfwd->default_value = value;
	/* If current-buffer doesn't shadow default_value,
	 *  we must set the CURRENT-VALUE slot too */
	if (NILP (bfwd->current_alist_element))
	  store_symval_forwarding (sym, bfwd->current_value, value);
	return (value);
      }

    default:
      return Fset (sym, value);
    }
  RETURN_NOT_REACHED(Qnil)	/* suppress compiler warning */
}

DEFUN ("setq-default", Fsetq_default, 2, UNEVALLED, 0, /*
Set the default value of variable VAR to VALUE.
VAR, the variable name, is literal (not evaluated);
VALUE is an expression and it is evaluated.
The default value of a variable is seen in buffers
that do not have their own values for the variable.

More generally, you can use multiple variables and values, as in
  (setq-default SYM VALUE SYM VALUE...)
This sets each SYM's default value to the corresponding VALUE.
The VALUE for the Nth SYM can refer to the new default values
of previous SYMs.
*/
       (args))
{
  /* This function can GC */
  Lisp_Object args_left;
  Lisp_Object val, sym;
  struct gcpro gcpro1;

  if (NILP (args))
    return Qnil;

  args_left = args;
  GCPRO1 (args);

  do
    {
      val = Feval (Fcar (Fcdr (args_left)));
      sym = Fcar (args_left);
      Fset_default (sym, val);
      args_left = Fcdr (Fcdr (args_left));
    }
  while (!NILP (args_left));

  UNGCPRO;
  return val;
}

/* Lisp functions for creating and removing buffer-local variables.  */

DEFUN ("make-variable-buffer-local", Fmake_variable_buffer_local, 1, 1,
       "vMake Variable Buffer Local: ", /*
Make VARIABLE have a separate value for each buffer.
At any time, the value for the current buffer is in effect.
There is also a default value which is seen in any buffer which has not yet
set its own value.
Using `set' or `setq' to set the variable causes it to have a separate value
for the current buffer if it was previously using the default value.
The function `default-value' gets the default value and `set-default'
sets it.
*/
       (variable))
{
  Lisp_Object valcontents;

  CHECK_SYMBOL (variable);

 retry:
  verify_ok_for_buffer_local (variable, Qmake_variable_buffer_local);

  valcontents = XSYMBOL (variable)->value;

 retry_2:
  if (SYMBOL_VALUE_MAGIC_P (valcontents))
    {
      switch (XSYMBOL_VALUE_MAGIC_TYPE (valcontents))
	{
	case SYMVAL_LISP_MAGIC:
	  if (!UNBOUNDP (maybe_call_magic_handler
			 (variable, Qmake_variable_buffer_local, 0)))
	    return variable;
	  valcontents = XSYMBOL_VALUE_LISP_MAGIC (valcontents)->shadowed;
	  /* semi-change-o */
	  goto retry_2;

	case SYMVAL_VARALIAS:
	  variable = follow_varalias_pointers (variable,
					       Qmake_variable_buffer_local);
	  /* presto change-o! */
	  goto retry;

	case SYMVAL_FIXNUM_FORWARD:
	case SYMVAL_BOOLEAN_FORWARD:
	case SYMVAL_OBJECT_FORWARD:
	case SYMVAL_UNBOUND_MARKER:
	  break;

	case SYMVAL_CURRENT_BUFFER_FORWARD:
	case SYMVAL_BUFFER_LOCAL:
	  /* Already per-each-buffer */
	  return (variable);

	case SYMVAL_SOME_BUFFER_LOCAL:
	  /* Transmogrify */
	  XSYMBOL_VALUE_BUFFER_LOCAL (valcontents)->magic.type =
	    SYMVAL_BUFFER_LOCAL;
	  return (variable);

	default:
	  abort ();
	}
    }
  
  {
    struct symbol_value_buffer_local *bfwd
      = alloc_lcrecord (sizeof (struct symbol_value_buffer_local),
			lrecord_symbol_value_buffer_local);
    Lisp_Object foo = Qnil;
    bfwd->magic.type = SYMVAL_BUFFER_LOCAL;

    bfwd->default_value = find_symbol_value (variable);
    bfwd->current_value = valcontents;
    bfwd->current_alist_element = Qnil;
    bfwd->current_buffer = Fcurrent_buffer ();
    XSETSYMBOL_VALUE_MAGIC (foo, bfwd);
    *value_slot_past_magic (variable) = foo;
#if 1				/* #### Yuck!   FSFmacs bug-compatibility*/
    /* This sets the default-value of any make-variable-buffer-local to nil.
       That just sucks.	 User can just use setq-default to effect that,
       but there's no way to do makunbound-default to undo this lossage. */
    if (UNBOUNDP (valcontents))
      bfwd->default_value = Qnil;
#endif
#if 0				/* #### Yuck! */
    /* This sets the value to nil in this buffer.
       User could use (setq variable nil) to do this.
       It isn't as egregious to do this automatically
       as it is to do so to the default-value, but it's
       still really dubious. */
    if (UNBOUNDP (valcontents))
      Fset (variable, Qnil);
#endif
    return (variable);
  }
}

DEFUN ("make-local-variable", Fmake_local_variable, 1, 1, "vMake Local Variable: ", /*
Make VARIABLE have a separate value in the current buffer.
Other buffers will continue to share a common default value.
\(The buffer-local value of VARIABLE starts out as the same value
VARIABLE previously had.  If VARIABLE was void, it remains void.)
See also `make-variable-buffer-local'.

If the variable is already arranged to become local when set,
this function causes a local value to exist for this buffer,
just as setting the variable would do.

Do not use `make-local-variable' to make a hook variable buffer-local.
Use `make-local-hook' instead.
*/
       (variable))
{
  Lisp_Object valcontents;
  struct symbol_value_buffer_local *bfwd;

  CHECK_SYMBOL (variable);

 retry:
  verify_ok_for_buffer_local (variable, Qmake_local_variable);

  valcontents = XSYMBOL (variable)->value;

 retry_2:
  if (SYMBOL_VALUE_MAGIC_P (valcontents))
    {
      switch (XSYMBOL_VALUE_MAGIC_TYPE (valcontents))
	{
	case SYMVAL_LISP_MAGIC:
	  if (!UNBOUNDP (maybe_call_magic_handler
			 (variable, Qmake_local_variable, 0)))
	    return variable;
	  valcontents = XSYMBOL_VALUE_LISP_MAGIC (valcontents)->shadowed;
	  /* semi-change-o */
	  goto retry_2;

	case SYMVAL_VARALIAS:
	  variable = follow_varalias_pointers (variable, Qmake_local_variable);
	  /* presto change-o! */
	  goto retry;

	case SYMVAL_FIXNUM_FORWARD:
	case SYMVAL_BOOLEAN_FORWARD:
	case SYMVAL_OBJECT_FORWARD:
	case SYMVAL_UNBOUND_MARKER:
	  break;

	case SYMVAL_BUFFER_LOCAL:
	case SYMVAL_CURRENT_BUFFER_FORWARD:
	  {
	    /* Make sure the symbol has a local value in this particular
	       buffer, by setting it to the same value it already has.	*/
	    Fset (variable, find_symbol_value (variable));
	    return (variable);
	  }

	case SYMVAL_SOME_BUFFER_LOCAL:
	  {
	    if (!NILP (buffer_local_alist_element (current_buffer,
						   variable,
						   (XSYMBOL_VALUE_BUFFER_LOCAL
						    (valcontents)))))
	      goto already_local_to_current_buffer;
	    else
	      goto already_local_to_some_other_buffer;
	  }

	default:
	  abort ();
	}
    }

  /* Make sure variable is set up to hold per-buffer values */
  bfwd = alloc_lcrecord (sizeof (struct symbol_value_buffer_local),
			 lrecord_symbol_value_buffer_local);
  bfwd->magic.type = SYMVAL_SOME_BUFFER_LOCAL;

  bfwd->current_buffer = Qnil;
  bfwd->current_alist_element = Qnil;
  bfwd->current_value = valcontents;
  /* passing 0 is OK because this should never be a
     SYMVAL_CURRENT_BUFFER_FORWARD or SYMVAL_SELECTED_CONSOLE_FORWARD
     variable. */
  bfwd->default_value = do_symval_forwarding (valcontents, 0, 0);

#if 0
  if (UNBOUNDP (bfwd->default_value))
    bfwd->default_value = Qnil; /* Yuck! */
#endif

  XSETSYMBOL_VALUE_MAGIC (valcontents, bfwd);
  *value_slot_past_magic (variable) = valcontents;

 already_local_to_some_other_buffer:

  /* Make sure this buffer has its own value of variable */
  bfwd = XSYMBOL_VALUE_BUFFER_LOCAL (valcontents);

  if (UNBOUNDP (bfwd->default_value))
    {
      /* If default value is unbound, set local value to nil. */
      XSETBUFFER (bfwd->current_buffer, current_buffer);
      bfwd->current_alist_element = Fcons (variable, Qnil);
      current_buffer->local_var_alist =
	Fcons (bfwd->current_alist_element, current_buffer->local_var_alist);
      store_symval_forwarding (variable, bfwd->current_value, Qnil);
      return (variable);
    }

  current_buffer->local_var_alist
    = Fcons (Fcons (variable, bfwd->default_value),
	     current_buffer->local_var_alist);

  /* Make sure symbol does not think it is set up for this buffer;
     force it to look once again for this buffer's value */
  if (!NILP (bfwd->current_buffer) &&
      current_buffer == XBUFFER (bfwd->current_buffer))
    bfwd->current_buffer = Qnil;

 already_local_to_current_buffer:

  /* If the symbol forwards into a C variable, then swap in the
     variable for this buffer immediately.  If C code modifies the
     variable before we swap in, then that new value will clobber the
     default value the next time we swap.  */
  bfwd = XSYMBOL_VALUE_BUFFER_LOCAL (valcontents);
  if (SYMBOL_VALUE_MAGIC_P (bfwd->current_value))
    {
      switch (XSYMBOL_VALUE_MAGIC_TYPE (bfwd->current_value))
	{
	case SYMVAL_FIXNUM_FORWARD:
	case SYMVAL_BOOLEAN_FORWARD:
	case SYMVAL_OBJECT_FORWARD:
	case SYMVAL_DEFAULT_BUFFER_FORWARD:
	  set_up_buffer_local_cache (variable, bfwd, current_buffer, Qnil, 1);
	  break;

	case SYMVAL_UNBOUND_MARKER:
	case SYMVAL_CURRENT_BUFFER_FORWARD:
	  break;

	default:
	  abort ();
	}
    }

  return (variable);
}

DEFUN ("kill-local-variable", Fkill_local_variable, 1, 1, "vKill Local Variable: ", /*
Make VARIABLE no longer have a separate value in the current buffer.
From now on the default value will apply in this buffer.
*/
       (variable))
{
  Lisp_Object valcontents;

  CHECK_SYMBOL (variable);

 retry:
  valcontents = XSYMBOL (variable)->value;

 retry_2:
  if (!SYMBOL_VALUE_MAGIC_P (valcontents))
    return (variable);

  switch (XSYMBOL_VALUE_MAGIC_TYPE (valcontents))
    {
    case SYMVAL_LISP_MAGIC:
      if (!UNBOUNDP (maybe_call_magic_handler
		     (variable, Qkill_local_variable, 0)))
	return variable;
      valcontents = XSYMBOL_VALUE_LISP_MAGIC (valcontents)->shadowed;
      /* semi-change-o */
      goto retry_2;

    case SYMVAL_VARALIAS:
      variable = follow_varalias_pointers (variable, Qkill_local_variable);
      /* presto change-o! */
      goto retry;

    case SYMVAL_CURRENT_BUFFER_FORWARD:
      {
	CONST struct symbol_value_forward *fwd
	  = XSYMBOL_VALUE_FORWARD (valcontents);
	int offset = ((char *) symbol_value_forward_forward (fwd) 
			       - (char *) &buffer_local_flags);
	int mask =
	  XINT (*((Lisp_Object *) symbol_value_forward_forward (fwd)));

	if (mask > 0)
	  {
	    int (*magicfun) (Lisp_Object sym, Lisp_Object *val,
			     Lisp_Object in_object, int flags) =
			       symbol_value_forward_magicfun (fwd);
	    Lisp_Object oldval = * (Lisp_Object *)
	      (offset + (char *) XBUFFER (Vbuffer_defaults));
	    if (magicfun)
	      (magicfun) (variable, &oldval, make_buffer (current_buffer), 0);
	    *(Lisp_Object *) (offset + (char *) current_buffer)
	      = oldval;
	    current_buffer->local_var_flags &= ~mask;
	  }
	return (variable);
      }

    case SYMVAL_BUFFER_LOCAL:
    case SYMVAL_SOME_BUFFER_LOCAL:
      {
	/* Get rid of this buffer's alist element, if any */
	struct symbol_value_buffer_local *bfwd
	  = XSYMBOL_VALUE_BUFFER_LOCAL (valcontents);
	Lisp_Object alist = current_buffer->local_var_alist;
	Lisp_Object alist_element
	  = buffer_local_alist_element (current_buffer, variable, bfwd);

	if (!NILP (alist_element))
	  current_buffer->local_var_alist = Fdelq (alist_element, alist);

	/* Make sure symbol does not think it is set up for this buffer;
	   force it to look once again for this buffer's value */
	if (!NILP (bfwd->current_buffer) &&
	    current_buffer == XBUFFER (bfwd->current_buffer))
	  bfwd->current_buffer = Qnil;

	/* We just changed the value in the current_buffer.  If this
	   variable forwards to a C variable, we need to change the
	   value of the C variable.  set_up_buffer_local_cache()
	   will do this.  It doesn't hurt to do it always,
	   so just go ahead and do that. */
	set_up_buffer_local_cache (variable, bfwd, current_buffer, Qnil, 1);
      }
      return (variable);

    default:
      return (variable);
    }
  RETURN_NOT_REACHED(Qnil)	/* suppress compiler warning */
}


DEFUN ("kill-console-local-variable", Fkill_console_local_variable, 1, 1, "vKill Console Local Variable: ", /*
Make VARIABLE no longer have a separate value in the selected console.
From now on the default value will apply in this console.
*/
       (variable))
{
  Lisp_Object valcontents;

  CHECK_SYMBOL (variable);

 retry:
  valcontents = XSYMBOL (variable)->value;

 retry_2:
  if (!SYMBOL_VALUE_MAGIC_P (valcontents))
    return (variable);

  switch (XSYMBOL_VALUE_MAGIC_TYPE (valcontents))
    {
    case SYMVAL_LISP_MAGIC:
      if (!UNBOUNDP (maybe_call_magic_handler
		     (variable, Qkill_console_local_variable, 0)))
	return variable;
      valcontents = XSYMBOL_VALUE_LISP_MAGIC (valcontents)->shadowed;
      /* semi-change-o */
      goto retry_2;

    case SYMVAL_VARALIAS:
      variable = follow_varalias_pointers (variable,
					   Qkill_console_local_variable);
      /* presto change-o! */
      goto retry;

    case SYMVAL_SELECTED_CONSOLE_FORWARD:
      {
	CONST struct symbol_value_forward *fwd
	  = XSYMBOL_VALUE_FORWARD (valcontents);
	int offset = ((char *) symbol_value_forward_forward (fwd) 
			       - (char *) &console_local_flags);
	int mask =
	  XINT (*((Lisp_Object *) symbol_value_forward_forward (fwd)));

	if (mask > 0)
	  {
	    int (*magicfun) (Lisp_Object sym, Lisp_Object *val,
			     Lisp_Object in_object, int flags) =
			       symbol_value_forward_magicfun (fwd);
	    Lisp_Object oldval = * (Lisp_Object *)
	      (offset + (char *) XCONSOLE (Vconsole_defaults));
	    if (magicfun)
	      (magicfun) (variable, &oldval, Vselected_console, 0);
	    *(Lisp_Object *) (offset + (char *) XCONSOLE (Vselected_console))
	      = oldval;
	    XCONSOLE (Vselected_console)->local_var_flags &= ~mask;
	  }
	return (variable);
      }

    default:
      return (variable);
    }
  RETURN_NOT_REACHED(Qnil)	/* suppress compiler warning */
}

/* Used by specbind to determine what effects it might have.  Returns:
 *   0 if symbol isn't buffer-local, and wouldn't be after it is set
 *  <0 if symbol isn't presently buffer-local, but set would make it so
 *  >0 if symbol is presently buffer-local
 */
int
symbol_value_buffer_local_info (Lisp_Object symbol, struct buffer *buffer)
{
  Lisp_Object valcontents;

 retry:
  valcontents = XSYMBOL (symbol)->value;

 retry_2:
  if (SYMBOL_VALUE_MAGIC_P (valcontents))
    {
      switch (XSYMBOL_VALUE_MAGIC_TYPE (valcontents))
	{
	case SYMVAL_LISP_MAGIC:
	  /* #### kludge */
	  valcontents = XSYMBOL_VALUE_LISP_MAGIC (valcontents)->shadowed;
	  /* semi-change-o */
	  goto retry_2;

	case SYMVAL_VARALIAS:
	  symbol = follow_varalias_pointers (symbol, Qt /* #### kludge */);
	  /* presto change-o! */
	  goto retry;

	case SYMVAL_CURRENT_BUFFER_FORWARD:
	  {
	    CONST struct symbol_value_forward *fwd
	      = XSYMBOL_VALUE_FORWARD (valcontents);
	    int mask = XINT (*((Lisp_Object *)
			       symbol_value_forward_forward (fwd)));
	    if ((mask <= 0) || (buffer && (buffer->local_var_flags & mask)))
	      /* Already buffer-local */
	      return (1);
	    else
	      /* Would be buffer-local after set */
	      return (-1);
	  }
	case SYMVAL_BUFFER_LOCAL:
	case SYMVAL_SOME_BUFFER_LOCAL:
	  {
	    struct symbol_value_buffer_local *bfwd
	      = XSYMBOL_VALUE_BUFFER_LOCAL (valcontents);
	    if (buffer
		&& !NILP (buffer_local_alist_element (buffer, symbol, bfwd)))
	      return (1);
	    else
	      return ((bfwd->magic.type == SYMVAL_BUFFER_LOCAL)
		      ? -1	 /* Automatically becomes local when set */
		      : 0);
	  }
	default:
	  return (0);
	}
    }
  return (0);
}


DEFUN ("symbol-value-in-buffer", Fsymbol_value_in_buffer, 2, 3, 0, /*
Return the value of SYMBOL in BUFFER, or UNBOUND-VALUE if it is unbound.
*/
       (symbol, buffer, unbound_value))
{
  Lisp_Object value;
  CHECK_SYMBOL (symbol);
  CHECK_BUFFER (buffer);
  value = symbol_value_in_buffer (symbol, buffer);
  if (UNBOUNDP (value))
    return (unbound_value);
  else
    return (value);
}

DEFUN ("symbol-value-in-console", Fsymbol_value_in_console, 2, 3, 0, /*
Return the value of SYMBOL in CONSOLE, or UNBOUND-VALUE if it is unbound.
*/
       (symbol, console, unbound_value))
{
  Lisp_Object value;
  CHECK_SYMBOL (symbol);
  CHECK_CONSOLE (console);
  value = symbol_value_in_console (symbol, console);
  if (UNBOUNDP (value))
    return (unbound_value);
  else
    return (value);
}

DEFUN ("built-in-variable-type", Fbuilt_in_variable_type, 1, 1, 0, /*
If SYM is a built-in variable, return info about this; else return nil.
The returned info will be a symbol, one of

`object'		A simple built-in variable.
`const-object'		Same, but cannot be set.
`integer'		A built-in integer variable.
`const-integer'		Same, but cannot be set.
`boolean'		A built-in boolean variable.
`const-boolean'		Same, but cannot be set.
`const-specifier'	Always contains a specifier; e.g. `has-modeline-p'.
`current-buffer'	A built-in buffer-local variable.
`const-current-buffer'	Same, but cannot be set.
`default-buffer'	Forwards to the default value of a built-in
			buffer-local variable.
`selected-console'	A built-in console-local variable.
`const-selected-console' Same, but cannot be set.
`default-console'	Forwards to the default value of a built-in
			console-local variable.
*/
       (sym))
{
  REGISTER Lisp_Object valcontents;

  CHECK_SYMBOL (sym);

 retry:
  valcontents = XSYMBOL (sym)->value;
 retry_2:

  if (SYMBOL_VALUE_MAGIC_P (valcontents))
    {
      switch (XSYMBOL_VALUE_MAGIC_TYPE (valcontents))
	{
	case SYMVAL_LISP_MAGIC:
	  valcontents = XSYMBOL_VALUE_LISP_MAGIC (valcontents)->shadowed;
	  /* semi-change-o */
	  goto retry_2;
	  
	case SYMVAL_VARALIAS:
	  sym = follow_varalias_pointers (sym, Qt);
	  /* presto change-o! */
	  goto retry;

	case SYMVAL_BUFFER_LOCAL:
	case SYMVAL_SOME_BUFFER_LOCAL:
	  valcontents =
	    XSYMBOL_VALUE_BUFFER_LOCAL (valcontents)->current_value;
	  /* semi-change-o */
	  goto retry_2;

	case SYMVAL_FIXNUM_FORWARD:
	  return Qinteger;

	case SYMVAL_CONST_FIXNUM_FORWARD:
	  return Qconst_integer;

	case SYMVAL_BOOLEAN_FORWARD:
	  return Qboolean;

	case SYMVAL_CONST_BOOLEAN_FORWARD:
	  return Qconst_boolean;

	case SYMVAL_OBJECT_FORWARD:
	  return Qobject;

	case SYMVAL_CONST_OBJECT_FORWARD:
	  return Qconst_object;

	case SYMVAL_CONST_SPECIFIER_FORWARD:
	  return Qconst_specifier;

	case SYMVAL_DEFAULT_BUFFER_FORWARD:
	  return Qdefault_buffer;

	case SYMVAL_CURRENT_BUFFER_FORWARD:
	  return Qcurrent_buffer;

	case SYMVAL_CONST_CURRENT_BUFFER_FORWARD:
	  return Qconst_current_buffer;

	case SYMVAL_DEFAULT_CONSOLE_FORWARD:
	  return Qdefault_console;

	case SYMVAL_SELECTED_CONSOLE_FORWARD:
	  return Qselected_console;

	case SYMVAL_CONST_SELECTED_CONSOLE_FORWARD:
	  return Qconst_selected_console;

	case SYMVAL_UNBOUND_MARKER:
	  return Qnil;

	default:
	  abort ();
	}
    }

  return Qnil;
}


DEFUN ("local-variable-p", Flocal_variable_p, 2, 3, 0, /*
Return t if SYMBOL's value is local to BUFFER.
If optional third arg AFTER-SET is true, return t if SYMBOL would be
buffer-local after it is set, regardless of whether it is so presently.
A nil value for BUFFER is *not* the same as (current-buffer), but means
"no buffer".  Specifically:

-- If BUFFER is nil and AFTER-SET is nil, a return value of t indicates that
   the variable is one of the special built-in variables that is always
   buffer-local. (This includes `buffer-file-name', `buffer-read-only',
   `buffer-undo-list', and others.)

-- If BUFFER is nil and AFTER-SET is t, a return value of t indicates that
   the variable has had `make-variable-buffer-local' applied to it.
*/
       (symbol, buffer, after_set))
{
  int local_info;
  
  CHECK_SYMBOL (symbol);
  if (!NILP (buffer))
    {
      buffer = get_buffer (buffer, 1);
      local_info = symbol_value_buffer_local_info (symbol, XBUFFER (buffer));
    }
  else
    {
      local_info = symbol_value_buffer_local_info (symbol, 0);
    }
  
  if (NILP (after_set))
    return ((local_info > 0) ? Qt : Qnil);
  else
    return ((local_info != 0) ? Qt : Qnil);
}


/* 
I've gone ahead and partially implemented this because it's
super-useful for dealing with the compatibility problems in supporting
the old pointer-shape variables, and preventing people from `setq'ing
the new variables.  Any other way of handling this problem is way
ugly, likely to be slow, and generally not something I want to waste
my time worrying about.

The interface and/or function name is sure to change before this
gets into its final form.  I currently like the way everything is
set up and it has all the features I want it to have, except for
one: I really want to be able to have multiple nested handlers,
to implement an `advice'-like capabiility.  This would allow,
for example, a clean way of implementing `debug-if-set' or
`debug-if-referenced' and such.

NOTE NOTE NOTE NOTE NOTE NOTE NOTE:
************************************************************
**Only** the `set-value', `make-unbound', and `make-local'
handler types are currently implemented.  Implementing the
get-value and bound-predicate handlers is somewhat tricky
because there are lots of subfunctions (e.g. find_symbol_value()).
find_symbol_value(), in fact, is called from outside of
this module.  You'd have to have it do this:

-- check for a `bound-predicate' handler, call that if so;
   if it returns nil, return Qunbound
-- check for a `get-value' handler and call it and return
   that value

It gets even trickier when you have to deal with
sub-subfunctions like find_symbol_value_1(), and esp.
when you have to properly handle variable aliases, which
can lead to lots of tricky situations.  So I've just
punted on this, since the interface isn't officially
exported and we can get by with just a `set-value'
handler.

Actions in unimplemented handler types will correctly
ignore any handlers, and will not fuck anything up or
go awry.

WARNING WARNING: If you do go and implement another
type of handler, make *sure* to change
would_be_magic_handled() so it knows about this,
or dire things could result.
************************************************************
NOTE NOTE NOTE NOTE NOTE NOTE NOTE

Real documentation is as follows.  

Set a magic handler for VARIABLE.
This allows you to specify arbitrary behavior that results from
accessing or setting a variable.  For example, retrieving the
variable's value might actually retrieve the first element off of
a list stored in another variable, and setting the variable's value
might add an element to the front of that list. (This is how the
obsolete variable `unread-command-event' is implemented.)

In general it is NOT good programming practice to use magic variables
in a new package that you are designing.  If you feel the need to
do this, it's almost certainly a sign that you should be using a
function instead of a variable.  This facility is provided to allow
a package to support obsolete variables and provide compatibility
with similar packages with different variable names and semantics.
By using magic handlers, you can cleanly provide obsoleteness and
compatibility support and separate this support from the core
routines in a package.

VARIABLE should be a symbol naming the variable for which the
magic behavior is provided.  HANDLER-TYPE is a symbol specifying
which behavior is being controlled, and HANDLER is the function
that will be called to control this behavior.  HARG is a
value that will be passed to HANDLER but is otherwise
uninterpreted.  KEEP-EXISTING specifies what to do with existing
handlers of the same type; nil means \"erase them all\", t means
\"keep them but insert at the beginning\", the list (t) means
\"keep them but insert at the end\", a function means \"keep
them but insert before the specified function\", a list containing
a function means \"keep them but insert after the specified
function\".

You can specify magic behavior for any type of variable at all,
and for any handler types that are unspecified, the standard
behavior applies.  This allows you, for example, to use
`defvaralias' in conjunction with this function. (For that
matter, `defvaralias' could be implemented using this function.)

The behaviors that can be specified in HANDLER-TYPE are

get-value		(SYM ARGS FUN HARG HANDLERS)
    This means that one of the functions `symbol-value',
    `default-value', `symbol-value-in-buffer', or
    `symbol-value-in-console' was called on SYM.

set-value		(SYM ARGS FUN HARG HANDLERS)
    This means that one of the functions `set' or `set-default'
    was called on SYM.

bound-predicate		(SYM ARGS FUN HARG HANDLERS)
    This means that one of the functions `boundp', `globally-boundp',
    or `default-boundp' was called on SYM.

make-unbound		(SYM ARGS FUN HARG HANDLERS)
    This means that the function `makunbound' was called on SYM.

local-predicate		(SYM ARGS FUN HARG HANDLERS)
    This means that the function `local-variable-p' was called
    on SYM.

make-local		(SYM ARGS FUN HARG HANDLERS)
    This means that one of the functions `make-local-variable',
    `make-variable-buffer-local', `kill-local-variable',
    or `kill-console-local-variable' was called on SYM.

The meanings of the arguments are as follows:

   SYM is the symbol on which the function was called, and is always
   the first argument to the function.
   
   ARGS are the remaining arguments in the original call (i.e. all
   but the first).  In the case of `set-value' in particular,
   the first element of ARGS is the value to which the variable
   is being set.  In some cases, ARGS is sanitized from what was
   actually given.  For example, whenever `nil' is passed to an
   argument and it means `current-buffer', the current buffer is
   substituted instead.
   
   FUN is a symbol indicating which function is being called.
   For many of the functions, you can determine the corresponding
   function of a different class using
   `symbol-function-corresponding-function'.
   
   HARG is the argument that was given in the call
   to `set-symbol-value-handler' for SYM and HANDLER-TYPE.
   
   HANDLERS is a structure containing the remaining handlers
   for the variable; to call one of them, use
   `chain-to-symbol-value-handler'.

NOTE: You may *not* modify the list in ARGS, and if you want to
keep it around after the handler function exits, you must make
a copy using `copy-sequence'. (Same caveats for HANDLERS also.)
*/

static enum lisp_magic_handler
decode_magic_handler_type (Lisp_Object symbol)
{
  if (EQ (symbol, Qget_value))
    return MAGIC_HANDLER_GET_VALUE;
  if (EQ (symbol, Qset_value))
    return MAGIC_HANDLER_SET_VALUE;
  if (EQ (symbol, Qbound_predicate))
    return MAGIC_HANDLER_BOUND_PREDICATE;
  if (EQ (symbol, Qmake_unbound))
    return MAGIC_HANDLER_MAKE_UNBOUND;
  if (EQ (symbol, Qlocal_predicate))
    return MAGIC_HANDLER_LOCAL_PREDICATE;
  if (EQ (symbol, Qmake_local))
    return MAGIC_HANDLER_MAKE_LOCAL;
  signal_simple_error ("Unrecognized symbol value handler type", symbol);
  abort ();
  return MAGIC_HANDLER_MAX;
}

static enum lisp_magic_handler
handler_type_from_function_symbol (Lisp_Object funsym, int abort_if_not_found)
{
  if (EQ (funsym, Qsymbol_value)
      || EQ (funsym, Qdefault_value)
      || EQ (funsym, Qsymbol_value_in_buffer)
      || EQ (funsym, Qsymbol_value_in_console))
    return MAGIC_HANDLER_GET_VALUE;

  if (EQ (funsym, Qset)
      || EQ (funsym, Qset_default))
    return MAGIC_HANDLER_SET_VALUE;

  if (EQ (funsym, Qboundp)
      || EQ (funsym, Qglobally_boundp)
      || EQ (funsym, Qdefault_boundp))
    return MAGIC_HANDLER_BOUND_PREDICATE;

  if (EQ (funsym, Qmakunbound))
    return MAGIC_HANDLER_MAKE_UNBOUND;

  if (EQ (funsym, Qlocal_variable_p))
    return MAGIC_HANDLER_LOCAL_PREDICATE;

  if (EQ (funsym, Qmake_variable_buffer_local)
      || EQ (funsym, Qmake_local_variable))
    return MAGIC_HANDLER_MAKE_LOCAL;

  if (abort_if_not_found)
    abort ();
  signal_simple_error ("Unrecognized symbol-value function", funsym);
  return MAGIC_HANDLER_MAX;
}

static int
would_be_magic_handled (Lisp_Object sym, Lisp_Object funsym)
{
  /* does not take into account variable aliasing. */
  Lisp_Object valcontents = XSYMBOL (sym)->value;
  enum lisp_magic_handler slot;

  if (!SYMBOL_VALUE_LISP_MAGIC_P (valcontents))
    return 0;
  slot = handler_type_from_function_symbol (funsym, 1);
  if (slot != MAGIC_HANDLER_SET_VALUE && slot != MAGIC_HANDLER_MAKE_UNBOUND
      && slot != MAGIC_HANDLER_MAKE_LOCAL)
    /* #### temporary kludge because we haven't implemented
       lisp-magic variables completely */
    return 0;
  return !NILP (XSYMBOL_VALUE_LISP_MAGIC (valcontents)->handler[slot]);
}

static Lisp_Object
fetch_value_maybe_past_magic (Lisp_Object sym,
			      Lisp_Object follow_past_lisp_magic)
{
  Lisp_Object value = XSYMBOL (sym)->value;
  if (SYMBOL_VALUE_LISP_MAGIC_P (value)
      && (EQ (follow_past_lisp_magic, Qt)
	  || (!NILP (follow_past_lisp_magic)
	      && !would_be_magic_handled (sym, follow_past_lisp_magic))))
    value = XSYMBOL_VALUE_LISP_MAGIC (value)->shadowed;
  return value;
}

static Lisp_Object *
value_slot_past_magic (Lisp_Object sym)
{
  Lisp_Object *store_pointer = &XSYMBOL (sym)->value;

  if (SYMBOL_VALUE_LISP_MAGIC_P (*store_pointer))
    store_pointer = &XSYMBOL_VALUE_LISP_MAGIC (sym)->shadowed;
  return store_pointer;
}

static Lisp_Object
maybe_call_magic_handler (Lisp_Object sym, Lisp_Object funsym, int nargs, ...)
{
  va_list vargs;
  Lisp_Object args[20]; /* should be enough ... */
  int i;
  enum lisp_magic_handler htype;
  Lisp_Object legerdemain;
  struct symbol_value_lisp_magic *bfwd;

  assert (nargs >= 0 && nargs < 20);
  legerdemain = XSYMBOL (sym)->value;
  assert (SYMBOL_VALUE_LISP_MAGIC_P (legerdemain));
  bfwd = XSYMBOL_VALUE_LISP_MAGIC (legerdemain);

  va_start (vargs, nargs);
  for (i = 0; i < nargs; i++)
    args[i] = va_arg (vargs, Lisp_Object);
  va_end (vargs);

  htype = handler_type_from_function_symbol (funsym, 1);
  if (NILP (bfwd->handler[htype]))
    return Qunbound;
  /* #### should be reusing the arglist, not always consing anew.
     Repeated handler invocations should not cause repeated consing.
     Doesn't matter for now, because this is just a quick implementation
     for obsolescence support. */
  return call5 (bfwd->handler[htype], sym, Flist (nargs, args), funsym,
		bfwd->harg[htype], Qnil);
}

DEFUN ("dontusethis-set-symbol-value-handler",
       Fdontusethis_set_symbol_value_handler, 3, 5, 0, /*
Don't you dare use this.
If you do, suffer the wrath of Ben, who is likely to rename
this function (or change the semantics of its arguments) without
pity, thereby invalidating your code.
*/
       (variable, handler_type, handler, harg, keep_existing))
{
  Lisp_Object valcontents;
  struct symbol_value_lisp_magic *bfwd;
  enum lisp_magic_handler htype;
  int i;

  /* #### WARNING, only some handler types are implemented.  See above.
     Actions of other types will ignore a handler if it's there.

     #### Also, `chain-to-symbol-value-handler' and
     `symbol-function-corresponding-function' are not implemented. */
  CHECK_SYMBOL (variable);
  CHECK_SYMBOL (handler_type);
  htype = decode_magic_handler_type (handler_type);
  valcontents = XSYMBOL (variable)->value;
  if (!SYMBOL_VALUE_LISP_MAGIC_P (valcontents))
    {
      bfwd = alloc_lcrecord (sizeof (struct symbol_value_lisp_magic),
			     lrecord_symbol_value_lisp_magic);
      bfwd->magic.type = SYMVAL_LISP_MAGIC;
      for (i = 0; i < MAGIC_HANDLER_MAX; i++)
	{
	  bfwd->handler[i] = Qnil;
	  bfwd->harg[i] = Qnil;
	}
      bfwd->shadowed = valcontents;
      XSETSYMBOL_VALUE_MAGIC (XSYMBOL (variable)->value, bfwd);
    }
  else
    bfwd = XSYMBOL_VALUE_LISP_MAGIC (valcontents);
  bfwd->handler[htype] = handler;
  bfwd->harg[htype] = harg;

  for (i = 0; i < MAGIC_HANDLER_MAX; i++)
    if (!NILP (bfwd->handler[i]))
      break;

  if (i == MAGIC_HANDLER_MAX)
    /* there are no remaining handlers, so remove the structure. */
    XSYMBOL (variable)->value = bfwd->shadowed;

  return Qnil;
}


/* functions for working with variable aliases.  */

/* Follow the chain of variable aliases for OBJECT.  Return the
   resulting symbol, whose value cell is guaranteed not to be a
   symbol-value-varalias.

   Also maybe follow past symbol-value-lisp-magic -> symbol-value-varalias.
   If FUNSYM is t, always follow in such a case.  If FUNSYM is nil,
   never follow; stop right there.  Otherwise FUNSYM should be a
   recognized symbol-value function symbol; this means, follow
   unless there is a special handler for the named function.

   OK, there is at least one reason why it's necessary for
   FOLLOW-PAST-LISP-MAGIC to be specified correctly: So that we
   can always be sure to catch cyclic variable aliasing.  If we never
   follow past Lisp magic, then if the following is done:

   (defvaralias 'a 'b)
   add some magic behavior to a, but not a "get-value" handler
   (defvaralias 'b 'a)

   then an attempt to retrieve a's or b's value would cause infinite
   looping in `symbol-value'.

   We (of course) can't always follow past Lisp magic, because then
   we make any variable that is lisp-magic -> varalias behave as if
   the lisp-magic is not present at all.
 */

static Lisp_Object
follow_varalias_pointers (Lisp_Object object,
			  Lisp_Object follow_past_lisp_magic)
{
  Lisp_Object tortoise = object; 
  Lisp_Object hare = object;

  /* quick out just in case */
  if (!SYMBOL_VALUE_MAGIC_P (XSYMBOL (object)->value))
    return object;

  /* based off of indirect_function() */
  for (;;)
    {
      Lisp_Object value;

      value = fetch_value_maybe_past_magic (hare, follow_past_lisp_magic);
      if (!SYMBOL_VALUE_VARALIAS_P (value))
	break;
      hare = symbol_value_varalias_aliasee (XSYMBOL_VALUE_VARALIAS (value));
      value = fetch_value_maybe_past_magic (hare, follow_past_lisp_magic);
      if (!SYMBOL_VALUE_VARALIAS_P (value))
	break;
      hare = symbol_value_varalias_aliasee (XSYMBOL_VALUE_VARALIAS (value));

      value = fetch_value_maybe_past_magic (tortoise, follow_past_lisp_magic);
      tortoise = symbol_value_varalias_aliasee
	(XSYMBOL_VALUE_VARALIAS (value));

      if (EQ (hare, tortoise))
	return (Fsignal (Qcyclic_variable_indirection, list1 (object)));
    }

  return hare;
}

DEFUN ("defvaralias", Fdefvaralias, 2, 2, 0, /*
Define a variable as an alias for another variable.
Thenceforth, any operations performed on VARIABLE will actually be
performed on ALIAS.  Both VARIABLE and ALIAS should be symbols.
If ALIAS is nil, remove any aliases for VARIABLE.
ALIAS can itself be aliased, and the chain of variable aliases
will be followed appropriately.
If VARIABLE already has a value, this value will be shadowed
until the alias is removed, at which point it will be restored.
Currently VARIABLE cannot be a built-in variable, a variable that
has a buffer-local value in any buffer, or the symbols nil or t.
(ALIAS, however, can be any type of variable.)
*/
       (variable, alias))
{
  struct symbol_value_varalias *bfwd;
  Lisp_Object valcontents;

  CHECK_SYMBOL (variable);
  reject_constant_symbols (variable, Qunbound, 0, Qt);

  valcontents = XSYMBOL (variable)->value;

  if (NILP (alias))
    {
      if (SYMBOL_VALUE_VARALIAS_P (valcontents))
	{
	  XSYMBOL (variable)->value =
	    symbol_value_varalias_shadowed
	      (XSYMBOL_VALUE_VARALIAS (valcontents));
	}
      return Qnil;
    }
      
  CHECK_SYMBOL (alias);
  if (SYMBOL_VALUE_VARALIAS_P (valcontents))
    {
      /* transmogrify */
      XSYMBOL_VALUE_VARALIAS (valcontents)->aliasee = alias;
      return Qnil;
    }

  if (SYMBOL_VALUE_MAGIC_P (valcontents)
      && !UNBOUNDP (valcontents))
    signal_simple_error ("Variable is magic and cannot be aliased", variable);
  reject_constant_symbols (variable, Qunbound, 0, Qt);

  bfwd = alloc_lcrecord (sizeof (struct symbol_value_varalias),
			 lrecord_symbol_value_varalias);
  bfwd->magic.type = SYMVAL_VARALIAS;
  bfwd->aliasee = alias;
  bfwd->shadowed = valcontents;
  
  XSETSYMBOL_VALUE_MAGIC (valcontents, bfwd);
  XSYMBOL (variable)->value = valcontents;
  return Qnil;
}

DEFUN ("variable-alias", Fvariable_alias, 1, 2, 0, /*
If VARIABLE is aliased to another variable, return that variable.
VARIABLE should be a symbol.  If VARIABLE is not aliased, return nil.
Variable aliases are created with `defvaralias'.  See also
`indirect-variable'.
*/
       (variable, follow_past_lisp_magic))
{
  Lisp_Object valcontents;

  CHECK_SYMBOL (variable);
  if (!NILP (follow_past_lisp_magic) && !EQ (follow_past_lisp_magic, Qt))
    {
      CHECK_SYMBOL (follow_past_lisp_magic);
      (void) handler_type_from_function_symbol (follow_past_lisp_magic, 0);
    }

  valcontents = fetch_value_maybe_past_magic (variable,
					      follow_past_lisp_magic);

  if (SYMBOL_VALUE_VARALIAS_P (valcontents))
    return symbol_value_varalias_aliasee
      (XSYMBOL_VALUE_VARALIAS (valcontents));
  else
    return Qnil;
}

DEFUN ("indirect-variable", Findirect_variable, 1, 2, 0, /*
Return the variable at the end of OBJECT's variable-alias chain.
If OBJECT is a symbol, follow all variable aliases and return
the final (non-aliased) symbol.  Variable aliases are created with
the function `defvaralias'.
If OBJECT is not a symbol, just return it.
Signal a cyclic-variable-indirection error if there is a loop in the
variable chain of symbols.
*/
       (object, follow_past_lisp_magic))
{
  if (!SYMBOLP (object))
    return object;
  if (!NILP (follow_past_lisp_magic) && !EQ (follow_past_lisp_magic, Qt))
    {
      CHECK_SYMBOL (follow_past_lisp_magic);
      (void) handler_type_from_function_symbol (follow_past_lisp_magic, 0);
    }
  return follow_varalias_pointers (object, follow_past_lisp_magic);
}


/************************************************************************/
/*                            initialization                            */
/************************************************************************/

/* A dumped XEmacs image has a lot more than 1511 symbols.  Last
   estimate was that there were actually around 6300.  So let's try
   making this bigger and see if we get better hashing behavior. */
#define OBARRAY_SIZE 16411

#ifndef Qzero
Lisp_Object Qzero;
#endif

/* some losing systems can't have static vars at function scope... */
static struct symbol_value_magic guts_of_unbound_marker =
  { { { lrecord_symbol_value_forward }, 0, 69}, SYMVAL_UNBOUND_MARKER };

void
init_symbols_once_early (void)
{
  Qnil = Fmake_symbol (make_pure_pname ((CONST Bufbyte *) "nil", 3, 1));
  /* Bootstrapping problem: Qnil isn't set when make_pure_pname is
     called the first time. */
  XSYMBOL (Qnil)->name->plist = Qnil;
  XSYMBOL (Qnil)->value = Qnil; /* Nihil ex nihil */
  XSYMBOL (Qnil)->plist = Qnil;
  
#ifndef Qzero
  Qzero = make_int (0);	/* Only used if Lisp_Object is a union type */
#endif
  
  Vobarray = make_vector (OBARRAY_SIZE, Qzero);
  initial_obarray = Vobarray;
  staticpro (&initial_obarray);
  /* Intern nil in the obarray */
  {
    /* These locals are to kludge around a pyramid compiler bug. */
    int hash;
    Lisp_Object *tem;
    
    hash = hash_string (string_data (XSYMBOL (Qnil)->name), 3);
    /* Separate statement here to avoid VAXC bug. */
    hash %= OBARRAY_SIZE;
    tem = &vector_data (XVECTOR (Vobarray))[hash];
    *tem = Qnil;
  }
  
  {
    /* Required to get around a GCC syntax error on certain
       architectures */
    struct symbol_value_magic *tem = &guts_of_unbound_marker;
    
    XSETSYMBOL_VALUE_MAGIC (Qunbound, tem);
  }
  if ((CONST void *) XPNTR (Qunbound) !=
      (CONST void *)&guts_of_unbound_marker)
    {
      /* This might happen on DATA_SEG_BITS machines. */
      /* abort (); */
      /* Can't represent a pointer to constant C data using a Lisp_Object.
	 So heap-allocate it. */
      struct symbol_value_magic *urk = xmalloc (sizeof (*urk));
      memcpy (urk, &guts_of_unbound_marker, sizeof (*urk));
      XSETSYMBOL_VALUE_MAGIC (Qunbound, urk);
    }
  
  XSYMBOL (Qnil)->function = Qunbound;
  
  defsymbol (&Qt, "t");
  XSYMBOL (Qt)->value = Qt;	/* Veritas aetera */
  Vquit_flag = Qnil;
}

void
defsymbol (Lisp_Object *location, CONST char *name)
{
  *location = Fintern (make_pure_pname ((CONST Bufbyte *) name,
                                        strlen (name), 1),
		       Qnil);
  staticpro (location);
}

void
defkeyword (Lisp_Object *location, CONST char *name)
{
  defsymbol (location, name);
  Fset (*location, *location);
}

void
defsubr (struct Lisp_Subr *subr)
{
  Lisp_Object sym = intern (subr_name (subr));

#ifdef DEBUG_XEMACS
  /* Check that nobody spazzed writing a DEFUN. */
  assert (subr->min_args >= 0);
  assert (subr->min_args <= SUBR_MAX_ARGS);

  if (subr->max_args != MANY && subr->max_args != UNEVALLED)
    {
      /* Need to fix lisp.h and eval.c if SUBR_MAX_ARGS too small */
      assert (subr->max_args <= SUBR_MAX_ARGS);
      assert (subr->min_args <= subr->max_args);
    }
  
  assert (UNBOUNDP (XSYMBOL (sym)->function));
#endif /* DEBUG_XEMACS */

  XSETSUBR (XSYMBOL (sym)->function, subr);
}

void
deferror (Lisp_Object *symbol, CONST char *name, CONST char *messuhhj,
	  Lisp_Object inherits_from)
{
  Lisp_Object conds;
  defsymbol (symbol, name);

  assert (SYMBOLP (inherits_from));
  conds = Fget (inherits_from, Qerror_conditions, Qnil);
  pure_put (*symbol, Qerror_conditions, Fcons (*symbol, conds));
  /* NOT build_translated_string ().  This function is called at load time
     and the string needs to get translated at run time.  (This happens
     in the function (display-error) in cmdloop.el.) */
  pure_put (*symbol, Qerror_message, build_string (messuhhj));
}

void
syms_of_symbols (void)
{
  defsymbol (&Qvariable_documentation, "variable-documentation");
  defsymbol (&Qvariable_domain, "variable-domain");	/* I18N3 */
  defsymbol (&Qad_advice_info, "ad-advice-info");
  defsymbol (&Qad_activate, "ad-activate");

  defsymbol (&Qget_value, "get-value");
  defsymbol (&Qset_value, "set-value");
  defsymbol (&Qbound_predicate, "bound-predicate");
  defsymbol (&Qmake_unbound, "make-unbound");
  defsymbol (&Qlocal_predicate, "local-predicate");
  defsymbol (&Qmake_local, "make-local");

  defsymbol (&Qboundp, "boundp");
  defsymbol (&Qfboundp, "fboundp");
  defsymbol (&Qglobally_boundp, "globally-boundp");
  defsymbol (&Qmakunbound, "makunbound");
  defsymbol (&Qsymbol_value, "symbol-value");
  defsymbol (&Qset, "set");
  defsymbol (&Qdefault_boundp, "default-boundp");
  defsymbol (&Qdefault_value, "default-value");
  defsymbol (&Qset_default, "set-default");
  defsymbol (&Qmake_variable_buffer_local, "make-variable-buffer-local");
  defsymbol (&Qmake_local_variable, "make-local-variable");
  defsymbol (&Qkill_local_variable, "kill-local-variable");
  defsymbol (&Qkill_console_local_variable, "kill-console-local-variable");
  defsymbol (&Qsymbol_value_in_buffer, "symbol-value-in-buffer");
  defsymbol (&Qsymbol_value_in_console, "symbol-value-in-console");
  defsymbol (&Qlocal_variable_p, "local-variable-p");

  defsymbol (&Qconst_integer, "const-integer");
  defsymbol (&Qconst_boolean, "const-boolean");
  defsymbol (&Qconst_object, "const-object");
  defsymbol (&Qconst_specifier, "const-specifier");
  defsymbol (&Qdefault_buffer, "default-buffer");
  defsymbol (&Qcurrent_buffer, "current-buffer");
  defsymbol (&Qconst_current_buffer, "const-current-buffer");
  defsymbol (&Qdefault_console, "default-console");
  defsymbol (&Qselected_console, "selected-console");
  defsymbol (&Qconst_selected_console, "const-selected-console");

  DEFSUBR (Fintern);
  DEFSUBR (Fintern_soft);
  DEFSUBR (Funintern);
  DEFSUBR (Fmapatoms);
  DEFSUBR (Fapropos_internal);

  DEFSUBR (Fsymbol_function);
  DEFSUBR (Fsymbol_plist);
  DEFSUBR (Fsymbol_name);
  DEFSUBR (Fmakunbound);
  DEFSUBR (Ffmakunbound);
  DEFSUBR (Fboundp);
  DEFSUBR (Fglobally_boundp);
  DEFSUBR (Ffboundp);
  DEFSUBR (Ffset);
  DEFSUBR (Fdefine_function);
  DEFSUBR (Fsetplist);
  DEFSUBR (Fsymbol_value_in_buffer);
  DEFSUBR (Fsymbol_value_in_console);
  DEFSUBR (Fbuilt_in_variable_type);
  DEFSUBR (Fsymbol_value);
  DEFSUBR (Fset);
  DEFSUBR (Fdefault_boundp);
  DEFSUBR (Fdefault_value);
  DEFSUBR (Fset_default);
  DEFSUBR (Fsetq_default);
  DEFSUBR (Fmake_variable_buffer_local);
  DEFSUBR (Fmake_local_variable);
  DEFSUBR (Fkill_local_variable);
  DEFSUBR (Fkill_console_local_variable);
  DEFSUBR (Flocal_variable_p);
  DEFSUBR (Fdefvaralias);
  DEFSUBR (Fvariable_alias);
  DEFSUBR (Findirect_variable);
  DEFSUBR (Fdontusethis_set_symbol_value_handler);
}

/* Create and initialize a variable whose value is forwarded to C data */
void
defvar_mumble (CONST char *namestring, 
	       CONST void *magic, int sizeof_magic)
{
  Lisp_Object kludge;
  Lisp_Object sym = Fintern (make_pure_pname ((CONST Bufbyte *) namestring,
					      strlen (namestring),
					      1),
			     Qnil);

  /* Check that magic points somewhere we can represent as a Lisp pointer */
  XSETOBJ (kludge, Lisp_Record, magic);
  if (magic != (CONST void *) XPNTR (kludge))
    {
      /* This might happen on DATA_SEG_BITS machines. */
      /* abort (); */
      /* Copy it to somewhere which is representable. */
      void *f = xmalloc (sizeof_magic);
      memcpy (f, magic, sizeof_magic);
      XSETOBJ (XSYMBOL (sym)->value, Lisp_Record, f);
    }
  else
    XSETOBJ (XSYMBOL (sym)->value, Lisp_Record, magic);
}

void
vars_of_symbols (void)
{
  DEFVAR_LISP ("obarray", &Vobarray /*
Symbol table for use by `intern' and `read'.
It is a vector whose length ought to be prime for best results.
The vector's contents don't make sense if examined from Lisp programs;
to find all the symbols in an obarray, use `mapatoms'.
*/ );
  /* obarray has been initialized long before */
}
