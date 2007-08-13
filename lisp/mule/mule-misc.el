;; mule-misc.el --- Miscellaneous Mule functions.

;; Copyright (C) 1992,93,94,95 Free Software Foundation, Inc.
;; Copyright (C) 1995 Amdahl Corporation.
;; Copyright (C) 1995 Sun Microsystems.

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

;;;
;;;  protect specified local variables from kill-all-local-variables
;;;

(defvar self-insert-after-hook nil
  "Hook to run when extended self insertion command exits.  Should take
two arguments START and END corresponding to character position.")

(make-variable-buffer-local 'self-insert-after-hook)

(defun toggle-display-direction ()
  (interactive)
  (setq display-direction (not display-direction))
  (if (interactive-p) (redraw-display)))

;;;
;;; Utility functions for Mule
;;;

;(defun string-to-char-list (str)
;  (let ((len (length str))
;	(idx 0)
;	c l)
;    (while (< idx len)
;      (setq c (sref str idx))
;      (setq idx (+ idx (charset-dimension (char-charset c))))
;      (setq l (cons c l)))
;    (nreverse l)))

(defun string-to-char-list (str)
  (mapcar 'identity str))

(defun string-width (string)
  "Return number of columns STRING occupies when displayed.
Uses the charset-columns attribute of the characters in STRING,
which may not accurately represent the actual display width when
using a window system."
  (let ((col 0)
	(len (length string))
	(i 0))
    (while (< i len)
      (setq col (+ col (charset-columns (char-charset (aref string i)))))
      (setq i (1+ i)))
    col))

(defalias 'string-columns 'string-width)
(make-obsolete 'string-columns 'string-width)

(defun delete-text-in-column (from to)
  "Delete the text between column FROM and TO (exclusive) of the current line.
Nil of FORM or TO means the current column.

If there's a character across the borders, the character is replaced
with the same width of spaces before deleting."
  (save-excursion
    (let (p1 p2)
      (if from
	  (progn
	    (setq p1 (move-to-column from))
	    (if (> p1 from)
		(progn
		  (delete-char -1)
		  (insert-char ?  (- p1 (current-column)))
		  (forward-char (- from p1))))))
      (setq p1 (point))
      (if to
	  (progn
	    (setq p2 (move-to-column to))
	    (if (> p2 to)
		(progn
		  (delete-char -1)
		  (insert-char ?  (- p2 (current-column)))
		  (forward-char (- to p2))))))
      (setq p2 (point))
      (delete-region p1 p2))))

;; #### Someone translate this!!

(defun mc-normal-form-string (str)
  "$BJ8;zNs(B STR $B$N4A;zI8=`7AJ8;zNs$rJV$9!%(B"
  (let ((i 0))
    (while (setq i (string-match "\n" str i))
      (if (and (<= 1 i) (< i (1- (length str)))
	       (< (aref str (1- i)) 128)
	       (< (aref str (1+ i)) 128))
	  (aset str i ? ))
      (setq i (1+ i)))
    (if (string-match "\n" str 0)
	(let ((c 0) (i 0) new)
	  (while (setq i (string-match "\n" str i))
	    (setq i (1+ i))
	    (setq c (1+ c)))
	  (setq new (make-string (- (length str) c) 0))
	  (setq i 0 c 0)
	  (while (< i (length str))
	    (cond((not (= (aref str i) ?\n ))
		  (aset new c (aref str i))
		  (setq c (1+ c))))

	    (setq i (1+ i))
	    )
	  new)
      str)))


(defun string-memq (str list)
  "Returns non-nil if STR is an element of LIST.  Comparison done with string=.
The value is actually the tail of LIST whose car is STR.
If each element of LIST is not a string, it is converted to string
 before comparison."
  (let (find elm)
    (while (and (not find) list)
      (setq elm (car list))
      (if (numberp elm) (setq elm (char-to-string elm)))
      (if (string= str elm)
	  (setq find list)
	(setq list (cdr list))))
    find))

(defun cancel-undo-boundary ()
  "Cancel undo boundary."
  (if (and (consp buffer-undo-list)
	   ;; if car is nil.
	   (null (car buffer-undo-list)) )
      (setq buffer-undo-list (cdr buffer-undo-list)) ))


;;; Common API emulation functions for GNU Emacs-merged Mule.
;;; As suggested by MORIOKA Tomohiko

;; Following definition were imported from Emacs/mule-delta.

;; Function `truncate-string-to-width' was moved to mule-util.el.

;; end of imported definition


(defalias 'sref 'aref)
(defalias 'map-char-concat 'mapcar)
(defun char-bytes (character)
  "Return number of length a CHARACTER occupies in a string or buffer.
It returns only 1 in XEmacs.  It is for compatibility with MULE 2.3."
  1)
(defalias 'char-length 'char-bytes)

(defun char-width (character)
  "Return number of columns a CHARACTER occupies when displayed."
  (charset-columns (char-charset character)))

(defalias 'char-columns 'char-width)
(make-obsolete 'char-columns 'char-width)

(defalias 'charset-description 'charset-doc-string)

(defalias 'find-charset-string 'charsets-in-string)
(defalias 'find-charset-region 'charsets-in-region)

(defun find-non-ascii-charset-string (string)
  "Return a list of charsets in the STRING except ascii.
It might be available for compatibility with Mule 2.3,
because its `find-charset-string' ignores ASCII charset."
  (delq 'ascii (charsets-in-string string)))

(defun find-non-ascii-charset-region (start end)
  "Return a list of charsets except ascii in the region between START and END.
It might be available for compatibility with Mule 2.3,
because its `find-charset-string' ignores ASCII charset."
  (delq 'ascii (charsets-in-region start end)))


;;; Language environments

(defvar current-language-environment nil)

(defvar language-environment-list nil)

(defun current-language-environment ()
  "Return the current language environment as a symbol.
Returns nil if `set-language-environment' has not been called."
  current-language-environment)

(defun language-environment-list ()
  "Return a list of all currently defined language environments."
  language-environment-list)

(defun language-environment-p (sym)
  "True if SYM names a defined language environment."
  (memq sym (language-environment-list)))

(defun set-language-environment (env)
  "Set the current language environment to ENV."
  (interactive
   (list (intern (completing-read "Language environment: "
				  obarray 'language-environment-p
				  'require-match))))
  (when (not (string= (charset-registry 'ascii) "ISO8859-1"))
    (set-charset-registry 'ascii "ISO8859-1"))
  (let ((func (get env 'set-lang-environ)))
    (if (not (null func))
	(funcall func)))
  (setq current-language-environment env)
  (if (featurep 'egg)
      (egg-lang-switch-callback))
;;  (if (featurep 'quail)
;;      (quail-lang-switch-callback))
)

(defun define-language-environment (env-sym doc-string enable-function)
  "Define a new language environment, named by ENV-SYM.
DOC-STRING should be a string describing the environment.
ENABLE-FUNCTION should be a function of no arguments that will be called
when the language environment is made current."
  (put env-sym 'lang-environ-doc-string doc-string)
  (put env-sym 'set-lang-environ enable-function)
  (setq language-environment-list (cons env-sym language-environment-list)))

(defun define-egg-environment (env-sym doc-string enable-function)
  "Define a new language environment for egg, named by ENV-SYM.
DOC-STRING should be a string describing the environment.
ENABLE-FUNCTION should be a function of no arguments that will be called
when the language environment is made current."
  (put env-sym 'egg-environ-doc-string doc-string)
  (put env-sym 'set-egg-environ enable-function))

(defun define-quail-environment (env-sym doc-string enable-function)
  "Define a new language environment for quail, named by ENV-SYM.
DOC-STRING should be a string describing the environment.
ENABLE-FUNCTION should be a function of no arguments that will be called
when the language environment is made current."
  (put env-sym 'quail-environ-doc-string doc-string)
  (put env-sym 'set-quail-environ enable-function))

