;;; uni-case-conv.el --- Case-conversion support for Unicode

;; Copyright (C) 2017 Free Software Foundation, (C) 2010 Ben Wing

;; Keywords: multilingual, case, uppercase, lowercase, Unicode

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

;; Generate uni-case-conv.el. To do this, parse the UnicodeData.txt file. Do
;; not parse CaseFolding.txt, since it does not mark the case of the character
;; to be folded (a lower case character can be folded to a lower case
;; character, which is an issue at least for ?\u00b5 MICRO SIGN). This will
;; need to be replaced once we have better case support (once we attempt to
;; support SpecialCasing.txt.) This only needs to be done when UnicodeData.txt
;; is updated. First commit reflects UnicodeData.txt of 20160517, md5 sum
;; dde25b1cf9bbb4ba1140ac12e4128b0b.

;; Based on Ben's make-case-conv.py, which parsed CaseFolding.txt rather than
;; UnicodeData.txt

(require 'descr-text)

(let* ((output-filename "uni-case-conv.el")
       (output-buffer (get-buffer-create output-filename))
       (mapping (make-hash-table :test #'eql))
       case-fold-search lower upper)
  (format-into output-buffer
               #r";;; %s --- Case-conversion support for Unicode

;; Copyright (C) 2010 Ben Wing.

;; Keywords: multilingual, case, uppercase, lowercase, Unicode

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

;; DO NOT MODIFY THIS FILE!!!!!!!!!!!!!!!!
;; This file is autogenerated by %s.  Modify that
;; file instead.

;;; Code:

;; We process UnicodeData.txt in reverse order so that the more
;; desirable mappings, which come early, override less desirable later ones.
;; In particular, we definitely do not want the following bindings to work
;; both ways:

;;        (?\u017F ?\u0073) ;; LATIN SMALL LETTER LONG S
;;        (?\u212A ?\u006B) ;; KELVIN SIGN
;;        (?\u212B ?\u00E5) ;; ANGSTROM SIGN

;; The first two are especially bad as they will cause upcasing operations
;; on lowercase s and k to give strange results.  It's actually worse than
;; that -- for unknown reasons, with the bad mappings in place, the byte-
;; compiler produces broken code for some files, which results in a stack-
;; underflow crash upon loadup in preparation for dumping.
" output-filename (subseq (file-name-nondirectory load-file-name) 0
                          (if (eql (aref load-file-name
                                         (1- (length load-file-name)))
                                   ?c)
                              -1)))
  ;; This is a separate call just for the sake of indentation, so we don't
  ;; have a ?( in the first column.
  (write-sequence "(loop
  for (upper lower)
  in '(" output-buffer)
  (labels ((tounichar (value)
             (format (if (<= value #xFFFF) #r"?\u%04X" #r"?\U%08X") value))
           (choose-comment (upper lower)
             (let* ((upper-comment
                     (cadr (assoc "Name" (describe-char-unicode-data upper))))
                    (lower-comment
                     (cadr (assoc "Name" (describe-char-unicode-data lower))))
                    (folded-upper-comment
                     (replace-in-string upper-comment " CAPITAL LETTER " "" t)))
               (if (equal folded-upper-comment
                          (replace-in-string
                           lower-comment " SMALL LETTER " "" t))
                   (replace-in-string lower-comment
                                      " SMALL LETTER " " LETTER " t)
                 (concat upper-comment ", " lower-comment)))))
    (with-temp-buffer
      (insert-file-contents describe-char-unicodedata-file nil)
      (goto-char (point-max))
      (while (re-search-backward #r"
\([0-9A-F]+\);[^;]+;\(L[lu]\);\([^;]*;\)\{9,9\}\([^;]*\);\([^;]*\);" nil t)
       (if (equal (match-string 2) "Ll")
           (when (> (- (match-end 4) (match-beginning 4)) 0)
             (setq lower (parse-integer (match-string 1) :radix 16)
                   upper (parse-integer (match-string 4) :radix 16))
             (when (equal (cadr (assoc "Category" (describe-char-unicode-data upper)))
                          "uppercase letter")
               (format-into output-buffer "(%s %s) ;; %s\n       "
                            (tounichar upper) (tounichar lower) 
                            (choose-comment upper lower))
               (puthash upper lower mapping)))
         (when (> (- (match-end 5) (match-beginning 5)) 0)
           (setq upper (parse-integer (match-string 1) :radix 16)
                 lower (parse-integer (match-string 5) :radix 16))
           ;; We will generally encounter the lower-case characters first.
           (unless (eql (gethash upper mapping) lower)
             (when (equal (cadr (assoc "Category" (describe-char-unicode-data lower)))
                          "lowercase letter")
               (format-into output-buffer "(%s %s) ;; %s\n       "
                            (tounichar upper) (tounichar lower) 
                            (choose-comment upper lower))
               (puthash upper lower mapping))))))
      (format-into output-buffer ")
  with case-table = (standard-case-table)
  do
  (put-case-table-pair upper lower case-table))

\(provide '%.*s)

;;; %s ends here
" (position ?. output-filename :from-end t) output-filename output-filename))))

;; make-case-conv.el ends here
