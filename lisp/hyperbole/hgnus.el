;;!emacs
;;
;; FILE:         hgnus.el
;; SUMMARY:      Support Hyperbole buttons in news reader/poster: GNUS.
;; USAGE:        GNU Emacs Lisp Library
;; KEYWORDS:     hypermedia, news
;;
;; AUTHOR:       Bob Weiner
;; ORG:          Brown U.
;;
;; ORIG-DATE:    24-Dec-91 at 22:29:28 by Bob Weiner
;; LAST-MOD:      8-Aug-95 at 10:53:53 by Bob Weiner
;;
;; This file is part of Hyperbole.
;; Available for use and distribution under the same terms as GNU Emacs.
;;
;; Copyright (C) 1991-1995, Free Software Foundation, Inc.
;; Developed with support from Motorola Inc.
;;
;; DESCRIPTION:  
;;
;;   This only works with GNUS 3.15 or above, so be sure to check your
;;   newsreader version {M-ESC gnus-version RET} before reporting any
;;   problems.
;;
;;   Automatically configured for use in "hyperbole.el".
;;   If hsite loading fails prior to initializing Hyperbole Gnus support,
;;
;;       {M-x Gnus-init RTN}
;;
;;   will do it.
;;
;;
;;   Have not yet overloaded 'news-reply-yank-original'
;;   to yank and hide button data from news article buffer.
;;
;; DESCRIP-END.

;;; ************************************************************************
;;; Other required Elisp libraries
;;; ************************************************************************

(require 'hmail)
(require 'hsmail)
;; This is not in Gnus 5.x
;(require 'gnuspost)

;;; ************************************************************************
;;; Public variables
;;; ************************************************************************

(setq hnews:composer 'news-reply-mode
      hnews:lister   'gnus-summary-mode
      hnews:reader   'gnus-article-mode)


;;; ************************************************************************
;;; Public functions
;;; ************************************************************************

(defun Gnus-init ()
  "Initializes Hyperbole support for Gnus Usenet news reading."
  (interactive)
  nil)

(defun lnews:to ()
  "Sets current buffer to the Usenet news article summary listing buffer."
  (and (eq major-mode hnews:reader) (set-buffer gnus-summary-buffer)))

(defun rnews:to ()
  "Sets current buffer to the Usenet news article reader buffer."
  (and (eq major-mode hnews:lister) (set-buffer gnus-article-buffer)))

(defun rnews:summ-msg-to ()
  "Displays news message associated with current summary header."
  (let ((article (gnus-summary-article-number)))
    (if (or (null gnus-current-article)
	    (/= article gnus-current-article))
	;; Selected subject is different from current article's.
	(gnus-summary-display-article article))))


;;; Overlay 'gnus-inews-article' from "gnuspost.el" to make it include
;;; any signature before Hyperbole button data.  Does this by having
;;; signature inserted within narrowed buffer and then applies a hook to
;;; have the buffer widened before sending.
(hypb:function-symbol-replace
  'gnus-inews-article 'widen 'hmail:msg-narrow)

;;; Overload this function from "rnewspost.el" for supercite compatibility
;;; only when supercite is in use.
(if (hypb:supercite-p)
    (defun news-reply-yank-original (arg)
      "Supercite version of news-reply-yank-original.
Insert the message being replied to in the reply buffer. Puts point
before the mail headers and mark after body of the text.  Calls
mail-yank-original to actually yank the message into the buffer and
cite text.  

If mail-yank-original is not overloaded by supercite, each nonblank
line is indented ARG spaces (default 3).  Just \\[universal-argument]
as ARG means don't indent and don't delete any header fields."
      (interactive "P")
      (mail-yank-original arg)
      (exchange-point-and-mark)
      (run-hooks 'news-reply-header-hook))
  )

;;; ************************************************************************
;;; Private variables
;;; ************************************************************************
;;;
(var:append 'gnus-Inews-article-hook '(widen))
;;;
;;; Hide any Hyperbole button data and highlight buttons if possible
;;; in news article being read.
(var:append 'gnus-article-prepare-hook
	    (if (fboundp 'hproperty:but-create)
		'(hmail:msg-narrow hproperty:but-create)
	      '(hmail:msg-narrow)))

(if (fboundp 'hproperty:but-create)
    (var:append 'gnus-summary-prepare-hook '(hproperty:but-create)))

;;; Try to setup comment addition as the first element of these hooks.
(if (fboundp 'add-hook)
    ;; Called from 'news-post-news' if prev unsent article exists and user
    ;; says erase it.  Add a comment on Hyperbole button support.
    (progn
      (add-hook 'news-setup-hook 'smail:comment-add)
      ;; Called from 'news-post-news' if no prev unsent article exists.
      ;; Add a comment on Hyperbole button support.
      (add-hook 'news-reply-mode-hook 'smail:comment-add))
  (var:append 'news-setup-hook '(smail:comment-add))
  (var:append 'news-reply-mode-hook '(smail:comment-add)))

(provide 'hgnus)
