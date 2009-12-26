;;; arabic.el --- pre-loaded support for Arabic. -*- coding: iso-2022-7bit; -*-

;; Copyright (C) 1992,93,94,95 Free Software Foundation, Inc.
;; Copyright (C) 1995 Amdahl Corporation.
;; Copyright (C) 1995 Sun Microsystems.
;; Copyright (C) 2002 Ben Wing.

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

;;; Commentary:

;; Synched up with: Mule 2.3, FSF 21.1.

;;; Code:

(make-coding-system 'iso-8859-6 'iso2022
		    "ISO-8859-6 (Arabic)"
		    '(charset-g0 ascii
				 charset-g1 arabic-iso8859-6
				 charset-g2 t
				 charset-g3 t
				 no-iso6429 t
				 mnemonic "MIME/Arbc"
				 ))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; ARABIC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; (define-language-environment 'arabic
;;   "Arabic"
;;   (lambda ()
;;     (require 'arabic)))

;;; arabic.el ends here
