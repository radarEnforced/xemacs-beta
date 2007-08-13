;;; cus-load.el --- Batch load all available cus-load files

;; Copyright (C) 1997 by Free Software Foundation, Inc.

;; Author: Steven L Baur <steve@altair.xemacs.org>
;; Keywords: internal, help, faces

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

;;; Synched up with: Not in FSF

;;; Commentary:

;; In FSF all of the custom loads are in a single `cus-load' file.
;; However, we have them distributed across directories, with optional
;; incremental loading.  Here we simply collect the whole set.


;;; Code:

(require 'custom)

(defun custom-put (symbol property list)
  (let ((loads (get symbol property)))
    (dolist (el list)
      (unless (member el loads)
	(setq loads (nconc loads (list el)))))
    (put symbol property loads)
    (puthash symbol t custom-group-hash-table)))

(message "Loading customization dependencies...")

(mapc (lambda (dir)
	(load (expand-file-name "custom-load" dir) t t))
      load-path)

(message "Loading customization dependencies...done")

(provide 'cus-load)

;;; cus-load.el ends here