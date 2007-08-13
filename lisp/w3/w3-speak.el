;;; w3-speak.el,v --- Emacs-W3 speech interface
;; Author: wmperry
;; Original author: William Perry --<wmperry@cs.indiana.edu>
;; Cloned from emacspeak-w3.el
;; Created: 1996/10/16 20:56:40
;; Version: 1.14
;; Keywords: hypermedia, speech

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Copyright (c) 1996 by T.V. Raman (raman@adobe.com)
;;; Copyright (c) 1996 by William M. Perry (wmperry@spry.com)
;;;
;;; This file is not part of GNU Emacs, but the same permissions apply.
;;;
;;; GNU Emacs is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 2, or (at your option)
;;; any later version.
;;;
;;; GNU Emacs is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with GNU Emacs; see the file COPYING.  If not, write to the
;;; Free Software Foundation, Inc., 59 Temple Place - Suite 330,
;;; Boston, MA 02111-1307, USA.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; A replacement module for emacspeak-w3 that uses all the new functionality
;;; of Emacs-W3 3.0.
;;;
;;; This file would not be possible without the help of
;;; T.V. Raman (raman@adobe.com) and his continued efforts to make Emacs-W3
;;; even remotely useful. :)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;; This conforms to http://www4.inria.fr/speech2.html

