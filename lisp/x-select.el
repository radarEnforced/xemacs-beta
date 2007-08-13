;;; x-select.el --- Lisp interface to X Selections.

;; Copyright (C) 1990, 1997 Free Software Foundation, Inc.
;; Copyright (C) 1995 Sun Microsystems.

;; Maintainer: XEmacs Development Team
;; Keywords: extensions, dumped

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
;; Free Software Foundation, Inc., 59 Temple Place - Suite 330,
;; Boston, MA 02111-1307, USA.

;;; Synched up with: FSF 19.30 (select.el).

;;; Commentary:

;; This file is dumped with XEmacs (when X support is compiled in).

;; The selection code requires us to use certain symbols whose names are
;; all upper-case; this may seem tasteless, but it makes there be a 1:1
;; correspondence between these symbols and X Atoms (which are upcased).

;;; Code:

(define-obsolete-function-alias 'x-selection-exists-p 'selection-exists-p)
(define-obsolete-function-alias 'x-selection-owner-p 'selection-owner-p)
(define-obsolete-variable-alias 'x-selection-converter-alist 'selection-converter-alist)
(define-obsolete-variable-alias 'x-lost-selection-hooks 'lost-selection-hooks)
(define-obsolete-variable-alias 'x-selected-text-type 'selected-text-type)
(define-obsolete-function-alias 'x-valid-simple-selection-p 'valid-simple-selection-p)
(define-obsolete-function-alias 'x-own-selection 'own-selection)
(define-obsolete-function-alias 'x-disown-selection 'disown-selection)
(define-obsolete-function-alias 'x-delete-primary-selection 'delete-primary-selection)
(define-obsolete-function-alias 'x-copy-primary-selection 'copy-primary-selection)
(define-obsolete-function-alias 'x-kill-primary-selection 'kill-primary-selection)
(define-obsolete-function-alias 'x-select-make-extent-for-selection
  'select-make-extent-for-selection)
(define-obsolete-function-alias 'x-cut-copy-clear-internal 'cut-copy-clear-internal)
(define-obsolete-function-alias 'x-get-selection 'get-selection)
(define-obsolete-function-alias 'x-disown-selection-internal
  'disown-selection-internal)

(defun x-get-secondary-selection ()
  "Return text selected from some X window."
  (get-selection 'SECONDARY))

(defun x-get-clipboard ()
  "Return text pasted to the clipboard."
  (get-selection 'CLIPBOARD))

(defun x-own-secondary-selection (selection &optional type)
  "Make a secondary X Selection of the given argument.  The argument may be a
string or a cons of two markers (in which case the selection is considered to
be the text between those markers)."
  (interactive (if (not current-prefix-arg)
		   (list (read-string "Store text for pasting: "))
		 (list (cons ;; these need not be ordered.
			(copy-marker (point-marker))
			(copy-marker (mark-marker))))))
  (own-selection selection 'SECONDARY))

(defun x-notice-selection-requests (selection type successful)
  "for possible use as the value of x-sent-selection-hooks."
  (if (not successful)
      (message "Selection request failed to convert %s to %s"
	       selection type)
    (message "Sent selection %s as %s" selection type)))

(defun x-notice-selection-failures (selection type successful)
  "for possible use as the value of x-sent-selection-hooks."
  (or successful
      (message "Selection request failed to convert %s to %s"
	       selection type)))

;(setq x-sent-selection-hooks 'x-notice-selection-requests)
;(setq x-sent-selection-hooks 'x-notice-selection-failures)


;;; Selections in killed buffers
;;; this function is called by kill-buffer as if it were on the
;;; kill-buffer-hook (though it isn't really).

(defun xselect-kill-buffer-hook ()
  ;; Probably the right thing is to write a C function to return a list
  ;; of the selections which emacs owns, since it could conceivably own
  ;; a user-defined selection type that we've never heard of.
  (xselect-kill-buffer-hook-1 'PRIMARY)
  (xselect-kill-buffer-hook-1 'SECONDARY)
  (xselect-kill-buffer-hook-1 'CLIPBOARD))

(defun xselect-kill-buffer-hook-1 (selection)
  (let (value)
    (if (and (selection-owner-p selection)
	     (setq value (get-selection-internal selection '_EMACS_INTERNAL))
	     ;; The _EMACS_INTERNAL selection type has a converter registered
	     ;; for it that does no translation.  This only works if emacs is
	     ;; requesting the selection from itself.  We could have done this
	     ;; by writing a C function to return the raw selection data, and
	     ;; that might be the right way to do this, but this was easy.
	     (or (and (consp value)
		      (markerp (car value))
		      (eq (current-buffer) (marker-buffer (car value))))
		 (and (extent-live-p value)
		      (eq (current-buffer) (extent-object value)))
                 (and (extentp value) (not (extent-live-p value)))))
	(disown-selection-internal selection))))


;;; Cut Buffer support

;;; FSF name x-get-cut-buffer
(defun x-get-cutbuffer (&optional which-one)
  "Return the value of one of the 8 X server cut buffers.
Optional arg WHICH-ONE should be a number from 0 to 7, defaulting to 0.
Cut buffers are considered obsolete; you should use selections instead.
This function does nothing if support for cut buffers was not compiled
into Emacs."
  (and (fboundp 'x-get-cutbuffer-internal)
       (x-get-cutbuffer-internal
	(if which-one
	    (aref [CUT_BUFFER0 CUT_BUFFER1 CUT_BUFFER2 CUT_BUFFER3
			       CUT_BUFFER4 CUT_BUFFER5 CUT_BUFFER6 CUT_BUFFER7]
		  which-one)
	  'CUT_BUFFER0))))

;;; FSF name x-set-cut-buffer
(defun x-store-cutbuffer (string &optional push)
  "Store STRING into the X server's primary cut buffer.
If PUSH is non-nil, also rotate the cut buffers:
this means the previous value of the primary cut buffer moves the second
cut buffer, and the second to the third, and so on (there are 8 buffers.)
Cut buffers are considered obsolete; you should use selections instead.
This function does nothing if support for cut buffers was not compiled
into Emacs."
  (and (fboundp 'x-store-cutbuffer-internal)
       (progn
	 ;; Check the data type of STRING.
	 (substring string 0 0)
	 (if push
	     (x-rotate-cutbuffers-internal 1))
	 (x-store-cutbuffer-internal 'CUT_BUFFER0 string))))


;;; Random utility functions

(defun x-yank-clipboard-selection ()
  "Insert the current Clipboard selection at point."
  (interactive "*")
  (setq last-command nil)
  (setq this-command 'yank) ; so that yank-pop works.
  (let ((clip (x-get-clipboard)))
    (or clip (error "there is no clipboard selection"))
    (push-mark)
    (insert clip)))


;FSFmacs (provide 'select)

;;; x-select.el ends here.
