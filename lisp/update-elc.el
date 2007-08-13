;;; update-elc.el --- Bytecompile out-of-date dumped files

;; Copyright (C) 1997 Free Software Foundation, Inc.
;; Copyright (C) 1996 Unknown

;; Maintainer: XEmacs Development Team
;; Keywords: internal

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

;;; Commentary:

;; Byte compile the .EL files necessary to dump out xemacs.
;; Use this file like this:
;;
;; temacs -batch -l ../lisp/update-elc.el $lisp
;;
;; where $lisp comes from the Makefile.  .elc files listed in $lisp will
;; cause the corresponding .el file to be compiled.  .el files listed in
;; $lisp will be ignored.
;;
;; (the idea here is that you can bootstrap if your .ELC files
;; are missing or badly out-of-date)

;; Currently this code gets the list of files to check passed to it from
;; src/Makefile.  This must be fixed.  -slb

;;; Code:

(defvar processed nil)
(defvar update-elc-files-to-compile nil)

;(setq update-elc-files-to-compile
;      (delq nil
;	    (mapcar (function
;		     (lambda (x)
;		       (if (string-match "\.elc$" x)
;			   (let ((src (substring x 0 -1)))
;			     (if (file-newer-than-file-p src x)
;				 (progn
;				   (and (file-exists-p x)
;					(null (file-writable-p x))
;					(set-file-modes x (logior (file-modes x) 128)))
;				   src))))))
;		    ;; -batch gets filtered out.
;		    (nthcdr 3 command-line-args))))

(define-function 'defalias 'define-function)
(require 'packages)

(let ((autol (list-autoloads)))
  ;; (print (prin1-to-string autol))
  (while autol
    (let ((src (car autol)))
      (if (and (file-exists-p src)
	       (file-newer-than-file-p src (concat src "c")))
	  (setq update-elc-files-to-compile
		(cons src update-elc-files-to-compile))))
    (setq autol (cdr autol))))

;; We must have some lisp support at this point
(let ((temp-path (expand-file-name "." (car load-path))))
  (setq load-path (nconc (directory-files temp-path t "^[^-.]"
					  nil 'dirs-only)
			 (cons temp-path load-path))))

;(load "backquote")
;(load "bytecomp-runtime")
;(load "subr")
;(load "replace")
;(load "version.el")
;(load "cl")
;(load "featurep")

;; (print (prin1-to-string update-elc-files-to-compile))

(let (preloaded-file-list site-load-packages)
  (load (concat default-directory "../lisp/dumped-lisp.el"))
  ;; (print (prin1-to-string preloaded-file-list))
  (load (concat default-directory "../site-packages") t t)
  (setq preloaded-file-list
	(append packages-hardcoded-lisp
		preloaded-file-list
		packages-useful-lisp
		site-load-packages))
  (while preloaded-file-list
    (let ((arg (car preloaded-file-list)))
      ;; (print (prin1-to-string arg))
      (if (null (member arg packages-unbytecompiled-lisp))
	  (progn
	    (setq arg (locate-library arg))
	    (if (null arg)
		(progn
		  (print (format "Error: Library file %s not found"
				 (car preloaded-file-list)))
		  (kill-emacs)))
	    (if (string-match "\\.elc?\\'" arg)
		(setq arg (substring arg 0 (match-beginning 0))))
	    (if (and (null (member arg processed))
		     (file-exists-p (concat arg ".el"))
		     (file-newer-than-file-p (concat arg ".el")
					     (concat arg ".elc")))
		(setq processed (cons (concat arg ".el") processed)))))
      (setq preloaded-file-list (cdr preloaded-file-list)))))

(setq update-elc-files-to-compile (append update-elc-files-to-compile
					  processed))

;; (print (prin1-to-string update-elc-files-to-compile))

(if update-elc-files-to-compile
    (progn
      (setq command-line-args
	    (append '("-l" "loadup-el.el" "run-temacs"
		      "-batch" "-q" "-no-site-file"
		      "-l" "bytecomp" "-f" "batch-byte-compile")
		    update-elc-files-to-compile))
      (load "loadup-el.el"))
  (condition-case nil
      (delete-file "./NOBYTECOMPILE")
    (file-error nil)))

(kill-emacs)

;;; update-elc.el ends here