(require 'widget)
(require 'w3-forms)
(require 'advice)
;; This condition-case needs to be here or it completely chokes
;; byte-compilation for people who do not have Emacspeak installed.
;; *sigh*
(condition-case ()
    (progn
      (require 'emacspeak)
      (require 'dtk-voices)
      (require 'emacspeak-speak)
      (require 'emacspeak-sounds)
      (eval-when (compile)
		 (require 'emacspeak-fix-interactive)))
  (error (message "Emacspeak not found - speech will not work.")))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; How to get information summarizing a form field, so it can be spoken in
;;; a sane manner.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;{{{  putting and getting form field summarizer

(defsubst w3-speak-define-field-summarizer (type &optional function-name)
  "Associate the name of a function that describes this type of form field."
  (put type 'w3-speak-summarizer
       (or function-name (intern
			  (format "w3-speak-summarize-%s-field" type)))))

(defsubst w3-speak-get-field-summarizer  (type)
  "Retrieve function-name string for this voice"
  (get type 'w3-speak-summarizer))

;;}}}
;;{{{  Associate summarizer functions for form fields 

(w3-speak-define-field-summarizer 'text)
(w3-speak-define-field-summarizer 'option)
(w3-speak-define-field-summarizer 'checkbox)
(w3-speak-define-field-summarizer 'reset)
(w3-speak-define-field-summarizer 'submit)
(w3-speak-define-field-summarizer 'button)
(w3-speak-define-field-summarizer 'radio)
(w3-speak-define-field-summarizer 'multiline)
(w3-speak-define-field-summarizer 'image)

;;}}}


;;{{{  define the form field summarizer functions

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Now actually define the summarizers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(defsubst w3-speak-extract-form-field-label (data)
  ;;; FIXXX!!! Need to reimplement using the new forms implementation!
  (declare (special w3-form-labels))
  nil)

(defun w3-speak-summarize-text-field (data)
  "Summarize a text field given the field data."
  (let (
	(label (w3-speak-extract-form-field-label data))
	(name  (w3-form-element-name data))
	(value (widget-get (w3-form-element-widget data) :value)))
    (dtk-speak
     (format "Text  field  %s  %s " (or label (concat "called " name))
	     (concat "set to " value)))))

(defun w3-speak-summarize-multiline-field (data)
  "Summarize a text field given the field data."
  (let (
        (name (w3-form-element-name data))
        (label (w3-speak-extract-form-field-label data))
        (value (w3-form-element-value data)))
    (dtk-speak
     (format "Multiline text input  %s  %s" (or label (concat "called " name))
	     (concat "set to " value)))))

(defun w3-speak-summarize-checkbox-field (data)
  "Summarize a checkbox  field given the field data."
  (let (
	(name (w3-form-element-name data))
	(label (w3-speak-extract-form-field-label data))
	(checked (widget-value (w3-form-element-widget data))))
    (dtk-speak
     (format "Checkbox %s is %s" (or label name) (if checked "on" "off")))))

(defun w3-speak-summarize-option-field (data)
  "Summarize a options   field given the field data."
  (let (
	(name (w3-form-element-name data))
	(label (w3-speak-extract-form-field-label data))
	(default (w3-form-element-default-value data)))
    (dtk-speak
     (format "Choose an option %s  %s" (or label name)
	     (if (string=  "" default)
                 ""
               (format "default is %s" default))))))

;;; to handle brain dead nynex forms
(defun w3-speak-summarize-image-field (data)
  "Summarize a image   field given the field data.
Currently, only the NYNEX server uses this."
  (let (
	(name (w3-form-element-name data))
	(label (w3-speak-extract-form-field-label data)))
    (dtk-speak
     (substring name 1))))

(defun w3-speak-summarize-submit-field (data)
  "Summarize a submit field given the field data."
  (let  (
	 (type (w3-form-element-type data))
	 (label (w3-speak-extract-form-field-label data))
	 (button-text (widget-value (w3-form-element-widget data))))
    (message "%s" (or label button-text
		      (case type
			(submit "Submit Form")
			(reset "Reset Form")
			(button "A Button"))))))

(defalias 'w3-speak-summarize-reset-field 'w3-speak-summarize-submit-field)
(defalias 'w3-speak-summarize-button-field 'w3-speak-summarize-submit-field)

(defun w3-speak-summarize-radio-field (data)
  "Summarize a radio   field given the field data."
  (let (
	(name (w3-form-element-name data))
	(label (w3-speak-extract-form-field-label data))
	(checked (widget-value (w3-form-element-widget data))))
    (dtk-speak
     (format "Radio button   %s is %s" (or label name) (if checked
							"pressed"
						      "not pressed")))))

;;}}}

;;{{{  speaking form fields 

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Now for the guts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun w3-speak-extract-form-field-information ()
  (let* ((widget (widget-at (point)))
	 (data (and widget (widget-get widget 'w3-form-data))))
    data))

(defun w3-speak-summarize-form-field ()
  "Summarizes field under point if any."
  (let* ((data (w3-speak-extract-form-field-information))
         (type (and data (w3-form-element-type data)))
         (summarizer (and type (w3-speak-get-field-summarizer type))))
    (cond
     ((and data
           summarizer
           (fboundp summarizer))
      (funcall summarizer data))
     (data
      (message "Please define a summarizer function for %s"  type))
     (t nil))))

;;}}}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Movement notification
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defadvice w3-scroll-up (after emacspeak pre act comp)
  "Provide auditory feedback"
  (when (interactive-p)
	(let ((start (point )))
	  (emacspeak-auditory-icon 'scroll)
	  (save-excursion
	    (forward-line (window-height))
	    (emacspeak-speak-region start (point ))))))

(defadvice w3-follow-link (around emacspeak pre act)
  "Provide feedback on what you did. "
  (let ((data (emacspeak-w3-extract-form-field-information))
        (form-field-p nil)
        (this-zone nil)
        (opoint nil))
    (if data
	(setq form-field-p t 
	      opoint (point)))
    ad-do-it
    (when form-field-p
      (w3-speak-summarize-form-field)
      (case (w3-form-element-type data)
	((radio checkbox)
	 (emacspeak-auditory-icon 'button))
	;; fill in any others here
	(otherwise
	 nil)))
    ad-return-value))

(defadvice w3-revert-form (after emacspeak pre act)
  "Announce that you cleared the form. "
  (dtk-speak "Cleared the form. "))

(defadvice w3-finish-text-entry (after emacspeak pre act )
  "Announce what the field was set to."
  (when (interactive-p)
    (w3-speak-summarize-form-field)))

(defadvice w3-start-of-document (after emacspeak pre act)
  "Produce an auditory icon.  Also speak the first line. "
  (when (interactive-p)
    (emacspeak-speak-line)
    (emacspeak-auditory-icon 'large-movement)))

(defadvice w3-end-of-document (after emacspeak pre act)
  "Produce an auditory icon.  Also speak the first line."
  (when (interactive-p)
    (emacspeak-speak-line)
    (emacspeak-auditory-icon 'large-movement)))

(defadvice w3-goto-last-buffer (after emacspeak pre act)
  "Speak the modeline so I know where I am."
  (when (interactive-p)
    (emacspeak-auditory-icon 'select-object)
    (emacspeak-speak-mode-line)))

(defadvice w3-quit (after emacspeak pre act)
  "Speak the mode line of the new buffer."
  (when (interactive-p)
    (emacspeak-auditory-icon 'close-object)
    (emacspeak-speak-mode-line)))

(defadvice w3-fetch (around  emacspeak  act comp )
  "First produce an auditory icon to indicate retrieval.
After retrieval, 
set  voice-lock-mode to t after displaying the buffer,
and then speak the mode-line. "
  (declare (special dtk-punctuation-mode))
  (emacspeak-auditory-icon 'select-object)
  ad-do-it)

(defun w3-speak-mode-hook ()
  (set (make-local-variable 'voice-lock-mode) t)
  (setq dtk-punctuation-mode "some")
  (emacspeak-auditory-icon 'open-object)
  (emacspeak-speak-mode-line))

;;; This is really the only function you should need to call unless
;;; you are adding functionality.
(defun w3-speak-use-voice-locking (&optional arg) 
  "Tells w3 to start using voice locking.
This is done by setting the w3 variables so that anchors etc are not marked by
delimiters. We then turn on voice-lock-mode. 
Interactive prefix arg does the opposite. "
  (interactive "P")
  (declare (special w3-delimit-links w3-delimit-emphasis w3-echo-link))
  (setq w3-echo-link 'text)
  (if arg
      (progn
	(setq w3-delimit-links 'guess 
	      w3-delimit-emphasis 'guess)
	(remove-hook 'w3-mode-hook 'w3-speak-mode-hook))
    (setq w3-delimit-links nil
          w3-delimit-emphasis nil)
    (add-hook 'w3-mode-hook 'w3-speak-mode-hook)))

(defun w3-speak-browse-page ()
  "Browse a WWW page"
  (interactive)
  (emacspeak-audio-annotate-paragraphs)
  (emacspeak-execute-repeatedly 'forward-paragraph))

(declaim (special w3-mode-map))
(define-key w3-mode-map "." 'w3-speak-browse-page)

(defvar url-speak-last-progress-indication 0
  "Caches when we last produced a progress auditory icon")

(defadvice url-lazy-message (around emacspeak pre act)
  "Provide pleasant auditory feedback about progress"
  (declare (special url-speak-last-progress-indication ))
  (let ((now (nth 1 (current-time))))
    (when (> now
	     (+ 3 url-speak-last-progress-indication))
	  (setq url-speak-last-progress-indication now)
	  (emacspeak-auditory-icon 'progress))))

(provide 'w3-speak)
