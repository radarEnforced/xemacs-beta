/* Give this program DOC-mm.nn.oo as standard input and it outputs to
   standard output a file of nroff output containing the doc strings.

Copyright (C) 1987, 1994, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
  2008, 2009, 2010 Free Software Foundation, Inc.

This file is part of XEmacs.

XEmacs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

XEmacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XEmacs.  If not, see <http://www.gnu.org/licenses/>.


See also sorted-doc.c, which produces similar output
but in texinfo format and sorted by function/variable name.  */

/* Synced up with: GNU 23.1.92. */
/* Synced by: Ben Wing, 2-17-10. */

#ifdef emacs
#include <config.h>
#endif
#include <stdio.h>

#ifdef WIN32_NATIVE
#include <fcntl.h>		/* for O_BINARY */
#include <io.h>			/* for setmode */
#endif

int
main (int argc, char **argv)
{
  register int ch;
  register int notfirst = 0;

#ifdef WIN32_NATIVE
  /* DOC is a binary file.  */
  if (!isatty (fileno (stdin)))
    setmode (fileno (stdin), O_BINARY);
#endif

  printf (".TL\n");
  printf ("Command Summary for XEmacs\n");
  printf (".AU\nThe XEmacs Advocacy Group\n");
  while ((ch = getchar ()) != EOF)
    {
      if (ch == '\037')
	{
	  if (notfirst)
	    printf ("\n.DE");
	  else
	    notfirst = 1;

	  printf ("\n.SH\n");

	  ch = getchar ();
	  printf (ch == 'F' ? "Function " : "Variable ");

	  while ((ch = getchar ()) != '\n')  /* Changed this line */
	    {
	      if (ch != EOF)
		  putchar (ch);
	      else
		{
		  ungetc (ch, stdin);
		  break;
		}
	    }
	  printf ("\n.DS L\n");
	}
      else
	putchar (ch);
    }
  return 0;
}

/* arch-tag: 2ba2c9b0-4157-4eba-bd9f-967e3677e35f
   (do not change this comment) */
