;;; chinese.el --- Support for Chinese

;; Copyright (C) 1995 Electrotechnical Laboratory, JAPAN.
;; Licensed to the Free Software Foundation.
;; Copyright (C) 1997 MORIOKA Tomohiko

;; Keywords: multilingual, Chinese

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

;;; Commentary:

;; For Chinese, three character sets GB2312, BIG5, and CNS11643 are
;; supported.

;;; Code:

;; Syntax of Chinese characters.
(modify-syntax-entry 'chinese-gb2312 "w")
(loop for row in '(33 34 41)
      do (modify-syntax-entry `[chinese-gb2312 ,row] "."))
;;(loop for row from 35 to  40
;;      do (modify-syntax-entry `[chinese-gb2312 ,row] "w"))
;;(loop for row from 42 to 126
;;      do (modify-syntax-entry `[chinese-gb2312 ,row] "w"))

(modify-syntax-entry 'chinese-cns11643-1  "w")
(modify-syntax-entry 'chinese-cns11643-2  "w")
(modify-syntax-entry 'chinese-big5-1 "w")
(modify-syntax-entry 'chinese-big5-2 "w")

;; CNS11643 Plane3 thru Plane7
;; These represent more and more obscure Chinese characters.
;; By the time you get to Plane 7, we're talking about characters
;; that appear once in some ancient manuscript and whose meaning
;; is unknown.

(flet
    ((make-chinese-cns11643-charset
      (name plane final)
      (make-charset
       name (concat "CNS 11643 Plane " plane " (Chinese traditional)")
       `(registry 
         ,(concat "CNS11643[.-]\\(.*[.-]\\)?" plane "$")
         dimension 2
         chars 94
         final ,final
         graphic 0))
      (modify-syntax-entry   name "w")
      (modify-category-entry name ?t)
      ))
  (make-chinese-cns11643-charset 'chinese-cns11643-3 "3" ?I)
  (make-chinese-cns11643-charset 'chinese-cns11643-4 "4" ?J)
  (make-chinese-cns11643-charset 'chinese-cns11643-5 "5" ?K)
  (make-chinese-cns11643-charset 'chinese-cns11643-6 "6" ?L)
  (make-chinese-cns11643-charset 'chinese-cns11643-7 "7" ?M)
  )

;; ISO-IR-165 (CCITT Extended GB)
;;    It is based on CCITT Recommendation T.101, includes GB 2312-80 +
;;    GB 8565-88 table A4 + 293 characters.
(make-charset
 'chinese-isoir165
 "ISO-IR-165 (CCITT Extended GB; Chinese simplified)"
 `(registry "isoir165"
   dimension 2
   chars 94
   final ?E
   graphic 0))

;; PinYin-ZhuYin
(make-charset 'sisheng "PinYin-ZhuYin"
	      '(registry "sisheng_cwnn\\|OMRON_UDC_ZH"
		dimension 1
		chars 94
		final ?0
		graphic 0
		))

;; If you prefer QUAIL to EGG, please modify below as you wish.
;;(when (and (featurep 'egg) (featurep 'wnn))
;;  (setq wnn-server-type 'cserver)
;;  (load "pinyin")
;;  (setq its:*standard-modes*
;;        (cons (its:get-mode-map "PinYin") its:*standard-modes*)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Chinese (general)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; (make-coding-system
;;  'chinese-iso-7bit 2 ?C
;;  "ISO 2022 based 7bit encoding for Chinese GB and CNS (MIME:ISO-2022-CN)"
;;  '(ascii
;;    (nil chinese-gb2312 chinese-cns11643-1)
;;    (nil chinese-cns11643-2)
;;    (nil chinese-cns11643-3 chinese-cns11643-4 chinese-cns11643-5
;;         chinese-cns11643-6 chinese-cns11643-7)
;;    nil ascii-eol ascii-cntl seven locking-shift single-shift nil nil nil
;;    init-bol))

;; (define-coding-system-alias 'iso-2022-cn 'chinese-iso-7bit)
;; (define-coding-system-alias 'iso-2022-cn-ext 'chinese-iso-7bit)

;; (define-prefix-command 'describe-chinese-environment-map)
;; (define-key-after describe-language-environment-map [Chinese]
;;   '("Chinese" . describe-chinese-environment-map)
;;   t)

;; (define-prefix-command 'setup-chinese-environment-map)
;; (define-key-after setup-language-environment-map [Chinese]
;;   '("Chinese" . setup-chinese-environment-map)
;;   t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Chinese GB2312 (simplified) 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; (make-coding-system
;;  'chinese-iso-8bit 2 ?c
;;  "ISO 2022 based EUC encoding for Chinese GB2312 (MIME:CN-GB-2312)"
;;  '((ascii t) chinese-gb2312 chinese-sisheng nil
;;    nil ascii-eol ascii-cntl nil nil single-shift nil))

(make-coding-system
 'cn-gb-2312 'iso2022
 "Coding-system of Chinese EUC (Extended Unix Code)."
 '(charset-g0 ascii
   charset-g1 chinese-gb2312
   charset-g2 sisheng
   charset-g3 t
   mnemonic "Zh-GB/EUC"
   ))

;; (define-coding-system-alias 'cn-gb-2312 'chinese-iso-8bit)
;; (define-coding-system-alias 'euc-china 'chinese-iso-8bit)

(copy-coding-system 'cn-gb-2312 'gb2312)
(copy-coding-system 'cn-gb-2312 'chinese-euc)

;; (make-coding-system
;;  'chinese-hz 0 ?z
;;  "Hz/ZW 7-bit encoding for Chinese GB2312 (MIME:HZ-GB-2312)"
;;  nil)
;; (put 'chinese-hz 'post-read-conversion 'post-read-decode-hz)
;; (put 'chinese-hz 'pre-write-conversion 'pre-write-encode-hz)

(make-coding-system
 'hz-gb-2312 'no-conversion
 "Coding-system of Hz/ZW used for Chinese."
 '(mnemonic "Zh-GB/Hz"
   eol-type lf
   post-read-conversion post-read-decode-hz
   pre-write-conversion pre-write-encode-hz))

;; (define-coding-system-alias 'hz-gb-2312 'chinese-hz)
;; (define-coding-system-alias 'hz 'chinese-hz)

(copy-coding-system 'hz-gb-2312 'hz)
(copy-coding-system 'hz-gb-2312 'chinese-hz)

(defun post-read-decode-hz (len)
  (let ((pos (point)))
    (decode-hz-region pos (+ pos len))))

(defun pre-write-encode-hz (from to)
  (let ((buf (current-buffer))
	(work (get-buffer-create " *pre-write-encoding-work*")))
    (set-buffer work)
    (erase-buffer)
    (if (stringp from)
	(insert from)
      (insert-buffer-substring buf from to))
    (encode-hz-region 1 (point-max))
    nil))
	   
(set-language-info-alist
 "Chinese-GB" '((setup-function . (setup-chinese-gb-environment
				   . setup-chinese-environment-map))
		(charset . (chinese-gb2312 sisheng))
		(coding-system
		 . (cn-gb-2312 iso-2022-7bit hz-gb-2312))
		(sample-text . "Chinese ($AVPND(B,$AFUM(;0(B,$A::So(B)	$ADc:C(B")
		(documentation . ("Support for Chinese GB2312 character set."
				  . describe-chinese-environment-map))
		))

(defun setup-chinese-gb-environment ()
  "Setup multilingual environment (MULE) for Chinese GB2312 users."
  (interactive)
  (setup-english-environment)
  (set-coding-category-system 'iso-8-2 'cn-gb-2312)
  (set-coding-category-system 'big5 'big5)
  (set-coding-category-system 'iso-7 'iso-2022-7bit)
  (set-coding-priority-list
   '(iso-8-2
     big5
     iso-7))
  (set-default-coding-systems 'cn-gb-2312))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Chinese BIG5 (traditional)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; (make-coding-system
;;  'chinese-big5 3 ?B "BIG5 8-bit encoding for Chinese (MIME:CN-BIG5)")

(make-coding-system
 'big5 'big5
 "Coding-system of BIG5."
 '(mnemonic "Zh/Big5"))

;; (define-coding-system-alias 'big5 'chinese-big5)
;; (define-coding-system-alias 'cn-big5 'chinese-big5)

(copy-coding-system 'big5 'cn-big5)
(copy-coding-system 'big5 'chinese-big5)

;; Big5 font requires special encoding.
(define-ccl-program ccl-encode-big5-font
  `(0
    ;; In:  R0:chinese-big5-1 or chinese-big5-2
    ;;      R1:position code 1
    ;;      R2:position code 2
    ;; Out: R1:font code point 1
    ;;      R2:font code point 2
    ((r2 = ((((r1 - ?\x21) * 94) + r2) - ?\x21))
     (if (r0 == ,(charset-id 'chinese-big5-2)) (r2 += 6280))
     (r1 = ((r2 / 157) + ?\xA1))
     (r2 %= 157)
     (if (r2 < ?\x3F) (r2 += ?\x40) (r2 += ?\x62))))
  "CCL program to encode a Big5 code to code point of Big5 font.")

;; (setq font-ccl-encoder-alist
;;       (cons (cons "big5" ccl-encode-big5-font) font-ccl-encoder-alist))

(set-charset-ccl-program 'chinese-big5-1 ccl-encode-big5-font)
(set-charset-ccl-program 'chinese-big5-2 ccl-encode-big5-font)

(set-language-info-alist
 "Chinese-BIG5" '((setup-function . (setup-chinese-big5-environment
				     . setup-chinese-environment-map))
		  (charset . (chinese-big5-1 chinese-big5-2))
		  (coding-system . (big5 iso-2022-7bit))
		  (sample-text . "Cantonese ($(0GnM$(B,$(0N]0*Hd(B)	$(0*/=((B, $(0+$)p(B")
		  (documentation . ("Support for Chinese Big5 character set."
				    . describe-chinese-environment-map))
		  ))

(defun setup-chinese-big5-environment ()
  "Setup multilingual environment (MULE) for Chinese Big5 users."
  (interactive)
  (setup-english-environment)
  (set-coding-category-system 'big5 'big5)
  (set-coding-category-system 'iso-8-2 'cn-gb-2312)
  (set-coding-category-system 'iso-7 'iso-2022-7bit)
  (set-coding-priority-list
   '(big5
     iso-8-2
     iso-7))
  (set-default-coding-systems 'big5))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Chinese CNS11643 (traditional)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; (set-language-info-alist
;;  "Chinese-CNS" '((setup-function . (setup-chinese-cns-environment
;;                                     . setup-chinese-environment-map))
;;                  (charset . (chinese-cns11643-1 chinese-cns11643-2
;;                              chinese-cns11643-3 chinese-cns11643-4
;;                              chinese-cns11643-5 chinese-cns11643-6
;;                              chinese-cns11643-7))
;;                  (coding-system . (chinese-iso-7bit))
;;                  (documentation . ("Support for Chinese CNS character sets."
;;                                    . describe-chinese-environment-map))
;;                  ))

;;; chinese.el ends here