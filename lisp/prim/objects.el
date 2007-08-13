;;; objects.el --- Lisp interface to C window-system objects
;; Keywords: faces internal

;; Copyright (C) 1994 Board of Trustees, University of Illinois
;; Copyright (C) 1995 Ben Wing

;; Author: Chuck Thompson <cthomp@cs.uiuc.edu>,
;;         Ben Wing <wing@666.com>

;; This file is part of XEmacs.

;; XEmacs is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.

;; XEmacs is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;; General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with XEmacs; see the file COPYING.  If not, write to the 
;; Free Software Foundation, 59 Temple Place - Suite 330,
;; Boston, MA 02111-1307, USA.

;;; Synched up with: Not in FSF.

(defun ws-object-property-1 (function object domain &optional matchspec)
  (let ((instance (if matchspec
		      (specifier-matching-instance object matchspec domain)
		    (specifier-instance object domain))))
    (and instance (funcall function instance))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; font specifiers

(defun make-font-specifier (spec-list)
  "Create a new `font' specifier object with the given specification list.
SPEC-LIST can be a list of specifications (each of which is a cons of a
locale and a list of instantiators), a single instantiator, or a list
of instantiators.  See `make-specifier' for more information about
specifiers."
  (make-specifier-and-init 'font spec-list))

(defun font-name (font &optional domain charset)
  "Return the name of the FONT in the specified DOMAIN, if any.
FONT should be a font specifier object and DOMAIN is normally a window
and defaults to the selected window if omitted.  This is equivalent
to using `specifier-instance' and applying `font-instance-name' to
the result.  See `make-specifier' for more information about specifiers."
  (ws-object-property-1 'font-instance-name font domain charset))

(defun font-ascent (font &optional domain charset)
  "Return the ascent of the FONT in the specified DOMAIN, if any.
FONT should be a font specifier object and DOMAIN is normally a window
and defaults to the selected window if omitted.  This is equivalent
to using `specifier-instance' and applying `font-instance-ascent' to
the result.  See `make-specifier' for more information about specifiers."
  (ws-object-property-1 'font-instance-ascent font domain charset))

(defun font-descent (font &optional domain charset)
  "Return the descent of the FONT in the specified DOMAIN, if any.
FONT should be a font specifier object and DOMAIN is normally a window
and defaults to the selected window if omitted.  This is equivalent
to using `specifier-instance' and applying `font-instance-descent' to
the result.  See `make-specifier' for more information about specifiers."
  (ws-object-property-1 'font-instance-descent font domain charset))

(defun font-width (font &optional domain charset)
  "Return the width of the FONT in the specified DOMAIN, if any.
FONT should be a font specifier object and DOMAIN is normally a window
and defaults to the selected window if omitted.  This is equivalent
to using `specifier-instance' and applying `font-instance-width' to
the result.  See `make-specifier' for more information about specifiers."
  (ws-object-property-1 'font-instance-width font domain charset))

(defun font-height (font &optional domain charset)
  "Return the height of the FONT in the specified DOMAIN, if any.
FONT should be a font specifier object and DOMAIN is normally a window
and defaults to the selected window if omitted.  This is equivalent
to using `specifier-instance' and applying `font-instance-height' to
the result.  See `make-specifier' for more information about specifiers."
  (ws-object-property-1 'font-instance-height font domain charset))

(defun font-proportional-p (font &optional domain charset)
  "Return whether FONT is proportional in the specified DOMAIN, if known.
FONT should be a font specifier object and DOMAIN is normally a window
and defaults to the selected window if omitted.  This is equivalent
to using `specifier-instance' and applying `font-instance-proportional-p' to
the result.  See `make-specifier' for more information about specifiers."
  (ws-object-property-1 'font-instance-proportional-p font domain charset))

(defun font-properties (font &optional domain charset)
  "Return the properties of the FONT in the specified DOMAIN, if any.
FONT should be a font specifier object and DOMAIN is normally a window
and defaults to the selected window if omitted.  This is equivalent
to using `specifier-instance' and applying `font-instance-properties'
to the result.  See `make-specifier' for more information about specifiers."
  (ws-object-property-1 'font-instance-properties font domain charset))

(defun font-truename (font &optional domain charset)
  "Return the truename of the FONT in the specified DOMAIN, if any.
FONT should be a font specifier object and DOMAIN is normally a window
and defaults to the selected window if omitted.  This is equivalent
to using `specifier-instance' and applying `font-instance-truename'
to the result.  See `make-specifier' for more information about specifiers."
  (ws-object-property-1 'font-instance-truename font domain charset))

(defun font-instance-height (font-instance)
  "Return the height in pixels of FONT-INSTANCE.
The returned value is the maximum height for all characters in the font,\n\
and is equivalent to the sum of the font instance's ascent and descent."
  (+ (font-instance-ascent font-instance)
     (font-instance-descent font-instance)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; color specifiers

(defun make-color-specifier (spec-list)
  "Create a new `color' specifier object with the given specification list.
SPEC-LIST can be a list of specifications (each of which is a cons of a
locale and a list of instantiators), a single instantiator, or a list
of instantiators.  See `make-specifier' for a detailed description of
how specifiers work."
  (make-specifier-and-init 'color spec-list))

(defun color-name (color &optional domain)
  "Return the name of the COLOR in the specified DOMAIN, if any.
COLOR should be a color specifier object and DOMAIN is normally a window
and defaults to the selected window if omitted.  This is equivalent
to using `specifier-instance' and applying `color-instance-name' to
the result.  See `make-specifier' for more information about specifiers."
  (ws-object-property-1 'color-instance-name color domain))

(defun color-rgb-components (color &optional domain)
  "Return the RGB components of the COLOR in the specified DOMAIN, if any.
COLOR should be a color specifier object and DOMAIN is normally a window
and defaults to the selected window if omitted.  This is equivalent
to using `specifier-instance' and applying `color-instance-rgb-components'
to the result.  See `make-specifier' for more information about specifiers."
  (ws-object-property-1 'color-instance-rgb-components color domain))

;;; objects.el ends here.

