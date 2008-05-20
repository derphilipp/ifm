;; ifm-mode.el --- IFM editing commands for Emacs.

;; Filename: ifm-mode.el
;; Copyright (C) 2006 Glenn Hutchings
;; Author: Glenn Hutchings <zondo42@googlemail.com>
;; Maintainer: Glenn Hutchings <zondo42@googlemail.com>
;; Created: 19 Apr 2001
;; Description: IFM map editing mode.
;; Version 0.4 (11 Apr 2008)

;; This file is not part of GNU Emacs.

;; This file is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published
;; by the Free Software Foundation; either version 2, or (at your
;; option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GNU Emacs; see the file COPYING.  If not, write to the
;; Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
;; Boston, MA 02110-1301, USA.

;; Commentary:

;; Major mode for editing Interactive Fiction Mapper (IFM) maps.  To enable
;; it, put (require 'ifm-mode) in your .emacs file and make sure this file
;; is in your lisp search path.

;; Change Log:

;; Version 0.1 - 19 Apr 2001
;;   * First version.
;;
;; Version 0.2 - 9 Jul 2002
;;   * Use font-lock mode.
;;
;; Version 0.3 - 6 Sep 2006
;;   * Merge in code from Lee Bigelow <ligelowbee@yahoo.com> to do syntax
;;     checking and display maps, items and tasks.
;;   * Add commands to display variables and map sections.
;;   * Add customization group and variables.
;;
;; Version 0.4 - 20 May 2008
;;   * Add commands to show recording and debugging task output.
;;   * Add ifm-program-args with default "-nowarn" to avoid warnings getting
;;     printed to temporary PostScript files.
;;   * Add flymake support.

;; TODO:

;; Add display of user-selected map sections only.  Maybe scan map section
;; output for section names at startup and add them to the menu, for just
;; displaying that section.

;; Code:

(require 'easymenu)
(require 'flymake)

(defgroup ifm nil
  "Major mode for editing IFM Interactive Fiction Maps."
  :group 'languages
  :prefix "ifm-")

(defcustom ifm-program "ifm"
  "IFM program to run."
  :group 'ifm
  :type 'string)

(defcustom ifm-program-args "-nowarn"
  "Extra arguments to pass to IFM program."
  :group 'ifm
  :type 'string)

(defcustom ifm-viewer "gv"
  "Program to view PostScript generated by IFM."
  :group 'ifm
  :type 'string)

(defcustom ifm-viewer-args ""
  "Extra arguments to invoke PostScript viewer with."
  :group 'ifm
  :type 'string)

(defvar ifm-mode-map nil
  "Keymap used in IFM mode.")

(unless ifm-mode-map
  (let ((map (make-sparse-keymap)))
    (define-key map "\t" 'indent-relative)
    (define-key map "\C-c\C-c" 'ifm-check-syntax)
    (define-key map "\C-c\C-m" 'ifm-show-maps)
    (define-key map "\C-c\C-i" 'ifm-show-items)
    (define-key map "\C-c\C-t" 'ifm-show-tasks)
    (define-key map "\C-c\C-r" 'ifm-show-tasks-rec)
    (define-key map "\C-c\C-d" 'ifm-show-tasks-debug)
    (define-key map "\C-c\C-s" 'ifm-show-sections)
    (define-key map "\C-c\C-v" 'ifm-show-vars)
    (setq ifm-mode-map map)))

(defvar ifm-mode-syntax-table nil
  "Syntax table used in IFM mode.")

(defvar ifm-mode-hook '())

(defconst ifm-structure-regexp
  (regexp-opt '("room") 'words)
  "Regexp matching structure keywords in IFM mode.")

(defconst ifm-direction-regexp
  (regexp-opt '("n" "north" "ne" "northeast" "e" "east" "se" "southeast"
		"s" "south" "sw" "southwest" "w" "west" "nw" "northwest")
	      'words)
  "Regexp matching direction names in IFM mode.")

(defconst ifm-special-regexp
  (regexp-opt '("endstyle" "style" "title" "map" "start" "finish" "safe"
		"require") 'words)
  "Regexp matching special keywords in IFM mode.")

(defconst ifm-builtin-regexp
  (regexp-opt '("all" "any" "it" "last" "none" "undef") 'words)
  "Regexp matching builtin names in IFM mode.")

(defconst ifm-keyword-regexp
  (regexp-opt '("after" "before" "cmd" "d" "do" "down" "dir" "drop" "except"
		"exit" "follow" "from" "get" "give" "go" "goto" "hidden"
		"ignore" "in" "item" "join" "keep" "leave" "length" "link"
		"lose" "lost" "need" "nodrop" "nolink" "nopath" "note"
		"oneway" "out" "score" "tag" "task" "to" "u" "up" "until"
		"with") 'words)
  "Regexp matching general keywords in IFM mode.")

(defconst ifm-obsolete-regexp
  (regexp-opt '("given" "times") 'words)
  "Regexp matching obsolete keywords in IFM mode.")

(defconst ifm-font-lock-keywords
  (list
   (cons "#.*" font-lock-comment-face)
   (cons "\"[^\"]*\"" font-lock-string-face)
   (cons ifm-special-regexp font-lock-constant-face)
   (cons ifm-structure-regexp font-lock-function-name-face)
   (cons ifm-direction-regexp font-lock-variable-name-face)
   (cons ifm-keyword-regexp font-lock-keyword-face)
   (cons ifm-builtin-regexp font-lock-builtin-face)
   (cons ifm-obsolete-regexp font-lock-warning-face))
  "Font-lock keywords in IFM mode.")

(easy-menu-define ifm-mode-menu ifm-mode-map "IFM mode menu"
 '("IFM"
   ["Check syntax"            ifm-check-syntax t]
   "---"
   ["Show maps"               ifm-show-maps t]
   ["Show items"              ifm-show-items t]
   "---"
   ["Show tasks"              ifm-show-tasks t]
   ["Show tasks (recording)"  ifm-show-tasks-rec t]
   ["Show tasks (debugging)"  ifm-show-tasks-debug t]
   "---"
   ["Show map sections"       ifm-show-sections t]
   ["Show variables"          ifm-show-vars t]))

(defun ifm-mode ()
  "Major mode for editing Interactive Fiction maps in IFM format.

As well as highlighting the IFM syntax, this mode can run IFM to
automatically generate the maps, item lists and task lists, and run a
PostScript viewer to view the maps.  You can set up the viewer to
automatically watch for changes; see `ifm-show-maps' for more details.

\\{ifm-mode-map}

Calling this function invokes the function(s) listed in `ifm-mode-hook'
before doing anything else."
  (interactive)
  (kill-all-local-variables)

  (setq comment-start "# ")
  (setq comment-end "")
  (setq comment-column 0)
  (setq comment-start-skip "#[ \t]*")

  ;; Become the current major mode.
  (setq major-mode 'ifm-mode)
  (setq mode-name "IFM")

  ;; Activate syntax table.
  (unless ifm-mode-syntax-table
    (setq ifm-mode-syntax-table (make-syntax-table))
    (modify-syntax-entry ?_ "w" ifm-mode-syntax-table))

  (set-syntax-table ifm-mode-syntax-table)

  ;; Activate keymap.
  (use-local-map ifm-mode-map)

  ;; Set up font lock.
  (make-local-variable 'font-lock-defaults)
  (setq font-lock-defaults '(ifm-font-lock-keywords t))

  ;; Run startup hooks.
  (run-hooks 'ifm-mode-hook))

(defun ifm-show-maps (arg)
  "Display IFM maps in a PostScript viewer.
With prefix arg, write maps to PostScript file instead.

When viewing maps, a temporary file is written.  Then, if a viewer is not
running, one is started (the program defined by `ifm-viewer').  Otherwise,
nothing more is done.  If the PostScript viewer has the ability to watch
files for changes (as does 'gv' for example) you can add the command-line
argument to enable that in `ifm-viewer-args'.  For 'gv', that is either
'-watch' or '--watch' depending on the program version."
  (interactive "P")

  (ifm-check)
  (let* ((file (ifm-get-filename "PostScript file" ".ps" arg))
	 (args (split-string (concat ifm-viewer-args " " file))))
    ;; Write PostScript to file.
    (ifm-run " *ifm map*" file nil "-map")

    ;; Feed it to viewer if required.
    (unless (or arg (get-process "ifm-viewer"))
      (apply 'start-process "ifm-viewer" nil ifm-viewer args))))

(defun ifm-show-items (arg)
  "Show IFM item list in another window.
With prefix arg, write item list to file instead."
  (interactive "P")
  (ifm-check)

  (let ((file (if arg
		  (ifm-get-filename "Item list file" "-items.txt" arg)
		nil)))
    (ifm-run "*IFM items*" file (not file) "-items")))

(defun ifm-show-tasks (arg)
  "Show IFM task list in another window.
With prefix arg, write item list to file instead."
  (interactive "P")
  (ifm-check)

  (let ((file (if arg
		  (ifm-get-filename "Task list file" "-tasks.txt" arg)
		nil)))
    (ifm-run "*IFM tasks*" file (not file) "-tasks")))

(defun ifm-show-tasks-rec (arg)
  "Show IFM task list as a game recording in another window.
With prefix arg, write item list to file instead."
  (interactive "P")
  (ifm-check)

  (let ((file (if arg
		  (ifm-get-filename "Task list file" "-tasks-rec.txt" arg)
		nil)))
    (ifm-run "*IFM tasks (recording)*" file (not file)
	     "-tasks" "-format" "rec")))

(defun ifm-show-tasks-debug (arg)
  "Show IFM task list with debugging info in another window.
With prefix arg, write item list to file instead."
  (interactive "P")
  (ifm-check)

  (let ((file (if arg
		  (ifm-get-filename "Task list file" "-tasks-debug.txt" arg)
		nil)))
    (ifm-run "*IFM tasks (debugging)*" file (not file)
	     "-tasks" "-style" "verbose")))

(defun ifm-show-sections ()
  "Show IFM map sections in another window."
  (interactive)
  (ifm-run "*IFM map sections*" nil t "-show" "maps" "-"))

(defun ifm-show-vars ()
  "Show IFM variable list in another window."
  (interactive)
  (ifm-run "*IFM variables*" nil t "-show" "vars"))

(defun ifm-get-filename (prompt suffix arg)
  "Get filename to write IFM output to."
  (let* ((dirname (file-name-directory (buffer-file-name)))
	 (bufname (file-name-nondirectory (buffer-file-name)))
	 (name (if bufname
		   (file-name-sans-extension bufname)
		 "untitled"))
	 (filename (concat name suffix))
	 (path (concat temporary-file-directory filename)))
    (if arg
	(read-file-name (concat prompt " (default " filename "): ")
			dirname filename)
      (concat temporary-file-directory filename))))

(defun ifm-check-syntax ()
  "Check syntax of IFM input in the current buffer."
  (interactive)
  (ifm-check)
  (message "Syntax appears OK"))

(defun ifm-check ()
  "Run IFM and check for syntax errors."
  (let ((line nil)
	(msg nil)
	(buf " *ifm check*"))
    ;; Run IFM in another buffer.
    (ifm-run buf nil nil)

    ;; Scan it for errors.
    (save-excursion
      (set-buffer buf)
      (goto-char (point-min))
      (if (string-match "error: .+line \\([0-9]+\\): \\(.+\\)" (buffer-string))
	  (progn
	    (setq line (string-to-int (substring (buffer-string)
						 (match-beginning 1)
						 (match-end 1))))
	    (setq msg (substring (buffer-string)
				 (match-beginning 2) (match-end 2))))))

    ;; If error found, go to line and raise it.
    (if line
	(progn
	  (goto-line line)
	  (error "IFM error on line %d: %s" line msg)))))

(defun ifm-run (buf file view &rest args)
  "Run IFM on the current buffer and display output in buffer BUF.
Write output to file FILE, if non-nil.  Display buffer in view mode, if
VIEW is non-nil.  Pass ARGS to IFM."
  ;; Kill any existing buffer.
  (if (get-buffer buf)
      (kill-buffer buf))

  ;; Run IFM on current buffer.
  (apply 'call-process-region (point-min) (point-max)
	 ifm-program nil buf t ifm-program-args args)

  ;; Write to file if required.
  (when file
      (save-excursion
	(set-buffer buf)
	(write-region (point-min) (point-max) file nil 'novisit))
      (message "Wrote %s" file))

  ;; Display buffer if required.
  (when view
    (view-buffer buf)
    (bury-buffer buf)
    (goto-char (point-min))))

(defun ifm-mode-after-find-file ()
  (when (string-match "\\.ifm$" (buffer-file-name))
    (ifm-mode)))

(add-hook 'find-file-hooks 'ifm-mode-after-find-file)

(defun flymake-ifm-init ()
  (let* ((temp-file (flymake-init-create-temp-buffer-copy
		     'flymake-create-temp-inplace))
     	 (local-file (file-relative-name
		      temp-file
		      (file-name-directory buffer-file-name))))
    (list ifm-program (list local-file))))

(setq flymake-allowed-file-name-masks
      (cons '(".+\\.ifm$"
	      flymake-ifm-init
	      flymake-simple-cleanup
	      flymake-get-real-file-name)
	    flymake-allowed-file-name-masks))

(provide 'ifm-mode)
