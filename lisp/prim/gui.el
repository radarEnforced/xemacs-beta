;;; gui.el --- Basic GUI functions for XEmacs.
;; Copyright (C) 1996 Ben Wing
 
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

(defvar dialog-frame-plist '(width 60 height 20)
  "Plist of frame properties for initially creating a dialog frame.
Properties specified here supersede the values given in
`default-frame-plist'.")

(defun make-dialog-frame (&optional props parent)
  "Create a frame suitable for use as a dialog box.
The frame is made a child of PARENT (defaults to the selected frame),
and has additional properties PROPS, as well as `dialog-frame-plist'.
Normally it also has no modelines, menubars, or toolbars."
  (or parent (setq parent (selected-frame)))
  (let* ((ftop (frame-property parent 'top))
	 (fleft (frame-property parent 'left))
	 (fwidth (frame-pixel-width parent))
	 (fheight (frame-pixel-height parent))
	 (fonth (font-height (face-font 'default)))
	 (fontw (font-width (face-font 'default)))
	 (props (append props dialog-frame-plist))
	 (dfheight (plist-get props 'height))
	 (dfwidth (plist-get props 'width))
	 ;; under FVWM at least, if I don't specify the initial position,
	 ;; it ends up always at (0, 0).  xwininfo doesn't tell me
	 ;; that there are any program-specified position hints, so
	 ;; it must be an FVWM bug.  So just be smashing and position
	 ;; in the center of the selected frame.
	 (frame (make-frame
		 (append props
			 `(popup ,parent initially-unmapped t
				 menubar-visible-p nil
				 has-modeline-p nil
				 default-toolbar-visible-p nil
				 modeline-shadow-thickness 0
				 left ,(+ fleft (- (/ fwidth 2)
						   (/ (* dfwidth fontw)
						      2)))
				 top ,(+ ftop (- (/ fheight 2)
						 (/ (* dfheight fonth)
						    2))))))))
    (set-face-foreground 'modeline [default foreground] frame)
    (set-face-background 'modeline [default background] frame)
    (make-frame-visible frame)
    frame))

(defvar gui-button-shadow-thickness 2)

(defun gui-button-p (object)
  "True if OBJECT is a GUI button."
  (and (vectorp object)
       (> (length object) 0)
       (eq 'gui-button (aref object 0))))

(make-face 'gui-button-face "Face used for gui buttons")
(if (not (face-differs-from-default-p 'gui-button-face))
    (progn
      (set-face-background 'gui-button-face "grey75")
      (set-face-foreground 'gui-button-face "black")))

(defun make-gui-button (string &optional action user-data)
  "Make a GUI button whose label is STRING and whose action is ACTION.
If the button is inserted in a buffer and then clicked on, and ACTION
is non-nil, ACTION will be called with one argument, USER-DATA."
  (vector 'gui-button
	  (if (featurep 'xpm)
	      (xpm-button-create
	       string gui-button-shadow-thickness
	       (color-instance-name (face-foreground-instance 'gui-button-face))
	       (color-instance-name (face-background-instance 'gui-button-face)))
	    (xbm-button-create string gui-button-shadow-thickness))
	  action user-data))

(defun insert-gui-button (button &optional pos buffer)
  "Insert GUI button BUTTON at POS in BUFFER."
  (check-argument-type 'gui-button-p button)
  (let ((annotation
	 (make-annotation (make-glyph (car (aref button 1)))
			  pos 'text buffer nil
			  (make-glyph (cadr (aref button 1)))))
	(action (aref button 2)))
    (and action
	 (progn
	   (set-annotation-action annotation action)
	   (set-annotation-data annotation (aref button 3))))))
