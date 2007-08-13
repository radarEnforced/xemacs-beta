;;; find-gc.el --- detect functions that call the garbage collector

;; Copyright (C) 1992 Free Software Foundation, Inc.

;; Maintainer: FSF
;; Keywords: maint

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

;;; Synched up with: FSF 19.30.

;;; #### before this is really usable, it should be rewritten to call
;;; Makefile to compile the files.

;;; Commentary:

;;; Produce in unsafe-list the set of all functions that may invoke GC.
;;; This expects the Emacs sources to live in emacs-source-directory.
;;; It creates a temporary working directory /tmp/esrc.

;;; Code:

;; Set this to point to your XEmacs source directory.
(setq emacs-source-directory "/net/prosper/opt/xemacs/editor/src")

;; Set this to the include directories neeed for compilation.  
(setq include-directives "-I/net/prosper/opt/xemacs/editor/src -I/usr/dt/include -I/net/prosper/opt/SUNWmotif/include -I/net/prosper/opt/xemacs/editor/import/xpm/include -I/net/prosper/opt/xemacs/editor/import/include -I/usr/demo/SOUND/include -I/usr/openwin/include -I/usr/openwin/include/desktop -I/usr/openwin/include/desktop   -I/net/prosper/opt/xemacs/editor/src/../lwlib  -g -v -DNeedFunctionPrototypes -xildoff -I/usr/dt/include -I/net/prosper/opt/SUNWmotif/include -I/net/prosper/opt/xemacs/editor/import/xpm/include -I/net/prosper/opt/xemacs/editor/import/include -I/usr/demo/SOUND/include -I/usr/demo/SOUND/include")

;; Set this to the source files you want to check.
(setq source-files
  '(
"EmacsFrame.c" "EmacsManager.c" "EmacsShell.c" "abbrev.c" "alloc.c"
"blocktype.c" "buffer.c" "bytecode.c" "callint.c" "callproc.c" "casefiddle.c"
"casetab.c" "cm.c" "cmds.c" "data.c" "debug.c" "device-stream.c"
"device-tty.c" "device-x.c" "device.c" "dired.c" "doc.c" "doprnt.c"
"dynarr.c""editfns.c" "elhash.c" "emacs.c" "eval.c" "event-Xt.c"
"event-stream.c" "event-tty.c" "events.c" "extents.c" "faces.c" "fileio.c"
"filelock.c" "filemode.c" "floatfns.c" "fns.c" "font-lock.c" "frame-tty.c"
"frame-x.c" "frame.c" "getloadavg.c" "glyphs.c" "gmalloc.c" "hash.c"
"indent.c" "insdel.c" "intl.c" "keyboard.c" "keymap.c" "lastfile.c" "lread.c"
"lstream.c" "macros.c" "marker.c" "md5.c" "menubar-x.c" "menubar.c"
"minibuf.c" "objects-x.c" "objects.c" "opaque.c" "print.c" "process.c"
"pure.c" "redisplay-output.c" "redisplay-tty.c" "redisplay-x.c" "redisplay.c"
"regex.c" "scrollbar.c" "search.c" "sound.c" "specifier.c" "sunplay.c"
"sunpro.c" "symbols.c" "syntax.c" "sysdep.c" "terminfo.c" "toolbar-x.c"
"toolbar.c" "tooltalk.c" "undo.c" "unexsol2.c" "vm-limit.c" "window.c"
"xgccache.c" "xselect.c"))

;;;

(defun find-gc-unsafe ()
  (trace-call-tree nil)
  (trace-use-tree)
  (find-unsafe-funcs 'Fgarbage_collect)
  (setq unsafe-list (sort unsafe-list
			  (function (lambda (x y)
				      (string-lessp (car x) (car y))))))
  (princ (format "%s\n" unsafe-list))
  (setq unsafe-list nil)
  (find-unsafe-funcs 'Fgarbage_collect_1)
  (setq unsafe-list (sort unsafe-list
			  (function (lambda (x y)
				      (string-lessp (car x) (car y))))))
  (princ (format "%s\n" unsafe-list)))


;;; This does a depth-first search to find all functions that can
;;; ultimately call the function "target".  The result is an a-list
;;; in unsafe-list; the cars are the unsafe functions, and the cdrs
;;; are (one of) the unsafe functions that these functions directly
;;; call.

(defun find-unsafe-funcs (target)
  (setq unsafe-list (list (list target)))
  (trace-unsafe target))

(defun trace-unsafe (func)
  (let ((used (assq func subrs-used)))
    (or used
	(error "No subrs-used for %s" (car unsafe-list)))
    (while (setq used (cdr used))
      (or (assq (car used) unsafe-list)
	  (memq (car used) noreturn-list)
	  (progn
	    (setq unsafe-list (cons (cons (car used) func) unsafe-list))
	    (trace-unsafe (car used)))))))


;;; Functions on this list are safe, even if they appear to be able
;;; to call the target.

(setq noreturn-list '( signal_error error Fthrow wrong_type_argument ))


;;; This produces an a-list of functions in subrs-called.  The cdr of
;;; each entry is a list of functions which the function in car calls.

(defun trace-call-tree (&optional already-setup)
  (or already-setup
      (progn
	(princ (format "Setting up directories...\n"))
	;; Gee, wouldn't a built-in "system" function be handy here.
	(call-process "sh" nil nil nil "-c" "rm -rf /tmp/esrc")
	(call-process "sh" nil nil nil "-c" "mkdir /tmp/esrc")
	(call-process "sh" nil nil nil "-c"
		      (format "ln -s %s/*.[ch] /tmp/esrc"
			      emacs-source-directory))))
  (save-excursion
    (set-buffer (get-buffer-create "*Trace Call Tree*"))
    (setq subrs-called nil)
    (let ((case-fold-search nil)
	  (files source-files)
	  name entry)
      (while files
	(princ (format "Compiling %s...\n" (car files)))
	(call-process "sh" nil nil nil "-c"
	     (format "cd /tmp/esrc; gcc -dr -c %s /tmp/esrc/%s -o /dev/null"
			      include-directives (car files)))
	(erase-buffer)
	(insert-file-contents (concat "/tmp/esrc/" (car files) ".rtl"))
	(while (re-search-forward ";; Function \\|(call_insn " nil t)
	  (if (= (char-after (- (point) 3)) ?o)
	      (progn
		(looking-at "[a-zA-Z0-9_]+")
		(setq name (intern (buffer-substring (match-beginning 0)
						     (match-end 0))))
		(princ (format "%s : %s\n" (car files) name))
		(setq entry (list name)
		      subrs-called (cons entry subrs-called)))
	    (if (looking-at ".*\n?.*\"\\([A-Za-z0-9_]+\\)\"")
		(progn
		  (setq name (intern (buffer-substring (match-beginning 1)
						       (match-end 1))))
		  (or (memq name (cdr entry))
		      (setcdr entry (cons name (cdr entry))))))))
	;;(delete-file (concat "/tmp/esrc/" (car files) ".rtl"))
	(setq files (cdr files))))))


;;; This produces an inverted a-list in subrs-used.  The cdr of each
;;; entry is a list of functions that call the function in car.

(defun trace-use-tree ()
  (setq subrs-used (mapcar 'list (mapcar 'car subrs-called)))
  (let ((ptr subrs-called)
	p2 found)
    (while ptr
      (setq p2 (car ptr))
      (while (setq p2 (cdr p2))
	(if (setq found (assq (car p2) subrs-used))
	    (setcdr found (cons (car (car ptr)) (cdr found)))))
      (setq ptr (cdr ptr)))))

;;; find-gc.el ends here