;;; loadup.el --- load up standardly loaded Lisp files for XEmacs.

;; Copyright (C) 1985, 1986, 1992, 1994, 1997 Free Software Foundation, Inc.
;; Copyright (C) 1996 Richard Mlynarik.
;; Copyright (C) 1995, 1996 Ben Wing.

;; Maintainer: XEmacs Development Team
;; Keywords: internal, dumped

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
;; along with XEmacs; see the file COPYING.  If not, write to the Free
;; Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
;; 02111-1307, USA.

;;; Synched up with: Last synched with FSF 19.30, with wild divergence since.

;;; Commentary:

;; Please do not edit this file.  Use site-init.el, site-load.el, or
;; packaged dumped-lisp.el's instead.

;; This is loaded into a bare XEmacs to make a dumpable one.

;;; Code:

(if (fboundp 'error)
    (error "loadup.el already loaded!"))

(define-function 'defalias 'define-function)
(defvar running-xemacs t
  "Non-nil when the current emacs is XEmacs.")
(defvar preloaded-file-list nil
  "List of files preloaded into the XEmacs binary image.")

;; This is awfully damn early to be getting an error, right?
(call-with-condition-handler 'really-early-error-handler
    #'(lambda ()
	;; message not defined yet ...
	(external-debugging-output (format "\nUsing load-path %s" load-path))

	;; We don't want to have any undo records in the dumped XEmacs.
	(buffer-disable-undo (get-buffer "*scratch*"))

	;; lread.c (or src/Makefile.in.in) has prepended
	;; "${srcdir}/../lisp/" to load-path, which is how this file
	;; has been found.  At this point, enough of XEmacs has been
	;; initialized that we can start dumping "standard" lisp.
	;; Dumped lisp from external packages is added when we search
	;; the `package-path'.
	;; #### This code is duplicated in two other places.
	(let ((temp-path (expand-file-name "." (car load-path))))
	  (setq source-directory temp-path)
	  (setq load-path (nconc (mapcar
				  #'(lambda (i) (concat i "/"))
				  (directory-files temp-path t "^[^-.]"
						   nil 'dirs-only))
				 (cons (file-name-as-directory temp-path)
				       load-path))))

	(setq load-warn-when-source-newer t ; Used to be set to nil at the end
	      load-warn-when-source-only  t) ; Set to nil at the end

	;; Inserted for debugging.  Something is corrupting a single symbol
	;; somewhere to have an integer 0 property list.  -slb 6/28/1997.
	(defun test-atoms ()
	  (mapatoms
	   #'(lambda (symbol)
	       (condition-case nil
		   (get symbol 'custom-group)
		 (t (princ
		     (format "Bad plist in %s, %s\n"
			     (symbol-name symbol)
			     (prin1-to-string (object-plist symbol)))))))))

	;; garbage collect after loading every file in an attempt to
	;; minimize the size of the dumped image (if we don't do this,
	;; there will be lots of extra space in the data segment filled
	;; with garbage-collected junk)
	(defun load-gc (file)
	  (let ((full-path (locate-file file
					load-path
					(if load-ignore-elc-files
					    ".el:"
					  ".elc:.el:"))))
	    (if full-path
		(prog1
		  (load full-path)
		  ;; '(test-atoms)
		  '(garbage-collect))
	      (external-debugging-output (format "\nLoad file %s: not found\n"
						 file))
	      nil)))

	(load (concat default-directory "../lisp/dumped-lisp.el"))
	(let ((dumped-lisp-packages preloaded-file-list)
	      file)
	  (while (setq file (car dumped-lisp-packages))
	    (or (load-gc file)
	      (progn
		(external-debugging-output "Fatal error during load, aborting")
		(kill-emacs 1)))
	    (setq dumped-lisp-packages (cdr dumped-lisp-packages)))
	  (if (not (featurep 'toolbar))
	      (progn
		;; else still define a few functions.
		(defun toolbar-button-p    (obj) "No toolbar support." nil)
		(defun toolbar-specifier-p (obj) "No toolbar support." nil)))
	  (fmakunbound 'load-gc))
	)) ;; end of call-with-condition-handler

;; Fix up the preloaded file list
(setq preloaded-file-list (mapcar #'file-name-sans-extension
				  preloaded-file-list))

(setq load-warn-when-source-newer t ; set to t at top of file
      load-warn-when-source-only nil)

(setq debugger 'debug)

(when (member "no-site-file" command-line-args)
  (setq site-start-file nil))

;; If you want additional libraries to be preloaded and their
;; doc strings kept in the DOC file rather than in core,
;; you may load them with a "site-load.el" file.
;; But you must also cause them to be scanned when the DOC file
;; is generated.  For VMS, you must edit ../../vms/makedoc.com.
;; For other systems, you must edit ../../src/Makefile.in.in.
(if (load "site-load" t)
    (garbage-collect))

;;FSFmacs randomness
;;(if (fboundp 'x-popup-menu)
;;    (precompute-menubar-bindings))
;;; Turn on recording of which commands get rebound,
;;; for the sake of the next call to precompute-menubar-bindings.
;(setq define-key-rebound-commands nil)


;; Note: all compiled Lisp files loaded above this point
;; must be among the ones parsed by make-docfile
;; to construct DOC.  Any that are not processed
;; for DOC will not have doc strings in the dumped XEmacs.

;; Don't bother with these if we're running temacs, i.e. if we're
;; just debugging don't waste time finding doc strings.

;; purify-flag is nil if called from loadup-el.el.
(when purify-flag
  (message "Finding pointers to doc strings...")
  ;; (test-atoms) ; Debug -- Doesn't happen here
  (Snarf-documentation "DOC")
  ;; (test-atoms) ; Debug -- Doesn't happen here
  (message "Finding pointers to doc strings...done")
  (Verify-documentation)
  ;; (test-atoms) ; Debug -- Doesn't happen here
  )

;; Note: You can cause additional libraries to be preloaded
;; by writing a site-init.el that loads them.
;; See also "site-load" above.
(if (stringp site-start-file)
    (load "site-init" t))
(setq current-load-list nil)
(garbage-collect)

;;; At this point, we're ready to resume undo recording for scratch.
(buffer-enable-undo "*scratch*")

;; Dump into the name `xemacs' (only)
(when (member "dump" command-line-args)
    (message "Dumping under the name xemacs")
    ;; This is handled earlier in the build process.
    ;; (condition-case () (delete-file "xemacs") (file-error nil))
    (test-atoms)
    (when (fboundp 'really-free)
      (really-free))
    (test-atoms)
    (dump-emacs "xemacs" "temacs")
    (test-atoms)
    (kill-emacs))

(when (member "run-temacs" command-line-args)
  (message "\nBootstrapping from temacs...")
  (setq purify-flag nil)
  (setq inhibit-package-init t)
  (setq inhibit-update-dumped-lisp t)
  (setq inhibit-update-autoloads t)
  ;; Remove all args up to and including "run-temacs"
  (apply #'run-emacs-from-temacs (cdr (member "run-temacs" command-line-args)))
  ;; run-emacs-from-temacs doesn't actually return anyway.
  (kill-emacs))

;; Avoid error if user loads some more libraries now.
(setq purify-flag nil)

;; XEmacs change
;; If you are using 'recompile', then you should have used -l loadup-el.el
;; so that the .el files always get loaded (the .elc files may be out-of-
;; date or bad).
(when (member "recompile" command-line-args)
  (let ((command-line-args-left (cdr (member "recompile" command-line-args))))
    (batch-byte-recompile-directory)
    (kill-emacs)))

;; For machines with CANNOT_DUMP defined in config.h,
;; this file must be loaded each time Emacs is run.
;; So run the startup code now.

(when (not (fboundp 'dump-emacs))
  ;; Avoid loading loadup.el a second time!
  (setq command-line-args (cdr (cdr command-line-args)))
  (eval top-level))

;;; loadup.el ends here
