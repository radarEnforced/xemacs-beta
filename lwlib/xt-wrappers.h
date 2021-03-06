/* Wrappers for Xt functions and macros

   Copyright (C) 2008 Free Software Foundation

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

/* Original author: Stephen J. Turnbull for 21.5.29 */

/* #### Factor out into xaw-wrappers (which would #include this file)? */
#ifdef HAVE_ATHENA_I18N
#define XAW_INTERNATIONALIZATION 1
#endif

/* Generic utility macros, including coping with G++ whining.
   Used in lwlib via lwlib.h and X consoles via console-x.h.

   We would prefer to find another way to shut up G++.  The issue is that
   recent versions of the C++ standard deprecate implicit conversions
   across function boundaries like

   typedef char *String;
   void foo (String string);
   foo ("bar");

   because "bar" should be allowed to be a read-only array of chars.  But of
   course lots of legacy code (== X11) declares things as char * and expects
   to assign literal strings to them.  Now, the typedef in the example is
   important because in G++ 4.3.2 at least, this

   void foo (const String string);
   foo ("bar");

   does not work as expected!  G++ still warns about this construct.  However,
   if foo is declared

   void foo (const char *string);

   G++ does not complain.  (#### There are two possibilities I can think of.
   (a) G++ is buggy.  (b) "const String" is interpreted as "char * const".)

   The upshot is that to avoid warnings with Xt's String typedef, we need to
   arrange to cast literal strings to String, rather than use "const String"
   in declarations.  (My <X11/Intrinsic.h> says that the actual internal
   typedef used is _XtString, so that String can be #define'd to something
   else for the purposes of C++.  But that doesn't really help us much.)

   It's not very satisfactory to do it this way -- it would be much better to
   have const Strings where they make sense -- but it does eliminate a few
   hundred warnings from the C++ build.  And in any case we don't control the
   many objects declared with String components in Intrinsic.h.  The remaining
   issues are the WEXTTEXT macro used in src/emacs.c, and Emacs.ad.h (where
   instead of String we use const char * in src/event-Xt.c in the array that
   #includes it).
*/

#ifndef INCLUDED_xt_wrappers_h_
#define INCLUDED_xt_wrappers_h_

/* Wrap XtResource, with the same elements as arguments.
   The cast to String shuts up G++ 4.3's whining about const char *.
   The invocation of sizeof should be pretty safe, and the cast to XtPointer
   surely is, since that's how that member of XtResource is declared.  It
   doesn't hide potential problems, because XtPointer is a "generic" type in
   any case -- the actual object will have a different type, that will be
   cast to XtPointer. */

#define Xt_RESOURCE(name,_class,intrepr,type,member,extrepr,value)	\
  { (String) name, (String) _class, (String) intrepr, sizeof(type),	\
    member, extrepr, (XtPointer) value }

/* Wrap XtSetArg, with the same arguments.
   The cast to String shuts up G++ 4.3's whining about const char *. */

#define Xt_SET_ARG(al, resource, x) do {	\
    XtSetArg ((al), (String) (resource), (x));	\
  } while (0)

/* Convenience macros for getting/setting one resource value. */

#define Xt_SET_VALUE(widget, resource, value) do {	\
  Arg al__;						\
  Xt_SET_ARG (al__, resource, value);			\
  XtSetValues (widget, &al__, 1);			\
} while (0)

#define Xt_GET_VALUE(widget, resource, location) do {	\
  Arg al__;						\
  Xt_SET_ARG (al__, resource, location);		\
  XtGetValues (widget, &al__, 1);			\
} while (0)

#endif /* INCLUDED_xt_wrappers_h_ */
