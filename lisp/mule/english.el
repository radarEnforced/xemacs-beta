;;; english.el --- English support -*- coding: iso-2022-7bit; -*-

;; Copyright (C) 1997,1999 Electrotechnical Laboratory, JAPAN.
;; Copyright (C) 2001, 2002 Ben Wing.
;; Licensed to the Free Software Foundation.

;; Keywords: multibyte character, character set, syntax, category

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

;;; Commentary:

;; We need nothing special to support English on Emacs.  Selecting
;; English as a language environment is one of the ways to reset
;; various multilingual environment to the original setting.

;;; Code

(set-language-info-alist
 "English" '((tutorial . "TUTORIAL")
	     (locale "en" "C")
	     (charset ascii)
             (coding-system iso-8859-1)
             (coding-priority iso-8859-1)
             (native-coding-system iso-8859-1)
	     (sample-text . "Hello!, Hi!, How are you?")
	     (documentation . "\
Nothing special is needed to handle English.")
	     ))

;; Make "ASCII" an alias of "English" language environment.
(set-language-info-alist
 "ASCII" (cdr (assoc "English" language-info-alist)))

;;; english.el ends here
