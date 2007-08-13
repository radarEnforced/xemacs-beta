/* Generic GUI code. (menubars, scrollbars, toolbars, dialogs)
   Copyright (C) 1995 Board of Trustees, University of Illinois.
   Copyright (C) 1995, 1996 Ben Wing.
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

/* Synched up with: Not in FSF. */

/* Written by kkm on 12/24/97 */

#ifndef _XEMACS_GUI_H_
#define _XEMACS_GUI_H_

int separator_string_p (CONST char *s);
void get_gui_callback (Lisp_Object, Lisp_Object *, Lisp_Object *);

extern int popup_up_p;

/************************************************************************/
/*			Image Instance Object				*/
/************************************************************************/

DECLARE_LRECORD (gui_item, struct Lisp_Gui_Item);
#define XGUI_ITEM(x) \
  XRECORD (x, gui_item, struct Lisp_Gui_Item)
#define XSETGUI_ITEM(x, p) XSETRECORD (x, p, gui_item)
#define GUI_ITEMP(x) RECORDP (x, gui_item)
#define GC_GUI_ITEMP(x) GC_RECORDP (x, gui_item)
#define CHECK_GUI_ITEM(x) CHECK_RECORD (x, gui_item)
#define CONCHECK_GUI_ITEM(x) CONCHECK_RECORD (x, gui_item)

/* This structure describes gui button,
   menu item or submenu properties */
struct Lisp_Gui_Item
{
  struct lcrecord_header header;
  Lisp_Object name;		/* String */
  Lisp_Object callback;		/* Symbol or form */
  Lisp_Object suffix;		/* String */
  Lisp_Object active;		/* Form */
  Lisp_Object included;		/* Form */
  Lisp_Object config;		/* Anything EQable */
  Lisp_Object filter;		/* Form */
  Lisp_Object style;		/* Symbol */
  Lisp_Object selected;		/* Form */
  Lisp_Object keys;		/* String */
  Lisp_Object accelerator;	/* Char or Symbol  */
};

extern Lisp_Object Q_accelerator, Q_active, Q_config, Q_filter, Q_included;
extern Lisp_Object Q_keys, Q_selected, Q_suffix, Qradio, Qtoggle;
extern Lisp_Object Q_key_sequence, Q_label, Q_callback;

void gui_item_add_keyval_pair (Lisp_Object,
			       Lisp_Object key, Lisp_Object val,
			       Error_behavior errb);
Lisp_Object gui_parse_item_keywords (Lisp_Object item);
Lisp_Object gui_parse_item_keywords_no_errors (Lisp_Object item);
int  gui_item_active_p (Lisp_Object);
int  gui_item_selected_p (Lisp_Object);
int  gui_item_included_p (Lisp_Object, Lisp_Object into);
Lisp_Object gui_item_accelerator (Lisp_Object gui_item);
Lisp_Object gui_name_accelerator (Lisp_Object name);
int  gui_item_id_hash (Lisp_Object, Lisp_Object gui_item, int);
unsigned int gui_item_display_flush_left  (Lisp_Object pgui_item,
					   char* buf, Bytecount buf_len);
unsigned int gui_item_display_flush_right (Lisp_Object gui_item,
					   char* buf, Bytecount buf_len);

Lisp_Object allocate_gui_item ();
void gui_item_init (Lisp_Object gui_item);

/* this is mswindows biased but reasonably safe I think */
#define GUI_ITEM_ID_SLOTS 8
#define GUI_ITEM_ID_MIN(s) (s * 0x2000)
#define GUI_ITEM_ID_MAX(s) (0x1FFF + GUI_ITEM_ID_MIN (s))
#define GUI_ITEM_ID_BITS(x,s) (((x) & 0x1FFF) + GUI_ITEM_ID_MIN (s))

#define MAX_MENUITEM_LENGTH 128

#endif /* _XEMACS_GUI_H_ */
