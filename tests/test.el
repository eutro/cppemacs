;; -*- lexical-binding: t -*-
;
;; Copyright (C) 2024 Eutro <https://eutro.dev>
;;
;; This file is part of cppemacs.
;;
;; cppemacs is free software: you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; cppemacs is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
;; General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with cppemacs. If not, see <https://www.gnu.org/licenses/>.
;;
;; SPDX-FileCopyrightText: 2024 Eutro <https://eutro.dev>
;;
;; SPDX-License-Identifier: GPL-3.0-or-later

(require 'cl-lib)

(setq backtrace-on-error-noninteractive nil)

(module-load (pop command-line-args-left))

(let ((status
       (cppemacs-test
        (vconcat
         command-line-args-left
         (split-string-and-unquote (or (getenv "CATCH2_FLAGS") ""))))))
  (unless (= status 0)
    (kill-emacs status)))

