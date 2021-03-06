;; a short example how to use the new Drag'n'Drop API in
;; combination with extents.

;; Copyright (C) 1998 Oliver Graf <ograf@fga.de>

;; This file is part of XEmacs.

;; XEmacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by the
;; Free Software Foundation, either version 3 of the License, or (at your
;; option) any later version.

;; XEmacs is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
;; FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
;; for more details.

;; You should have received a copy of the GNU General Public License
;; along with XEmacs.  If not, see <http://www.gnu.org/licenses/>.

;;; Synched up with: Not in FSF.

(defun dnd-drop-message (event object text)
  (message "Dropped %s with :%s" text object)
  ;; signal that we have done something with the data
  t)

(defun do-nothing (event object)
  ;; signal that the data is still unprocessed
  nil)

(defun start-drag (event what &optional typ)
  ;; short drag interface, until the real one is implemented
  (cond ((featurep 'cde)
	 (if (not typ)
	     (funcall (intern "cde-start-drag-internal") event nil (list what))
	   (funcall (intern "cde-start-drag-internal") event t what)))
	(t display-message 'error "no valid drag protocols implemented")))

(defun start-region-drag (event)
  (interactive "_e")
  (if (click-inside-extent-p event zmacs-region-extent)
      ;; okay, this is a drag
      (cond ((featurep 'cde)
	     (cde-start-drag-region event
				    (extent-start-position zmacs-region-extent)
				    (extent-end-position zmacs-region-extent)))
	    (t (error "No CDE support compiled in")))))

(defun make-drop-targets ()
  (let ((buf (get-buffer-create "*DND misc-user extent test buffer*"))
	(s nil)
	(e nil))
    (set-buffer buf)
    (pop-to-buffer buf)
    (setq s (point))
    (insert "[ DROP TARGET 1]")
    (setq e (point))
    (setq ext (make-extent s e))
    (set-extent-property ext
			 'experimental-dragdrop-drop-functions
			 '((do-nothing t t)
			   (dnd-drop-message t t "on target 1")))
    (set-extent-property ext 'mouse-face 'highlight)
    (insert "    ")
    (setq s (point))
    (insert "[ DROP TARGET 2]")
    (setq e (point))
    (setq ext (make-extent s e))
    (set-extent-property ext
			 'experimental-dragdrop-drop-functions
			 '((dnd-drop-message t t "on target 2")))
    (set-extent-property ext 'mouse-face 'highlight)
    (insert "    ")
    (setq s (point))
    (insert "[ DROP TARGET 3]")
    (setq e (point))
    (setq ext (make-extent s e))
    (set-extent-property ext
			 'experimental-dragdrop-drop-functions
			 '((dnd-drop-message t t "on target 3")))
    (set-extent-property ext 'mouse-face 'highlight)
    (newline 2)))

(defun make-drag-starters ()
  (let ((buf (get-buffer-create "*DND misc-user extent test buffer*"))
	(s nil)
	(e nil)
	(ext nil)
	(kmap nil))
    (set-buffer buf)
    (pop-to-buffer buf)
    (erase-buffer buf)
    (insert "Try to drag data from one of the upper extents to one\nof the lower extents. Make sure that your minibuffer is big\ncause it is used to display the data.\n\nYou may also try to select some of this text and drag it with button2.\n\nTo ")
    (setq s (point))
    (insert "EXIT")
    (setq e (point))
    (insert " this demo, press 'q'.")
    (setq ext (make-extent s e))
    (setq kmap (make-keymap))
    (define-key kmap [button1] 'end-dnd-demo)
    (set-extent-property ext 'keymap kmap)
    (set-extent-property ext 'mouse-face 'highlight)
    (newline 2)
    (setq s (point))
    (insert "[ TEXT DRAG TEST ]")
    (setq e (point))
    (setq ext (make-extent s e))
    (set-extent-property ext 'mouse-face 'isearch)
    (setq kmap (make-keymap))
    (define-key kmap [button1] 'text-drag)
    (set-extent-property ext 'keymap kmap)
    (insert "    ")
    (setq s (point))
    (insert "[ FILE DRAG TEST ]")
    (setq e (point))
    (setq ext (make-extent s e))
    (set-extent-property ext 'mouse-face 'isearch)
    (setq kmap (make-keymap))
    (if (featurep 'cde)
	(define-key kmap [button1] 'cde-file-drag)
      (define-key kmap [button1] 'file-drag))
    (set-extent-property ext 'keymap kmap)
    (insert "    ")
    (setq s (point))
    (insert "[ FILES DRAG TEST ]")
    (setq e (point))
    (setq ext (make-extent s e))
    (set-extent-property ext 'mouse-face 'isearch)
    (setq kmap (make-keymap))
    (define-key kmap [button1] 'files-drag)
    (set-extent-property ext 'keymap kmap)
    (insert "    ")
    (setq s (point))
    (insert "[ URL DRAG TEST ]")
    (setq e (point))
    (setq ext (make-extent s e))
    (set-extent-property ext 'mouse-face 'isearch)
    (setq kmap (make-keymap))
    (if (featurep 'cde)
	(define-key kmap [button1] 'cde-file-drag)
      (define-key kmap [button1] 'url-drag))
    (set-extent-property ext 'keymap kmap)
    (newline 3)))
    
(defun text-drag (event)
  (interactive "@e")
  (start-drag event "That's a test"))

(defun file-drag (event)
  (interactive "@e")
  (start-drag event "/tmp/DropTest.xpm" 2))

(defun cde-file-drag (event)
  (interactive "@e")
  (start-drag event '("/tmp/DropTest.xpm") t))

(defun url-drag (event)
  (interactive "@e")
  (start-drag event "http://www.xemacs.org/" 8))

(defun files-drag (event)
  (interactive "@e")
  (start-drag event '("/tmp/DropTest.html" "/tmp/DropTest.xpm" "/tmp/DropTest.tex") 3))

(setq experimental-dragdrop-drop-functions '((do-nothing t t)
				;; CDE does not have any button info...
				(dnd-drop-message 0 t "cde-drop somewhere else")
				(dnd-drop-message 2 t "region somewhere else")
				(dnd-drop-message 1 t "drag-source somewhere else")
				(do-nothing t t)))

(make-drag-starters)
(make-drop-targets)

(defun end-dnd-demo ()
  (interactive)
  (global-set-key [button2] button2-func)
  (bury-buffer))

(setq lmap (make-keymap))
(use-local-map lmap)
(local-set-key [q] 'end-dnd-demo)
(setq button2-func (lookup-key global-map [button2]))
(global-set-key [button2] 'start-region-drag)
