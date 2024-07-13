;; check-license.el -- Check license headers in files -*- lexical-binding: t -*-

;;; Commentary:

;; Script for checking and inserting license headers.

;;; Code:

(require 'pcase)
(require 'cl-lib)

(defvar check-license-expected nil "The expected license header of the file.")
(defvar check-license-modes '(check) "List of modes: `check', `add', `replace'.")
(defvar check-license-log nil "Log of application results.")

(defun check-license-anylicense-p (str)
  "Return non-nil if STR is probably a license."
  (let ((case-fold-search t))
    (string-match-p "LICENSE" str)))

(defun check-license-common (pattern line-pref insert-prefix insert-suffix skip-lines)
  "Common check-license function.

PATTERN is the regexp to use to match the comment.  LINE-PREF
describes how the content is derived from the license,
INSERT-PREFIX and INSERT-SUFFIX are the prefix and suffix to wrap
the content with when adding.  SKIP-LINES is the number of lines
to skip before inserting.

Returns the result: `present', `added', `replaced', `incorrect',
or `absent'."
  (catch 'return
    (let* ((raw-expect (string-split (string-trim-right check-license-expected) "\n" nil))
           (pref-expect (mapcar (lambda (line) (string-join (list line-pref line))) raw-expect))
           (joined-expect (string-join pref-expect "\n"))
           (start-marker (make-marker))
           (end-marker (make-marker))

           (continue t)
           (joined-actual "")
           is-correct)
      (set-marker-insertion-type start-marker nil) ;; don't advance
      (set-marker-insertion-type end-marker t)     ;; do advance

      (goto-char (point-min))
      (forward-line skip-lines)
      (set-marker start-marker (point))

      (while continue
        (cond
         ;; found something, check if it's a license
         ((re-search-forward pattern nil t)
          (set-marker start-marker (match-beginning 1))
          (set-marker end-marker (match-end 1))
          (setq
           joined-actual (match-string 1)
           ;; break if it is a license
           continue (not (check-license-anylicense-p joined-actual))))

         ;; nothing found, add it
         ((memq 'add check-license-modes)
          (goto-char start-marker)
          (insert insert-prefix)
          (insert joined-expect)
          (insert insert-suffix)

          (throw 'return 'added))

         ;; nothing found ('check, 'replace)
         (t (throw 'return 'absent))))

      (setq is-correct
            (string-equal
             joined-expect
             joined-actual))

      ;; we have:
      ;; PREFIX start-marker <license-or-nothing> end-marker SUFFIX
      (cond
       (is-correct 'present)
       ((memq 'replace check-license-modes)
        (delete-region start-marker end-marker)
		(goto-char start-marker)
		(insert joined-expect)
        'replaced)
       (t 'incorrect)))))

(defun check-license-delimited (prefix line-pref suffix skip-lines)
  "Check-license function for delimited license header style.

PREFIX is the start of a comment, SUFFIX is the end, LINE-PREF
is the prefix to each line of the hader, and SKIP-LINES are the
lines to skip before the comment.

Returns the result: `present', `added', `replaced', `incorrect',
or `absent'."
  (check-license-common
   (string-join
    (list
     (regexp-quote prefix)
     "\\(\\(?:[^\n]\\|\n\\)*?\\)"
     (regexp-quote suffix)))
   line-pref prefix suffix
   skip-lines))

(defun check-license-line-comments (line-pref skip-lines)
  "Check-license function for line-comment header style.

LINE-PREF is the line comment prefix, SKIP-LINES are the lines
to skip before the comment.

Returns the result: `present', `added', `replaced', `incorrect',
or `absent'."
  (check-license-common
   (string-join
    (list
     "\\("
     (regexp-quote line-pref)
     "[^\n]*\\(?:\n"
     (regexp-quote line-pref)
     "[^\n]*\\)*\\)"))
   line-pref "" ""
   skip-lines))

(defconst check-license-alist
  `(("\\.[hc]pp$" check-license-delimited "/*\n" " *" "*/" 0)
    ("\\.el$" check-license-line-comments ";;" 2)
    ("\\.cmake$\\|^\\(?:CmakeLists.txt\\|Makefile\\|\\.gitignore\\)$" check-license-line-comments "#" 0)
    ("\\.\\(?:html\\|xml\\)$" check-license-delimited "<!--\n" "-" "\n-->" 1)
    ("\\.md$" check-license-delimited "<!--\n" "-" "\n-->" 0)))
"Alist of file name regexp to license-check function and arguments."

(defun check-license-find-fun (file-name)
  "Find the function and arguments to license-check FILE-NAME with."
  (setq file-name (file-name-nondirectory file-name))
  (catch 'found
    (pcase-dolist (`(,regexp . ,fun+args) check-license-alist)
      (when (string-match regexp file-name nil t)
        (throw 'found fun+args)))
    nil))

(defun check-license-slurp (source-file)
  "Get the contents of SOURCE-FILE."
  (with-temp-buffer
    (insert-file-contents (expand-file-name source-file default-directory))
    (buffer-substring-no-properties (point-min) (point-max))))

(defun check-license-apply-to-file (file)
  "Check the license of FILE only."
  (setq file (expand-file-name file default-directory))
  (pcase (check-license-find-fun file)
    ('nil (error "No license function for file: %s" file))
    (`(,fun . ,args)
     (with-temp-file file
       (save-excursion
         (insert-file-contents file))

       (push
        (list
         file
         (apply fun args))
        check-license-log)))
    (it (error "Malformed license function: %S" it))))

(defun check-license-apply (file spec-chain)
  "Check the license of FILE according to SPEC-CHAIN.

SPEC-CHAIN is a list of specs, where elements of the first are either:

- Strings, file globs which should be applied with no spec.

- '.. to include the parent spec

- (not GLOB) file globs which should be excluded

- (license LICENSE-FILE) the file to slurp the license from when
  the spec is entered.

- (recurse (FILE-GLOBS) &rest NESTED-SPEC) the file glob to recurse
  on, with the new NESTED-SPEC."
  (let ((default-directory
         (if (file-directory-p file)
             (expand-file-name file default-directory)
           default-directory))
        (check-license-expected check-license-expected)
        ;; hash of file name to nested specs
        (files-and-specs (make-hash-table :test 'equal))

        (spec (car spec-chain)))

    (while spec
      (pcase (pop spec)
        ((and (pred stringp) glob)
         (dolist (child (file-expand-wildcards glob))
           (push nil (gethash child files-and-specs))))
        (`(recurse ,file-globs . ,nested-spec)
         (dolist (glob file-globs)
           (dolist (child (if (equal "." glob) '(".")
                            (file-expand-wildcards glob)))
             (push nested-spec (gethash child files-and-specs)))))

        (`(not ,(and (pred stringp) glob))
         (dolist (child (file-expand-wildcards glob))
           (remhash child files-and-specs)))

        (`(license ,(and (pred stringp) license-file))
         (setq check-license-expected (check-license-slurp license-file)))

        ('.. (setq spec (nconc spec (cadr spec-chain))))

        (op (error "Unrecognised op: %S" op))))

    (if (not (file-directory-p file))
        (check-license-apply-to-file file)
      (maphash
       (lambda (child specs)
         (check-license-apply
          (expand-file-name child file)
          (cons
           (seq-uniq (apply #'seq-concatenate 'list specs))
           spec-chain)))
       files-and-specs))))

(defun check-license-run (root spec modes)
  "Run check-license on ROOT with SPEC and MODES.

Return the log."
  (let ((check-license-modes modes)
        check-license-log)
    (check-license-apply (expand-file-name root default-directory) (list spec))
    check-license-log))

(check-license-run
 ".."
 '((license "scripts/license-header-gpl.txt")
   (recurse
    (".")
    "CMakeLists.txt"
    "*.cmake"
    "Makefile"
    "*.md"
    "*.el"
    (recurse ("scripts" "include/cppemacs" "tests") ..)

    (recurse
     ("docs")
     "CMakeLists.txt"
     (recurse ("examples") "*.cpp")
     (recurse ("*.html" "*.xml") (license "../scripts/license-header-gfdl.txt")))))
 '(check))

(provide 'check-license)
;;; check-license.el ends here
